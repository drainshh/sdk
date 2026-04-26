#pragma once

#include "../../sdk/sdk.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace features::movement::pixelsurf_calculator {

	enum class target_kind_t {
		floor_point,
		pixelsurf_point
	};

	enum class action_type_t {
		carry,
		carry_duck,
		stand_jump,
		crouch_jump,
		minijump,
		longjump,
		jumpbug
	};

	enum class event_type_t {
		action,
		pixelsurf,
		floor,
		headbang,
		jumpbug
	};

	struct target_t {
		vec3_t pos{};
		vec3_t normal{};
		target_kind_t kind = target_kind_t::pixelsurf_point;
		int marker_index = -1;
		float radius = 24.f;
		bool allow_stand = true;
		bool allow_duck = true;
		bool jump_stand = true;
		bool jump_crouch = true;
		bool minijump_stand = true;
		bool minijump_crouch = true;
		bool longjump_stand = true;
		bool longjump_crouch = true;
		bool crouch_hop_stand = true;
		bool crouch_hop_crouch = true;
		bool jumpbug_stand = true;
		bool jumpbug_crouch = true;
		bool mini_crouch_hop_stand = true;
		bool mini_crouch_hop_crouch = true;
	};

	struct sim_state_t {
		vec3_t pos{};
		vec3_t vel{};
		bool on_ground = false;
		bool prev_on_ground = false;
		bool ducking = false;
		bool prev_ducking = false;
		float ground_z = 0.f;
		float stamina = 0.f;
		int ground_ticks = 0;
		int air_ticks = 0;
		int tick = 0;
	};

	struct event_t {
		event_type_t type = event_type_t::action;
		action_type_t action = action_type_t::carry;
		int tick = 0;
		int target_index = -1;
		vec3_t pos{};
		vec3_t vel{};
		bool ducking = false;
		int delay_ticks = 0;
	};

	struct result_t {
		bool success = false;
		int tick = -1;
		int elapsed_time_ms = 0;
		int combinations_tested = 0;
		int reached_targets = 0;
		int total_targets = 0;
		int failed_stage = 0;
		int closest_tick = -1;
		vec3_t start_pos{};
		vec3_t pos{};
		vec3_t vel{};
		float score = 0.f;
		float closest_xy_delta = 0.f;
		float closest_z_delta = 0.f;
		float airborne_sample_z = 0.f;
		float landing_position_z = 0.f;
		float landed_ground_z = 0.f;
		float target_floor_z = 0.f;
		float compared_floor_z = 0.f;
		float resolved_floor_z = 0.f;
		float previous_floor_z = 0.f;
		float start_floor_z = 0.f;
		float expected_floor_rise = 0.f;
		float actual_floor_rise = 0.f;
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
		std::string combo_name;
		std::string bind_sequence;
		std::string sequence;
		std::string full_sequence;
		std::string failure_reason;
		std::string stage_failure_detail;
		int hidden_events = 0;
		std::vector<event_t> events;
	};

	struct request_t {
		std::vector<target_t> targets;
		int placed_points = 0;
		int placed_floor_points = 0;
		int placed_pixelsurf_points = 0;
		bool chained_mode = false;
		std::string target_order;
	};

	struct debug_stats_t {
		int calculate_requests = 0;
		int placed_points = 0;
		int placed_floor_points = 0;
		int placed_pixelsurf_points = 0;
		int points_loaded = 0;
		int exact_stage_count = 0;
		bool chained_mode = false;
		int current_target_index = 0;
		int current_target_stage_ticks = 0;
		int current_pass = 0;
		int current_combo_size = 0;
		int elapsed_time_ms = 0;
		int nodes_expanded = 0;
		int branch_limit = 0;
		int depth_limit = 0;
		int tick_budget = 0;
		int total_tick_budget = 0;
		int max_nodes = 0;
		bool combination_space_exact = false;
		int combination_space_total = 0;
		int combinations_generated = 0;
		int combinations_tested = 0;
		int segments_tested = 0;
		int ticks_simulated = 0;
		int candidates_pruned = 0;
		int success_candidates = 0;
		int failure_candidates = 0;
		int partial_updates = 0;
		float best_candidate_score = 0.f;
		int best_candidate_tick = -1;
		int best_near_success_stage = 0;
		int best_near_success_tick = -1;
		float best_near_success_xy_delta = 0.f;
		float best_near_success_z_delta = 0.f;
		bool best_near_success_landing = false;
		bool best_near_success_window = false;
		bool best_near_success_contact = false;
		int best_near_success_window_enter_tick = -1;
		int best_near_success_window_exit_tick = -1;
		int best_near_success_landing_tick = -1;
		std::string status;
		std::string target_order;
		std::string segment_summary;
		std::string combo_formula;
		std::string candidate_generation_summary;
		std::string action_alphabet;
		std::string current_phase;
		std::string current_target_pos;
		std::string current_stage_transition;
		std::string current_combo;
		std::string current_events;
		std::string last_successful_combo;
		std::string best_candidate_sequence;
		std::string best_near_success_reason;
		std::string failure_reason;
		std::string propagation_warning;
		std::string movement_constants;
		std::string log_path;
	};

	void reset();
	void update(c_usercmd* cmd, const request_t& request, bool calculate_now);
	void draw();
	void cancel();
	void clear_results();
	void request_calculation();
	bool consume_requested_calculation();
	bool is_running();
	const std::string& latest_log_path();
	const result_t& last_result();
	const std::vector<result_t>& last_results();
	const debug_stats_t& last_stats();
}
