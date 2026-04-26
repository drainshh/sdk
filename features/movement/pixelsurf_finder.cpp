#include "pixelsurf_finder.hpp"

#include "../../menu/config/config.hpp"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace features::movement::pixelsurf_finder {
	namespace {
		inline std::vector<point_t> g_last_points{};
		inline std::string g_last_map{};
		inline vec3_t g_last_origin{};
		inline int g_last_scan_tick = -1;

		constexpr float k_ground_normal_min = 0.7f;
		constexpr float k_wall_normal_limit = 0.15f;
		constexpr float k_ground_scan_height = 48.0f;
		constexpr float k_ground_scan_depth = 192.0f;
		constexpr float k_edge_sample_radius = 14.0f;
		constexpr float k_edge_sample_depth = 26.0f;
		constexpr float k_edge_height_epsilon = 3.5f;
		constexpr float k_min_point_spacing = 18.0f;
		constexpr float k_refresh_distance = 40.0f;
		constexpr int k_refresh_ticks = 12;
		constexpr float k_brush_align_offset = 15.97803f;
		constexpr float k_displacement_align_offset = 16.001f;

		vec3_t player_mins(bool ducking) {
			if (interfaces::game_movement)
				return interfaces::game_movement->get_player_mins(ducking);

			return vec3_t(-16.0f, -16.0f, 0.0f);
		}

		vec3_t player_maxs(bool ducking) {
			if (interfaces::game_movement)
				return interfaces::game_movement->get_player_maxs(ducking);

			return ducking ? vec3_t(16.0f, 16.0f, 54.0f) : vec3_t(16.0f, 16.0f, 72.0f);
		}

		bool trace_player_hull(const vec3_t& start, const vec3_t& end, bool ducking, trace_t& trace) {
			if (!interfaces::trace_ray)
				return false;

			ray_t ray;
			ray.initialize(start, end, player_mins(ducking), player_maxs(ducking));

			trace_world_only filter;
			interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID_BRUSHONLY, &filter, &trace);
			return true;
		}

		bool trace_ground_ray(const vec3_t& start, trace_t& trace) {
			if (!interfaces::trace_ray)
				return false;

			ray_t ray;
			ray.initialize(start + vec3_t(0.0f, 0.0f, k_ground_scan_height), start - vec3_t(0.0f, 0.0f, k_ground_scan_depth));

			trace_world_only filter;
			interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID_BRUSHONLY, &filter, &trace);
			return trace.flFraction < 1.0f && trace.plane.normal.z >= k_ground_normal_min;
		}

		bool trace_edge_ray(const vec3_t& start, trace_t& trace) {
			if (!interfaces::trace_ray)
				return false;

			ray_t ray;
			ray.initialize(start + vec3_t(0.0f, 0.0f, 2.0f), start - vec3_t(0.0f, 0.0f, k_edge_sample_depth));

			trace_world_only filter;
			interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID_BRUSHONLY, &filter, &trace);
			return trace.flFraction < 1.0f && trace.plane.normal.z >= k_ground_normal_min;
		}

		bool fits_player(const vec3_t& pos, bool ducking) {
			trace_t trace{};
			if (!trace_player_hull(pos + vec3_t(0.0f, 0.0f, 2.0f), pos + vec3_t(0.0f, 0.0f, 2.0f), ducking, trace))
				return false;

			if (trace.startSolid || trace.allsolid)
				return false;

			trace_t ground_trace{};
			return trace_edge_ray(pos, ground_trace);
		}

		bool find_nearby_wall(const vec3_t& ground_pos, vec3_t& out_normal, vec3_t& out_hit, bool& out_displacement) {
			if (!interfaces::trace_ray)
				return false;

			const std::array<vec3_t, 8> directions = {
				vec3_t(1.0f, 0.0f, 0.0f),
				vec3_t(-1.0f, 0.0f, 0.0f),
				vec3_t(0.0f, 1.0f, 0.0f),
				vec3_t(0.0f, -1.0f, 0.0f),
				vec3_t(0.7071f, 0.7071f, 0.0f),
				vec3_t(-0.7071f, 0.7071f, 0.0f),
				vec3_t(0.7071f, -0.7071f, 0.0f),
				vec3_t(-0.7071f, -0.7071f, 0.0f)
			};

			trace_world_only filter;
			const vec3_t scan_start = ground_pos + vec3_t(0.0f, 0.0f, 32.0f);
			float best_distance = FLT_MAX;
			bool found = false;

			for (const auto& dir : directions) {
				trace_t trace{};
				ray_t ray;
				ray.initialize(scan_start, scan_start + (dir * 24.0f));
				interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID_BRUSHONLY, &filter, &trace);

				if (trace.flFraction >= 1.0f)
					continue;

				if (std::fabs(trace.plane.normal.z) > k_wall_normal_limit)
					continue;

				const float distance = (trace.end - scan_start).length();
				if (distance >= best_distance)
					continue;

				best_distance = distance;
				out_normal = trace.plane.normal;
				out_hit = trace.end;
				out_displacement = std::strstr(trace.surface.name, "displacement") != nullptr;
				found = true;
			}

			return found;
		}

		bool is_edge_surface(const vec3_t& candidate_pos, float ground_z) {
			const std::array<vec3_t, 5> offsets = {
				vec3_t(0.0f, 0.0f, 0.0f),
				vec3_t(k_edge_sample_radius, 0.0f, 0.0f),
				vec3_t(-k_edge_sample_radius, 0.0f, 0.0f),
				vec3_t(0.0f, k_edge_sample_radius, 0.0f),
				vec3_t(0.0f, -k_edge_sample_radius, 0.0f)
			};

			int supported = 0;
			int unsupported = 0;

			for (const auto& offset : offsets) {
				trace_t trace{};
				if (trace_edge_ray(candidate_pos + offset, trace) &&
					std::fabs(trace.end.z - ground_z) <= k_edge_height_epsilon) {
					++supported;
				}
				else {
					++unsupported;
				}
			}

			return supported >= 2 && unsupported >= 1;
		}

		void add_candidate(std::vector<point_t>& points, const point_t& candidate) {
			for (auto& existing : points) {
				if (existing.pos.distance_to(candidate.pos) > k_min_point_spacing)
					continue;

				if (candidate.score < existing.score)
					existing = candidate;
				return;
			}

			points.push_back(candidate);
		}
	}

	void reset() {
		g_last_points.clear();
		g_last_map.clear();
		g_last_origin = {};
		g_last_scan_tick = -1;
	}

	void update() {
		if (!c::movement::pscalc_finder_enable || !g::local || !interfaces::engine || !interfaces::engine->is_in_game() || !g::local->is_alive()) {
			reset();
			return;
		}

		const std::string current_map = interfaces::engine->get_level_name() ? interfaces::engine->get_level_name() : "";
		const vec3_t player_origin = g::local->origin();

		if (current_map == g_last_map &&
			player_origin.distance_to(g_last_origin) <= k_refresh_distance &&
			g_last_scan_tick != -1 &&
			interfaces::globals->tick_count - g_last_scan_tick < k_refresh_ticks) {
			return;
		}

		std::vector<point_t> scanned_points;
		const float max_radius = std::clamp(c::movement::pscalc_finder_radius, 64.0f, 1024.0f);
		const int max_points = std::clamp(c::movement::pscalc_finder_max_points, 1, 64);
		const int radial_steps = std::clamp(static_cast<int>(max_radius / 48.0f), 1, 8);
		const float angle_step = 360.0f / 24.0f;

		for (int radial_step = 1; radial_step <= radial_steps; ++radial_step) {
			const float radius = (std::min)(max_radius, radial_step * 48.0f);

			for (float angle = 0.0f; angle < 360.0f; angle += angle_step) {
				const float radians = deg2rad(angle);
				const vec3_t sample_dir(std::cos(radians), std::sin(radians), 0.0f);
				const vec3_t sample_origin = player_origin + (sample_dir * radius);

				trace_t ground_trace{};
				if (!trace_ground_ray(sample_origin, ground_trace))
					continue;

				vec3_t wall_normal{};
				vec3_t wall_hit{};
				bool is_displacement = false;
				if (!find_nearby_wall(ground_trace.end, wall_normal, wall_hit, is_displacement))
					continue;

				point_t candidate{};
				candidate.normal = wall_normal;
				candidate.pos = wall_hit + (wall_normal * (is_displacement ? k_displacement_align_offset : k_brush_align_offset));
				candidate.pos.z = ground_trace.end.z;

				if (!is_edge_surface(candidate.pos, ground_trace.end.z))
					continue;

				candidate.stand = fits_player(candidate.pos, false);
				candidate.duck = fits_player(candidate.pos, true);
				if (!candidate.stand && !candidate.duck)
					continue;

				candidate.score = radius;
				if (candidate.stand)
					candidate.score -= 4.0f;
				if (candidate.duck)
					candidate.score -= 2.0f;

				add_candidate(scanned_points, candidate);
			}
		}

		std::sort(scanned_points.begin(), scanned_points.end(), [](const point_t& lhs, const point_t& rhs) {
			return lhs.score < rhs.score;
		});

		if (static_cast<int>(scanned_points.size()) > max_points)
			scanned_points.resize(max_points);

		g_last_points = std::move(scanned_points);
		g_last_map = current_map;
		g_last_origin = player_origin;
		g_last_scan_tick = interfaces::globals->tick_count;
	}

	const std::vector<point_t>& last_points() {
		return g_last_points;
	}
}
