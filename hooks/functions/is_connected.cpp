#include "../hooks.hpp"
#include "../../menu/config/config.hpp"

bool __stdcall sdk::hooks::is_connected::is_connected() {
	return ofunc(interfaces::engine);
}