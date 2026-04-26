#include "../hooks.hpp"
#include "../../features/visuals/visuals.hpp"

char __stdcall sdk::hooks::insecure::insecure_bypass()
{
	if (c::misc::insecure_bypass) {
		return true;
	}
	return sdk::hooks::insecure::ofunc();
}