#include "visuals.hpp"
#include "../../sdk/interfaces/v_collide_t.h"
#include "../skins/skins.hpp"

static vcollide_t precipitation_collideable{};
static bool precipitation_collision_loaded = false;
static bool created_rain = false;
static precipitation_t* rain_entity = nullptr;
static bool last_state = false;
static decltype(interfaces::client->get_all_classes()) precipitation = nullptr;
int m_timer = -1;

namespace {
    constexpr int k_precip_slot = MAX_EDICTS - 1;
    constexpr int k_data_update_created = 0;

    // Particle precipitation enum used by Source/CS:GO CPrecipitation.
    // 4 = rain, 5 = ash, 6 = rain storm, 7 = snow.
    int map_weather_type() {
        switch (c::visuals::weather_type) {
        case 0: return 4; // rain
        case 1: return 5; // ash
        case 2: return 6; // rain storm
        case 3: return 7; // snow
        default: return 4;
        }
    }

    void force_precipitation_cvars() {
        if (!interfaces::console)
            return;

        if (auto cvar = interfaces::console->get_convar("r_DrawRain"))
            cvar->set_value(1);
        if (auto cvar = interfaces::console->get_convar("r_DrawPrecipitation"))
            cvar->set_value(1);
        if (auto cvar = interfaces::console->get_convar("r_RainSimulate"))
            cvar->set_value(1);
        if (auto cvar = interfaces::console->get_convar("r_RainParticleDensity"))
            cvar->set_value(1.0f);
        if (auto cvar = interfaces::console->get_convar("r_RainRadius"))
            cvar->set_value(4096.0f);
        if (auto cvar = interfaces::console->get_convar("r_RainHack"))
            cvar->set_value(1);
        if (auto cvar = interfaces::console->get_convar("r_rainalpha"))
            cvar->set_value(1.0f);
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

    void write_precip_type(precipitation_t* entity, int type) {
        if (!entity)
            return;

        entity->precip_type() = static_cast<precipitation_type_t>(type);
        entity->solid_type() = 1;        // SOLID_BSP
        entity->solid_flags() |= 0x20;   // FSOLID_VOLUME_CONTENTS
        entity->solid_flags() &= ~0x04;  // clear FSOLID_NOT_SOLID
    }
}

void features::weather::reset_weather(const bool cleanup) {
    created_rain = false;
    last_state = false;

    if (cleanup && interfaces::ent_list) {
        auto* slot_entity = reinterpret_cast<precipitation_t*>(interfaces::ent_list->get_client_entity(k_precip_slot));
        if (slot_entity) {
            if (auto* collideable = slot_entity->collideable()) {
                collideable->mins() = { 0.f, 0.f, 0.f };
                collideable->maxs() = { 0.f, 0.f, 0.f };
            }

            slot_entity->pre_data_change(k_data_update_created);
            slot_entity->on_data_changed(k_data_update_created);
            slot_entity->post_data_update(k_data_update_created);

            if (slot_entity->networkable())
                slot_entity->net_release();

            debug::log("PRECIP RESET: released precipitation entity from slot=%d", k_precip_slot);
        }
    }

    if (precipitation_collision_loaded && interfaces::physics_collision) {
        interfaces::physics_collision->v_collide_unload(&precipitation_collideable);
        debug::log("PRECIP RESET: unloaded vcollide");
    }

    memset(&precipitation_collideable, 0, sizeof(precipitation_collideable));
    precipitation_collision_loaded = false;
    rain_entity = nullptr;
    m_timer = -1;
}

void* features::weather::getv_collideble() {
    return precipitation_collision_loaded ? &precipitation_collideable : nullptr;
}

bool features::weather::should_override_vcollide(int model_index) {
    return model_index == -1 && precipitation_collision_loaded;
}

void features::weather::render_overlay_precipitation() {
    // Real precipitation is handled by update_weather().
}

void features::weather::update_weather() {
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

    if (!precipitation) {
        for (auto client_class = interfaces::client->get_all_classes(); client_class && !precipitation;
            client_class = client_class->next_ptr) {
            if (client_class->class_id == cprecipitation)
                precipitation = client_class;
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

    const int weather_type = map_weather_type();
    log_weather_state_once_per_second(weather_type);

    static std::optional<int> last_type{};
    if (last_type.has_value() && last_type.value() != weather_type) {
        debug::log("PRECIP: type changed %d -> %d, resetting", last_type.value(), weather_type);
        reset_weather();
    }
    last_type = weather_type;

    if (!c::visuals::enable_weather || !c::visuals::weather_use_entity_precip) {
        if (created_rain || rain_entity || precipitation_collision_loaded)
            reset_weather();
        last_state = false;
        return;
    }

    force_precipitation_cvars();

    auto* existing_slot_entity = reinterpret_cast<precipitation_t*>(interfaces::ent_list->get_client_entity(k_precip_slot));
    if (created_rain && rain_entity && existing_slot_entity) {
        last_state = true;
        return;
    }

    if (created_rain && (!existing_slot_entity || !rain_entity)) {
        debug::log("PRECIP: backend thought it was created, but slot/entity is missing. Rebuilding.");
        reset_weather(false);
    }

    memset(&precipitation_collideable, 0, sizeof(precipitation_collideable));
    precipitation_collision_loaded = false;

    if (!precipitation->create_fn) {
        debug::log("PRECIP FAIL: CPrecipitation create_fn is null");
        return;
    }

    auto* created_networkable = precipitation->create_fn(k_precip_slot, 0);
    if (!created_networkable) {
        debug::log("PRECIP FAIL: create_fn returned false for slot=%d", k_precip_slot);
        return;
    }

    // Lobo edicts.cpp resolves entity from returned networkable -> unknown -> base entity.
    // Fallback to ent_list slot only if this SDK path is unavailable/null.
    rain_entity = nullptr;

    rain_entity = reinterpret_cast<precipitation_t*>(
        interfaces::ent_list->get_client_entity(k_precip_slot)
        );

    debug::log("PRECIP PTR CHECK: networkable=%p entity_list[%d]=%p",
        created_networkable, k_precip_slot, rain_entity);

    auto* slot_entity = reinterpret_cast<precipitation_t*>(interfaces::ent_list->get_client_entity(k_precip_slot));
    debug::log("PRECIP PTR CHECK: networkable=%p lobo_entity=%p entity_list[%d]=%p",
        created_networkable, rain_entity, k_precip_slot, slot_entity);

    if (!rain_entity)
        rain_entity = slot_entity;

    if (!rain_entity) {
        debug::log("PRECIP FAIL: entity is null after create_fn");
        return;
    }

    auto* collideable = rain_entity->collideable();
    if (!collideable) {
        debug::log("PRECIP FAIL: rain_entity collideable is null");
        return;
    }

    rain_entity->net_pre_data_update(0);
    rain_entity->pre_data_change(0);

    rain_entity->get_index() = -1;
    rain_entity->get_model_index() = -1;
    *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(rain_entity) + 0x254) = -1;

    write_precip_type(rain_entity, weather_type);

    rain_entity->mins() = { -32768.f, -32768.f, -32768.f };
    rain_entity->maxs() = { 32768.f,  32768.f,  32768.f };
    rain_entity->clr_render() = 0xFFFFFFFF;

    collideable->mins() = rain_entity->mins();
    collideable->maxs() = rain_entity->maxs();

    debug::log("PRECIP CREATE: entity=%p type=%d slot=%d collideable=%p model=%d index=%d clr=0x%X",
        rain_entity,
        static_cast<int>(rain_entity->precip_type()),
        k_precip_slot,
        collideable,
        rain_entity->get_model_index(),
        rain_entity->get_index(),
        rain_entity->clr_render());

    debug::log("PRECIP BOUNDS SET: mins(%.1f %.1f %.1f) maxs(%.1f %.1f %.1f)",
        rain_entity->mins().x, rain_entity->mins().y, rain_entity->mins().z,
        rain_entity->maxs().x, rain_entity->maxs().y, rain_entity->maxs().z);

    interfaces::physics_collision->v_collide_load(
        &precipitation_collideable,
        1,
        reinterpret_cast<const char*>(collide_data),
        sizeof(collide_data)
    );

    precipitation_collision_loaded = true;

    if (interfaces::model_info) {
        auto fn = get_vfunc<void* (__thiscall*)(void*, int)>(interfaces::model_info, 6);
        void* manual = fn ? fn(interfaces::model_info, -1) : nullptr;
        debug::log("PRECIP: pre-update GetVCollide(-1) returned %p", manual);
    }

    rain_entity->on_data_changed(0);
    rain_entity->post_data_update(0);

    debug::log("PRECIP FINAL UPDATE: type=%d solid=%d flags=0x%X model=%d index=%d clr=0x%X",
        static_cast<int>(rain_entity->precip_type()),
        rain_entity->solid_type(),
        rain_entity->solid_flags(),
        rain_entity->get_model_index(),
        rain_entity->get_index(),
        rain_entity->clr_render());

    if (interfaces::model_info) {
        auto fn = get_vfunc<void* (__thiscall*)(void*, int)>(interfaces::model_info, 6);
        void* manual = fn ? fn(interfaces::model_info, -1) : nullptr;
        debug::log("PRECIP: manual GetVCollide(-1) returned %p", manual);
    }

    created_rain = true;
    last_state = true;

    debug::log("PRECIP SUCCESS: CPrecipitation entity=%p type=%d slot=%d collision_loaded=%d",
        rain_entity,
        weather_type,
        k_precip_slot,
        precipitation_collision_loaded ? 1 : 0);
}
