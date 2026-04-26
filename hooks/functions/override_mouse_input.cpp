#include "../hooks.hpp"
#include "../../menu/config/config.hpp"
#include "../../features/movement/movement.hpp"
#include "../../features/movement/moveutil.h"

void __fastcall sdk::hooks::override_mouse_input::override_mouse_input(void* this_, int edx, float* x, float* y) {

	if (c::movehelp::edgebug_lock && features::movement::detect_data.ticks_left) {
		switch (c::movement::edge_bug_lock_type) {
		case 0:
			*x *= 1 - c::movement::edge_bug_lock_amount / 100;
			*y *= 1 - c::movement::edge_bug_lock_amount / 100;
			break;
		case 1:
			*x *= 0;
			break;
		}
	}
	else if (c::movehelp::pixelsurf_lock && features::movement::should_ps) {
		*x *= 1 - c::movement::ps_lock_amount / 100;
		*y *= 1 - c::movement::ps_lock_amount / 100;
	}

	return ofunc(this_, edx, x, y);
}