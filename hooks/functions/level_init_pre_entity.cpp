#include "../hooks.hpp"
#include "../../menu/config/config.hpp"
#include "../../features/misc/misc.hpp"
#include "../../features/visuals/visuals.hpp"
#include "../../features/movement/prediction/prediction.hpp"
#include "../../features/movement/moveutil.h"

void __stdcall sdk::hooks::level_init_pre_entity::level_init_pre_entity(const char* map) {
	g::last_hook = "level_init_pre_entity";
	debug::log("Level Init: %s", map);
	EdgebugAssistant.ticks_left = 0;
	EdgebugAssistant.strafing = false;
	features::weather::reset_weather(true);

	ofunc(interfaces::client, map);
}

void __stdcall sdk::hooks::level_shutdown::level_shutdown() {
	g::last_hook = "level_shutdown";
	debug::log("Level Shutdown");
	EdgebugAssistant.ticks_left = 0;
	EdgebugAssistant.strafing = false;
	features::weather::reset_weather(true);
	ofunc(interfaces::client);
}
