#include "../../features/movement/prediction/prediction.hpp"
#include "../features/movement/movement.hpp"
#include "../../features/scripts/scripts.hpp"
#include "../../features/visuals/visuals.hpp"
#include "../../features/aimbot/aimbot.hpp"
#include "../../features/misc/misc.hpp"
#include "../../features/visuals/pixelsurf_db.hpp"
#include "../features/movement/moveutil.h"
#include "../../menu/config/config.hpp"
#include "../../sdk/math/math.hpp"
#include "../../sdk/sdk.hpp"
#include "../hooks.hpp"

extern void update_media_tag();

bool __fastcall sdk::hooks::create_move::create_move(registers, float sampletime, c_usercmd* cmd) {
	g::last_hook = "create_move";
	bool ret = sdk::hooks::create_move::ofunc(ecx, edx, sampletime, cmd);
	if (!cmd || !cmd->command_number)
		return ret;

	g::local = static_cast<player_t*>(interfaces::ent_list->get_client_entity(interfaces::engine->get_local_player()));
	g::cmd = cmd;

	MovementHooks->pre(cmd);

	prediction::backup_originals(cmd);
	prediction::begin(cmd);

	aimbot::run(cmd);
	backtrack.on_move(cmd);

	prediction::end();

	MovementHooks->post(cmd);
	features::visuals::pixelsurf_db::on_create_move();

	update_media_tag();

	if (!interfaces::prediction || !interfaces::prediction->split)
		return ret;

	if (interfaces::prediction->split->commands_predicted > 1)
		prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);

	if (interfaces::client_state && interfaces::client_state->net_channel != nullptr) {
		if (c::backtrack::fake_latency)
			fakeping.update_sequence(interfaces::client_state->net_channel);
		else
			fakeping.clear_sequence();
	}

	return false;
}