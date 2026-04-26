#pragma once

namespace sdk::hooks::precache_precipitation {
    using fn = void* (__cdecl*)(void*, void*);
    inline fn ofunc = nullptr;

    void* __cdecl hook(void* a1, void* a2);
}