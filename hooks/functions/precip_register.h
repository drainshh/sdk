#pragma once

namespace sdk::hooks::precip_register {
    using fn = void(__thiscall*)(void*, void*, void*, void*);
    inline fn ofunc = nullptr;

    void __fastcall hook(void* ecx, void* edx, void* a1, void* a2, void* a3);
}