#pragma once

#include "../../sdk/sdk.hpp"

#include <vector>

namespace features::movement::pixelsurf_finder {
	struct point_t {
		vec3_t pos{};
		vec3_t normal{};
		bool stand = true;
		bool duck = true;
		float score = 0.f;
	};

	void reset();
	void update();
	const std::vector<point_t>& last_points();
}
