#include "../hooks.hpp"
#include "../../features/misc/misc.hpp"
#include "../../menu/config/config.hpp"
#include "../../features/visuals/visuals.hpp"
#include "../../features/misc/misc.hpp"
#include "../../features/movement/prediction/prediction.hpp"
#include "../../features/skins/skins.hpp"
#include "../../features/movement/movement.hpp"
#include "../../features/movement/moveutil.h"

void __stdcall sdk::hooks::frame_stage_notify::frame_stage_notify( int stage ) {
	g::last_hook = "frame_stage_notify";
	g::last_frame_stage = stage;
	if (!interfaces::client_state || !interfaces::engine || !interfaces::ent_list)
		return ofunc(interfaces::client, stage);

	g::local = static_cast<player_t*>(interfaces::ent_list->get_client_entity(interfaces::engine->get_local_player()));

	const bool fully_connected =
		interfaces::engine->is_connected() &&
		interfaces::engine->is_in_game() &&
		interfaces::client_state->signon_state_count >= 6;

	i_net_channel* net_channel = interfaces::client_state->net_channel;

	const auto safe_for_local_entity_mutations = []() -> bool {
		if (!interfaces::engine->is_connected() || !interfaces::engine->is_in_game())
			return false;

		// `is_in_game()` can be true during early connection phases where entity/inventory state is still settling.
		// Gate risky local mutations on client signon state as well.
		if (!interfaces::client_state)
			return false;

		// Source1 signon: SIGNONSTATE_FULL == 6 (see engine `SignonState_t`).
		if (interfaces::client_state->signon_state_count < 6)
			return false;

		const auto local = g::local;
		if (!local)
			return false;

		// During connect / team select / spectator states, lots of local-adjacent pointers are transient.
		// Several features here historically assumed "spawned CT/T with a stable pawn".
		if (!local->is_alive())
			return false;

		const int team = local->team();
		if (team != 2 && team != 3)
			return false;

		return true;
	};

	if (!interfaces::engine->is_in_game()) {
		features::weather::reset_weather(false);
		fakeping.clear_sequence();
		return ofunc(interfaces::client, stage);
	}

	static int weather_stage_dbg = 0;
	if (++weather_stage_dbg <= 20)
		debug::log("WEATHER STAGE5 CALL connected=%d ingame=%d enable=%d entity_precip=%d",
			interfaces::engine && interfaces::engine->is_connected() ? 1 : 0,
			interfaces::engine && interfaces::engine->is_in_game() ? 1 : 0,
			c::visuals::enable_weather ? 1 : 0,
			c::visuals::weather_use_entity_precip ? 1 : 0);

	if (stage == 5) {
		features::weather::update_weather();

		if (safe_for_local_entity_mutations()) {
			features::visuals::nosmoke();
			features::visuals::flashalpha();
			features::visuals::skybox_changer();
			features::visuals::custom_sun();
			backtrack.setup_records();
		}
	}

	else if (stage == frame_render_end) {
		// ConVar writes can crash during early connection even if pointers look non-null.
		// Keep render-end visual tweaks behind the same "fully signed on + spawned team" gate as gameplay mutations.
		if (safe_for_local_entity_mutations()) {
			if (c::visuals::enable_darkmode) {
				static auto mat_force_tonemap_scale = interfaces::console ? interfaces::console->get_convar("mat_force_tonemap_scale") : nullptr;
				if (mat_force_tonemap_scale) {
					mat_force_tonemap_scale->set_value(c::visuals::darkmode_val);
				}
			}
			else {
				static auto mat_force_tonemap_scale = interfaces::console ? interfaces::console->get_convar("mat_force_tonemap_scale") : nullptr;
				if (mat_force_tonemap_scale) {
					mat_force_tonemap_scale->set_value(0);
				}
			}
			features::visuals::fog();
		}
	}
    else if (stage == frame_net_update_postdataupdate_start) {
		if (safe_for_local_entity_mutations()) {
			features::skins::agent_changer();
			features::skins::knife_changer();
			features::skins::gloves_changer();
			features::skins::full_update();
		}
    }
	else if (stage == frame_net_update_end) {
        
	}
	else if (stage == frame_start) {
		if (EdgebugAssistant.ticks_left && EdgebugAssistant.strafing) {
			vec3_t edgebugva = vec3_t{ features::movement::first_viewangles.x, EdgebugAssistant.startingyaw , features::movement::first_viewangles.z };

			float to_eb_time = ticks_to_time(EdgebugAssistant.edgebugtick) - ticks_to_time(EdgebugAssistant.detecttick);
			float from_detect_time = interfaces::globals->cur_time - ticks_to_time(EdgebugAssistant.detecttick);

			float addedyaw = math::normalize_yaw(EdgebugAssistant.yawdelta * (EdgebugAssistant.eblength * (from_detect_time / to_eb_time)));
			edgebugva.y += addedyaw;
			interfaces::engine->set_view_angles(edgebugva);
			UtilityAssistant.handle_fix(g::cmd, edgebugva);
		}
	}

	ofunc( interfaces::client, stage );
}
