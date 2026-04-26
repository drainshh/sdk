#include "visuals.hpp"
#include "../../sdk/interfaces/v_collide_t.h"
#include "../skins/skins.hpp"

static vcollide_t precipitation_collideable{};
static bool precipitation_collision_loaded = false;
static bool created_rain = false;
static precipitation_t* rain_entity = nullptr;
static decltype(interfaces::client->get_all_classes()) precipitation = nullptr;
int m_timer = -1;

namespace {
    constexpr int k_precip_slot = MAX_EDICTS - 1; // Lobo: 2048 - 1
    constexpr int k_data_update_created = 0;

    int map_weather_type() {
        switch (c::visuals::weather_type) {
        case 0: return 4; // particle/rain
        case 1: return 5; // ash
        case 2: return 6; // rain storm
        case 3: return 7; // particle/snow
        default: return 4;
        }
    }

    void log_weather_state_once_per_second(int weather_type) {
        static int tick = 0;
        if (++tick % 300 == 0) {
            debug::log("WEATHER RUNNING: enable=%d type=%d mapped=%d created=%d entity=%p collision_loaded=%d precip_class=%p",
                c::visuals::enable_weather ? 1 : 0,
                c::visuals::weather_type,
                weather_type,
                created_rain ? 1 : 0,
                rain_entity,
                precipitation_collision_loaded ? 1 : 0,
                precipitation);
        }
    }

    void set_precip_type_lobo_style(precipitation_t* entity, int type) {
        if (!entity)
            return;

        // Keep both: SDK accessor for logs/compat, raw Lobo offset for real CPrecipitation storage.
        entity->precip_type() = static_cast<precipitation_type_t>(type);
        *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(entity) + 0xA00) = type;
    }
}

void features::weather::reset_weather(const bool cleanup) {
    if (cleanup && interfaces::ent_list) {
        auto* slot_entity = reinterpret_cast<precipitation_t*>(interfaces::ent_list->get_client_entity(k_precip_slot));
        if (slot_entity) {
            if (slot_entity->networkable()) {
                slot_entity->net_pre_data_update(k_data_update_created);
                slot_entity->pre_data_change(k_data_update_created);
            }

            *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(slot_entity) + 0xA00) = -1;

            if (auto* collideable = slot_entity->collideable()) {
                collideable->mins() = { 0.f, 0.f, 0.f };
                collideable->maxs() = { 0.f, 0.f, 0.f };
            }

            if (slot_entity->networkable()) {
                slot_entity->on_data_changed(k_data_update_created);
                slot_entity->post_data_update(k_data_update_created);
                slot_entity->net_release();
            }

            debug::log("PRECIP RESET: released precipitation entity from slot=%d", k_precip_slot);
        }
    }

    if (precipitation_collision_loaded && interfaces::physics_collision) {
        interfaces::physics_collision->v_collide_unload(&precipitation_collideable);
        debug::log("PRECIP RESET: unloaded vcollide");
    }

    memset(&precipitation_collideable, 0, sizeof(precipitation_collideable));
    precipitation_collision_loaded = false;
    created_rain = false;
    rain_entity = nullptr;
    m_timer = -1;
}

void* features::weather::getv_collideble() {
    return &precipitation_collideable;
}

bool features::weather::should_override_vcollide(int model_index) {
    return model_index == -1 && precipitation_collision_loaded;
}

void features::weather::render_overlay_precipitation() {
    // Real precipitation is handled by update_weather().
}

void features::weather::update_weather() {
    if (!c::visuals::enable_weather || !c::visuals::weather_use_entity_precip) {
        if (created_rain || rain_entity || precipitation_collision_loaded)
            reset_weather();
        return;
    }

    if (!interfaces::client || !interfaces::ent_list || !interfaces::physics_collision) {
        static bool logged_missing_interfaces = false;
        if (!logged_missing_interfaces) {
            debug::log("PRECIP FAIL: missing interface client=%d ent_list=%d physics=%d",
                interfaces::client ? 1 : 0,
                interfaces::ent_list ? 1 : 0,
                interfaces::physics_collision ? 1 : 0);
            logged_missing_interfaces = true;
        }
        return;
    }

    const int weather_type = map_weather_type();
    log_weather_state_once_per_second(weather_type);

    if (m_timer > -1) {
        --m_timer;
        if (m_timer == 0)
            reset_weather();
    }

    static std::optional<int> last_type{};
    if (last_type.has_value() && last_type.value() != weather_type) {
        debug::log("PRECIP: type changed %d -> %d, resetting", last_type.value(), weather_type);
        reset_weather();
    }
    last_type = weather_type;

    if (created_rain)
        return;

    memset(&precipitation_collideable, 0, sizeof(precipitation_collideable));
    precipitation_collision_loaded = false;

    if (!precipitation) {
        for (auto client_class = interfaces::client->get_all_classes(); client_class; client_class = client_class->next_ptr) {
            if (client_class->class_id == cprecipitation) {
                precipitation = client_class;
                break;
            }
        }

        if (precipitation)
            debug::log("PRECIP OK: CPrecipitation class found: %p", precipitation);
        else {
            static bool logged_missing_class = false;
            if (!logged_missing_class) {
                debug::log("PRECIP FAIL: CPrecipitation class not found");
                logged_missing_class = true;
            }
            return;
        }
    }

    if (!precipitation->create_fn) {
        debug::log("PRECIP FAIL: CPrecipitation create_fn is null");
        return;
    }

    auto* created_networkable = precipitation->create_fn(k_precip_slot, 0);
    if (!created_networkable) {
        debug::log("PRECIP FAIL: create_fn returned false for slot=%d", k_precip_slot);
        return;
    }

    rain_entity = reinterpret_cast<precipitation_t*>(interfaces::ent_list->get_client_entity(k_precip_slot));
    debug::log("PRECIP PTR CHECK: networkable=%p entity_list[%d]=%p", created_networkable, k_precip_slot, rain_entity);

    if (!rain_entity) {
        debug::log("PRECIP FAIL: entity slot is null after create_fn");
        return;
    }

    if (!rain_entity->networkable()) {
        debug::log("PRECIP FAIL: rain_entity networkable is null");
        return;
    }

    auto* collideable = rain_entity->collideable();
    if (!collideable) {
        debug::log("PRECIP FAIL: rain_entity collideable is null");
        return;
    }

    // Lobo edicts.cpp order.
    rain_entity->net_pre_data_update(k_data_update_created);
    rain_entity->pre_data_change(k_data_update_created);

    rain_entity->get_index() = -1;
    set_precip_type_lobo_style(rain_entity, weather_type);

    collideable->mins() = { -32768.f, -32768.f, -32768.f };
    collideable->maxs() = { 32768.f, 32768.f, 32768.f };

    interfaces::physics_collision->v_collide_load(
        &precipitation_collideable,
        1,
        reinterpret_cast<const char*>(collide_data),
        sizeof(collide_data)
    );
    precipitation_collision_loaded = true;

    rain_entity->get_model_index() = -1;

    rain_entity->on_data_changed(k_data_update_created);
    rain_entity->post_data_update(k_data_update_created);

    created_rain = true;

    debug::log("PRECIP SUCCESS: Lobo-edicts style entity=%p type=%d raw_type=%d slot=%d model=%d index=%d collision_loaded=%d",
        rain_entity,
        static_cast<int>(rain_entity->precip_type()),
        *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(rain_entity) + 0xA00),
        k_precip_slot,
        rain_entity->get_model_index(),
        rain_entity->get_index(),
        precipitation_collision_loaded ? 1 : 0);
}
