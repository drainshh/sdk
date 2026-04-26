#include "../hooks.hpp"
#include "../functions/particle_collection_simulate.h"

namespace sdk::hooks::particle_collection_simulate {

    void __fastcall hook(void* ecx, void* edx)
    {
        static bool once = false;

        ofunc(ecx);

        if (!once) {
            debug::log("PARTICLE SIM FIRST HIT ecx=%p", ecx);
            once = true;
        }
    }

}
