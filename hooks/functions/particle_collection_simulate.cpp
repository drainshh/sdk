#include "../hooks.hpp"
#include "../functions/particle_collection_simulate.h"

struct particle_system_definition_t {
    unsigned char pad0[308];
    char* name;
};

struct particle_collection_t {
    unsigned char pad0[48];
    int active_particles;
    unsigned char pad1[12];
    void* def_ref; // may differ in your SDK, so don't use if crashes
};
