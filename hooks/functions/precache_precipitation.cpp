#include "../hooks.hpp"
#include "precache_precipitation.h"

namespace sdk::hooks::precache_precipitation {

    void* __cdecl hook(void* a1, void* a2)
    {
        debug::log("PRECACHE PRECIPITATION HIT a1=%p a2=%p", a1, a2);

        return ofunc(a1, a2);
    }

}