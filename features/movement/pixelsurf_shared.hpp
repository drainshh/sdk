#pragma once

#include <string>

namespace features::movement::pixelsurf_shared {

	constexpr float k_tick_rate = 64.0f;
	constexpr float k_dt = 1.0f / 64.0f;
	constexpr float k_gravity = 800.0f;
	constexpr float k_gravity_per_tick = k_gravity * k_dt;
	constexpr float k_jump_impulse = 276.992f;
	constexpr float k_jump_launch_lift = 0.1953125f;
	constexpr float k_stand_anchor_z = 9.0f;
	constexpr float k_duck_anchor_z = 8.9999704f;

	inline float anchor_z_for_stance(const bool ducking) {
		return ducking ? k_duck_anchor_z : k_stand_anchor_z;
	}

	inline float discrete_z_after_ticks(const int airborne_ticks) {
		float z = k_jump_launch_lift;
		float vz = k_jump_impulse;
		for (int tick = 0; tick < airborne_ticks; ++tick) {
			z += vz * k_dt;
			if (tick > 0)
				vz -= k_gravity_per_tick;
		}
		return z;
	}

	inline std::string discrete_formula_summary() {
		return "z += vz*dt, gravity after airborne ticks, dt=1/64, gravity=800, jump=276.992";
	}
}
