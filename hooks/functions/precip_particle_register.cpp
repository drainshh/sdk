#include "../../hooks/hooks.hpp"
#include "precip_particle_register.h"

namespace sdk::hooks::precip_particle_register {

    void __fastcall hook(void* ecx, void* edx, void* a1, void* a2, void* a3)
    {
        static bool once = false;
        if (!once) {
            debug::log("PRECIP PARTICLE REGISTER HIT ecx=%p a1=%p a2=%p a3=%p", ecx, a1, a2, a3);
            once = true;
        }

        ofunc(ecx, a1, a2, a3);
    }

}