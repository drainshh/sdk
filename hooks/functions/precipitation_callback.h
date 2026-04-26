#pragma once

namespace sdk::hooks::precipitation_callback {
    using fn = void* (__cdecl*)(void*, void*);
    inline fn ofunc = nullptr;

    void* __cdecl hook(void* a1, void* a2);
}