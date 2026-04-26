#pragma once

namespace sdk::hooks::particle_collection_simulate {
    using fn = void(__thiscall*)(void*);
    inline fn ofunc = nullptr;

    void __fastcall hook(void* ecx, void* edx);
}