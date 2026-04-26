#include "../hooks.hpp"
#include "precipitation_callback.h"

namespace sdk::hooks::precipitation_callback {

    void* __cdecl hook(void* a1, void* a2)
    {
        debug::log("CPRECIPITATION CALLBACK HIT a1=%p a2=%p", a1, a2);
        return ofunc(a1, a2);
    }

}