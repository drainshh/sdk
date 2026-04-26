#include "../hooks.hpp"
#include "../functions/particle_collection_simulate.h"

struct c_utl_string_lite {
    char* m_buffer;
};

struct particle_system_definition_t {
    unsigned char pad0[308];
    c_utl_string_lite m_name;
};

template <typename T>
struct c_utl_reference_lite {
    T* m_obj;
};

struct particle_collection_t {
    unsigned char pad0[48];
    int m_active_particles;
    unsigned char pad1[12];
    c_utl_reference_lite<particle_system_definition_t> m_def;
    unsigned char pad2[60];
    particle_collection_t* m_parent;
};

namespace sdk::hooks::particle_collection_simulate {

    void __fastcall hook(void* ecx, void* edx)
    {
        ofunc(ecx);

        auto* collection = reinterpret_cast<particle_collection_t*>(ecx);
        if (!collection)
            return;

        auto* root = collection;
        for (int i = 0; root && root->m_parent && i < 16; ++i)
            root = root->m_parent;

        if (!root || !root->m_def.m_obj || !root->m_def.m_obj->m_name.m_buffer)
            return;

        const char* name = root->m_def.m_obj->m_name.m_buffer;

        static int printed = 0;
        if (printed < 80) {
            debug::log("PARTICLE SIM name=%s active=%d root=%p child=%p",
                name,
                collection->m_active_particles,
                root,
                collection);
            ++printed;
        }

        if (!strcmp(name, "rain") || !strcmp(name, "rain_storm") || !strcmp(name, "snow") || !strcmp(name, "ash")) {
            static int precip_printed = 0;
            if (precip_printed < 40) {
                debug::log("PRECIP PARTICLE FOUND name=%s active=%d", name, collection->m_active_particles);
                ++precip_printed;
            }
        }
    }

}
