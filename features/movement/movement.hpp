#pragma once
#include "../../features/visuals/visuals.hpp"
#include "../../sdk/sdk.hpp"
#include <windows.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <cstddef>
#include <cstdio>

namespace features::movement {
	//Inline Data




	inline bool should_duck_next;
	inline bool should_align_next_tick;
	inline bool HITGODA{ false };
	inline bool HITGODA2{ false };
	inline bool AlertJB = false;
	inline bool AlertJB2 = false;
	inline bool scanned_surf = false;
	inline bool scanned_headbang = false;
	inline bool should_airstuck = false;
	inline bool should_pixelladder = false;
	inline bool m_opened = false;
	inline bool should_ast = false;
	inline bool should_bst = false;
	inline bool can_shoot = false;
	inline bool fake_jb = false;
	inline bool should_onehop;
	inline bool should_lock_eb;
	inline bool should_lock_ps;
	inline bool should_lock_hb;
	inline bool edgebug_is_predicted;
	inline bool should_fireman;
	inline bool should_edge_bug;
	inline bool is_standing_eb;
	inline bool detected_ladder_glide;
	inline bool detected_normal_jump_bug = false;
	inline bool cannot_jump_bug = false;
	inline bool detected_normal_edge_bug = false;
	inline bool detected_normal_pixel_surf = false;
	inline bool detected_headbang = false;
	inline bool should_ps;
	inline bool should_hb;
	inline bool should_b;
	inline bool should_lj;
	inline bool should_ej;
	inline bool should_mj;
	inline bool should_lb;
	inline bool should_align;
	inline bool should_post_align;
	inline bool should_air = false;
	inline bool hb = false;
	inline bool should_ps_standing = false;
	inline bool do_long_jump = false;
	inline bool do_mini_jump = false;
	inline bool did_jump_last = false;
	inline static int previous_tick = 0;
	inline int last_tick = 0;
	inline int last_delta = 0;
	inline int last_vel = 0;
	inline int pre = 0;
	inline int edgebugs = 0;
	inline int edgebug_combo = 0;
	inline int flags_backup;
	inline int saved_tick_jb = 0;
	inline int saved_tick_lj = 0;
	inline int saved_tick_ej = 0;
	inline int saved_tick_mj = 0;
	inline int saved_tick_fm = 0;
	inline int saved_tick_ast = 0;
	inline int saved_tick_bst = 0;
	inline int saved_tick_air = 0;
	inline float last_update_time = 0.f;
	inline static float last_edgebug;
	inline float effect_alpha;
	inline float hiteffect_alpha;
	inline float last_tick_y;
	inline vec3_t first_viewangles;
	inline vec3_t bug_origin;
	inline vec3_t origv;
	inline int indicator_pos = 0;
	inline float pressdelay = 0.f;

	//Jumpstats
	inline float jumpstat_units = 0.0f;
	inline int jumpstat_strafes = 0;
	inline float jumpstat_pre = 0.0f;
	inline float jumpstat_max_vel = 0.0f;
	inline float jumpstat_max_height = 0.0f;
	inline int jumpstat_bhops = 0;

	inline float preserved_z = 0.f;
	inline vec3_t** angle_set = nullptr;
	inline int angle_set_size = 128;

	//Functions
	void velocity_indicator();
	void velocity_indicator_anim();
	void stamina_indicator();
	void indicators();
	void gather_vel_graph_data();
	void velocity_graph_draw();
	void gather_stam_graph_data();
	void stamina_graph_draw();


	struct css_slam_state_t {
		bool enabled = true;               // enable/disable slam
		float trigger_height = 20.0f;      // vertical drop distance before slam
		float slam_velocity = -1024.0f;    // velocity.z to apply on slam
		bool slam_once = true;             // only slam once per jump
		bool already_slammed = false;      // internal state flag
		float height_marker = 0.0f;        // captured jump height
	};

	inline css_slam_state_t css_slam_state; // <-- define instance

