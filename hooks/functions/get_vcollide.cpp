#include "../hooks.hpp"
#include "../../features/visuals/visuals.hpp"

void* __fastcall sdk::hooks::get_vcollide::get_vcollide(void* ecx, void* edx, int model_index)
{
    static int call_count = 0;

    if (call_count < 20) {
        debug::log("GETVCOLLIDE CALL #%d model_index=%d override=%d loaded=%d",
            call_count,
            model_index,
            features::weather::should_override_vcollide(model_index) ? 1 : 0,
            features::weather::getv_collideble() ? 1 : 0
        );
        ++call_count;
    }

    if (features::weather::should_override_vcollide(model_index)) {
        void* collide = features::weather::getv_collideble();

        debug::log("GETVCOLLIDE: overriding model_index=%d collideable=%p",
            model_index, collide);

        if (collide)
            return collide;
    }

    return ofunc(ecx, edx, model_index);
}