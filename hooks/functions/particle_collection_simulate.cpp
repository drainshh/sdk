#include "../hooks.hpp"
#include "../functions/particle_collection_simulate.h"

namespace sdk::hooks::particle_collection_simulate {

    void __fastcall hook(void* ecx, void* edx)
    {
        ofunc(ecx);

        static bool once = false;
        if (!once) {
            debug::log("PARTICLE SIM FIRST HIT ecx=%p", ecx);
            once = true;
        }
    }

}