	struct headbang_data_t {
		bool headbanging = false;
		bool predicted_hb = false;
		bool headbangducking = false;
		int headbangtick = 0;
		c_usercmd* headbang_cmd = nullptr;
	};
	inline headbang_data_t hb_data;

	struct ps_data_t {
		bool predicted_ps = false;
		bool pixelsurfing = false;
		bool pixelducking = false;
		int counter = 0;
		int pixeltick = 0;
		int old_tick = 0;
		float last_set = 0.f;
		c_usercmd* pixelsurf_cmd = nullptr;
		vec3_t view_point = vec3_t(0, 0, 0);
		float forward_move = 0.f;
		float side_move = 0.f;
	};
	inline ps_data_t ps_data;

	struct edge_bug_detection {
		bool crouched;
		int detecttick;
		int edgebugtick;
		bool strafing;
		float yawdelta;
		float forwardmove, sidemove;
		float startingyaw;
		int eblength;
		int ticks_left;
		int did_edgebug;
		int did_edgebug_strafe;
		int buttons;
	};
	inline edge_bug_detection detect_data;

	struct velocity_data_t {
		float speed;
		bool on_ground;
		bool edge_bug;
		bool jump_bug;
		bool pixel_surf;
	};

	inline std::vector<velocity_data_t> velocity_data;

	struct tick_info {
		tick_info() {
			velocity = g::local->velocity();
			origin = g::local->origin();
			stamina = g::local->stamina();
			in_air = !(g::local->flags() & fl_onground);
		}
		vec3_t velocity;
		vec3_t origin;
		float stamina;
		bool in_air;
	};

	inline std::deque<tick_info> move_info;

	struct pixelsurf_data_t {
		bool m_predicted_succesful = false, m_in_pixel_surf = false, m_should_duck = false;
		bool should_pixel_surf = false;
		bool predicted_ps = false;
		int px_tick;
		bool ps_detect = false;
	}; inline pixelsurf_data_t m_pixelsurf_data;

	struct points_check_t {
		vec3_t pos;
		std::string map;
		float currentScale = 0.f;
		bool open_settings = false;
		bool jump = true;
		bool minijump = true;
		bool longjump = true;
		bool jumpbug = true;
		bool crouch_hop = true;
		bool mini_crouch_hop = true;
		bool c_jump = true;
		bool c_minijump = true;
		bool c_longjump = true;
		bool c_jumpbug = true;
		bool c_crouch_hop = true;
		bool c_mini_crouch_hop = true;
		bool active = true;
		float radius = 300.f;
		float delta_strafe = 0.5f;
		points_check_t(const vec3_t& Pos = { }, const std::string& Map = "") : pos(Pos), map(Map) {}
	};
	inline std::vector< points_check_t > m_bounce_points_check{ };
	inline std::vector< points_check_t > m_pixelsurf_points_check{ };
	void check_ps(c_usercmd* cmd);
	void pixelsurf_assist(c_usercmd* cmd);
	void assist_render();
	void RenderPoints();
	void assist_createmove(c_usercmd* cmd);
	void assist_endscene();
	void bounce_assist(c_usercmd* cmd);

}

namespace features::calculator {

	void run();
	void searchwall();
	void set_point();
	void remove_point();
	void clear_points();
	void draw_circles();
	void add_circle_position(const vec3_t& position);
	//void calculate(c_usercmd* cmd);

	inline std::vector<vec3_t> circle_positions;
	//inline vec3_t pointvec;
}

class savingroute {
public:
	explicit savingroute(const char*);
	void load(size_t);
	void save(size_t, std::string name) const;
	void add(const char*);
	void remove(size_t);
	void rename(size_t, const char*);
	void listroutes();

	void update(c_usercmd* cmd);

	constexpr auto& getRoutes() {
		return routes;
	}

private:
	std::filesystem::path path;
	std::vector<std::string> routes;
};

typedef savingroute;

inline std::unique_ptr<savingroute> route;

class mrecorder {
public:
	void draw();
	void create_move(c_usercmd* cmd);
	void drawroute();
	void infowindow();
	void forcestop();
	vec3_t origin_r;
};

inline mrecorder* recorder;