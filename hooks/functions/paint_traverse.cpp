#include "../hooks.hpp"
#include "../../features/visuals/visuals.hpp"
#include "../../features/scripts/scripts.hpp"
#include "../../features/misc/misc.hpp"
#include "../../features/movement/movement.hpp"
#include "../../features/movement/moveutil.h"
#include "../../features/visuals/pixelsurf_db.hpp"

void __stdcall sdk::hooks::paint_traverse::paint_traverse(unsigned int panel, bool force_repaint, bool allow_force) {
	g::last_hook = "paint_traverse";
	auto panel_to_draw = fnv::hash(interfaces::panel->get_panel_name(panel));

	switch (panel_to_draw) {
	case fnv::hash("MatSystemTopPanel"):
		ofunc(interfaces::panel, panel, force_repaint, allow_force);
		break;
	case fnv::hash("FocusOverlayPanel"):

		g::local = static_cast<player_t*>(interfaces::ent_list->get_client_entity(interfaces::engine->get_local_player()));

		//World To Screen;
		if (interfaces::engine->is_in_game() && interfaces::engine->is_connected() && g::local) {
			for (int i = 0; i < 4; i++)
				for (int j = 0; j < 4; j++)
					math::view_matrix[i][j] = interfaces::engine->world_to_screen_matrix()[i][j];
		}

		//ImGui Overlay Rendering
		im_render.add_to_render([]() {
			features::visuals::player::run();
			features::misc::force_crosshair();
			features::misc::sniper_crosshair();
			features::movement::velocity_indicator();
			features::movement::velocity_indicator_anim();
			features::movement::stamina_indicator();
			features::movement::indicators();
			features::visuals::draw_hit_markers();
			EdgebugAssistant.draw();
			RouteStudio.draw();
			recorder->drawroute();
			features::visuals::pixelsurf_db::on_paint_traverse();
			});
		ofunc(interfaces::panel, panel, force_repaint, allow_force);

		break;

	case fnv::hash("GameConsole"): case fnv::hash("CompletionList"):

		features::visuals::console_vis = true;
		ofunc(interfaces::panel, panel, force_repaint, allow_force);
		features::visuals::console_vis = false;
		return;
	}

	ofunc(interfaces::panel, panel, force_repaint, allow_force);
}
