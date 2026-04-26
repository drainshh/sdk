#include "../hooks.hpp"
#include "../../features/visuals/visuals.hpp"

void* __fastcall sdk::hooks::get_vcollide::get_vcollide(void* ecx, void* edx, int model_index)
{
    if (model_index == -1)
        return features::weather::getv_collideble();

    return ofunc(ecx, edx, model_index);
}
