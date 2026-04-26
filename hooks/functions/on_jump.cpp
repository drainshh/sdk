#include "../hooks.hpp"
#include "../../menu/config/config.hpp"
#include "../../features/misc/misc.hpp"

void __stdcall sdk::hooks::on_jump::on_jump(float stamina) {

    ofunc(stamina);

}