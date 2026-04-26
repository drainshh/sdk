#include "../../features/aimbot/aimbot.hpp"
#include "../../features/visuals/visuals.hpp"
#include "../../features/misc/misc.hpp"
#include "../../menu/config/config.hpp"
#include "../../sdk/math/math.hpp"
#include "../../sdk/sdk.hpp"
#include "../hooks.hpp"

#define MAX_COORD_FLOAT  16384.0f
#define MIN_COORD_FLOAT -MAX_COORD_FLOAT

int __fastcall sdk::hooks::list_leaves_in_box::list_leaves_in_box(
    void* bsp, void* edx, const vec3_t& mins, const vec3_t& maxs,
    unsigned short* list, int list_max)
{
    static auto list_leaves = find_pattern("client.dll", "56 52 FF 50 18") + 5;
    auto info = *reinterpret_cast<renderable_info_t**>(
        reinterpret_cast<uintptr_t>(_AddressOfReturnAddress()) + 0x14);

    // Validate return address & ensure chams are enabled
    if (!_ReturnAddress() || _ReturnAddress() != list_leaves || !c::chams::invisible_chams)
        return ofunc(bsp, mins, maxs, list, list_max);

    // Ensure valid renderable info
    if (!info || !info->renderable)
        return ofunc(bsp, mins, maxs, list, list_max);

    // Get entity from renderable
    auto entity = call_vfunc<entity_t*>(info->renderable - 4, 7);
    if (!entity)
        return ofunc(bsp, mins, maxs, list, list_max);

    // Ensure only player entities are processed
    if (entity->client_class()->class_id != class_ids::ccsplayer)
        return ofunc(bsp, mins, maxs, list, list_max);

    // Get visibility state for perfect chams transition
    bool is_enemy = entity->team() != g::local->team();
    bool is_visible = entity->spotted();

    // Apply rendering logic for hidden and regular chams
    if (is_enemy) {
        info->flags &= ~0x100; // Disable occlusion
        info->flags2 |= 0x40;  // Force rendering through walls
        info->flags |= 0x80; // Enable backface rendering

        /*if (!is_visible) {
            info->flags |= 0x200;
        }*/
    }

    static vec3_t map_min(MIN_COORD_FLOAT, MIN_COORD_FLOAT, MIN_COORD_FLOAT);
    static vec3_t map_max(MAX_COORD_FLOAT, MAX_COORD_FLOAT, MAX_COORD_FLOAT);

    return ofunc(bsp, map_min, map_max, list, list_max);
}