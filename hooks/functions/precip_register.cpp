#include "../../hooks/hooks.hpp"
#include "precip_register.h"

namespace sdk::hooks::precip_register {

    void __fastcall hook(void* ecx, void* edx, void* a1, void* a2, void* a3)
    {
        debug::log("PRECIP REGISTER HIT ecx=%p a1=%p a2=%p a3=%p", ecx, a1, a2, a3);

        ofunc(ecx, a1, a2, a3);
    }
<<<<<<< HEAD
}
=======
}
>>>>>>> 66e22c9 (hello)
