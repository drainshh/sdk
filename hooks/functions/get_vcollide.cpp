#include "../hooks.hpp"
#include "../../features/visuals/visuals.hpp"

void* __fastcall sdk::hooks::get_vcollide::get_vcollide(void* ecx, void* edx, int model_index)
{
    if (model_index == -1) {
        static int printed = 0;
        void* collide = features::weather::getv_collideble();

        if (printed < 20) {
            debug::log("GETVCOLLIDE -1 HIT collide=%p", collide);
            ++printed;
        }

        return collide;
    }

    return ofunc(ecx, edx, model_index);
}
