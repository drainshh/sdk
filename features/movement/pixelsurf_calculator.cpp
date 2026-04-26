#include "pixelsurf_calculator.hpp"
#include "pixelsurf_shared.hpp"

#include "../../menu/config/config.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace features::movement::pixelsurf_calculator {
	namespace {
		struct sim_input_t {
			bool jump = false;
			bool duck = false;
			bool move = false;
			vec3_t wish_dir{};
			float wish_speed = 0.0f;
		};

		struct planned_action_t {
			action_type_t type = action_type_t::stand_jump;
			std::string sequence_name{};
			std::string combo_label{};
			int priority = 0;
			int duck_start_tick = -1;
			int duck_end_tick = -1;
			int min_prune_ticks = 24;
			int max_stage_ticks = 64;
			bool require_duck_terminal = false;
		};

		struct target_eval_t {
			float horizontal_distance = FLT_MAX;
			float vertical_distance = FLT_MAX;
			float score_distance = FLT_MAX;
			float wall_alignment = 0.0f;
			float raw_candidate_z = 0.0f;
			float grounded_candidate_z = 0.0f;
			float raw_target_z = 0.0f;
			float applied_anchor_z = 0.0f;
			float compared_candidate_z = 0.0f;
			float compared_target_z = 0.0f;
			float height_epsilon = 0.0f;
			target_kind_t stage_kind = target_kind_t::pixelsurf_point;
			bool matched = false;
			bool lax_match = false;
			bool radius_match = false;
			bool lax_radius_match = false;
			bool height_match = false;
			bool lax_height_match = false;
			bool edge_ready = false;
			bool near_wall = false;
			bool contact_ready = false;
		};

		struct stage_hit_debug_t {
			int stage_index = -1;
			int tick = -1;
			vec3_t candidate_pos{};
			vec3_t target_pos{};
			float xy_delta = FLT_MAX;
			float z_delta = FLT_MAX;
			float airborne_sample_z = 0.0f;
			float landing_position_z = 0.0f;
			float landed_ground_z = 0.0f;
			float target_floor_z = 0.0f;
			float compared_floor_z = 0.0f;
			float resolved_floor_z = 0.0f;
			float previous_floor_z = 0.0f;
			float start_floor_z = 0.0f;
			float expected_floor_rise = 0.0f;
			float actual_floor_rise = 0.0f;
			float raw_candidate_z = 0.0f;
			float grounded_candidate_z = 0.0f;
			float raw_target_z = 0.0f;
			float applied_anchor_z = 0.0f;
			float compared_candidate_z = 0.0f;
			float compared_target_z = 0.0f;
			float height_epsilon = 0.0f;
			target_kind_t stage_kind = target_kind_t::pixelsurf_point;
			bool landing_detected = false;
			bool stage_window_entered = false;
			bool contact_detected = false;
			bool height_match = false;
			bool radius_match = false;
			bool edge_ready = false;
			int window_enter_tick = -1;
			int window_exit_tick = -1;
			int landing_tick = -1;
			bool stale_ground_reused = false;
			bool floor_probe_hit = false;
			bool floor_probe_height_match = false;
			bool floor_probe_disagreed_with_ground = false;
			bool landed_on_lower_floor = false;
			bool target_local_probe_mismatch = false;
			std::string compared_floor_source;
			std::string reject_reason;
		};

		struct floor_probe_result_t {
			bool hit = false;
			bool height_match = false;
			vec3_t sample_pos{};
			float surface_z = 0.0f;
			float target_delta = FLT_MAX;
			std::string source = "none";
		};

		struct floor_landing_snapshot_t {
			bool valid = false;
			bool target_floor_probe = false;
			bool stale_ground_reused = false;
			bool floor_probe_hit = false;
			bool floor_probe_height_match = false;
			bool floor_probe_disagreed_with_ground = false;
			bool landed_on_lower_floor = false;
			bool target_local_probe_mismatch = false;
			int tick = -1;
			vec3_t landing_pos{};
			float airborne_sample_z = 0.0f;
			float landing_position_z = 0.0f;
			float landed_ground_z = 0.0f;
			float target_floor_z = 0.0f;
			float compared_floor_z = 0.0f;
			float resolved_floor_z = 0.0f;
			float previous_floor_z = 0.0f;
			float start_floor_z = 0.0f;
			float expected_floor_rise = 0.0f;
			float actual_floor_rise = 0.0f;
			float height_delta = FLT_MAX;
			std::string compared_using = "none";
		};

		struct height_match_t {
			bool strict_match = false;
			bool lax_match = false;
			float vertical_distance = FLT_MAX;
			float applied_anchor_z = 0.0f;
			float compared_candidate_z = 0.0f;
			float compared_target_z = 0.0f;
			float strict_epsilon = 0.0f;
			float lax_epsilon = 0.0f;
		};

		struct action_run_t {
			bool success = false;
			bool landed = false;
			bool generated_action = false;
			bool jumpbug_triggered = false;
			bool made_progress = false;
			bool had_airborne_phase = false;
			int ticks_used = 0;
			float distance = FLT_MAX;
			float progress = 0.0f;
			float max_upward_velocity = 0.0f;
			float horizontal_displacement = 0.0f;
			sim_state_t end_state{};
			std::vector<event_t> events;
			std::string reject_reason;
			stage_hit_debug_t stage_debug{};
		};

		struct stage_boundary_t {
			int stage_index = 0;
			target_kind_t target_kind = target_kind_t::floor_point;
			vec3_t target_pos{};
			sim_state_t start_state{};
			sim_state_t end_state{};
			bool success = false;
			int ticks_used = 0;
		};

		struct combo_run_t {
			bool success = false;
			int current_target_index = 0;
			int current_stage_ticks = 0;
			int segments_tested = 0;
			int ticks_used = 0;
			float score = FLT_MAX;
			std::string failure_reason;
			std::string stage_transition_summary;
			std::string propagation_warning;
			result_t result{};
			stage_hit_debug_t failed_stage_debug{};
			std::vector<stage_boundary_t> stage_boundaries{};
		};

		struct formatted_sequence_t {
			std::string summary;
			std::string full;
			int hidden = 0;
		};

		struct route_validation_t {
			bool valid = false;
			bool has_airborne_phase = false;
			bool has_pixelsurf_event = false;
			bool has_pixelsurf_target = false;
			int action_count = 0;
			float max_upward_velocity = 0.0f;
			float horizontal_displacement = 0.0f;
			std::string failure_reason;
		};

		struct solver_job_t {
			bool active = false;
			request_t request{};
			std::vector<target_t> targets{};
			std::vector<std::vector<planned_action_t>> stage_alphabets{};
			sim_state_t start_state{};
			debug_stats_t stats{};
			std::vector<result_t> successes{};
			result_t best_success{};
			result_t best_partial{};
			bool has_success = false;
			bool has_partial = false;
			int exact_stage_count = 0;
			int current_pass = 1;
			std::size_t combo_cursor = 0;
			int progress_update_interval = 32;
			int stage_tick_budget = 64;
			int total_tick_budget = 256;
			int max_nodes = 50000;
			std::string log_path;
			std::chrono::steady_clock::time_point started_at{};
		};

		void append_log_line(solver_job_t& job, const std::string& line);
		void refresh_elapsed(solver_job_t& job);
		std::string vec3_to_string(const vec3_t& value);
		float stage_boundary_z(const sim_state_t& state);
		bool branch_height_matches(const float base_z, const std::vector<float>& offsets, const float target_z, const float epsilon);
		std::vector<float> landing_z_offsets_for_action(const target_t& target, const action_type_t action);
		target_eval_t validate_floor_stage(const sim_state_t& state, const target_t& target);
		target_eval_t validate_pixelsurf_stage(const sim_state_t& state, const target_t& target, const action_type_t action);
		void publish_progress_snapshot(const solver_job_t& job);
		void publish_final_snapshot(
			const solver_job_t& job,
			const result_t& result,
			const std::vector<result_t>& results,
			const bool update_render_targets);
		void sync_visible_state();
		void stop_worker(const bool wait_for_join);

		inline result_t g_last_result{};
		inline std::vector<result_t> g_last_results{};
		inline std::vector<target_t> g_last_targets{};
		inline debug_stats_t g_last_stats{};

		inline result_t g_render_result{};
		inline std::vector<result_t> g_render_results{};
		inline std::vector<target_t> g_render_targets{};

		struct shared_snapshot_t {
			bool ready = false;
			bool replace_results = false;
			bool replace_render_targets = false;
			result_t result{};
			std::vector<result_t> results{};
			std::vector<target_t> targets{};
			debug_stats_t stats{};
		};

		inline std::mutex g_snapshot_mutex{};
		inline shared_snapshot_t g_pending_snapshot{};
		inline std::thread g_solver_thread{};
		inline std::atomic<bool> g_solver_running = false;
		inline std::atomic<bool> g_cancel_requested = false;
		inline std::atomic<bool> g_calculation_requested = false;
		inline int g_calculate_request_counter = 0;

		constexpr float k_sim_dt = features::movement::pixelsurf_shared::k_dt;
		constexpr float k_jump_impulse = features::movement::pixelsurf_shared::k_jump_impulse;
		constexpr float k_gravity = features::movement::pixelsurf_shared::k_gravity;
		constexpr float k_jump_launch_lift = features::movement::pixelsurf_shared::k_jump_launch_lift;
		constexpr float k_jump_stamina_gain = 23.222f;
		constexpr float k_stamina_decay_per_tick = 0.94f;
		constexpr float k_ground_friction = 5.2f;
		constexpr float k_stop_speed = 80.0f;
		constexpr float k_ground_accelerate = 5.5f;
		constexpr float k_air_accelerate = 12.0f;
		constexpr float k_air_wishspeed_cap = 30.0f;
		constexpr float k_ground_wishspeed = 250.0f;
		constexpr float k_duck_speed_scale = 0.34f;
		constexpr float k_ground_normal_min = 0.7f;
		constexpr float k_ground_snap_distance = 8.0f;
		constexpr float k_ground_epsilon = 2.0f;
		constexpr float k_floor_z_epsilon = 4.0f;
		constexpr float k_floor_lax_z_epsilon = 6.0f;
		constexpr float k_edge_sample_radius = 14.0f;
		constexpr float k_edge_sample_depth = 18.0f;
		constexpr float k_edge_height_epsilon = 4.0f;
		constexpr float k_ceiling_normal_limit = -0.7f;
		constexpr float k_longjump_min_speed = 200.0f;
		constexpr float k_jumpbug_window = 16.0f;
		constexpr float k_wall_normal_limit = 0.15f;
		constexpr float k_min_visible_vertical_velocity = 5.0f;
		constexpr float k_min_route_displacement = 12.0f;
		constexpr float k_min_segment_progress = 6.0f;
		constexpr int k_floor_stage_settle_ticks = 6;
		constexpr int k_pixelsurf_contact_window_ticks = 4;
		constexpr int k_stage_window_landing_grace_ticks = 8;
		constexpr float k_assist_crouch_offset = features::movement::pixelsurf_shared::k_stand_anchor_z;
		constexpr float k_assist_longjump_offset = features::movement::pixelsurf_shared::k_duck_anchor_z;
		constexpr int k_ticks_per_target_stage = 64;
		constexpr int k_internal_success_cap = 32;

		int stage_tick_budget() {
			return std::clamp(c::movement::pscalc_model_b_stage_ticks, 16, 128);
		}

		int total_tick_budget(const int stage_count) {
			const int configured = std::clamp(c::movement::pscalc_model_b_total_ticks, 64, 512);
			const int derived = std::clamp(stage_tick_budget() * (std::max)(1, stage_count), 64, 512);
			return (std::max)(configured, derived);
		}

		int combo_nodes_budget() {
			return std::clamp(c::movement::pscalc_model_b_max_nodes, 64, 200000);
		}

		int combos_per_update_budget() {
			return std::clamp(c::movement::pscalc_model_b_combos_per_frame, 1, 10000);
		}

		int min_prune_ticks_budget() {
			return std::clamp(c::movement::pscalc_min_prune_ticks, 16, 32);
		}

		float strict_z_tolerance() {
			return std::clamp(c::movement::pscalc_z_tolerance, 0.25f, 6.0f);
		}

		float lax_z_tolerance() {
			return std::clamp(c::movement::pscalc_lax_z_tolerance, 0.5f, 12.0f);
		}

		bool cancel_requested() {
			return g_cancel_requested.load(std::memory_order_relaxed);
		}

		int elapsed_time_ms(const solver_job_t& job) {
			using namespace std::chrono;
			return static_cast<int>(duration_cast<milliseconds>(steady_clock::now() - job.started_at).count());
		}

		const char* target_kind_name(const target_kind_t kind) {
			return kind == target_kind_t::floor_point ? "floor" : "pixelsurf";
		}

		bool is_ducked_action(const action_type_t action) {
			return action == action_type_t::crouch_jump ||
				action == action_type_t::minijump ||
				action == action_type_t::longjump ||
				action == action_type_t::jumpbug;
		}

		bool is_jump_like_action(const action_type_t action) {
			return action == action_type_t::stand_jump ||
				action == action_type_t::crouch_jump ||
				action == action_type_t::minijump ||
				action == action_type_t::longjump ||
				action == action_type_t::jumpbug;
		}

		std::string planned_action_name(const action_type_t action) {
			switch (action) {
			case action_type_t::stand_jump:
				return "jump";
			case action_type_t::crouch_jump:
				return "crouch jump";
			case action_type_t::minijump:
				return "minijump";
			case action_type_t::longjump:
				return "longjump";
			case action_type_t::jumpbug:
				return "jumpbug";
			default:
				return {};
			}
		}

		std::string planned_combo_name(const planned_action_t& action) {
			if (!action.combo_label.empty())
				return action.combo_label;
			return planned_action_name(action.type);
		}

		std::string planned_sequence_name(const planned_action_t& action) {
			if (!action.sequence_name.empty())
				return action.sequence_name;
			return planned_action_name(action.type);
		}

		std::string format_planned_combo(const std::vector<planned_action_t>& combo) {
			if (combo.empty())
				return "(none)";

			std::string text;
			for (std::size_t i = 0; i < combo.size(); ++i) {
				const std::string label = planned_combo_name(combo[i]);
				if (label.empty())
					continue;

				if (!text.empty())
					text.append(" -> ");
				text.append(label);
			}

			return text.empty() ? std::string("(none)") : text;
		}

		std::string action_name(const event_t& event) {
			return planned_action_name(event.action);
		}

		std::string event_name(const event_t& event) {
			switch (event.type) {
			case event_type_t::action:
				return action_name(event);
			case event_type_t::pixelsurf:
				return event.ducking ? "pixelsurf (ducked)" : "pixelsurf (stand)";
			case event_type_t::floor:
				return {};
			case event_type_t::headbang:
				return event.ducking ? "headbang (ducked)" : "headbang";
			case event_type_t::jumpbug:
				return "jumpbug";
			default:
				return {};
			}
		}

		formatted_sequence_t format_sequence(const std::vector<event_t>& events, const int max_visible = 5) {
			struct label_t {
				std::string text;
				int tick = -1;
			};

			std::vector<label_t> labels;
			labels.reserve(events.size());

			for (const auto& event : events) {
				const std::string label = event_name(event);
				if (label.empty())
					continue;

				if (!labels.empty() && labels.back().text == label && labels.back().tick == event.tick)
					continue;

				labels.push_back({ label, event.tick });
			}

			if (labels.empty())
				return { "no route", "no route", 0 };

			formatted_sequence_t formatted{};
			for (std::size_t i = 0; i < labels.size(); ++i) {
				if (i > 0)
					formatted.full.append(" -> ");
				formatted.full.append(labels[i].text);
			}

			if (static_cast<int>(labels.size()) <= max_visible) {
				formatted.summary = formatted.full;
				return formatted;
			}

			formatted.hidden = static_cast<int>(labels.size()) - max_visible;
			formatted.summary = "...";
			for (int i = static_cast<int>(labels.size()) - max_visible; i < static_cast<int>(labels.size()); ++i) {
				formatted.summary.append(" -> ");
				formatted.summary.append(labels[i].text);
			}

			return formatted;
		}

		float horizontal_distance_between(const vec3_t& lhs, const vec3_t& rhs) {
			const float dx = lhs.x - rhs.x;
			const float dy = lhs.y - rhs.y;
			return std::sqrt((dx * dx) + (dy * dy));
		}

		route_validation_t evaluate_route_validity(const result_t& result, const std::vector<target_t>& targets) {
			route_validation_t validation{};
			validation.horizontal_displacement = horizontal_distance_between(result.start_pos, result.pos);

			for (const auto& target : targets) {
				if (target.kind == target_kind_t::pixelsurf_point) {
					validation.has_pixelsurf_target = true;
					break;
				}
			}

			for (const auto& event : result.events) {
				if (event.type == event_type_t::action && is_jump_like_action(event.action)) {
					validation.action_count++;
					validation.max_upward_velocity = (std::max)(validation.max_upward_velocity, event.vel.z);
					if (event.vel.z > k_min_visible_vertical_velocity)
						validation.has_airborne_phase = true;
				}
				else if (event.type == event_type_t::jumpbug) {
					validation.has_airborne_phase = true;
				}
				else if (event.type == event_type_t::pixelsurf) {
					validation.has_pixelsurf_event = true;
				}
			}

			const auto formatted = format_sequence(result.events);
			if (!validation.has_pixelsurf_target && !validation.has_pixelsurf_event)
				validation.failure_reason = "route does not contain a pixelsurf target";
			else if (validation.action_count <= 0)
				validation.failure_reason = "no jump-based action was simulated";
			else if (validation.max_upward_velocity <= k_min_visible_vertical_velocity)
				validation.failure_reason = "all branches pruned due to no vertical movement";
			else if (!validation.has_airborne_phase)
				validation.failure_reason = "no valid airborne path found";
			else if (validation.horizontal_displacement < k_min_route_displacement && !result.success)
				validation.failure_reason = "candidate never traversed enough of the route";
			else if (formatted.full.empty() || formatted.full == "no route")
				validation.failure_reason = "no valid airborne path found";
			else
				validation.valid = true;

			return validation;
		}

		vec3_t player_mins(const bool ducking) {
			if (interfaces::game_movement)
				return interfaces::game_movement->get_player_mins(ducking);

			return vec3_t(-16.0f, -16.0f, 0.0f);
		}

		vec3_t player_maxs(const bool ducking) {
			if (interfaces::game_movement)
				return interfaces::game_movement->get_player_maxs(ducking);

			return ducking ? vec3_t(16.0f, 16.0f, 54.0f) : vec3_t(16.0f, 16.0f, 72.0f);
		}

		bool trace_player_hull(const vec3_t& start, const vec3_t& end, const bool ducking, trace_t& trace) {
			if (!interfaces::trace_ray)
				return false;

			ray_t ray;
			ray.initialize(start, end, player_mins(ducking), player_maxs(ducking));

			trace_world_only filter;
			interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID_BRUSHONLY, &filter, &trace);
			return true;
		}

		bool trace_ground_hull(
			const vec3_t& pos,
			const bool ducking,
			float& ground_z,
			trace_t* out_trace = nullptr,
			const float max_depth = k_ground_snap_distance)
		{
			trace_t trace{};
			if (!trace_player_hull(pos + vec3_t(0.0f, 0.0f, 2.0f), pos - vec3_t(0.0f, 0.0f, max_depth), ducking, trace))
				return false;

			if (trace.flFraction >= 1.0f || trace.plane.normal.z < k_ground_normal_min)
				return false;

			ground_z = trace.end.z;
			if (out_trace)
				*out_trace = trace;
			return true;
		}

		bool trace_ground_distance(
			const vec3_t& pos,
			const bool ducking,
			const float max_depth,
			float& out_distance,
			float* out_ground_z = nullptr)
		{
			trace_t trace{};
			if (!trace_player_hull(pos + vec3_t(0.0f, 0.0f, 2.0f), pos - vec3_t(0.0f, 0.0f, max_depth), ducking, trace))
				return false;

			if (trace.flFraction >= 1.0f || trace.plane.normal.z < k_ground_normal_min)
				return false;

			out_distance = (pos.z + 2.0f) - trace.end.z;
			if (out_ground_z)
				*out_ground_z = trace.end.z;
			return true;
		}

		bool trace_ground_ray(const vec3_t& pos, float& ground_z) {
			if (!interfaces::trace_ray)
				return false;

			trace_t trace{};
			ray_t ray;
			ray.initialize(pos + vec3_t(0.0f, 0.0f, 1.0f), pos - vec3_t(0.0f, 0.0f, k_edge_sample_depth));

			trace_world_only filter;
			interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID_BRUSHONLY, &filter, &trace);

			if (trace.flFraction >= 1.0f || trace.plane.normal.z < k_ground_normal_min)
				return false;

			ground_z = trace.end.z;
			return true;
		}

		bool trace_floor_surface_near_target(
			const vec3_t& xy_pos,
			const float target_z,
			float& surface_z,
			bool* out_height_match = nullptr)
		{
			if (!interfaces::trace_ray)
				return false;

			trace_t trace{};
			ray_t ray;
			const vec3_t start(xy_pos.x, xy_pos.y, target_z + 128.0f);
			const vec3_t end(xy_pos.x, xy_pos.y, target_z - 128.0f);
			ray.initialize(start, end);

			trace_world_only filter;
			interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID_BRUSHONLY, &filter, &trace);

			if (trace.flFraction >= 1.0f || trace.plane.normal.z < k_ground_normal_min)
				return false;

			surface_z = trace.end.z;
			const bool height_match = std::fabs(surface_z - target_z) <= k_floor_z_epsilon;
			if (out_height_match)
				*out_height_match = height_match;
			return true;
		}

		floor_probe_result_t probe_floor_surface_for_stage(
			const vec3_t& landing_pos,
			const target_t& target)
		{
			floor_probe_result_t best{};
			const float probe_radius = (std::min)(16.0f, (std::max)(4.0f, target.radius * 0.5f));

			auto consider_probe = [&](const vec3_t& sample_pos, const char* source) {
				float surface_z = 0.0f;
				bool height_match = false;
				if (!trace_floor_surface_near_target(sample_pos, target.pos.z, surface_z, &height_match))
					return;

				const float target_delta = std::fabs(surface_z - target.pos.z);
				const bool prefer =
					!best.hit ||
					(height_match && !best.height_match) ||
					(height_match == best.height_match && target_delta < best.target_delta);

				if (!prefer)
					return;

				best.hit = true;
				best.height_match = height_match;
				best.sample_pos = sample_pos;
				best.surface_z = surface_z;
				best.target_delta = target_delta;
				best.source = source;
			};

			// Target-local samples are checked first so a raised floor marker can correct stale
			// grounded Z from the previous lower floor when the landing is inside the route window.
			consider_probe(target.pos, "target_xy_floor_probe");
			consider_probe(landing_pos, "landing_xy_floor_probe");
			consider_probe(target.pos + vec3_t(probe_radius, 0.0f, 0.0f), "target_xy_floor_probe(+x)");
			consider_probe(target.pos + vec3_t(-probe_radius, 0.0f, 0.0f), "target_xy_floor_probe(-x)");
			consider_probe(target.pos + vec3_t(0.0f, probe_radius, 0.0f), "target_xy_floor_probe(+y)");
			consider_probe(target.pos + vec3_t(0.0f, -probe_radius, 0.0f), "target_xy_floor_probe(-y)");

			return best;
		}

		bool find_nearby_wall(const vec3_t& pos, vec3_t* out_normal = nullptr) {
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
			const vec3_t scan_start = pos + vec3_t(0.0f, 0.0f, 32.0f);
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
				found = true;
				if (out_normal)
					*out_normal = trace.plane.normal;
			}

			return found;
		}

		bool is_edge_surface(const sim_state_t& state) {
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
				float sample_ground = 0.0f;
				if (trace_ground_ray(state.pos + offset, sample_ground) &&
					std::fabs(sample_ground - state.ground_z) <= k_edge_height_epsilon) {
					++supported;
				}
				else {
					++unsupported;
				}
			}

			return supported >= 2 && unsupported >= 1 && find_nearby_wall(state.pos);
		}

		vec3_t clip_velocity(const vec3_t& in, const vec3_t& normal) {
			const float backoff = in.dot_product(normal);
			vec3_t out = in - (normal * backoff);

			if (std::fabs(out.x) < 0.01f)
				out.x = 0.0f;
			if (std::fabs(out.y) < 0.01f)
				out.y = 0.0f;
			if (std::fabs(out.z) < 0.01f)
				out.z = 0.0f;

			return out;
		}

		void push_event(std::vector<event_t>& events, const event_t& event) {
			if (!events.empty()) {
				const auto& last = events.back();
				if (last.type == event.type &&
					last.tick == event.tick &&
					last.target_index == event.target_index &&
					last.action == event.action) {
					return;
				}
			}

			events.push_back(event);
		}

		void apply_friction(sim_state_t& state) {
			if (!state.on_ground)
				return;

			vec3_t flat_velocity(state.vel.x, state.vel.y, 0.0f);
			const float speed = flat_velocity.length();
			if (speed < 0.1f)
				return;

			const float control = (std::max)(speed, k_stop_speed);
			const float drop = control * k_ground_friction * k_sim_dt;
			const float new_speed = (std::max)(0.0f, speed - drop);
			if (new_speed == speed)
				return;

			const float scale = new_speed / speed;
			state.vel.x *= scale;
			state.vel.y *= scale;
		}

		void accelerate_horizontal(sim_state_t& state, const vec3_t& wish_dir, const float wish_speed, const float accel) {
			const float current_speed = (state.vel.x * wish_dir.x) + (state.vel.y * wish_dir.y);
			const float add_speed = wish_speed - current_speed;
			if (add_speed <= 0.0f)
				return;

			float accel_speed = accel * k_sim_dt * (std::max)(wish_speed, k_stop_speed);
			if (!state.on_ground)
				accel_speed = accel * k_sim_dt * (std::min)(wish_speed, k_air_wishspeed_cap);

			if (accel_speed > add_speed)
				accel_speed = add_speed;

			state.vel.x += wish_dir.x * accel_speed;
			state.vel.y += wish_dir.y * accel_speed;
		}

		vec3_t normalized_flat_dir(const vec3_t& dir) {
			vec3_t flat(dir.x, dir.y, 0.0f);
			const float length = flat.length();
			if (length > 0.001f)
				return flat / length;
			return vec3_t(1.0f, 0.0f, 0.0f);
		}

		sim_input_t build_movement_input(
			const sim_state_t& stage_start_state,
			const sim_state_t& current_state,
			const target_t& target,
			const action_type_t action,
			const int local_tick)
		{
			sim_input_t input{};
			vec3_t wish_dir = normalized_flat_dir(target.pos - stage_start_state.pos);
			if ((target.pos - stage_start_state.pos).length_2d() <= 0.5f) {
				vec3_t velocity_dir(current_state.vel.x, current_state.vel.y, 0.0f);
				if (velocity_dir.length() > 0.5f)
					wish_dir = normalized_flat_dir(velocity_dir);
			}

			float wish_speed = k_ground_wishspeed;
			if (is_ducked_action(action))
				wish_speed *= k_duck_speed_scale;

			if (action == action_type_t::longjump) {
				const float carry_speed = (std::max)(current_state.vel.length_2d(), k_longjump_min_speed);
				wish_speed = (std::max)(wish_speed, carry_speed);
			}

			if (target.kind == target_kind_t::floor_point && local_tick > 0) {
				const float target_distance = (target.pos - current_state.pos).length_2d();
				if (target_distance < 96.0f)
					wish_speed = (std::min)(wish_speed, target_distance * 2.5f + 50.0f);
			}

			input.move = true;
			input.wish_dir = wish_dir;
			input.wish_speed = wish_speed;
			return input;
		}

		target_eval_t evaluate_target(const sim_state_t& state, const target_t& target, const action_type_t action = action_type_t::carry) {
			if (target.kind == target_kind_t::floor_point)
				return validate_floor_stage(state, target);
			return validate_pixelsurf_stage(state, target, action);
		}

		floor_landing_snapshot_t capture_floor_landing_snapshot(
			const sim_state_t& landing_state,
			const sim_state_t& stage_start_state,
			const target_t& target,
			const float airborne_sample_z,
			const int landing_tick,
			const bool inside_target_radius)
		{
			floor_landing_snapshot_t snapshot{};
			snapshot.valid = landing_state.on_ground;
			snapshot.tick = landing_tick;
			snapshot.landing_pos = landing_state.pos;
			snapshot.airborne_sample_z = airborne_sample_z;
			snapshot.landing_position_z = landing_state.pos.z;
			snapshot.landed_ground_z = landing_state.ground_z;
			snapshot.target_floor_z = target.pos.z;
			snapshot.start_floor_z = stage_boundary_z(stage_start_state);
			snapshot.previous_floor_z = snapshot.start_floor_z;
			snapshot.expected_floor_rise = snapshot.target_floor_z - snapshot.start_floor_z;
			snapshot.resolved_floor_z = landing_state.ground_z;
			snapshot.actual_floor_rise = snapshot.resolved_floor_z - snapshot.start_floor_z;
			snapshot.compared_floor_z = snapshot.resolved_floor_z;
			snapshot.compared_using = "landed_ground";

			if (inside_target_radius) {
				const floor_probe_result_t probe = probe_floor_surface_for_stage(landing_state.pos, target);
				snapshot.floor_probe_hit = probe.hit;
				snapshot.floor_probe_height_match = probe.height_match;
				if (probe.hit) {
					snapshot.target_floor_probe = true;
					snapshot.resolved_floor_z = probe.surface_z;
					snapshot.compared_floor_z = probe.surface_z;
					snapshot.actual_floor_rise = probe.surface_z - snapshot.start_floor_z;
					snapshot.compared_using = probe.source;
					if (probe.height_match)
						snapshot.compared_using += "_height_match";
					else
						snapshot.compared_using += "_height_mismatch";
				}
			}

			snapshot.floor_probe_disagreed_with_ground =
				snapshot.floor_probe_hit &&
				std::fabs(snapshot.resolved_floor_z - landing_state.ground_z) > k_floor_z_epsilon;
			snapshot.target_local_probe_mismatch =
				snapshot.floor_probe_hit &&
				!snapshot.floor_probe_height_match;
			snapshot.landed_on_lower_floor =
				inside_target_radius &&
				!snapshot.floor_probe_height_match &&
				snapshot.landed_ground_z < snapshot.target_floor_z - k_floor_z_epsilon;

			if (snapshot.floor_probe_hit && snapshot.floor_probe_height_match) {
				snapshot.target_floor_probe = true;
			}

			snapshot.height_delta = std::fabs(snapshot.compared_floor_z - snapshot.target_floor_z);
			snapshot.stale_ground_reused =
				inside_target_radius &&
				!snapshot.floor_probe_hit &&
				snapshot.height_delta > k_floor_z_epsilon &&
				std::fabs(snapshot.compared_floor_z - snapshot.start_floor_z) <= 0.1f &&
				std::fabs(snapshot.target_floor_z - snapshot.start_floor_z) > k_floor_z_epsilon;
			return snapshot;
		}

		target_eval_t apply_floor_landing_snapshot(
			target_eval_t eval,
			const floor_landing_snapshot_t& snapshot)
		{
			if (!snapshot.valid)
				return eval;

			eval.raw_candidate_z = snapshot.airborne_sample_z;
			eval.grounded_candidate_z = snapshot.resolved_floor_z;
			eval.raw_target_z = snapshot.target_floor_z;
			eval.applied_anchor_z = 0.0f;
			eval.compared_candidate_z = snapshot.compared_floor_z;
			eval.compared_target_z = snapshot.target_floor_z;
			eval.vertical_distance = snapshot.height_delta;
			eval.score_distance = eval.horizontal_distance + (eval.vertical_distance * 2.0f);
			eval.height_epsilon = k_floor_z_epsilon;
			eval.height_match = snapshot.height_delta <= k_floor_z_epsilon;
			eval.lax_height_match = snapshot.height_delta <= k_floor_lax_z_epsilon;
			eval.contact_ready = true;
			eval.matched = eval.radius_match && eval.height_match;
			eval.lax_match = eval.lax_radius_match && eval.lax_height_match;
			return eval;
		}

		bool target_supports_action(const target_t& target, const action_type_t action) {
			switch (action) {
			case action_type_t::stand_jump:
				return target.jump_stand || target.jump_crouch;
			case action_type_t::crouch_jump:
				return target.crouch_hop_stand || target.crouch_hop_crouch ||
					target.mini_crouch_hop_stand || target.mini_crouch_hop_crouch;
			case action_type_t::minijump:
				return target.minijump_stand || target.minijump_crouch;
			case action_type_t::longjump:
				return target.longjump_stand || target.longjump_crouch;
			case action_type_t::jumpbug:
				return target.kind == target_kind_t::pixelsurf_point &&
					(target.jumpbug_stand || target.jumpbug_crouch);
			default:
				return false;
			}
		}

		planned_action_t make_stage_candidate(
			const action_type_t action,
			const std::string& sequence_name,
			const std::string& combo_label,
			const int priority,
			const int duck_start_tick,
			const int duck_end_tick,
			const bool require_duck_terminal)
		{
			planned_action_t candidate{};
			candidate.type = action;
			candidate.sequence_name = sequence_name;
			candidate.combo_label = combo_label;
			candidate.priority = priority;
			candidate.duck_start_tick = duck_start_tick;
			candidate.duck_end_tick = duck_end_tick;
			candidate.min_prune_ticks = min_prune_ticks_budget();
			candidate.max_stage_ticks = stage_tick_budget();
			candidate.require_duck_terminal = require_duck_terminal;
			return candidate;
		}

		bool opener_action_allowed(const target_t& target, const action_type_t action) {
			if (!target_supports_action(target, action))
				return false;

			switch (action) {
			case action_type_t::stand_jump:
				return c::movement::pscalc_allow_jump;
			case action_type_t::minijump:
				return c::movement::pscalc_allow_minijump;
			case action_type_t::longjump:
				return c::movement::pscalc_allow_longjump;
			case action_type_t::crouch_jump:
			case action_type_t::jumpbug:
				return false;
			default:
				return false;
			}
		}

		bool continuation_action_allowed(const target_t& target, const action_type_t action) {
			if (!target_supports_action(target, action))
				return false;

			switch (action) {
			case action_type_t::stand_jump:
				return c::movement::pscalc_allow_jump;
			case action_type_t::crouch_jump:
				return c::movement::pscalc_allow_crouch_jump;
			case action_type_t::minijump:
				return c::movement::pscalc_allow_minijump;
			case action_type_t::longjump:
				return c::movement::pscalc_allow_longjump;
			case action_type_t::jumpbug:
				return c::movement::pscalc_allow_jumpbug;
			default:
				return false;
			}
		}

		void append_stage_candidate(
			std::vector<planned_action_t>& alphabet,
			const target_t& target,
			const int stage_index,
			const action_type_t action,
			const std::string& sequence_name,
			const std::string& combo_label,
			const int priority,
			const int duck_start_tick,
			const int duck_end_tick,
			const bool require_duck_terminal = false)
		{
			const bool allowed = stage_index == 0
				? opener_action_allowed(target, action)
				: continuation_action_allowed(target, action);
			if (!allowed)
				return;

			alphabet.push_back(make_stage_candidate(
				action,
				sequence_name,
				combo_label,
				priority,
				duck_start_tick,
				duck_end_tick,
				require_duck_terminal));
		}

		std::vector<planned_action_t> build_stage_action_alphabet(
			const target_t& target,
			const int stage_index)
		{
			std::vector<planned_action_t> alphabet;
			alphabet.reserve(5);

			if (target.kind == target_kind_t::floor_point) {
				append_stage_candidate(alphabet, target, stage_index, action_type_t::stand_jump, "J_FLOOR", "jump", 0, -1, -1);
				append_stage_candidate(alphabet, target, stage_index, action_type_t::minijump, "MJ_FLOOR", "minijump", 1, 1, 1);
				append_stage_candidate(alphabet, target, stage_index, action_type_t::longjump, "LJ_FLOOR", "longjump", 2, 1, 3, true);
				append_stage_candidate(alphabet, target, stage_index, action_type_t::crouch_jump, "CJ_FLOOR", "crouch jump", 3, 0, 5, true);
				return alphabet;
			}

			append_stage_candidate(alphabet, target, stage_index, action_type_t::minijump, "MJ_PX", "minijump", 0, 1, 1);
			append_stage_candidate(alphabet, target, stage_index, action_type_t::stand_jump, "J_PX", "jump", 1, -1, -1);
			append_stage_candidate(alphabet, target, stage_index, action_type_t::longjump, "LJ_PX", "longjump", 2, 1, 3, true);
			append_stage_candidate(alphabet, target, stage_index, action_type_t::crouch_jump, "CJ_PX", "crouch jump", 3, 0, 5, true);
			append_stage_candidate(alphabet, target, stage_index, action_type_t::jumpbug, "JB_PX", "jumpbug", 4, 0, 64, true);

			return alphabet;
		}

		std::string format_action_alphabet(const std::vector<planned_action_t>& alphabet, const bool sequence_names = false) {
			if (alphabet.empty())
				return "(none)";

			std::string text;
			for (std::size_t i = 0; i < alphabet.size(); ++i) {
				const std::string label = sequence_names
					? planned_sequence_name(alphabet[i])
					: planned_combo_name(alphabet[i]);
				if (label.empty())
					continue;

				if (!text.empty())
					text.append(" -> ");
				text.append(label);
			}

			return text.empty() ? std::string("(none)") : text;
		}

		std::string format_combo_debug(const std::vector<planned_action_t>& combo) {
			const std::string human = format_planned_combo(combo);
			const std::string sequence = format_action_alphabet(combo, true);
			if (sequence.empty() || sequence == human || sequence == "(none)")
				return human;
			return human + " (" + sequence + ")";
		}

		std::string build_exact_combo_formula(const std::vector<std::vector<planned_action_t>>& stage_alphabets) {
			if (stage_alphabets.empty())
				return "0";

			std::ostringstream stream;
			for (std::size_t i = 0; i < stage_alphabets.size(); ++i) {
				if (i > 0)
					stream << " * ";
				stream << "S" << (i + 1) << "(" << stage_alphabets[i].size() << ")";
			}
			return stream.str();
		}

		int exact_combo_total(const std::vector<std::vector<planned_action_t>>& stage_alphabets, bool& exact) {
			exact = !stage_alphabets.empty();
			std::uint64_t total = 1;
			for (const auto& stage : stage_alphabets) {
				if (stage.empty()) {
					exact = true;
					return 0;
				}

				total *= static_cast<std::uint64_t>(stage.size());
				if (total > static_cast<std::uint64_t>(INT_MAX)) {
					exact = false;
					return INT_MAX;
				}
			}

			return static_cast<int>(total);
		}

		std::vector<planned_action_t> decode_combo(
			const std::vector<std::vector<planned_action_t>>& stage_alphabets,
			std::size_t combo_index)
		{
			std::vector<planned_action_t> combo(stage_alphabets.size());
			for (int stage_index = static_cast<int>(stage_alphabets.size()) - 1; stage_index >= 0; --stage_index) {
				const auto& alphabet = stage_alphabets[stage_index];
				if (alphabet.empty())
					continue;

				const std::size_t radix = alphabet.size();
				const std::size_t digit = combo_index % radix;
				combo_index /= radix;
				combo[stage_index] = alphabet[digit];
			}

			return combo;
		}

		bool branch_fraction_match(const float lhs, const float rhs) {
			if (std::fabs(lhs - rhs) <= 0.25f)
				return true;

			const float lhs_fraction = std::fabs(lhs - std::floor(lhs));
			const float rhs_fraction = std::fabs(rhs - std::floor(rhs));
			return std::fabs(lhs_fraction - rhs_fraction) <= 0.03f;
		}

		bool branch_height_matches(const float base_z, const std::vector<float>& offsets, const float target_z, const float epsilon) {
			for (const float offset : offsets) {
				const float adjusted_z = base_z + offset;
				if (std::fabs(adjusted_z - target_z) <= epsilon)
					return true;
				if (branch_fraction_match(adjusted_z, target_z))
					return true;
			}
			return false;
		}

		height_match_t evaluate_floor_height(const sim_state_t& state, const target_t& target) {
			height_match_t match{};
			match.strict_epsilon = k_floor_z_epsilon;
			match.lax_epsilon = k_floor_lax_z_epsilon;
			match.applied_anchor_z = 0.0f;
			match.compared_candidate_z = state.on_ground ? state.ground_z : state.pos.z;
			match.compared_target_z = target.pos.z;
			match.vertical_distance = std::fabs(match.compared_candidate_z - match.compared_target_z);
			match.strict_match = match.vertical_distance <= match.strict_epsilon;
			match.lax_match = match.vertical_distance <= match.lax_epsilon;
			return match;
		}

		height_match_t evaluate_pixelsurf_height(
			const sim_state_t& state,
			const target_t& target,
			const action_type_t action)
		{
			height_match_t best{};
			best.strict_epsilon = strict_z_tolerance();
			best.lax_epsilon = lax_z_tolerance();
			best.compared_target_z = target.pos.z;
			const float base_z = state.on_ground ? state.ground_z : state.pos.z;
			const auto height_offsets = landing_z_offsets_for_action(target, action);

			for (const float offset : height_offsets) {
				const float adjusted_z = base_z + offset;
				const float delta = std::fabs(adjusted_z - target.pos.z);
				const bool fraction_match = branch_fraction_match(adjusted_z, target.pos.z);
				const bool strict_match = delta <= best.strict_epsilon || fraction_match;
				const bool lax_match = delta <= best.lax_epsilon || fraction_match;

				bool take = false;
				if (best.vertical_distance == FLT_MAX)
					take = true;
				else if (strict_match != best.strict_match)
					take = strict_match;
				else if (lax_match != best.lax_match)
					take = lax_match;
				else if (delta < best.vertical_distance)
					take = true;

				if (!take)
					continue;

				best.strict_match = strict_match;
				best.lax_match = lax_match;
				best.vertical_distance = delta;
				best.applied_anchor_z = offset;
				best.compared_candidate_z = adjusted_z;
			}

			if (best.vertical_distance == FLT_MAX) {
				best.vertical_distance = std::fabs(base_z - target.pos.z);
				best.compared_candidate_z = base_z;
			}

			return best;
		}

		target_eval_t validate_floor_stage(const sim_state_t& state, const target_t& target) {
			target_eval_t evaluation{};
			const vec3_t delta = state.pos - target.pos;
			const auto height = evaluate_floor_height(state, target);

			evaluation.stage_kind = target_kind_t::floor_point;
			evaluation.horizontal_distance = std::sqrt((delta.x * delta.x) + (delta.y * delta.y));
			evaluation.vertical_distance = height.vertical_distance;
			evaluation.score_distance = evaluation.horizontal_distance + (evaluation.vertical_distance * 2.0f);
			evaluation.contact_ready = state.on_ground && std::fabs(state.pos.z - state.ground_z) <= k_ground_epsilon;
			evaluation.raw_candidate_z = state.pos.z;
			evaluation.grounded_candidate_z = state.ground_z;
			evaluation.raw_target_z = target.pos.z;
			evaluation.applied_anchor_z = 0.0f;
			evaluation.compared_candidate_z = height.compared_candidate_z;
			evaluation.compared_target_z = height.compared_target_z;
			evaluation.height_epsilon = height.strict_epsilon;
			evaluation.radius_match = evaluation.horizontal_distance <= target.radius;
			evaluation.lax_radius_match = evaluation.horizontal_distance <= target.radius * 1.5f;
			evaluation.height_match = height.strict_match;
			evaluation.lax_height_match = height.lax_match;
			evaluation.edge_ready = false;
			evaluation.matched = evaluation.contact_ready &&
				evaluation.radius_match &&
				evaluation.height_match;
			evaluation.lax_match = state.on_ground &&
				evaluation.lax_radius_match &&
				evaluation.lax_height_match;
			return evaluation;
		}

		target_eval_t validate_pixelsurf_stage(
			const sim_state_t& state,
			const target_t& target,
			const action_type_t action)
		{
			target_eval_t evaluation{};
			const vec3_t delta = state.pos - target.pos;
			const auto height = evaluate_pixelsurf_height(state, target, action);

			evaluation.stage_kind = target_kind_t::pixelsurf_point;
			evaluation.horizontal_distance = std::sqrt((delta.x * delta.x) + (delta.y * delta.y));
			evaluation.vertical_distance = height.vertical_distance;
			evaluation.score_distance = evaluation.horizontal_distance + (evaluation.vertical_distance * 2.0f);
			evaluation.contact_ready = state.on_ground && std::fabs(state.pos.z - state.ground_z) <= k_ground_epsilon;
			evaluation.raw_candidate_z = state.pos.z;
			evaluation.grounded_candidate_z = state.ground_z;
			evaluation.raw_target_z = target.pos.z;
			evaluation.applied_anchor_z = height.applied_anchor_z;
			evaluation.compared_candidate_z = height.compared_candidate_z;
			evaluation.compared_target_z = height.compared_target_z;
			evaluation.height_epsilon = height.strict_epsilon;
			evaluation.height_match = height.strict_match;
			evaluation.lax_height_match = height.lax_match;
			evaluation.radius_match = evaluation.horizontal_distance <= target.radius;
			evaluation.lax_radius_match = evaluation.horizontal_distance <= target.radius * 1.5f;

			if (evaluation.horizontal_distance <= target.radius * 2.0f) {
				vec3_t wall_normal{};
				evaluation.near_wall = find_nearby_wall(state.pos, &wall_normal);
				if (evaluation.near_wall && target.normal.not_null())
					evaluation.wall_alignment = wall_normal.dot_product(target.normal);
			}

			if (state.on_ground &&
				evaluation.lax_radius_match &&
				evaluation.lax_height_match) {
				evaluation.edge_ready = is_edge_surface(state);
			}

			evaluation.lax_match = evaluation.lax_radius_match &&
				evaluation.lax_height_match &&
				(evaluation.edge_ready || (!state.on_ground && state.vel.z < 0.0f));

			evaluation.matched = evaluation.contact_ready &&
				evaluation.radius_match &&
				evaluation.height_match &&
				evaluation.edge_ready &&
				(!evaluation.near_wall || target.normal.null() || evaluation.wall_alignment > -0.25f);

			if (evaluation.edge_ready)
				evaluation.score_distance -= 8.0f;
			if (!state.on_ground && state.vel.z < 0.0f && evaluation.vertical_distance <= height.lax_epsilon)
				evaluation.score_distance -= 6.0f;

			return evaluation;
		}

		std::vector<float> landing_z_offsets_for_action(const target_t& target, const action_type_t action) {
			std::vector<float> offsets;
			auto add_offset = [&](const float offset) {
				if (std::find(offsets.begin(), offsets.end(), offset) == offsets.end())
					offsets.push_back(offset);
			};

			switch (action) {
			case action_type_t::stand_jump:
				if (target.jump_stand)
					add_offset(0.0f);
				if (target.jump_crouch) {
					add_offset(k_assist_crouch_offset);
					add_offset(-k_assist_crouch_offset);
				}
				break;
			case action_type_t::minijump:
				if (target.minijump_stand)
					add_offset(0.0f);
				if (target.minijump_crouch) {
					add_offset(k_assist_crouch_offset);
					add_offset(-k_assist_crouch_offset);
				}
				break;
			case action_type_t::longjump:
				if (target.longjump_stand) {
					add_offset(0.0f);
					add_offset(k_assist_longjump_offset);
				}
				if (target.longjump_crouch) {
					add_offset(-k_assist_crouch_offset);
					add_offset(k_assist_crouch_offset + k_assist_longjump_offset);
				}
				break;
			case action_type_t::crouch_jump:
				if (target.crouch_hop_stand || target.mini_crouch_hop_stand) {
					add_offset(0.0f);
					add_offset(-k_assist_crouch_offset);
				}
				if (target.crouch_hop_crouch || target.mini_crouch_hop_crouch) {
					add_offset(0.0f);
					add_offset(k_assist_crouch_offset);
				}
				break;
			case action_type_t::jumpbug:
				if (target.jumpbug_stand)
					add_offset(0.0f);
				if (target.jumpbug_crouch)
					add_offset(k_assist_crouch_offset);
				break;
			default:
				break;
			}

			if (offsets.empty())
				add_offset(0.0f);
			return offsets;
		}

		stage_hit_debug_t make_stage_hit_debug(
			const int stage_index,
			const sim_state_t& state,
			const target_t& target,
			const target_eval_t& eval,
			const bool landed,
			const bool stage_window_entered,
			const int window_enter_tick,
			const int window_exit_tick,
			const int landing_tick,
			const std::string& reason,
			const floor_landing_snapshot_t* floor_snapshot = nullptr)
		{
			stage_hit_debug_t debug{};
			debug.stage_index = stage_index;
			debug.tick = state.tick;
			debug.candidate_pos = state.pos;
			debug.target_pos = target.pos;
			debug.xy_delta = eval.horizontal_distance;
			debug.z_delta = eval.vertical_distance;
			debug.raw_candidate_z = eval.raw_candidate_z;
			debug.grounded_candidate_z = eval.grounded_candidate_z;
			debug.raw_target_z = eval.raw_target_z;
			debug.applied_anchor_z = eval.applied_anchor_z;
			debug.compared_candidate_z = eval.compared_candidate_z;
			debug.compared_target_z = eval.compared_target_z;
			debug.height_epsilon = eval.height_epsilon;
			debug.stage_kind = eval.stage_kind;
			debug.landing_detected = landed;
			debug.stage_window_entered = stage_window_entered;
			debug.contact_detected = eval.contact_ready;
			debug.height_match = eval.height_match;
			debug.radius_match = eval.radius_match;
			debug.edge_ready = eval.edge_ready;
			debug.window_enter_tick = window_enter_tick;
			debug.window_exit_tick = window_exit_tick;
			debug.landing_tick = landing_tick;
			if (floor_snapshot && floor_snapshot->valid) {
				debug.airborne_sample_z = floor_snapshot->airborne_sample_z;
				debug.landing_position_z = floor_snapshot->landing_position_z;
				debug.landed_ground_z = floor_snapshot->landed_ground_z;
				debug.target_floor_z = floor_snapshot->target_floor_z;
				debug.compared_floor_z = floor_snapshot->compared_floor_z;
				debug.resolved_floor_z = floor_snapshot->resolved_floor_z;
				debug.previous_floor_z = floor_snapshot->previous_floor_z;
				debug.start_floor_z = floor_snapshot->start_floor_z;
				debug.expected_floor_rise = floor_snapshot->expected_floor_rise;
				debug.actual_floor_rise = floor_snapshot->actual_floor_rise;
				debug.stale_ground_reused = floor_snapshot->stale_ground_reused;
				debug.floor_probe_hit = floor_snapshot->floor_probe_hit;
				debug.floor_probe_height_match = floor_snapshot->floor_probe_height_match;
				debug.floor_probe_disagreed_with_ground = floor_snapshot->floor_probe_disagreed_with_ground;
				debug.landed_on_lower_floor = floor_snapshot->landed_on_lower_floor;
				debug.target_local_probe_mismatch = floor_snapshot->target_local_probe_mismatch;
				debug.compared_floor_source = floor_snapshot->compared_using;
			}
			else {
				debug.airborne_sample_z = eval.raw_candidate_z;
				debug.landing_position_z = state.pos.z;
				debug.landed_ground_z = eval.grounded_candidate_z;
				debug.target_floor_z = target.pos.z;
				debug.compared_floor_z = eval.compared_candidate_z;
				debug.resolved_floor_z = eval.compared_candidate_z;
				debug.previous_floor_z = 0.0f;
				debug.start_floor_z = 0.0f;
				debug.expected_floor_rise = 0.0f;
				debug.actual_floor_rise = 0.0f;
				debug.compared_floor_source = state.on_ground ? "state_grounded" : "airborne";
			}
			debug.reject_reason = reason;
			return debug;
		}

		std::string describe_stage_rejection(const target_t& target, const stage_hit_debug_t& debug) {
			if (target.kind == target_kind_t::floor_point) {
				if (debug.stale_ground_reused)
					return "ERROR: stale grounded floor Z reused for floor-stage validation";
				if (debug.floor_probe_disagreed_with_ground && debug.height_match)
					return "target-local floor probe corrected stale grounded Z";
				if (debug.landed_on_lower_floor)
					return "landed on lower floor, target floor not reached";
				if (debug.target_local_probe_mismatch)
					return "target-local floor probe mismatch";
				if (!debug.stage_window_entered && !debug.landing_detected)
					return "floor stage never entered the arrival window";
				if (!debug.radius_match)
					return "floor stage radius mismatch";
				if (!debug.height_match)
					return "floor stage height mismatch";
				if (!debug.contact_detected)
					return "floor stage never settled on ground";
				return "candidate missed the floor stage";
			}

			if (!debug.landing_detected)
				return "terminal pixelsurf never landed";
			if (!debug.stage_window_entered)
				return "terminal pixelsurf never entered contact window";
			if (!debug.radius_match)
				return "terminal pixelsurf radius mismatch";
			if (!debug.height_match)
				return "terminal pixelsurf height mismatch";
			if (!debug.edge_ready)
				return "terminal pixelsurf missing edge contact";
			if (!debug.contact_detected)
				return "terminal pixelsurf contact not grounded";
			return "terminal pixelsurf validation failed";
		}

		sim_input_t input_for_action(
			const sim_state_t& stage_start_state,
			const sim_state_t& state,
			const target_t& target,
			const planned_action_t& action,
			const int local_tick,
			const float ground_distance)
		{
			sim_input_t input = build_movement_input(stage_start_state, state, target, action.type, local_tick);
			if (action.duck_start_tick >= 0 && action.duck_end_tick >= action.duck_start_tick)
				input.duck = local_tick >= action.duck_start_tick && local_tick <= action.duck_end_tick;

			switch (action.type) {
			case action_type_t::stand_jump:
				input.jump = local_tick == 0;
				break;
			case action_type_t::crouch_jump:
				input.jump = local_tick == 0;
				break;
			case action_type_t::minijump:
				input.jump = local_tick == 0;
				break;
			case action_type_t::longjump:
				input.jump = local_tick == 0;
				break;
			case action_type_t::jumpbug:
				input.move = true;
				input.duck = ground_distance <= (k_jumpbug_window + 2.0f) || state.ducking;
				input.jump = ground_distance <= 2.0f;
				break;
			default:
				break;
			}

			return input;
		}

		void step_physics(
			sim_state_t& state,
			const sim_input_t& input,
			const action_type_t action,
			std::vector<event_t>& events,
			const float ground_distance_before_step,
			const int current_target_index)
		{
			state.prev_on_ground = state.on_ground;
			state.prev_ducking = state.ducking;
			state.ducking = input.duck;

			if (state.on_ground) {
				state.ground_ticks++;
				state.air_ticks = 0;
			}
			else {
				state.air_ticks++;
				state.ground_ticks = 0;
			}

			const bool jumpbug_window = action == action_type_t::jumpbug &&
				!state.on_ground &&
				state.vel.z < 0.0f &&
				ground_distance_before_step > 0.0f &&
				ground_distance_before_step <= k_jumpbug_window;
			bool jumped_this_tick = false;

			if (state.on_ground)
				apply_friction(state);

			if (input.move) {
				if (state.on_ground)
					accelerate_horizontal(state, input.wish_dir, input.wish_speed, k_ground_accelerate);
				else
					accelerate_horizontal(state, input.wish_dir, input.wish_speed, k_air_accelerate);
			}

			if (input.jump && state.on_ground) {
				state.vel.z = k_jump_impulse;
				state.on_ground = false;
				state.pos.z += k_jump_launch_lift;
				state.stamina = (std::min)(40.0f, state.stamina + k_jump_stamina_gain);
				jumped_this_tick = true;

				event_t event{};
				event.type = event_type_t::action;
				event.action = action;
				event.tick = state.tick;
				event.target_index = current_target_index;
				event.pos = state.pos;
				event.vel = state.vel;
				event.ducking = is_ducked_action(action);
				push_event(events, event);
			}

			const vec3_t desired_pos = state.pos + (state.vel * k_sim_dt);

			trace_t move_trace{};
			if (trace_player_hull(state.pos, desired_pos, state.ducking, move_trace)) {
				if (move_trace.flFraction < 1.0f) {
					const bool hit_walkable_plane = move_trace.plane.normal.z >= k_ground_normal_min;
					const bool leaving_floor = hit_walkable_plane &&
						state.vel.z > 0.0f &&
						(jumped_this_tick || state.prev_on_ground);

					if (move_trace.plane.normal.z <= k_ceiling_normal_limit && state.vel.z > 0.0f) {
						event_t headbang_event{};
						headbang_event.type = event_type_t::headbang;
						headbang_event.tick = state.tick;
						headbang_event.target_index = current_target_index;
						headbang_event.pos = move_trace.end;
						headbang_event.vel = state.vel;
						headbang_event.ducking = state.ducking;
						push_event(events, headbang_event);
					}

					if (leaving_floor) {
						state.pos = desired_pos;
						state.on_ground = false;
					}
					else if (hit_walkable_plane && state.vel.z <= 0.0f) {
						state.pos = move_trace.end;
						state.on_ground = true;
						state.ground_z = move_trace.end.z;
						state.vel = clip_velocity(state.vel, move_trace.plane.normal);
						state.vel.z = 0.0f;
					}
					else {
						state.pos = move_trace.end;
						state.on_ground = false;
						state.vel = clip_velocity(state.vel, move_trace.plane.normal);
					}
				}
				else {
					state.pos = desired_pos;
					state.on_ground = false;
				}
			}
			else {
				state.pos = desired_pos;
				state.on_ground = false;
			}

			float ground_z = 0.0f;
			trace_t ground_trace{};
			if (state.vel.z <= 0.0f && trace_ground_hull(state.pos, state.ducking, ground_z, &ground_trace, 12.0f)) {
				if (state.pos.z - ground_z <= k_ground_snap_distance) {
					state.pos.z = ground_z;
					state.ground_z = ground_z;
					state.on_ground = true;
					state.vel.z = 0.0f;
				}
			}

			if (jumpbug_window && state.on_ground && !state.prev_on_ground) {
				event_t jumpbug_event{};
				jumpbug_event.type = event_type_t::jumpbug;
				jumpbug_event.action = action_type_t::jumpbug;
				jumpbug_event.tick = state.tick;
				jumpbug_event.target_index = current_target_index;
				jumpbug_event.pos = state.pos;
				jumpbug_event.vel = state.vel;
				jumpbug_event.ducking = true;
				push_event(events, jumpbug_event);

				state.vel.z = k_jump_impulse;
				state.on_ground = false;
				state.pos.z += k_jump_launch_lift;
				state.stamina = (std::min)(40.0f, state.stamina + k_jump_stamina_gain);
				jumped_this_tick = true;

				event_t followup_jump{};
				followup_jump.type = event_type_t::action;
				followup_jump.action = action_type_t::jumpbug;
				followup_jump.tick = state.tick;
				followup_jump.target_index = current_target_index;
				followup_jump.pos = state.pos;
				followup_jump.vel = state.vel;
				followup_jump.ducking = true;
				push_event(events, followup_jump);
			}

			if (state.on_ground)
				state.vel.z = 0.0f;
			else if (!jumped_this_tick)
				state.vel.z -= k_gravity * k_sim_dt;

			state.stamina = (std::max)(0.0f, state.stamina - k_stamina_decay_per_tick);
		}

		void finalize_result(result_t& result) {
			const auto formatted = format_sequence(result.events);
			result.sequence = formatted.summary;
			result.full_sequence = formatted.full;
			result.hidden_events = formatted.hidden;
			result.combo_name = result.sequence;
			result.score = static_cast<float>((result.tick >= 0 ? result.tick : 9999) + static_cast<int>(result.events.size() * 3) + (result.hidden_events * 2));
		}

		float partial_score(
			const result_t& result,
			const int current_target_index,
			const std::vector<target_t>& targets,
			const sim_state_t& state,
			const int current_stage_ticks)
		{
			float distance_score = 512.0f;
			if (current_target_index >= 0 && current_target_index < static_cast<int>(targets.size()))
				distance_score = evaluate_target(state, targets[current_target_index]).score_distance;

			const float remaining_targets = static_cast<float>((std::max)(0, static_cast<int>(targets.size()) - current_target_index));
			const float displacement = horizontal_distance_between(result.start_pos, state.pos);
			return (remaining_targets * 4096.0f) +
				(distance_score * 8.0f) +
				static_cast<float>(state.tick) +
				static_cast<float>(current_stage_ticks * 4) -
				(displacement * 0.35f);
		}

		float stage_boundary_z(const sim_state_t& state) {
			return state.on_ground ? state.ground_z : state.pos.z;
		}

		void normalize_stage_boundary_state(sim_state_t& state) {
			if (!state.on_ground)
				return;

			float traced_ground_z = 0.0f;
			if (trace_ground_hull(state.pos, state.ducking, traced_ground_z, nullptr, 16.0f)) {
				state.ground_z = traced_ground_z;
				if (std::fabs(state.pos.z - traced_ground_z) <= (k_ground_snap_distance + 1.0f))
					state.pos.z = traced_ground_z;
			}
			else {
				state.ground_z = state.pos.z;
			}

			state.vel.z = 0.0f;
			state.prev_on_ground = true;
			state.prev_ducking = state.ducking;
		}

		std::string format_stage_transition_line(
			const int from_stage_index,
			const sim_state_t& from_end_state,
			const int to_stage_index,
			const sim_state_t& to_start_state)
		{
			std::ostringstream stream;
			stream << std::fixed << std::setprecision(3)
				<< "S" << from_stage_index << " end Z " << stage_boundary_z(from_end_state)
				<< " -> S" << to_stage_index << " start Z " << stage_boundary_z(to_start_state)
				<< " (delta " << std::fabs(stage_boundary_z(from_end_state) - stage_boundary_z(to_start_state)) << ")";
			return stream.str();
		}

		action_run_t simulate_action(
			const planned_action_t& action,
			const sim_state_t& start_state,
			const target_t& target,
			const int remaining_stage_ticks,
			const int current_target_index,
			const std::vector<event_t>& incoming_events)
		{
			action_run_t run{};
			run.end_state = start_state;
			run.events = incoming_events;
			const int candidate_stage_budget = (std::min)(remaining_stage_ticks, action.max_stage_ticks);
			const int prune_after_ticks = (std::max)(action.min_prune_ticks, min_prune_ticks_budget());
			const bool allow_stage_zero_pre_prune = !(current_target_index == 0 && c::movement::pscalc_no_first_stage_prune);

			if (candidate_stage_budget <= 0) {
				run.reject_reason = "target unreachable within tick budget";
				return run;
			}

			sim_state_t state = start_state;
			std::vector<event_t> current_events = incoming_events;
			std::vector<event_t> best_events = current_events;
			sim_state_t best_state = state;
			const target_eval_t initial_eval = evaluate_target(state, target, action.type);
			float best_distance = initial_eval.score_distance;
			bool became_airborne = !state.on_ground;
			bool stage_window_entered = initial_eval.radius_match;
			int window_enter_tick = stage_window_entered ? state.tick : -1;
			int window_exit_tick = stage_window_entered ? state.tick : -1;
			int landing_tick = -1;
			int grounded_contact_ticks = -1;
			float latest_airborne_sample_z = state.pos.z;
			floor_landing_snapshot_t floor_landing_snapshot{};
			run.stage_debug = make_stage_hit_debug(
				current_target_index + 1,
				state,
				target,
				initial_eval,
				false,
				stage_window_entered,
				window_enter_tick,
				window_exit_tick,
				landing_tick,
				std::string());

			for (int local_tick = 0; local_tick < candidate_stage_budget; ++local_tick) {
				if (cancel_requested()) {
					run.reject_reason = "cancelled";
					break;
				}

				float ground_distance = 0.0f;
				trace_ground_distance(state.pos, state.ducking, 32.0f, ground_distance);
				const float pre_step_airborne_z = !state.on_ground ? state.pos.z : latest_airborne_sample_z;

				const sim_input_t input = input_for_action(start_state, state, target, action, local_tick, ground_distance);
				const std::size_t previous_event_count = current_events.size();
				step_physics(state, input, action.type, current_events, ground_distance, current_target_index);
				state.tick++;
				run.ticks_used++;

				if (!state.on_ground)
					became_airborne = true;
				run.had_airborne_phase = run.had_airborne_phase || !state.on_ground;
				run.max_upward_velocity = (std::max)(run.max_upward_velocity, state.vel.z);
				run.horizontal_displacement = (std::max)(
					run.horizontal_displacement,
					horizontal_distance_between(state.pos, start_state.pos));

				if (input.jump || action.type == action_type_t::jumpbug)
					run.generated_action = true;

				for (std::size_t event_index = previous_event_count; event_index < current_events.size(); ++event_index) {
					if (current_events[event_index].type == event_type_t::jumpbug) {
						run.jumpbug_triggered = true;
						break;
					}
				}

				const bool landed = became_airborne && !state.prev_on_ground && state.on_ground;
				auto eval = evaluate_target(state, target, action.type);
				if (!state.on_ground)
					latest_airborne_sample_z = state.pos.z;
				const int current_tick = state.tick - 1;

				if (eval.radius_match) {
					if (!stage_window_entered) {
						stage_window_entered = true;
						window_enter_tick = current_tick;
					}
					window_exit_tick = current_tick;
				}

				if (landed) {
					run.landed = true;
					landing_tick = current_tick;
					if (target.kind == target_kind_t::floor_point) {
						floor_landing_snapshot = capture_floor_landing_snapshot(
							state,
							start_state,
							target,
							pre_step_airborne_z,
							current_tick,
							eval.radius_match);
						eval = apply_floor_landing_snapshot(eval, floor_landing_snapshot);
					}
				}

				const bool landing_in_window =
					stage_window_entered &&
					landing_tick >= window_enter_tick &&
					landing_tick <= window_exit_tick + k_stage_window_landing_grace_ticks;

				if (eval.score_distance < best_distance) {
					best_distance = eval.score_distance;
					best_state = state;
					best_events = current_events;
				}

				if (target.kind == target_kind_t::floor_point) {
					const bool valid_floor_landing =
						landing_in_window &&
						eval.radius_match &&
						eval.contact_ready &&
						eval.height_match;

					if (valid_floor_landing) {
						if (floor_landing_snapshot.valid) {
							state.pos.z = floor_landing_snapshot.resolved_floor_z;
							state.ground_z = floor_landing_snapshot.resolved_floor_z;
							state.on_ground = true;
							state.vel.z = 0.0f;
						}

						event_t floor_event{};
						floor_event.type = event_type_t::floor;
						floor_event.tick = current_tick;
						floor_event.target_index = current_target_index;
						floor_event.pos = state.pos;
						floor_event.vel = state.vel;
						floor_event.ducking = state.ducking;
						push_event(current_events, floor_event);

						run.success = true;
						normalize_stage_boundary_state(state);
						run.end_state = state;
						run.events = current_events;
						run.distance = 0.0f;
						run.progress = initial_eval.score_distance;
						run.stage_debug = make_stage_hit_debug(
							current_target_index + 1,
							state,
							target,
							eval,
							run.landed,
							stage_window_entered,
							window_enter_tick,
							window_exit_tick,
							landing_tick,
							std::string(),
							floor_landing_snapshot.valid ? &floor_landing_snapshot : nullptr);
						return run;
					}
				}
				else {
					if (landing_in_window && state.on_ground) {
						if (grounded_contact_ticks < 0)
							grounded_contact_ticks = 0;
						grounded_contact_ticks++;
					}

					const bool valid_pixelsurf_landing =
						became_airborne &&
						landing_in_window &&
						eval.contact_ready &&
						eval.height_match &&
						eval.edge_ready &&
						(!eval.near_wall || target.normal.null() || eval.wall_alignment > -0.25f);

					if (valid_pixelsurf_landing) {
						event_t pixelsurf_event{};
						pixelsurf_event.type = event_type_t::pixelsurf;
						pixelsurf_event.tick = current_tick;
						pixelsurf_event.target_index = current_target_index;
						pixelsurf_event.pos = state.pos;
						pixelsurf_event.vel = state.vel;
						pixelsurf_event.ducking = state.ducking;
						push_event(current_events, pixelsurf_event);

						run.success = true;
						normalize_stage_boundary_state(state);
						run.end_state = state;
						run.events = current_events;
						run.distance = 0.0f;
						run.progress = initial_eval.score_distance;
						run.stage_debug = make_stage_hit_debug(
							current_target_index + 1,
							state,
							target,
							eval,
							run.landed,
							stage_window_entered,
							window_enter_tick,
							window_exit_tick,
							landing_tick,
							std::string());
						return run;
					}
				}

				run.stage_debug = make_stage_hit_debug(
					current_target_index + 1,
					state,
					target,
					eval,
					run.landed,
					stage_window_entered,
					window_enter_tick,
					window_exit_tick,
					landing_tick,
					run.reject_reason,
					(target.kind == target_kind_t::floor_point && floor_landing_snapshot.valid) ? &floor_landing_snapshot : nullptr);

				const float progress_so_far = initial_eval.score_distance - best_distance;
				if (allow_stage_zero_pre_prune && local_tick + 1 >= prune_after_ticks) {
					if (is_jump_like_action(action.type) &&
						!became_airborne &&
						run.max_upward_velocity <= k_min_visible_vertical_velocity) {
						run.reject_reason = "no valid airborne path found";
						break;
					}

					if (action.type == action_type_t::jumpbug &&
						!run.landed &&
						ground_distance > (k_jumpbug_window + 8.0f) &&
						run.horizontal_displacement < k_min_segment_progress) {
						run.reject_reason = "jumpbug never reached a landing window";
						run.stage_debug.reject_reason = run.reject_reason;
						break;
					}

					if (target.kind == target_kind_t::pixelsurf_point &&
						run.landed &&
						!stage_window_entered) {
						run.reject_reason = describe_stage_rejection(target, run.stage_debug);
						run.stage_debug.reject_reason = run.reject_reason;
						break;
					}

					if (target.kind == target_kind_t::pixelsurf_point &&
						grounded_contact_ticks >= k_pixelsurf_contact_window_ticks) {
						run.reject_reason = describe_stage_rejection(target, run.stage_debug);
						run.stage_debug.reject_reason = run.reject_reason;
						break;
					}

					if (!run.landed &&
						progress_so_far < k_min_segment_progress &&
						run.horizontal_displacement < k_min_route_displacement &&
						local_tick + 1 >= candidate_stage_budget) {
						run.reject_reason = describe_stage_rejection(target, run.stage_debug);
						run.stage_debug.reject_reason = run.reject_reason;
						break;
					}
				}
			}

			normalize_stage_boundary_state(best_state);
			run.end_state = best_state;
			run.events = std::move(best_events);
			run.distance = best_distance;
			run.progress = initial_eval.score_distance - best_distance;
			run.made_progress = run.progress > k_min_segment_progress || run.horizontal_displacement >= k_min_route_displacement;

			if (run.ticks_used <= 0)
				run.reject_reason = "zero ticks simulated";
			else if (run.reject_reason.empty() && is_jump_like_action(action.type) && run.max_upward_velocity <= k_min_visible_vertical_velocity)
				run.reject_reason = "all branches pruned due to no vertical movement";
			else if (run.reject_reason.empty() && !run.had_airborne_phase && is_jump_like_action(action.type))
				run.reject_reason = "no valid airborne path found";
			else if (run.reject_reason.empty())
				run.reject_reason = describe_stage_rejection(target, run.stage_debug);

			run.stage_debug.reject_reason = run.reject_reason;

			return run;
		}

		combo_run_t simulate_combo(
			const std::vector<planned_action_t>& combo,
			const std::vector<target_t>& targets,
			const sim_state_t& start_state,
			const int stage_ticks_budget,
			const int total_ticks_budget)
		{
			combo_run_t outcome{};
			result_t result{};
			result.start_pos = start_state.pos;
			result.pos = start_state.pos;
			result.vel = start_state.vel;
			result.total_targets = static_cast<int>(targets.size());
			result.tick = 0;

			sim_state_t state = start_state;
			int current_target_index = 0;
			int current_stage_ticks = 0;
			int sequence_priority_total = 0;
			int repeated_flat_transition_count = 0;
			float last_success_end_z = stage_boundary_z(start_state);
			bool have_successful_boundary = false;

			for (const auto& action : combo) {
				if (cancel_requested()) {
					outcome.failure_reason = "cancelled";
					break;
				}

				while (current_target_index < static_cast<int>(targets.size()) &&
					targets[current_target_index].kind == target_kind_t::floor_point) {
					const auto carry_eval = evaluate_target(state, targets[current_target_index]);
					if (!carry_eval.matched)
						break;

					event_t floor_event{};
					floor_event.type = event_type_t::floor;
					floor_event.tick = state.tick;
					floor_event.target_index = current_target_index;
					floor_event.pos = state.pos;
					floor_event.vel = state.vel;
					floor_event.ducking = state.ducking;
					push_event(result.events, floor_event);

					current_target_index++;
					result.reached_targets = current_target_index;
					current_stage_ticks = 0;
				}

				if (current_target_index >= static_cast<int>(targets.size()))
					break;

				const target_t& target = targets[current_target_index];
				const sim_state_t stage_start_state = state;
				if (!outcome.stage_boundaries.empty()) {
					const auto& previous_boundary = outcome.stage_boundaries.back();
					const float propagation_delta = std::fabs(
						stage_boundary_z(previous_boundary.end_state) - stage_boundary_z(stage_start_state));
					outcome.stage_transition_summary = format_stage_transition_line(
						previous_boundary.stage_index,
						previous_boundary.end_state,
						current_target_index + 1,
						stage_start_state);
					if (propagation_delta > 0.05f) {
						outcome.propagation_warning = std::string("state propagation mismatch before stage ")
							.append(std::to_string(current_target_index + 1))
							.append(": expected ")
							.append(std::to_string(stage_boundary_z(previous_boundary.end_state)))
							.append(", got ")
							.append(std::to_string(stage_boundary_z(stage_start_state)));
					}
				}

				const int remaining_total_ticks = total_ticks_budget - outcome.ticks_used;
				if (remaining_total_ticks <= 0) {
					outcome.failure_reason = "total route tick budget exhausted";
					break;
				}

				const int remaining_stage_ticks = (std::min)(stage_ticks_budget, remaining_total_ticks);
				if (remaining_stage_ticks <= 0) {
					outcome.failure_reason = "target unreachable within tick budget";
					break;
				}

				const auto run = simulate_action(action, state, target, remaining_stage_ticks, current_target_index, result.events);
				outcome.segments_tested++;
				outcome.ticks_used += run.ticks_used;
				outcome.failed_stage_debug = run.stage_debug;
				sequence_priority_total += action.priority;

				sim_state_t carried_state = run.end_state;
				normalize_stage_boundary_state(carried_state);

				stage_boundary_t boundary{};
				boundary.stage_index = current_target_index + 1;
				boundary.target_kind = target.kind;
				boundary.target_pos = target.pos;
				boundary.start_state = stage_start_state;
				boundary.end_state = carried_state;
				boundary.success = run.success;
				boundary.ticks_used = run.ticks_used;
				outcome.stage_boundaries.push_back(boundary);

				state = carried_state;
				result.events = run.events;
				result.tick = state.tick;
				result.pos = state.pos;
				result.vel = state.vel;
				current_stage_ticks += run.ticks_used;

				if (run.success) {
					const float current_end_z = stage_boundary_z(state);
					if (have_successful_boundary &&
						std::fabs(current_end_z - last_success_end_z) < 0.1f &&
						std::fabs(target.pos.z - last_success_end_z) > (k_floor_z_epsilon + 0.5f)) {
						++repeated_flat_transition_count;
						if (repeated_flat_transition_count >= 1 && outcome.propagation_warning.empty()) {
							outcome.propagation_warning = std::string("no vertical progression across successive stages: previous end Z ")
								.append(std::to_string(last_success_end_z))
								.append(", current end Z ")
								.append(std::to_string(current_end_z))
								.append(", target Z ")
								.append(std::to_string(target.pos.z));
						}
					}
					else {
						repeated_flat_transition_count = 0;
					}

					last_success_end_z = current_end_z;
					have_successful_boundary = true;
					current_target_index++;
					result.reached_targets = current_target_index;
					current_stage_ticks = 0;
					if (current_target_index >= static_cast<int>(targets.size())) {
						result.success = true;
						result.combo_name = format_planned_combo(combo);
						finalize_result(result);
						const auto validation = evaluate_route_validity(result, targets);
						outcome.success = validation.valid;
						outcome.current_target_index = current_target_index;
						outcome.current_stage_ticks = current_stage_ticks;
						outcome.result = result;
						outcome.score = result.score + static_cast<float>(sequence_priority_total);
						outcome.failure_reason = validation.valid ? std::string() : validation.failure_reason;
						if (!validation.valid)
							outcome.result.success = false;
						outcome.result.score = outcome.score;
						outcome.result.failed_stage = validation.valid ? 0 : current_target_index + 1;
						return outcome;
					}
					continue;
				}

				outcome.failure_reason = run.reject_reason;
				break;
			}

			result.success = false;
			result.reached_targets = current_target_index;
			result.tick = state.tick;
			result.pos = state.pos;
			result.vel = state.vel;
			result.failed_stage = outcome.failed_stage_debug.stage_index > 0
				? outcome.failed_stage_debug.stage_index
				: (current_target_index < static_cast<int>(targets.size()) ? current_target_index + 1 : current_target_index);
			result.closest_tick = outcome.failed_stage_debug.tick;
			result.closest_xy_delta = outcome.failed_stage_debug.xy_delta == FLT_MAX ? 0.0f : outcome.failed_stage_debug.xy_delta;
			result.closest_z_delta = outcome.failed_stage_debug.z_delta == FLT_MAX ? 0.0f : outcome.failed_stage_debug.z_delta;
			result.airborne_sample_z = outcome.failed_stage_debug.airborne_sample_z;
			result.landing_position_z = outcome.failed_stage_debug.landing_position_z;
			result.landed_ground_z = outcome.failed_stage_debug.landed_ground_z;
			result.target_floor_z = outcome.failed_stage_debug.target_floor_z;
			result.compared_floor_z = outcome.failed_stage_debug.compared_floor_z;
			result.resolved_floor_z = outcome.failed_stage_debug.resolved_floor_z;
			result.previous_floor_z = outcome.failed_stage_debug.previous_floor_z;
			result.start_floor_z = outcome.failed_stage_debug.start_floor_z;
			result.expected_floor_rise = outcome.failed_stage_debug.expected_floor_rise;
			result.actual_floor_rise = outcome.failed_stage_debug.actual_floor_rise;
			result.landing_detected = outcome.failed_stage_debug.landing_detected;
			result.stage_window_entered = outcome.failed_stage_debug.stage_window_entered;
			result.contact_detected = outcome.failed_stage_debug.contact_detected;
			result.height_match = outcome.failed_stage_debug.height_match;
			result.radius_match = outcome.failed_stage_debug.radius_match;
			result.edge_ready = outcome.failed_stage_debug.edge_ready;
			result.window_enter_tick = outcome.failed_stage_debug.window_enter_tick;
			result.window_exit_tick = outcome.failed_stage_debug.window_exit_tick;
			result.landing_tick = outcome.failed_stage_debug.landing_tick;
			result.stale_ground_reused = outcome.failed_stage_debug.stale_ground_reused;
			result.floor_probe_hit = outcome.failed_stage_debug.floor_probe_hit;
			result.floor_probe_height_match = outcome.failed_stage_debug.floor_probe_height_match;
			result.floor_probe_disagreed_with_ground = outcome.failed_stage_debug.floor_probe_disagreed_with_ground;
			result.landed_on_lower_floor = outcome.failed_stage_debug.landed_on_lower_floor;
			result.target_local_probe_mismatch = outcome.failed_stage_debug.target_local_probe_mismatch;
			result.compared_floor_source = outcome.failed_stage_debug.compared_floor_source;
			result.stage_failure_detail = outcome.failed_stage_debug.reject_reason;
			result.combo_name = format_planned_combo(combo);
			finalize_result(result);

			if (outcome.failure_reason.empty()) {
				if (combo.empty())
					outcome.failure_reason = "no valid combinations generated";
				else if (current_target_index < static_cast<int>(targets.size()) &&
					static_cast<int>(combo.size()) <= current_target_index)
					outcome.failure_reason = "combo exhausted before all route stages were reached";
				else
					outcome.failure_reason = "no route reached the pixelsurf target";
			}

			outcome.success = false;
			outcome.current_target_index = current_target_index;
			outcome.current_stage_ticks = current_stage_ticks;
			outcome.score = partial_score(result, current_target_index, targets, state, current_stage_ticks) + static_cast<float>(sequence_priority_total);
			outcome.result = std::move(result);
			outcome.result.score = outcome.score;
			outcome.result.failure_reason = outcome.failure_reason;
			if (!outcome.propagation_warning.empty() && outcome.result.stage_failure_detail.empty())
				outcome.result.stage_failure_detail = outcome.propagation_warning;
			return outcome;
		}

		bool better_success_result(const result_t& lhs, const result_t& rhs) {
			if (lhs.success != rhs.success)
				return lhs.success;
			if (lhs.reached_targets != rhs.reached_targets)
				return lhs.reached_targets > rhs.reached_targets;
			if (lhs.score != rhs.score)
				return lhs.score < rhs.score;
			if (lhs.tick != rhs.tick)
				return lhs.tick < rhs.tick;
			if (lhs.events.size() != rhs.events.size())
				return lhs.events.size() < rhs.events.size();
			return lhs.full_sequence < rhs.full_sequence;
		}

		bool better_partial_result(const result_t& lhs, const result_t& rhs) {
			if (lhs.reached_targets != rhs.reached_targets)
				return lhs.reached_targets > rhs.reached_targets;
			if (lhs.failed_stage != rhs.failed_stage)
				return lhs.failed_stage > rhs.failed_stage;
			if (lhs.closest_xy_delta != rhs.closest_xy_delta)
				return lhs.closest_xy_delta < rhs.closest_xy_delta;
			if (lhs.closest_z_delta != rhs.closest_z_delta)
				return lhs.closest_z_delta < rhs.closest_z_delta;
			if (lhs.closest_tick != rhs.closest_tick)
				return lhs.closest_tick < rhs.closest_tick;
			if (lhs.score != rhs.score)
				return lhs.score < rhs.score;
			return lhs.full_sequence < rhs.full_sequence;
		}

		sim_state_t build_initial_state() {
			sim_state_t state{};
			state.pos = g::local->origin();
			state.vel = g::local->velocity();
			state.on_ground = (g::local->flags() & fl_onground) != 0;
			state.prev_on_ground = state.on_ground;
			state.ducking = (g::local->flags() & fl_ducking) != 0;
			state.prev_ducking = state.ducking;
			state.ground_z = state.pos.z;
			state.stamina = g::local->stamina();
			state.tick = 0;

			float ground_z = 0.0f;
			if (trace_ground_hull(state.pos, state.ducking, ground_z, nullptr, 16.0f))
				state.ground_z = ground_z;

			return state;
		}

		std::string vec3_to_string(const vec3_t& value) {
			std::ostringstream stream;
			stream << std::fixed << std::setprecision(3)
				<< "(" << value.x << ", " << value.y << ", " << value.z << ")";
			return stream.str();
		}

		std::string timestamp_string(const bool for_filename) {
			const std::time_t now = std::time(nullptr);
			std::tm local_time{};
			localtime_s(&local_time, &now);

			std::ostringstream stream;
			if (for_filename)
				stream << std::put_time(&local_time, "%Y%m%d_%H%M%S");
			else
				stream << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
			return stream.str();
		}

		std::string create_debug_log_path() {
			const std::filesystem::path directory("C:\\dna\\route_calc_logs");
			std::error_code error_code;
			std::filesystem::create_directories(directory, error_code);
			return (directory / ("route_calc_debug_" + timestamp_string(true) + ".txt")).string();
		}

		void append_log_line(solver_job_t& job, const std::string& line) {
			if (job.log_path.empty())
				return;

			std::ofstream file(job.log_path, std::ios::app);
			if (!file.is_open())
				return;

			file << line << "\n";
		}

		void update_debug_best_candidate(debug_stats_t& stats, const result_t& candidate) {
			stats.best_candidate_score = candidate.score;
			stats.best_candidate_tick = candidate.tick;
			stats.best_candidate_sequence = candidate.full_sequence.empty() ? std::string("no route") : candidate.full_sequence;
			if (!candidate.success) {
				stats.best_near_success_stage = candidate.failed_stage;
				stats.best_near_success_tick = candidate.closest_tick;
				stats.best_near_success_xy_delta = candidate.closest_xy_delta;
				stats.best_near_success_z_delta = candidate.closest_z_delta;
				stats.best_near_success_landing = candidate.landing_detected;
				stats.best_near_success_window = candidate.stage_window_entered;
				stats.best_near_success_contact = candidate.contact_detected;
				stats.best_near_success_window_enter_tick = candidate.window_enter_tick;
				stats.best_near_success_window_exit_tick = candidate.window_exit_tick;
				stats.best_near_success_landing_tick = candidate.landing_tick;
				stats.best_near_success_reason = !candidate.stage_failure_detail.empty()
					? candidate.stage_failure_detail
					: candidate.failure_reason;
			}
		}

		void publish_progress_snapshot(const solver_job_t& job) {
			shared_snapshot_t snapshot{};
			snapshot.ready = true;
			snapshot.replace_results = false;
			snapshot.replace_render_targets = true;
			snapshot.targets = job.targets;
			snapshot.stats = job.stats;

			std::lock_guard<std::mutex> lock(g_snapshot_mutex);
			g_pending_snapshot = std::move(snapshot);
		}

		void publish_final_snapshot(
			const solver_job_t& job,
			const result_t& result,
			const std::vector<result_t>& results,
			const bool update_render_targets)
		{
			shared_snapshot_t snapshot{};
			snapshot.ready = true;
			snapshot.replace_results = true;
			snapshot.replace_render_targets = update_render_targets;
			snapshot.result = result;
			snapshot.results = results;
			snapshot.targets = job.targets;
			snapshot.stats = job.stats;

			std::lock_guard<std::mutex> lock(g_snapshot_mutex);
			g_pending_snapshot = std::move(snapshot);
		}

		void sync_visible_state() {
			shared_snapshot_t snapshot{};
			{
				std::lock_guard<std::mutex> lock(g_snapshot_mutex);
				if (!g_pending_snapshot.ready)
					return;

				snapshot = std::move(g_pending_snapshot);
				g_pending_snapshot = {};
			}

			g_last_stats = snapshot.stats;
			g_last_targets = snapshot.targets;

			if (snapshot.replace_render_targets)
				g_render_targets = snapshot.targets;

			if (snapshot.replace_results) {
				g_last_result = snapshot.result;
				g_last_results = snapshot.results;
				g_render_result = snapshot.result;
				g_render_results = snapshot.results;
			}
		}

		void refresh_elapsed(solver_job_t& job) {
			job.stats.elapsed_time_ms = elapsed_time_ms(job);
		}

		std::string current_map_name() {
			if (!interfaces::engine)
				return "unknown";

			const char* level_name = interfaces::engine->get_level_name();
			return (level_name && *level_name) ? std::string(level_name) : std::string("unknown");
		}

		std::string route_segment_summary(const request_t& request) {
			if (request.targets.empty())
				return "none";

			std::ostringstream stream;
			for (std::size_t index = 0; index < request.targets.size(); ++index) {
				if (index > 0)
					stream << " | ";

				if (index == 0)
					stream << "segment 0: origin -> point 0 " << target_kind_name(request.targets[index].kind);
				else
					stream << "segment " << index << ": point " << (index - 1) << " -> point " << index << " " << target_kind_name(request.targets[index].kind);
			}

			return stream.str();
		}

		std::string candidate_generation_summary(
			const request_t& request,
			const std::vector<std::vector<planned_action_t>>& stage_alphabets)
		{
			if (request.targets.empty() || stage_alphabets.empty())
				return "none";

			std::ostringstream stream;
			for (std::size_t index = 0; index < request.targets.size(); ++index) {
				if (index > 0)
					stream << " | ";
				stream << "segment " << (index + 1)
					<< " -> " << target_kind_name(request.targets[index].kind)
					<< ": " << format_action_alphabet(stage_alphabets[index], true);
			}

			return stream.str();
		}

		result_t build_failure_result(const solver_job_t& job, const std::string& reason) {
			result_t result{};
			result.success = false;
			result.total_targets = static_cast<int>(job.targets.size());
			result.sequence = "no route";
			result.full_sequence = "no route";
			result.failure_reason = reason;
			result.combinations_tested = job.stats.combinations_tested;
			result.elapsed_time_ms = job.stats.elapsed_time_ms;
			return result;
		}

		void set_job_failure(solver_job_t& job, const std::string& reason) {
			job.active = false;
			job.stats.status = "failed";
			job.stats.failure_reason = reason;
			refresh_elapsed(job);

			result_t result = build_failure_result(job, reason);
			std::vector<result_t> results{ result };

			append_log_line(job, std::string("Status: failed"));
			append_log_line(job, std::string("Failure Reason: ").append(reason));
			append_log_line(job, std::string("Elapsed: ").append(std::to_string(job.stats.elapsed_time_ms)).append(" ms"));
			append_log_line(job, "Worker Shutdown: complete");
			publish_final_snapshot(job, result, results, true);
		}

		void publish_success_results(solver_job_t& job) {
			job.active = false;
			job.stats.status = "success";
			job.stats.failure_reason.clear();
			refresh_elapsed(job);

			std::sort(job.successes.begin(), job.successes.end(), better_success_result);
			if (static_cast<int>(job.successes.size()) > k_internal_success_cap)
				job.successes.resize(k_internal_success_cap);

			const int display_limit = std::clamp(c::movement::pscalc_max_displayed_combos, 1, 10);
			std::vector<result_t> visible_results = job.successes;
			if (static_cast<int>(visible_results.size()) > display_limit)
				visible_results.resize(display_limit);

			result_t visible_result = visible_results.empty() ? result_t{} : visible_results.front();
			visible_result.combinations_tested = job.stats.combinations_tested;
			visible_result.elapsed_time_ms = job.stats.elapsed_time_ms;
			visible_result.bind_sequence = visible_result.full_sequence;

			append_log_line(job, "Status: success");
			append_log_line(job, std::string("Combinations Tried: ").append(std::to_string(job.stats.combinations_tested)).append("/").append(std::to_string(job.stats.combination_space_total)));
			append_log_line(job, std::string("Ticks Simulated: ").append(std::to_string(job.stats.ticks_simulated)));
			append_log_line(job, std::string("Nodes Expanded: ").append(std::to_string(job.stats.nodes_expanded)));
			append_log_line(job, std::string("Best Candidate Score: ").append(std::to_string(job.stats.best_candidate_score)));
			append_log_line(job, std::string("Best Candidate Tick: ").append(std::to_string(job.stats.best_candidate_tick)));
			append_log_line(job, std::string("Best Candidate Sequence: ").append(job.stats.best_candidate_sequence));
			append_log_line(job, std::string("Winning Combo: ").append(visible_result.combo_name.empty() ? visible_result.full_sequence : visible_result.combo_name));
			append_log_line(job, std::string("Final Result: ").append(visible_result.full_sequence));
			append_log_line(job, std::string("Bind Export: ").append(visible_result.bind_sequence));
			append_log_line(job, "Pixelsurf Validation: passed");
			append_log_line(job, std::string("Elapsed: ").append(std::to_string(job.stats.elapsed_time_ms)).append(" ms"));

			if (!visible_result.events.empty()) {
				append_log_line(job, "Events:");
				for (const auto& event : visible_result.events) {
					append_log_line(job,
						std::string("  tick ").append(std::to_string(event.tick))
						.append(" | ")
						.append(event_name(event).empty() ? "(none)" : event_name(event))
						.append(" | pos ").append(vec3_to_string(event.pos))
						.append(" | vel ").append(vec3_to_string(event.vel)));
				}
			}

			append_log_line(job, "Worker Shutdown: complete");
			publish_final_snapshot(job, visible_result, visible_results, true);
		}

		void publish_failure_results(solver_job_t& job) {
			job.active = false;
			job.stats.status = "failed";
			refresh_elapsed(job);

			if (!job.has_partial) {
				result_t result = build_failure_result(job, job.stats.failure_reason.empty()
					? std::string("no supported action sequence reached the route targets")
					: job.stats.failure_reason);
				job.best_partial = result;
				job.has_partial = true;
			}

			job.best_partial.combinations_tested = job.stats.combinations_tested;
			job.best_partial.elapsed_time_ms = job.stats.elapsed_time_ms;
			std::vector<result_t> results{ job.best_partial };

			append_log_line(job, "Status: failed");
			append_log_line(job, std::string("Combinations Tried: ").append(std::to_string(job.stats.combinations_tested)).append("/").append(std::to_string(job.stats.combination_space_total)));
			append_log_line(job, std::string("Ticks Simulated: ").append(std::to_string(job.stats.ticks_simulated)));
			append_log_line(job, std::string("Nodes Expanded: ").append(std::to_string(job.stats.nodes_expanded)));
			append_log_line(job, std::string("Failure Reason: ").append(job.stats.failure_reason));
			append_log_line(job, std::string("Best Candidate Sequence: ").append(job.stats.best_candidate_sequence.empty() ? "no route" : job.stats.best_candidate_sequence));
			append_log_line(job, std::string("Best Near-Success Stage: ").append(std::to_string(job.stats.best_near_success_stage)));
			append_log_line(job, std::string("Best Near-Success XY Delta: ").append(std::to_string(job.stats.best_near_success_xy_delta)));
			append_log_line(job, std::string("Best Near-Success Z Delta: ").append(std::to_string(job.stats.best_near_success_z_delta)));
			append_log_line(job, std::string("Best Near-Success Tick: ").append(std::to_string(job.stats.best_near_success_tick)));
			append_log_line(job, std::string("Best Near-Success Landing: ").append(job.stats.best_near_success_landing ? "yes" : "no"));
			append_log_line(job, std::string("Best Near-Success Window: ").append(job.stats.best_near_success_window ? "yes" : "no"));
			append_log_line(job, std::string("Best Near-Success Contact: ").append(job.stats.best_near_success_contact ? "yes" : "no"));
			append_log_line(job,
				std::string("Best Near-Success Window Enter Tick: ").append(std::to_string(job.stats.best_near_success_window_enter_tick))
				.append(" | Window Exit Tick: ").append(std::to_string(job.stats.best_near_success_window_exit_tick))
				.append(" | Landing Tick: ").append(std::to_string(job.stats.best_near_success_landing_tick)));
			append_log_line(job, std::string("Best Near-Success Reason: ").append(job.stats.best_near_success_reason.empty() ? "n/a" : job.stats.best_near_success_reason));
			append_log_line(job, "Pixelsurf Validation: failed");
			append_log_line(job, std::string("Elapsed: ").append(std::to_string(job.stats.elapsed_time_ms)).append(" ms"));
			append_log_line(job,
				std::string("Best Partial Stage Detail: stage ").append(std::to_string(job.best_partial.failed_stage))
				.append("/").append(std::to_string(job.best_partial.total_targets))
				.append(" | xy ").append(std::to_string(job.best_partial.closest_xy_delta))
				.append(" | z ").append(std::to_string(job.best_partial.closest_z_delta))
				.append(" | landing ").append(job.best_partial.landing_detected ? "yes" : "no")
				.append(" | window ").append(job.best_partial.stage_window_entered ? "yes" : "no")
				.append(" | window ticks ").append(std::to_string(job.best_partial.window_enter_tick))
				.append("..").append(std::to_string(job.best_partial.window_exit_tick))
				.append(" | landing tick ").append(std::to_string(job.best_partial.landing_tick))
				.append(" | compared floor z ").append(std::to_string(job.best_partial.compared_floor_z))
				.append(" | source ").append(job.best_partial.compared_floor_source.empty() ? "n/a" : job.best_partial.compared_floor_source)
				.append(" | expected rise ").append(std::to_string(job.best_partial.expected_floor_rise))
				.append(" | actual rise ").append(std::to_string(job.best_partial.actual_floor_rise))
				.append(" | contact ").append(job.best_partial.contact_detected ? "yes" : "no")
				.append(" | detail ").append(job.best_partial.stage_failure_detail.empty() ? "n/a" : job.best_partial.stage_failure_detail));

			if (!job.best_partial.events.empty()) {
				append_log_line(job, "Best Candidate Events:");
				for (const auto& event : job.best_partial.events) {
					append_log_line(job,
						std::string("  tick ").append(std::to_string(event.tick))
						.append(" | ")
						.append(event_name(event).empty() ? "(none)" : event_name(event))
						.append(" | pos ").append(vec3_to_string(event.pos))
						.append(" | vel ").append(vec3_to_string(event.vel)));
				}
			}

			append_log_line(job, "Worker Shutdown: complete");
			publish_final_snapshot(job, job.best_partial, results, true);
		}

		void publish_cancelled_results(solver_job_t& job, const std::string& reason) {
			job.active = false;
			job.stats.status = "cancelled";
			job.stats.failure_reason = reason;
			refresh_elapsed(job);

			result_t result = job.has_partial ? job.best_partial : build_failure_result(job, reason);
			result.success = false;
			result.failure_reason = reason;
			result.combinations_tested = job.stats.combinations_tested;
			result.elapsed_time_ms = job.stats.elapsed_time_ms;
			std::vector<result_t> results{ result };

			append_log_line(job, "Status: cancelled");
			append_log_line(job, std::string("Combinations Tried Before Cancel: ").append(std::to_string(job.stats.combinations_tested)));
			append_log_line(job, std::string("Elapsed: ").append(std::to_string(job.stats.elapsed_time_ms)).append(" ms"));
			append_log_line(job, std::string("Last Combo: ").append(job.stats.current_combo.empty() ? "(none)" : job.stats.current_combo));
			append_log_line(job, "Worker Shutdown: complete");
			publish_final_snapshot(job, result, results, true);
		}

		solver_job_t create_job(const request_t& request) {
			solver_job_t job{};
			job.active = true;
			job.request = request;
			job.targets = request.targets;
			job.start_state = build_initial_state();
			job.exact_stage_count = static_cast<int>(request.targets.size());
			job.current_pass = 1;
			job.combo_cursor = 0;
			job.progress_update_interval = combos_per_update_budget();
			job.stage_tick_budget = stage_tick_budget();
			job.total_tick_budget = total_tick_budget(job.exact_stage_count);
			job.max_nodes = combo_nodes_budget();
			job.log_path = create_debug_log_path();
			job.started_at = std::chrono::steady_clock::now();
			job.stage_alphabets.reserve(request.targets.size());
			for (std::size_t stage_index = 0; stage_index < request.targets.size(); ++stage_index)
				job.stage_alphabets.push_back(build_stage_action_alphabet(request.targets[stage_index], static_cast<int>(stage_index)));

			job.stats.calculate_requests = ++g_calculate_request_counter;
			job.stats.status = "validating";
			job.stats.placed_points = request.placed_points;
			job.stats.placed_floor_points = request.placed_floor_points;
			job.stats.placed_pixelsurf_points = request.placed_pixelsurf_points;
			job.stats.points_loaded = static_cast<int>(request.targets.size());
			job.stats.exact_stage_count = job.exact_stage_count;
			job.stats.chained_mode = request.chained_mode;
			job.stats.target_order = request.target_order.empty() ? "(none)" : request.target_order;
			job.stats.segment_summary = route_segment_summary(request);
			job.stats.current_pass = 0;
			job.stats.current_combo_size = job.exact_stage_count;
			job.stats.current_target_index = request.targets.empty() ? 0 : 1;
			job.stats.current_target_stage_ticks = 0;
			job.stats.branch_limit = 0;
			for (const auto& stage_alphabet : job.stage_alphabets)
				job.stats.branch_limit = (std::max)(job.stats.branch_limit, static_cast<int>(stage_alphabet.size()));
			job.stats.depth_limit = job.exact_stage_count;
			job.stats.tick_budget = job.stage_tick_budget;
			job.stats.total_tick_budget = job.total_tick_budget;
			job.stats.max_nodes = job.max_nodes;
			job.stats.combo_formula = build_exact_combo_formula(job.stage_alphabets);
			job.stats.combination_space_total = exact_combo_total(job.stage_alphabets, job.stats.combination_space_exact);
			job.stats.combinations_generated = job.stats.combination_space_total;
			job.stats.candidate_generation_summary = candidate_generation_summary(request, job.stage_alphabets);
			std::vector<planned_action_t> union_alphabet{};
			for (const auto& stage_alphabet : job.stage_alphabets) {
				for (const auto& action : stage_alphabet) {
					const auto existing = std::find_if(union_alphabet.begin(), union_alphabet.end(), [&](const planned_action_t& saved) {
						return saved.sequence_name == action.sequence_name;
					});
					if (existing == union_alphabet.end())
						union_alphabet.push_back(action);
				}
			}
			job.stats.action_alphabet = format_action_alphabet(union_alphabet, true);
			job.stats.current_phase = "validating request";
			job.stats.current_target_pos = request.targets.empty() ? "n/a" : vec3_to_string(request.targets.front().pos);
			job.stats.current_combo = "waiting for worker";
			job.stats.current_events = "none";
			job.stats.movement_constants = std::string("jump=")
				.append(std::to_string(k_jump_impulse))
				.append(" gravity=").append(std::to_string(k_gravity))
				.append(" dt=").append(std::to_string(k_sim_dt))
				.append(" floor_z=").append(std::to_string(k_floor_z_epsilon))
				.append(" prune=").append(std::to_string(min_prune_ticks_budget()))
				.append(" z=").append(std::to_string(strict_z_tolerance()))
				.append(" lax_z=").append(std::to_string(lax_z_tolerance()))
				.append(" | ").append(features::movement::pixelsurf_shared::discrete_formula_summary());
			job.stats.log_path = job.log_path;
			refresh_elapsed(job);

			{
				std::ofstream file(job.log_path, std::ios::trunc);
				if (file.is_open()) {
					file << "route calculator debug log\n";
					file << "timestamp: " << timestamp_string(false) << "\n";
					file << "calculator version: deterministic_exact_stage_v3\n";
					file << "map: " << current_map_name() << "\n";
				}
			}

			append_log_line(job, "Thread Start Confirmation: pending");
			append_log_line(job, "Solver Version: deterministic_exact_stage_v3");
			append_log_line(job, std::string("Exact Stage Mode: ").append(c::movement::pscalc_exact_stage_mode ? "on" : "off"));
			append_log_line(job, std::string("First Stage Pre-Prune: ").append(c::movement::pscalc_no_first_stage_prune ? "disabled" : "enabled"));
			append_log_line(job, std::string("Verbose Stage Debug: ").append(c::movement::pscalc_log_stage_debug ? "on" : "off"));
			append_log_line(job, "Point Order Mode: placed_order");
			append_log_line(job, "Selected Point Feature: removed");
			append_log_line(job, std::string("Log Path: ").append(job.log_path));
			append_log_line(job, std::string("Placed Points: ").append(std::to_string(request.placed_points))
				.append(" (floor: ").append(std::to_string(request.placed_floor_points))
				.append(", pixelsurf: ").append(std::to_string(request.placed_pixelsurf_points)).append(")"));
			append_log_line(job, std::string("Route Segments: ").append(std::to_string(job.exact_stage_count)));
			append_log_line(job, std::string("Route Segment List: ").append(job.stats.segment_summary));
			append_log_line(job, std::string("Loaded Targets: ").append(std::to_string(static_cast<int>(request.targets.size()))));
			append_log_line(job, std::string("Chained Mode: ").append(request.chained_mode ? "on" : "off"));
			append_log_line(job, std::string("Target Order: ").append(job.stats.target_order));
			append_log_line(job, std::string("Stage Catalog Union: ").append(job.stats.action_alphabet));
			append_log_line(job, std::string("Candidate Generation Summary: ").append(job.stats.candidate_generation_summary));
			append_log_line(job, std::string("Combo Formula: ").append(job.stats.combo_formula));
			append_log_line(job, std::string("Exact Total Combinations: ").append(std::to_string(job.stats.combination_space_total)));
			append_log_line(job, std::string("Tick Budget Per Stage: ").append(std::to_string(job.stage_tick_budget)));
			append_log_line(job, std::string("Total Tick Budget: ").append(std::to_string(job.stats.total_tick_budget)));
			append_log_line(job, std::string("Max Combo Leaves: ").append(std::to_string(job.max_nodes)));
			append_log_line(job, std::string("Worker Progress Update Interval: ").append(std::to_string(job.progress_update_interval)));
			append_log_line(job, std::string("Player Origin: ").append(vec3_to_string(job.start_state.pos)));
			append_log_line(job, std::string("Start Velocity: ").append(vec3_to_string(job.start_state.vel)));
			append_log_line(job, std::string("Start Grounded: ").append(job.start_state.on_ground ? "true" : "false"));
			append_log_line(job, std::string("Physics: ").append(job.stats.movement_constants));

			for (std::size_t i = 0; i < request.targets.size(); ++i) {
				append_log_line(job,
					std::string("Point #").append(std::to_string(i))
					.append(": type=").append(target_kind_name(request.targets[i].kind))
					.append(" pos=").append(vec3_to_string(request.targets[i].pos))
					.append(" normal=").append(vec3_to_string(request.targets[i].normal))
					.append(" radius=").append(std::to_string(request.targets[i].radius))
					.append(" stage_catalog=").append(format_action_alphabet(job.stage_alphabets[i], true)));
			}

			return job;
		}

		void finalize_job(solver_job_t& job) {
			if (cancel_requested()) {
				publish_cancelled_results(job, "cancelled by user");
				return;
			}

			if (job.has_success)
				publish_success_results(job);
			else
				publish_failure_results(job);
		}

		void run_solver_job(solver_job_t job) {
			append_log_line(job, "Thread Start Confirmation: worker started");
			publish_progress_snapshot(job);

			const bool has_pixelsurf_target = std::any_of(job.targets.begin(), job.targets.end(), [](const target_t& target) {
				return target.kind == target_kind_t::pixelsurf_point;
			});

			const bool request_valid =
				!job.targets.empty() &&
				has_pixelsurf_target &&
				job.stats.combination_space_total > 0;
			append_log_line(job, std::string("Calculate bind accepted: ").append(request_valid ? "yes" : "no"));

			if (job.targets.empty()) {
				set_job_failure(job, "no planner points loaded");
				return;
			}

			if (!has_pixelsurf_target) {
				set_job_failure(job, "route does not contain a pixelsurf target");
				return;
			}

			if (job.stage_alphabets.empty() || job.stats.combination_space_total <= 0) {
				set_job_failure(job, "no valid combinations generated");
				return;
			}

			for (std::size_t stage_index = 0; stage_index < job.stage_alphabets.size(); ++stage_index) {
				if (job.stage_alphabets[stage_index].empty()) {
					set_job_failure(job, std::string("stage ").append(std::to_string(stage_index + 1)).append(" has no legal actions"));
					return;
				}
			}

			if (job.stats.combination_space_total > job.max_nodes) {
				set_job_failure(job,
					std::string("exact combination space ")
					.append(std::to_string(job.stats.combination_space_total))
					.append(" exceeds configured max combo leaves ")
					.append(std::to_string(job.max_nodes)));
				return;
			}

			job.stats.status = "running";
			job.stats.current_phase = "simulating exact stage catalogs";
			publish_progress_snapshot(job);
			job.stats.current_combo = "waiting for first combination";
			publish_progress_snapshot(job);

			while (job.active) {
				if (cancel_requested()) {
					publish_cancelled_results(job, "cancelled by user");
					return;
				}

				if (job.combo_cursor >= static_cast<std::size_t>(job.stats.combination_space_total)) {
					if (!job.has_success && job.stats.failure_reason.empty()) {
						if (job.stats.combination_space_total <= 0)
							job.stats.failure_reason = "no valid combinations generated";
						else if (job.stats.ticks_simulated <= 0)
							job.stats.failure_reason = "the planner never simulated a tick";
						else
							job.stats.failure_reason = "no route reached the pixelsurf target";
					}
					finalize_job(job);
					return;
				}

				const auto combo = decode_combo(job.stage_alphabets, job.combo_cursor);
				const int combo_size = static_cast<int>(combo.size());
				job.stats.current_pass = 1;
				job.stats.current_combo_size = combo_size;
				job.stats.current_combo = format_combo_debug(combo);
				job.stats.current_phase = "simulating exact stage catalogs";
				append_log_line(job,
					std::string("Combo Attempt ").append(std::to_string(job.stats.combinations_tested + 1))
					.append("/").append(std::to_string(job.stats.combination_space_total))
					.append(": ").append(job.stats.current_combo));

				const auto outcome = simulate_combo(
					combo,
					job.targets,
					job.start_state,
					job.stage_tick_budget,
					job.total_tick_budget);
				job.stats.combinations_tested++;
				job.stats.segments_tested += outcome.segments_tested;
				job.stats.nodes_expanded += outcome.segments_tested;
				job.stats.ticks_simulated += outcome.ticks_used;
				job.stats.current_target_index = outcome.current_target_index >= static_cast<int>(job.targets.size())
					? static_cast<int>(job.targets.size())
					: outcome.current_target_index + 1;
				job.stats.current_target_pos =
					(outcome.current_target_index >= 0 && outcome.current_target_index < static_cast<int>(job.targets.size()))
					? vec3_to_string(job.targets[outcome.current_target_index].pos)
					: std::string("complete");
				job.stats.current_target_stage_ticks = outcome.current_stage_ticks;
				job.stats.current_stage_transition = outcome.stage_transition_summary;
				job.stats.current_events = outcome.result.full_sequence.empty() ? "no route" : outcome.result.full_sequence;
				job.stats.propagation_warning = outcome.propagation_warning;
				refresh_elapsed(job);

				if (outcome.success) {
					job.stats.success_candidates++;
					job.stats.last_successful_combo = outcome.result.full_sequence;
					update_debug_best_candidate(job.stats, outcome.result);

					const auto duplicate = std::find_if(job.successes.begin(), job.successes.end(), [&](const result_t& existing) {
						return existing.full_sequence == outcome.result.full_sequence &&
							existing.tick == outcome.result.tick &&
							existing.reached_targets == outcome.result.reached_targets;
					});
					if (duplicate == job.successes.end())
						job.successes.push_back(outcome.result);

					if (!job.has_success || better_success_result(outcome.result, job.best_success))
						job.best_success = outcome.result;
					job.has_success = true;

					append_log_line(job,
						std::string("Success Candidate: combo=").append(job.stats.current_combo)
						.append(" | result=").append(outcome.result.full_sequence)
						.append(" | tick=").append(std::to_string(outcome.result.tick))
						.append(" | pos=").append(vec3_to_string(outcome.result.pos))
						.append(" | vel=").append(vec3_to_string(outcome.result.vel)));
				}
				else {
					job.stats.failure_candidates++;
					if (!outcome.failure_reason.empty())
						job.stats.failure_reason = outcome.failure_reason;
					append_log_line(job,
						std::string("Combo Rejected: combo=").append(job.stats.current_combo)
						.append(" | reason=").append(outcome.failure_reason.empty() ? "unknown" : outcome.failure_reason)
						.append(" | reached=").append(std::to_string(outcome.result.reached_targets))
						.append("/").append(std::to_string(outcome.result.total_targets))
						.append(" | ticks=").append(std::to_string(outcome.ticks_used)));
					if (c::movement::pscalc_log_stage_debug) {
						if (!outcome.stage_transition_summary.empty())
							append_log_line(job, std::string("  Stage Transition: ").append(outcome.stage_transition_summary));
						if (!outcome.propagation_warning.empty())
							append_log_line(job, std::string("  Propagation Warning: ").append(outcome.propagation_warning));
						for (const auto& boundary : outcome.stage_boundaries) {
							append_log_line(job,
								std::string("  Stage ")
								.append(std::to_string(boundary.stage_index))
								.append(" Start: pos=").append(vec3_to_string(boundary.start_state.pos))
								.append(" | vel=").append(vec3_to_string(boundary.start_state.vel))
								.append(" | grounded=").append(boundary.start_state.on_ground ? "yes" : "no")
								.append(" | z=").append(std::to_string(stage_boundary_z(boundary.start_state)))
								.append(" | target=").append(vec3_to_string(boundary.target_pos)));
							append_log_line(job,
								std::string("  Stage ")
								.append(std::to_string(boundary.stage_index))
								.append(" End: pos=").append(vec3_to_string(boundary.end_state.pos))
								.append(" | vel=").append(vec3_to_string(boundary.end_state.vel))
								.append(" | grounded=").append(boundary.end_state.on_ground ? "yes" : "no")
								.append(" | z=").append(std::to_string(stage_boundary_z(boundary.end_state)))
								.append(" | success=").append(boundary.success ? "yes" : "no")
								.append(" | ticks=").append(std::to_string(boundary.ticks_used)));
						}
						append_log_line(job,
							std::string("  Stage Type: ").append(outcome.failed_stage_debug.stage_kind == target_kind_t::floor_point ? "floor" : "pixelsurf"));
						append_log_line(job,
							std::string("  Failed Stage: ").append(std::to_string(outcome.result.failed_stage))
							.append("/").append(std::to_string(outcome.result.total_targets))
							.append(" | Candidate Pos: ").append(vec3_to_string(outcome.result.pos))
							.append(" | Target Pos: ").append(vec3_to_string(outcome.failed_stage_debug.target_pos)));
						append_log_line(job,
							std::string("  Raw Candidate Z: ").append(std::to_string(outcome.failed_stage_debug.raw_candidate_z))
							.append(" | Post-Land Grounded Z: ").append(std::to_string(outcome.failed_stage_debug.grounded_candidate_z))
							.append(" | Raw Target Z: ").append(std::to_string(outcome.failed_stage_debug.raw_target_z)));
						append_log_line(job,
							std::string("  Applied Anchor Z: ").append(std::to_string(outcome.failed_stage_debug.applied_anchor_z))
							.append(" | Effective Compared Z: ").append(std::to_string(outcome.failed_stage_debug.compared_candidate_z))
							.append(" vs ").append(std::to_string(outcome.failed_stage_debug.compared_target_z))
							.append(" | Height Epsilon: ").append(std::to_string(outcome.failed_stage_debug.height_epsilon)));
						append_log_line(job,
							std::string("  Airborne Sample Z: ").append(std::to_string(outcome.failed_stage_debug.airborne_sample_z))
							.append(" | Landing Position Z: ").append(std::to_string(outcome.failed_stage_debug.landing_position_z))
							.append(" | Landed Ground Z: ").append(std::to_string(outcome.failed_stage_debug.landed_ground_z))
							.append(" | Target Floor Z: ").append(std::to_string(outcome.failed_stage_debug.target_floor_z)));
						append_log_line(job,
							std::string("  Compared Floor Z: ").append(std::to_string(outcome.failed_stage_debug.compared_floor_z))
							.append(" | Compared Using: ").append(outcome.failed_stage_debug.compared_floor_source.empty() ? "n/a" : outcome.failed_stage_debug.compared_floor_source)
							.append(" | Resolved Floor Z: ").append(std::to_string(outcome.failed_stage_debug.resolved_floor_z))
							.append(" | Start Floor Z: ").append(std::to_string(outcome.failed_stage_debug.start_floor_z))
							.append(" | Previous Floor Z: ").append(std::to_string(outcome.failed_stage_debug.previous_floor_z))
							.append(" | Expected Rise: ").append(std::to_string(outcome.failed_stage_debug.expected_floor_rise))
							.append(" | Actual Landed Rise: ").append(std::to_string(outcome.failed_stage_debug.actual_floor_rise)));
						append_log_line(job,
							std::string("  Floor Probe Hit: ").append(outcome.failed_stage_debug.floor_probe_hit ? "yes" : "no")
							.append(" | Probe Height Match: ").append(outcome.failed_stage_debug.floor_probe_height_match ? "yes" : "no")
							.append(" | Probe Disagreed With Ground Cache: ").append(outcome.failed_stage_debug.floor_probe_disagreed_with_ground ? "yes" : "no")
							.append(" | Landed Lower Floor: ").append(outcome.failed_stage_debug.landed_on_lower_floor ? "yes" : "no")
							.append(" | Target Probe Mismatch: ").append(outcome.failed_stage_debug.target_local_probe_mismatch ? "yes" : "no"));
						if (outcome.failed_stage_debug.stale_ground_reused)
							append_log_line(job, "  ERROR: stale grounded floor Z reused for floor-stage validation");
						append_log_line(job,
							std::string("  XY Delta: ").append(std::to_string(outcome.result.closest_xy_delta))
							.append(" | Z Delta: ").append(std::to_string(outcome.result.closest_z_delta))
							.append(" | Landing Detected: ").append(outcome.result.landing_detected ? "yes" : "no")
							.append(" | Stage Window Entered: ").append(outcome.result.stage_window_entered ? "yes" : "no")
							.append(" | Pixelsurf Contact: ").append(outcome.result.contact_detected ? "yes" : "no"));
						append_log_line(job,
							std::string("  Window Enter Tick: ").append(std::to_string(outcome.failed_stage_debug.window_enter_tick))
							.append(" | Window Exit Tick: ").append(std::to_string(outcome.failed_stage_debug.window_exit_tick))
							.append(" | Landing Tick: ").append(std::to_string(outcome.failed_stage_debug.landing_tick))
							.append(" | Landing Grace Ticks: ").append(std::to_string(k_stage_window_landing_grace_ticks)));
						append_log_line(job,
							std::string("  Radius Match: ").append(outcome.result.radius_match ? "yes" : "no")
							.append(" | Height Match: ").append(outcome.result.height_match ? "yes" : "no")
							.append(" | Edge Ready: ").append(outcome.result.edge_ready ? "yes" : "no")
							.append(" | Detail: ").append(outcome.result.stage_failure_detail.empty() ? "n/a" : outcome.result.stage_failure_detail));
					}

					if (!job.has_partial || better_partial_result(outcome.result, job.best_partial)) {
						job.best_partial = outcome.result;
						job.best_partial.failure_reason = outcome.failure_reason;
						job.has_partial = true;
						job.stats.partial_updates++;
						update_debug_best_candidate(job.stats, job.best_partial);
					}
				}

				if (job.stats.combinations_tested == 1 ||
					job.stats.combinations_tested % job.progress_update_interval == 0) {
					append_log_line(job,
						std::string("Progress: ").append(std::to_string(job.stats.combinations_tested))
						.append("/").append(std::to_string(job.stats.combination_space_total))
						.append(" | current segment ").append(std::to_string(job.stats.current_target_index))
						.append("/").append(std::to_string(job.stats.points_loaded))
						.append(" | combo ").append(job.stats.current_combo));
					publish_progress_snapshot(job);
				}

				job.combo_cursor++;
			}
		}

		void stop_worker(const bool wait_for_join) {
			if (g_solver_thread.joinable()) {
				g_cancel_requested.store(true, std::memory_order_relaxed);
				if (wait_for_join || !g_solver_running.load(std::memory_order_relaxed))
					g_solver_thread.join();
			}
		}

		void start_worker(const request_t& request) {
			stop_worker(true);
			sync_visible_state();

			g_cancel_requested.store(false, std::memory_order_relaxed);
			solver_job_t job = create_job(request);
			publish_progress_snapshot(job);
			g_solver_running.store(true, std::memory_order_relaxed);

			g_solver_thread = std::thread([job = std::move(job)]() mutable {
				run_solver_job(std::move(job));
				g_solver_running.store(false, std::memory_order_relaxed);
			});
		}
	}

	void reset() {
		stop_worker(true);
		{
			std::lock_guard<std::mutex> lock(g_snapshot_mutex);
			g_pending_snapshot = {};
		}
		g_last_result = {};
		g_last_results.clear();
		g_last_targets.clear();
		g_last_stats = {};
		g_render_result = {};
		g_render_results.clear();
		g_render_targets.clear();
		g_cancel_requested.store(false, std::memory_order_relaxed);
		g_calculation_requested.store(false, std::memory_order_relaxed);
		g_solver_running.store(false, std::memory_order_relaxed);
		g_calculate_request_counter = 0;
	}

	void update(c_usercmd* cmd, const request_t& request, const bool calculate_now) {
		sync_visible_state();
		if (!g_solver_running.load(std::memory_order_relaxed) && g_solver_thread.joinable())
			g_solver_thread.join();

		const bool requested_now = calculate_now || consume_requested_calculation();
		if (!cmd || !g::local || !interfaces::engine || !interfaces::engine->is_in_game() || !g::local->is_alive()) {
			if (requested_now) {
				solver_job_t failed_job{};
				failed_job.request = request;
				failed_job.targets = request.targets;
				failed_job.log_path = create_debug_log_path();
				failed_job.started_at = std::chrono::steady_clock::now();
				failed_job.stats.calculate_requests = ++g_calculate_request_counter;
				failed_job.stats.status = "validating";
				failed_job.stats.placed_points = request.placed_points;
				failed_job.stats.placed_floor_points = request.placed_floor_points;
				failed_job.stats.placed_pixelsurf_points = request.placed_pixelsurf_points;
				failed_job.stats.points_loaded = static_cast<int>(request.targets.size());
				failed_job.stats.log_path = failed_job.log_path;
				refresh_elapsed(failed_job);
				{
					std::ofstream file(failed_job.log_path, std::ios::trunc);
					if (file.is_open()) {
						file << "route calculator debug log\n";
						file << "timestamp: " << timestamp_string(false) << "\n";
						file << "calculator version: deterministic_exact_stage_v3\n";
					}
				}
				append_log_line(failed_job, "Calculate bind accepted: no");
				append_log_line(failed_job, "Failure Reason: missing player state");
				set_job_failure(failed_job, "missing player state");
				sync_visible_state();
				stop_worker(true);
				return;
			}
			reset();
			return;
		}

		if (requested_now)
			start_worker(request);
	}

	void draw() {
		if (!c::movement::pscalc_enable_draw || !g::local || !interfaces::engine || !interfaces::engine->is_in_game() || !g::local->is_alive())
			return;

		const std::vector<target_t>* overlay_targets = nullptr;
		const result_t* overlay_result = nullptr;
		if (!g_render_targets.empty()) {
			overlay_targets = &g_render_targets;
			if (!g_render_results.empty())
				overlay_result = &g_render_results.front();
		}
		else if (!g_last_targets.empty()) {
			overlay_targets = &g_last_targets;
			if (!g_last_results.empty())
				overlay_result = &g_last_results.front();
		}

		if (!overlay_targets)
			return;

		const color_t success_color(90, 220, 120, 255);
		const color_t pending_color(255, 215, 90, 220);

		for (std::size_t i = 0; i < overlay_targets->size(); ++i) {
			vec3_t screen{};
			if (interfaces::debug_overlay->world_to_screen((*overlay_targets)[i].pos, screen)) {
				const color_t target_color = overlay_result && static_cast<int>(i) < overlay_result->reached_targets
					? success_color
					: pending_color;
				im_render.drawcircle(screen.x, screen.y, 6.0f, 24, target_color, 2.0f);
			}
		}

		if (overlay_result && overlay_result->success) {
			vec3_t landing_screen{};
			if (interfaces::debug_overlay->world_to_screen(overlay_result->pos, landing_screen))
				im_render.drawcircle(landing_screen.x, landing_screen.y, 8.0f, 32, success_color, 2.0f);
		}

		const debug_stats_t& overlay_stats = g_last_stats;
		const result_t& latest_result = g_last_result;
		if (overlay_stats.status.empty() && latest_result.full_sequence.empty() && latest_result.failure_reason.empty())
			return;

		int screen_x = 0;
		int screen_y = 0;
		interfaces::engine->get_screen_size(screen_x, screen_y);
		if (screen_x <= 0 || screen_y <= 0)
			return;

		auto fit_overlay_text = [](const std::string& text, const float max_width) {
			std::string fitted = text.empty() ? std::string("none") : text;
			while (fitted.size() > 4 &&
				im_render.measure_text((fitted + "...").c_str(), fonts::esp_name_font, 12.0f).x > max_width) {
				fitted.pop_back();
			}

			if (fitted != text && fitted.size() > 4)
				fitted.append("...");
			return fitted;
		};

		const std::string status_text = overlay_stats.status.empty() ? "idle" : overlay_stats.status;
		const std::string combo_text = fit_overlay_text(
			overlay_stats.current_combo.empty() ? std::string("waiting for calculation") : overlay_stats.current_combo,
			318.0f);
		const std::string progress_text = std::to_string(overlay_stats.combinations_tested)
			.append("/")
			.append(std::to_string(overlay_stats.combination_space_total));
		const std::string segment_text = std::to_string((std::max)(1, overlay_stats.current_target_index))
			.append("/")
			.append(std::to_string((std::max)(1, overlay_stats.points_loaded)));
		const std::string best_route_text = [&]() {
			if (latest_result.success)
				return fit_overlay_text(latest_result.sequence.empty() ? latest_result.full_sequence : latest_result.sequence, screen_x * 0.7f);
			if (!overlay_stats.best_candidate_sequence.empty())
				return fit_overlay_text(overlay_stats.best_candidate_sequence, screen_x * 0.7f);
			if (!overlay_stats.failure_reason.empty())
				return fit_overlay_text(overlay_stats.failure_reason, screen_x * 0.7f);
			return fit_overlay_text(combo_text, screen_x * 0.7f);
		}();

		const color_t overlay_background(8, 8, 8, 205);
		const color_t overlay_title(255, 255, 255, 255);
		const color_t overlay_value(220, 220, 220, 235);
		const color_t overlay_good(90, 220, 120, 255);
		const color_t overlay_bad(220, 90, 90, 255);
		const color_t overlay_warn(255, 215, 90, 255);
		const color_t overlay_state = latest_result.success
			? overlay_good
			: (status_text == "failed" ? overlay_bad : (status_text == "cancelled" ? overlay_warn : overlay_value));

		const float top_x = 20.0f;
		const float top_y = 20.0f;
		const float top_w = 360.0f;
		const float top_h = 86.0f;
		im_render.drawrectfilled(top_x, top_y, top_w, top_h, overlay_background);
		im_render.text(top_x + 8.0f, top_y + 6.0f, 12.0f, fonts::esp_name_font, "route calculator", false, overlay_title, false);
		im_render.text(top_x + 8.0f, top_y + 24.0f, 12.0f, fonts::esp_name_font,
			std::string("status: ").append(status_text).c_str(), false, overlay_state, false);
		im_render.text(top_x + 8.0f, top_y + 40.0f, 12.0f, fonts::esp_name_font,
			std::string("segment: ").append(segment_text).append("   combos: ").append(progress_text).c_str(),
			false, overlay_value, false);
		im_render.text(top_x + 8.0f, top_y + 56.0f, 12.0f, fonts::esp_name_font,
			std::string("combo: ").append(combo_text).c_str(), false, overlay_value, false);

		const std::string footer_title = latest_result.success ? "latest route" : "current best";
		const float bottom_w = (std::max)(520.0f, im_render.measure_text(best_route_text.c_str(), fonts::esp_name_font, 12.0f).x + 32.0f);
		const float bottom_h = 42.0f;
		const float bottom_x = (screen_x * 0.5f) - (bottom_w * 0.5f);
		const float bottom_y = screen_y - 72.0f;
		im_render.drawrectfilled(bottom_x, bottom_y, bottom_w, bottom_h, overlay_background);
		im_render.text(screen_x * 0.5f, bottom_y + 6.0f, 12.0f, fonts::esp_name_font, footer_title.c_str(), true, overlay_title, false);
		im_render.text(screen_x * 0.5f, bottom_y + 22.0f, 12.0f, fonts::esp_name_font, best_route_text.c_str(), true, overlay_state, false);
	}

	void cancel() {
		g_cancel_requested.store(true, std::memory_order_relaxed);
	}

	void clear_results() {
		sync_visible_state();
		g_last_result = {};
		g_last_results.clear();
		g_render_result = {};
		g_render_results.clear();
		if (!g_solver_running.load(std::memory_order_relaxed)) {
			g_last_stats.failure_reason.clear();
			g_last_stats.last_successful_combo.clear();
			g_last_stats.best_candidate_sequence.clear();
		}
	}

	void request_calculation() {
		g_calculation_requested.store(true, std::memory_order_relaxed);
	}

	bool consume_requested_calculation() {
		return g_calculation_requested.exchange(false, std::memory_order_relaxed);
	}

	bool is_running() {
		return g_solver_running.load(std::memory_order_relaxed);
	}

	const std::string& latest_log_path() {
		return g_last_stats.log_path;
	}

	const result_t& last_result() {
		return g_last_result;
	}

	const std::vector<result_t>& last_results() {
		return g_last_results;
	}

	const debug_stats_t& last_stats() {
		return g_last_stats;
	}
}
