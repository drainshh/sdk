#include "../hooks.hpp"
#include "../../features/visuals/visuals.hpp"
#include "../../features/misc/misc.hpp"
#include "../../menu/config/config.hpp"

void __fastcall sdk::hooks::override_view::override_view( void* _this, void* _edx, view_setup_t* setup ) {
	if (c::misc::enable_fov && g::local && g::local->is_alive() && !g::local->is_scoped())
		setup->fov += c::misc::field_of_view;

	features::visuals::motion_blur(setup);
	features::visuals::camera_fix(setup);
	features::misc::view_model();

	ofunc( interfaces::client_mode, _this, setup );
}

bool __stdcall sdk::hooks::is_depth_of_field_enabled::is_depth_of_field_enabled() {
	features::visuals::motion_blur(nullptr);
	features::visuals::camera_fix(nullptr);
	return false;
}