#pragma once
#include <string_view>
#include <vector>
#include "../../includes/nlohmann/json.hpp"

struct changed_entry {
	std::string name;
	std::string value;
};

namespace c {
	void create_directory();
	void update_configs();
	void create_file(const std::string_view name);
	void delete_file(const std::size_t index);
	void save(const std::size_t index);
	void load(const std::size_t index);
	inline std::string directory_path_convar;
	inline std::string directory_path_lua;
	inline std::string directory_path_fonts;
	inline std::string directory_path;
	inline std::string directory;

	inline static std::vector<changed_entry> changed_convars;

	namespace backtrack {
		inline bool backtrack = false;
		inline int time = 0;
		inline float fake = 0.7f;
		inline bool fake_latency = false;
		inline bool selected_tick = false;
		inline float aim_smoothing = 10.f;
		inline float aim_behavior_slider = 0.f;
		inline int backtrack_choke_ticks = 3;
	}

	namespace triggerbot {
		inline bool enabled = false;
		inline int triggerbot_key;
		inline int triggerbot_key_s = 1;
	}

	namespace aimbot {

		inline bool auto_shoot = true;
		inline bool auto_scope = false;
		inline bool slow_walk_on_shot = false;
		inline bool walk_between_shots = false;
		inline float aimbot_smoothing = 1.f;
		inline float recoil_scale = 1.f;
		inline bool aimbot;
		inline int aimbot_key;
		inline int aimbot_key_s = 1;
		inline int aimbot_only_enemy;
		inline bool aimbot_silent;
		inline bool rcs;
		inline bool non_sticky_aimbot;
		inline bool aim_at_bt = false;
		inline bool pistol_autowall;
		inline int pistol_autowall_dmg = 1;
		inline int pistol_aimbot_fov;
		inline int pistol_hitbox;
		inline bool heavy_pistol_autowall;
		inline int heavy_pistol_autowall_dmg = 1;
		inline int aimbot_hitbox;
		inline bool aimbot_autowall;
		inline int aimbot_fov;
		inline int aimbot_autowall_dmg = 1;
		inline int heavy_pistol_aimbot_fov;
		inline int heavy_pistol_hitbox;
		inline bool shotgun_autowall;
		inline int shotgun_autowall_dmg = 1;
		inline int shotgun_aimbot_fov;
		inline int shotgun_hitbox;
		inline bool heavy_autowall;
		inline int heavy_autowall_dmg = 1;
		inline int heavy_aimbot_fov;
		inline int heavy_hitbox;
		inline bool smg_autowall;
		inline int smg_autowall_dmg = 1;
		inline int smg_aimbot_fov;
		inline int smg_hitbox;
		inline bool rifle_autowall;
		inline int rifle_autowall_dmg = 1;
		inline int rifle_aimbot_fov;
		inline int rifle_hitbox;
		inline bool sniper_autowall;
		inline int sniper_autowall_dmg = 1;
		inline int sniper_aimbot_fov;
		inline int sniper_hitbox;
		inline bool autosniper_autowall;
		inline int autosniper_autowall_dmg = 1;
		inline int autosniper_aimbot_fov;
		inline int autosniper_hitbox;


		//HvH
		inline bool auto_accurate_walk;
	}

	namespace beta {
		inline bool strafe_assist;


	}

	namespace movehelp {
		//Jumps
		inline bool auto_duck;

		//Strafes
		inline bool strafe_assistance;
		inline bool smart_nulls;
		inline bool smart_msl;
		inline bool frame_msl;
		inline bool robot_strafe;
		inline bool tekno_strafe;

		//Engine-Bugs
		inline bool advanced_edgebug;
		inline bool edgebug_lock;
		inline bool pixelsurf_lock;
		inline int edgebug_locktype;
		inline int edgebug_lockamount;
		inline float edgebug_scan = 0.4f;
		inline float advanced_edgebug_scan = 0.65f;

		//Helpers
		inline bool pixelhop;
		inline bool pixelwalk;
		inline bool surfjump;
		inline bool surfmove;
		inline bool auto_onehop;
		inline bool headbounce_fix;
		inline bool extended_land;
		inline bool extended_land_autostrafe;
		inline bool dynamic_surf_jump;
		inline bool dynamic_surf_stamina;
		inline float pixelsurf_lock_amount;
		inline bool headbounce_lock;
		inline float headbounce_lock_amount;
		inline float surf_ticks = 1.f;
		inline float surf_align_ticks = 1.f;
		inline float surf_move_ticks = 1.f;
		inline bool jumpbug_autojump;
		inline bool free_angle = false;

		//AI
		inline bool ai_alignment;
		inline bool ai_pixelsurf;
		inline bool ai_headbang;
		inline bool ai_wallhop;
		inline float alignment_ticks;
		inline float alignment_velocity_smooth;
		inline float alignment_angle_smooth;
		inline float pixelsurf_ticks;
		inline float headbang_ticks;
		inline bool headbang_autoduck;
		inline float wallhop_ticks;
		inline float AI_scan_time;

		//Exploits
		inline bool force_custom_tick;
		inline bool fake_duck;

		//Binds
		inline int onehop_key = 0;
		inline int onehop_key_s = 1;
		inline int surfjump_key = 0;
		inline int surfjump_key_s = 1;


		//Indicators
		inline bool indicatorEnable = false;
		inline bool edgebug_indicator;
		inline bool longjump_indicator;
		inline bool minijump_indicator;
		inline bool edgejump_indicator;
		inline bool jumpbug_indicator;
		inline bool pixelladder_indicator;
		inline bool pixelsurf_indicator;
		inline bool airstuck_indicator;
		inline bool headbang_indicator;
		inline bool ladderbug_indicator;
		inline bool fireman_indicator;
		inline bool air_indicator;
		inline bool ast_indicator;
		inline bool bst_indicator;
		inline bool detection_indicator;
		inline bool can_fire_indicator = true;

		//Misc
		inline float predict_scale = 0.5f;
		inline bool low_power_mode;
		inline bool push_notice = true;

		//Shaders
		inline bool scaleform_shaders;

	}

	namespace assist
	{
		inline bool assist = true;
		inline bool pixelsurf_assist = false;
		inline bool bounce_assist = false;

		inline bool assist_broke_hop = false;
		inline float assist_stamina_value = 100.0f;
		inline bool assist_render = false;
		inline bool bounce_assist_render = false;
		inline int assist_render_style = 1;

		inline bool assist_bounce_broke_hop = false;

		inline bool assist_enable_pixelsurf_points = true;
		inline bool assist_enable_bounce_points = true;
		inline bool assist_enable_headbang_points = true;
		inline bool assist_enable_floor_points = true;
		inline bool assist_enable_ceiling_points = true;

		inline float pixelsurf_assist_ticks = 0.2f;

		inline int pixelsurf_max_points = 1;

		inline int pixelsurf_assist_type = 0;

		inline int pixelsurf_assist_key;
		inline int pixelsurf_assist_key_s = 1;

		inline int pixelsurf_line_key;
		inline int pixelsurf_line_key_s = 1;

		inline int pixelsurf_point_key;
		inline int pixelsurf_point_key_s = 1;

		inline int bounce_assist_key;
		inline int bounce_assist_key_s = 1;

		inline int bounce_point_key;
		inline int bounce_point_key_s = 1;

		inline int ps_calc_route_key;
		inline int ps_calc_route_key_s = 1;

		inline bool simple_display = true;
	}


	namespace movement {

		inline char watermark_nickname[256] = "ello314";
		inline int watermark_type = 0;
		inline int advanced_radius = 6;
		inline bool fast_stop = false;
		inline float jumpbug_angle = 0.2f;
		inline int max_render_distance = 1000;

		inline int add_floor_key = 0;
		inline int add_floor_key_s = 1;
		inline int add_pixelsurf_key = 0;
		inline int add_pixelsurf_key_s = 1;
		inline int delete_point_key = 0;
		inline int delete_point_key_s = 1;

		inline float alignment_smoothing = 0.32f;
		inline float alignment_rage = 0.2f;
		inline bool marker_align = false;
		inline bool ej_252pre = false;
		inline bool lj_252pre = false;
		inline bool mj_252pre = false;
		inline bool ej_adaptive_nulls = false;
		inline bool lj_adaptive_nulls = false;
		inline bool mj_adaptive_nulls = false;
		inline float ps_lock_amount = 20.f;
		inline bool auto_bounce = false;
		inline bool chain_assist = false;
		inline bool chain_strafe_correction = false;
		inline bool fast_bhop = false;
		inline bool points_system = false;
		inline bool jetpack_to_surf = false;
		inline int jetpack_to_surf_key = 0;
		inline int jetpack_to_surf_key_s = 1;

		inline bool silent_edgebug = false;
		inline bool auto_duck_collision = false;
		inline int auto_duck_collision_key = 0;
		inline int auto_duck_collision_key_s = 1;
		inline int auto_duck_collision_ticks = 6;

		inline bool bhopfix = true;
		inline bool enable_angle_limit = false;
		inline float edgebug_angle_limit = 3.f;
		inline int px_selection = 0;
		
		inline bool edgebug_rage = false;
		inline bool gain_helper = false;
		inline bool airstuck_indicator = false;
		inline bool airstuck_enabled = false;
		inline float airstuck_check_ticks = 0.2f;
		inline int airstuck_key = 0;
		inline int airstuck_key_s = 1;

		inline bool edgescanner = false;
		inline float edgescanner_assist = 0.2f;
		inline float edgescanner_scan = 0.2f;

		inline float jump_to_surf_scan = 0.2f;

		inline bool headbounce_fix = false;

		inline float ps_ticks = 0.2f;
		inline bool pscalc_enable = false;
		inline bool pscalc_enable_draw = false;
		inline bool pscalc_jump_to_pixelsurf = false;
		inline bool pscalc_jump_to_pixelsurf_advanced = false;
		inline float pscalc_scan_distance = 64.f;
		inline bool pscalc_chain_targets = false;
		inline int pscalc_scan_ticks = 64;
		inline int pscalc_model_b_stage_ticks = 64;
		inline int pscalc_model_b_total_ticks = 256;
		inline int pscalc_model_b_max_nodes = 50000;
		inline int pscalc_model_b_combos_per_frame = 256;
		inline bool pscalc_exact_stage_mode = true;
		inline bool pscalc_no_first_stage_prune = true;
		inline int pscalc_min_prune_ticks = 24;
		inline float pscalc_z_tolerance = 1.25f;
		inline float pscalc_lax_z_tolerance = 2.5f;
		inline bool pscalc_log_stage_debug = true;
		inline bool pscalc_allow_jump = true;
		inline bool pscalc_allow_crouch_jump = true;
		inline bool pscalc_allow_minijump = true;
		inline bool pscalc_allow_longjump = true;
		inline bool pscalc_allow_jumpbug = false;
		inline int pscalc_max_displayed_combos = 3;
		inline bool pscalc_debug_output = false;
		inline bool pscalc_finder_enable = false;
		inline bool pscalc_finder_draw = true;
		inline float pscalc_finder_radius = 256.f;
		inline int pscalc_finder_max_points = 16;

		inline int pscalc_drawpoint_key = 0;
		inline int pscalc_drawpoint_key_s = 1;

		inline int pscalc_clear_all_key = 0;
		inline int pscalc_clear_all_key_s = 1;

		inline int pscalc_jump_to_marker_key = 0;
		inline int pscalc_jump_to_marker_key_s = 1;

		inline bool jump_to_longjump = false;
		inline float jump_to_longjump_distance = 280.f;
		inline float jump_to_longjump_speed = 300.f;

		inline bool nulls_angle = false;
		inline bool nulls_forward_back = false;

		inline bool movement_unlocker = false;

		inline bool smooth_land = false;
		inline float smooth_land_fallspeed = 45.f;
		inline float smooth_land_popup_amount = 32.f;

		inline int edgebug_predrounds = 3;

		inline bool strafe_limiter = false;
		inline bool velocity_slam = false;

		inline int can_shoot_key = 0;
		inline int can_shoot_key_s = 0;

		inline bool unduck_after_lj = false;
		inline bool unduck_after_mj = false;
		inline float lock_aggression = 3.f;

		inline bool auto_headbang = false;

		inline bool perfect_bhop = false;
		inline bool css_hop = false;

		inline float pixelsurf_ticks = 0.2f;

		inline bool jumpbug_quickstop = false;
		inline bool jumpbug_ladderbug = false;

		inline bool ladderjump_ej = false;
		inline bool ladderjump_lj = false;
		inline bool ladderjump_mj = false;

		inline bool surf_strafe = false;
		inline bool surf_jump = false;

		inline bool align_strafe = false;

		inline bool p_strafe = false;

		inline bool enable_strafe_type = false;
		inline int strafe_type = 0;

		inline bool wallhop;
		inline bool headbang;
		inline bool pixelladder;
		inline int pixelladder_key = 0;
		inline int pixelladder_key_s = 0;
		inline float pixelladder_scan = 0.2f;
		inline float headbang_scan = 0.2f;
		inline float jumpbug_scan = 0.2f;
		inline bool pixel_helper;
		inline bool wire_bounce;
		inline bool auto_clip_quick_release;
		inline bool ai_playback;
		inline float ai_learnrate;
		inline int stage_amount = 1;
		inline float stage_push = 0.1f;
		inline bool gain_assist = false;
		inline bool auto_clip;
		inline bool smooth_mouse = false;
		inline bool fireman = false;
		inline int fireman_key = 0;
		inline int fireman_key_s = 1;
		inline float bug_frequency = 0.005f;
		inline float edgebug_plane_z = 0.7f;
		inline int  dh_tick = 1;
		inline float stamina_fade_clr[4]{ 1.f, 1.f, 1.f };
		inline bool auto_strafe = false;
		inline int  auto_strafe_key = 0;
		inline int  auto_strafe_key_s = 1;
		inline bool bhop = false;
		inline bool null_strafing = false;
		inline bool robot_strafing = false;
		inline bool ladder_bug = false;
		inline int ladder_bug_key = 0;
		inline int ladder_bug_key_s = 0;
		inline bool ledge_grab = false;
		inline int ledge_grab_key = 0;
		inline int ledge_grab_key_s = 1;
		inline bool delay_hop = false;
		inline int  delay_hop_key = 0;
		inline int  delay_hop_key_s = 1;

		inline bool crouch_bug = false;
		inline int crouch_bug_key = 0;
		inline int crouch_bug_key_s = 1;

		inline bool visualize_edge_bug = false;
		inline float visualize_edge_bug_clr[4]{ 1.f, 1.f, 1.f };

		inline bool edge_bug = false;
		inline bool edge_bug_advanced_search = false;
		inline bool edge_bug_strafe = true;
		inline int edge_bug_priority = 0;
		inline int edge_bug_key = 0;
		inline int edge_bug_key_s = 1;
		inline int edgebug_tick_count = 32;
		inline float edge_bug_ticks = 0.4f;
		inline int edge_bug_rape = 5;
		inline float edge_bug_angle_limit = 20.0f;
		inline float edge_bug_lock_amount = 20.0f;
		inline float edge_bug_slide_amount = 5.0f;
		inline int edge_bug_lock_type = 0;
		inline int edge_bug_type = 0;
		inline float edge_bug_health_boost_effect = 0.f;
		inline int edge_bug_detection_sound = 0;
		inline bool edge_bug_counter = false;

		inline bool edgejump = false;
		inline bool edge_jump_on_ladder = false;
		inline int edgejump_key = 0;
		inline int edgejump_key_s = 1;
		inline int long_jump_key = 0;
		inline int long_jump_key_s = 1;
		inline bool long_jump_on_edge = false;
		inline bool lj_null[4] = { false, false, false, false };
		inline bool mini_jump = false;
		inline bool mini_jump_ej = true;
		inline bool mini_jump_remain_crouched = false;
		inline int mini_jump_key = 0;
		inline int mini_jump_key_s = 1;
		inline bool adaptive_key_cancelling = false;
		inline bool adaptive_key_for[3] = { false, false, false };
		inline bool strafe_optimizer = false;
		inline int strafe_optimizer_key = 0;
		inline int strafe_optimizer_key_s = 1;
		inline float strafe_optimizer_desired_gain = 0.f;
		inline float strafe_optimizer_required_speed = 0.f;
		inline bool mouse_strafe_limiter = false;
		inline bool mouse_strafe_limiter_affect_pitch = false;
		inline float mouse_strafe_limiter_value = 20.f;
		inline int mouse_strafe_limiter_key = 0;
		inline int mouse_strafe_limiter_key_s = 1;

		inline bool jump_bug = false;
		inline int jump_bug_key = 0;
		inline int jump_bug_key_s = 1;
		inline bool lj_jb_miss = false;


		inline bool fastduck = false;

		inline bool post_auto_align = false;

		inline bool auto_align = false;
		inline bool land_on_surf = false;
		inline bool in_air_surfix = false;

		inline bool in_air_align_fix = false;
		inline bool on_ground_align_fix = false;

		inline bool auto_duck = false;
		inline int  auto_duck_key = 0;
		inline int  auto_duck_key_s = 1;
		inline bool pixel_surf_fix = false;
		inline bool pixel_surf = false;
		inline int  pixel_surf_key = 0;
		inline int  pixel_surf_key_s = 0;
		inline int	pixelscan_key = 0;
		inline int	pixelscan_key_s = 1;
		inline int	headbang_key = 0;
		inline int	headbang_key_s = 0;
		inline int pixel_surf_ticks = 7;
		inline bool fakebackwards = false;
		inline int  fakebackwardskey = 0;
		inline int  fakebackwardskey_s = 1;
		inline int  fakebackwards_angle = 0;
		inline bool edge_bug_detection_printf = false;
		inline bool jump_bug_detection_printf = false;
		inline bool long_jump_detection_printf = false;
		inline bool mini_jump_detection_printf = false;
		inline bool pixel_surf_detection_printf = false;
		inline bool ladder_bug_detection_printf = false;
		inline bool velocity_indicator = false;
		inline int velocity_indicator_type = 0;
		inline int velocity_indicator_position = 90;
		inline int indicators_position = 65;
		inline int indicators_gap = 4;
		inline float indicators_fading_speed = 1.8f;
		inline bool indicators_allow_animation = false;
		inline bool indicators_allow_move = false;
		inline float indicators_move_pos = 5.f;
		inline bool velocity_indicator_show_pre = false;
		inline bool velocity_indicator_show_ghost = false;
		inline bool velocity_indicator_disable_ong_show_pre = false;
		inline bool velocity_indicator_custom_color = false;
		inline bool velocity_indicator_sync_shadow = false;
		inline float velocity_indicator_positive_clr[4]{ 0.11765f, 1.00000f, 0.42745f };
		inline float velocity_indicator_negative_clr[4]{ 1.00000f, 0.46667f, 0.46667f };
		inline float velocity_indicator_neutral_clr[4]{ 1.00000f, 0.78039f, 0.34902f };
		inline float velocity_indicator_custom_clr[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float shadow_indicator_custom_clr[4]{ 1.f, 1.f, 1.f, 0.5f };
		inline float shadow_indicator_custom_clr2[4]{ 1.f, 1.f, 1.f, 0.f };
		inline float velocity_shadow_color[4]{ 0.f, 0.f, 0.f, 1.f };
		inline float velocity_indicator_custom_clr2[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float velocity_indicator_fade_clr3[4]{ 1.f, 1.f, 1.f, 0.f };
		inline float indicator_detect_clr[4]{ 0.0f, 0.85f, 0.0f, 1.f };
		inline bool indicators_show[8] = { false, false, false, false, false, false, false, false };
		inline bool allow_detection_clr = false;
		inline int detection_saved_tick = 15;
		inline bool detection_clr_for[9] = { false, false, false, false, false, false, false, false, false };
		inline bool stamina_indicator = false;
		inline bool stamina_indicator_fade = false;
		inline bool stamina_indicator_show_pre = false;
		inline bool stamina_indicator_disable_ong_show_pre = false;
		inline int stamina_indicator_position = 75;
		inline float stamina_indicator_clr[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool stamina_graph = false;
		inline float stamina_graph_color[3]{ 1.f, 1.f, 1.f };
		inline int stamina_graph_offset = -700;
		inline int stamina_graph_height = 100;
		inline int stamina_graph_offset_x = 500;
		inline int graph_fade = 1;
		inline bool velocity_graph = false;
		inline float velocity_graph_color[3]{ 1.f, 1.f, 1.f };
		inline bool velocity_graph_show_landed_speed = false;
		inline bool velocity_graph_show_edge_bug = false;
		inline bool velocity_graph_show_jump_bug = false;
		inline bool velocity_graph_show_pixel_surf = false;
		inline float velocity_linewidth = 3.f;
		inline int velocity_thickness = 1;
		inline float velocity_height = 4.f;
		inline int velocity_size = 185;
		inline float graph_xscreenpos = 0.800f;
		inline float velocity_tofade = 1.f;
		inline bool key_strokes;
		inline int key_strokes_position = 200;
		inline bool billware_wm = true;
	}

	namespace misc {

		inline bool insecure_bypass = true;
		inline bool progressbar_enable = true;
		inline bool remove_view_interp = false;
		inline bool cs_two_model = false;

		inline std::vector<std::string> custom_cvar;
		inline const char* custom_cvar_name;
		inline int cvar_val;
		inline bool playerlist = false;
		inline bool headshot_spoofer = false;
		inline bool anti_untrusted = true;
		inline bool unlock_inventory = false;
		inline bool custom_region = false;
		inline int custom_region_selection = 0;
		inline bool mouse_fix = false;
		inline bool keybind_list = false;
		inline bool door_spam = false;
		inline int door_spam_key = 0;
		inline int door_spam_key_s = 1;
		inline bool force_crosshair = false;
		inline bool sniper_crosshair = false;
		inline bool watermark = true;
		inline float watermark_shadowtext_clr[4]{ 1.f, 0.573f, 0.f, 0.9f };
		inline bool show_spotify_currently_playing = false;
		inline bool music_clantag = false;
		inline int spotify_time = false;
		inline bool spectators_list = true;
		inline float spectators_list_color_1[3]{ 1.f, 1.f, 1.f };
		inline float spectators_list_color_2[3]{ 1.f, 1.f, 1.f };
		inline float spectators_list_clr[3]{ 0.11765, 0.11765, 0.11765 };
		inline float spectators_list_clr_2[3]{ 0.11765, 0.11765, 0.11765 };
		inline int spectatorlist_type = 0;
		inline bool spectatorlist_show_target = true;
		inline int spectatorlist_x = 100;
		inline int spectatorlist_y = 50;
		inline int spectatorlist_w;
		inline int spectatorlist_h;
		inline bool practice = false;
		inline bool practice_window = false;
		inline int  savepos = 0;
		inline int  savepos_s = 1;
		inline int  loadpos = 0;
		inline int  loadpos_s = 1;
		inline int  nextkey = 0;
		inline int  nextkey_s = 1;
		inline int  prevkey = 0;
		inline int  prevkey_s = 1;
		inline int  undokey = 0;
		inline int  undokey_s = 1;
		inline bool enable_fov = false;
		inline float field_of_view = 0.f;
		inline bool view_model = false;
		inline float view_model_x = 0.f;
		inline float view_model_y = 0.f;
		inline float view_model_z = 0.f;
		inline int view_model_fov = 0;
		inline bool aspect_ratio = false;
		inline float aspect_ratio_amount = 1.6f;
		inline bool custom_console = false;
		inline float custom_console_clr[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float custom_console_clr2[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool discord_rpc = false;
		inline bool radaringame = false;
		inline bool jumpstats;
		inline bool jumpstats_show_clr_fail;
		inline bool jumpstats_show_fail;
		inline bool swayscale = false;
		inline float swayscale_value = 0.f;
		inline char misc_clantag_text[256] = "";
		inline bool misc_animated_clantag = false;
		inline bool misc_clantag_spammer = false;
		inline bool misc_clantag_rotation = false;
		inline int misc_clantag_type = 0;
		inline float misc_clantag_speed = 1.0f;
		inline bool misc_hitmarker = false;
		inline bool misc_hitmarker_sound = false;
		inline bool misc_hitmarker_screen_effect = false;
		inline bool misc_hit_info[2] = { false, false };
		inline int misc_hitmarker_sound_type = 0;
		inline bool misc_reveal_ranks = false;
		inline bool nadepred = false;
		inline float nadepred_clr[3]{ 1.f, 1.f, 1.f };
		inline bool autoaccept = false;
		inline bool vote_revealer = false;
		inline bool thirdperson = false;
		inline bool thirdperson_disabled_on_weapon = false;
		inline int thirdperson_distance = 150;
		inline int thirdperson_key = 0;
		inline int thirdperson_key_s = 1;
		inline bool freecam = false;
		inline int freecam_key = 0;
		inline int freecam_key_s = 1;
		inline bool misc_kill_msg = false;
		inline char misc_kill_message[256] = "Hello World!";
		inline int menu_variant = 0;
		inline int menu_fx_mode = 1;
		inline float menu_fx_density = 1.0f;
		inline int menu_key = 0x2D; //ins
		inline bool movement_rec = false;
		inline bool movement_rec_show_line = true;
		inline bool movement_rec_infowindow = false;
		inline float movement_rec_smoothing = 1.f;
		inline int movement_rec_maxrender = 528;
		inline float movement_rec_infowindowx;
		inline float movement_rec_infowindowy;
		inline bool movement_rec_lockva = true;
		inline float movement_rec_ringsize = 5;
		inline int movement_rec_keystartrecord = 0;
		inline int movement_rec_keystartrecord_s = 1;
		inline int movement_rec_keystoprecord = 0;
		inline int movement_rec_keystoprecord_s = 1;
		inline int movement_rec_keystartplayback = 0;
		inline int movement_rec_keystartplayback_s = 1;
		inline int movement_rec_keystopplayback = 0;
		inline int movement_rec_keystopplayback_s = 1;
		inline int movement_rec_keyclearrecord = 0;
		inline int movement_rec_keyclearrecord_s = 1;
		inline int movement_rec_keysaveclip = 1;
		inline int movement_rec_keysaveclip_s = 1;
		inline int movement_rec_angletype = 0;
	}

	namespace skins {
		inline bool agent_changer = false;
		inline int agent_t = 0;
		inline int agent_ct = 0;

		inline int knife_changer_wear = 0;
		inline int knife_changer_model = 0;
		inline int knife_changer_skin_id = 0;
		inline int knife_changer_paint_kit = 0;
		inline int knife_changer_vector_paint_kit = 0;
		inline int knife_skin = 0;

		inline bool gloves_endable = false;
		inline int gloves_model = 0;
		inline int gloves_skin = 0;
		inline int gloves_skin_id = 0;
		inline int gloves_wear = 0;
		inline int agent_model = 0;

		inline bool knife_changer_enable = false;
		inline bool skin_custom_clr = false;
		inline bool animated_pulse = true; // NEW toggle
		inline float skin_modulation1[3] = { 0.f, 0.9f, 0.f }; // Reddish
		inline float skin_modulation2[3] = { 0.f, 0.9f, 0.f }; // Green
		inline float skin_modulation3[3] = { 0.f, 0.9f, 0.f }; // Blue
		inline float skin_modulation4[3] = { 0.f, 0.9f, 0.f }; // Yellow

		inline bool weapon_endable = false;
		inline int weapons_page = 0;
		inline int weapons_model = 0;

	}

	namespace visuals {

		inline bool camera_fix_enabled = false;
		inline float camera_smoothing = 0.1325f;
		inline bool camrea_enable_smoothing = true;        
		inline bool camera_angle_fix = true;        
		inline bool camera_teleport_fix = true;     

		inline bool enable_backtrack_chams = false;
		inline bool enable_chams = false;
		inline bool style_points_combo = true;
		inline bool remove_original_model = false;
		inline bool style_points_background = true;
		inline bool watermark_use_custom_text_color = false;
		inline float watermark_text_color[4]{ 0.96f, 0.97f, 0.99f, 1.0f };
		inline bool watermark_use_custom_accent_color = false;
		inline float watermark_accent_color[4]{ 1.0f, 0.647f, 1.0f, 1.0f };
		inline bool watermark_text_outline = true;
		inline bool watermark_text_shadow = true;
		inline float watermark_text_glow_strength = 0.55f;
		inline float watermark_text_thickness = 1.0f;
		inline bool points_hud_show_mascot = true;
		inline bool points_hud_show_stars = true;
		inline float points_hud_mascot_scale = 1.35f;
		inline int points_hud_style = 0;
		inline bool points_hud_glow = true;
		inline bool points_hud_trails = true;
		inline float points_hud_glow_strength = 1.0f;
		inline bool movement_hud_combo_tracker = true;
		inline bool movement_hud_combo_multiplier = true;
		inline bool movement_hud_velocity_strip = true;
		inline bool movement_hud_trick_flash = true;
		inline bool movement_hud_timing_indicator = true;
		inline bool movement_hud_sound_tick = false;
		inline float speclist_x = 250.f;
		inline float speclist_y = 40.f;
		inline int mb_video_adapter = 0;
		inline bool enable_removals = false;
		inline bool backtrack_dot = false;
		inline float backtrack_dot_clr[4]{ 0.8f,0.f,0.f,1.f };
		inline bool enable_darkmode = false;
		inline float darkmode_val = 0.5f;

		inline bool cham_lights = false;
		inline bool backtrack_cham_lights = false;

		inline bool remove_phong = false;
		inline bool remove_envmap = false;
		inline bool remove_reflectivity = false;

		inline bool remove_bumpmap = false;
		inline bool remove_ssbump = false;
		inline bool remove_selfillum = false;
		inline bool remove_lightwarptexture = false;
		inline bool remove_halflambert = false;
		inline bool remove_ambientocclusion = false;
		inline bool remove_rimlight = false;

		inline bool remove_ignorez = false;
		inline bool remove_alphatest = false;
		inline bool remove_softwareskin = false;

		inline bool notifcation_system;
		inline bool movement_noti;
		inline bool old_font;
		inline bool remove_3dsky;
		inline bool remove_shadows;
		inline bool remove_post_processing;
		inline bool remove_panorama_blur;

		inline bool edit_bloom;
		inline float bloom_value;

		inline bool damage_indicator;
		inline float damage_indi_clr[3]{ 0.8f,0.f,0.f };
		inline float damage_indi_clr_shadow[3]{ 0.0f,0.f,0.f };
		inline float damage_indi_clr2[3]{ 0.8f,0.f,0.f };
		inline int weather_type = 0;
		inline bool enable_weather = false;
		inline bool weather_use_entity_precip = true;
		inline bool weather_overlay = true;
		inline float weather_overlay_density = 1.0f;
		inline float weather_overlay_speed = 1.0f;
		inline float weather_overlay_opacity = 0.55f;
		inline float weather_overlay_brightness = 1.0f;
		inline float weather_overlay_wind = 0.35f;
		inline float weather_overlay_tint[4]{ 0.82f, 0.88f, 1.0f, 1.0f };
		inline int world_preset_index = 0;
		inline int ui_panel_style = 0;
		inline bool ui_accent_pulse = true;
		inline bool ui_overlay_grain = false;
		inline float ui_overlay_grain_strength = 0.16f;
		inline int ui_line_weight = 0;
		inline bool enable_visuals = false;
		inline bool fade_animation = false;
		inline bool enemy_only = false;
		inline bool playerbox = false;
		inline bool playerbackground = false;
		inline bool apply_zoom = false;
		inline int apply_zoom_key = 0;
		inline int apply_zoom_key_s = 1;
		inline int boxtype = 0;
		inline float corner_lenght = 5.f;
		inline bool playerarmor = false;
		inline float playerarmor_clr[3]{ 0.f, 0.31f, 1.0f };
		inline bool playerammo = false;
		inline float playerammo_clr[3]{ 1.f, 1.f, 1.f };
		inline float player_weapon_color[3]{ 1.f, 1.f, 1.f };
		inline int dropped_weapon_type = 0;
		inline bool player_box_outline[2] = { true, true };
		inline float playerbox_color[3]{ 1.f, 1.f, 1.f };
		inline float playerbackground_color1[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float playerbackground_color2[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool playername = false;
		inline float playername_color[3]{ 1.f, 1.f, 1.f };
		inline bool playerweapon = false;
		inline bool playerweapon_icon = false;
		inline bool playerweapon_name = false;
		inline bool player_weapon_type[2] = { false, true };
		inline float dropped_weapon_icon_clr[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool dropped_weapon_name = false;
		inline bool dropped_weapon_icon = false;
		inline float dropped_weapon_name_clr[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float dropped_weapon_distance = 150.f;
		inline bool dropped_weapon_glow = false;
		inline float dropped_weapon_glow_clr[3]{ 1.f, 1.f, 1.f };
		inline bool bomb_esp = false;
		inline bool healthesp = false;
		inline int healthesp_s = 0;
		inline float health_bar[3]{ 1.f, 1.f, 1.f };
		inline float health_color[3]{ 1.f, 1.f, 1.f };
		inline float health_bar_gradient[3]{ 1.f, 1.f, 1.f };
		inline int health_bar_width = 2;
		inline bool override_bar = false;
		inline bool gradient_bar = false;
		inline bool skeleton = false;
		inline bool skeletonesp = false;
		inline float skeletonesp_clr[3]{ 1.f, 1.f, 1.f };
		inline float skeletonbt_clr[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float skeleton_last_clr[3]{ 1.f, 1.f, 1.f };
		inline float selected_tick_clr[3]{ 1.f, 1.f, 1.f };
		inline bool skeleton_last = false;
		inline bool glow = false;
		inline bool glow_weapon = false;
		inline int glow_style = 0;
		inline bool invisibleglow = false;
		inline bool visibleglow = false;
		inline float skeleton_glow_clr[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float glow_visible_clr[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float glow_invisible_clr[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float glow_weapon_clr[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool oof_arrows;
		inline int oof_arrows_size;
		inline int oof_arrows_dist;
		inline float oof_arrows_clr[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float oof_arrows_clr2[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool  radar = false;
		inline bool  mbenabled = false;
		inline bool  mbforwardEnabled = false;
		inline float mbfallingMin = 10.0f;
		inline float mbfallingMax = 20.0f;
		inline float mbfallingIntensity = 1.0f;
		inline float mbrotationIntensity = 2.0f;
		inline float mbstrength = 1.f;
		inline int mb_mode = 0;
		inline float mbsmoothing = 0.5f;
		inline bool fog;
		inline int fog_distance = 1500;
		inline int fog_density = 37;
		inline float fog_color[4]{ 1.f, 1.f, 1.f, 1.f };
		inline int skybox;
		inline int flashalpha;
		inline bool change_flashalpha = false;
		inline bool nosmoke = false;
		inline bool trails;
		inline float trails_clr1[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float trails_clr2[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool visuals_flags[6] = { false, false, false, false, false, false };
		inline bool removals[5] = { false, false , false , false , false };
		inline float world_color[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool world_modulate = false;
		inline bool world_modulate_props = true;
		inline bool world_modulate_skybox = false;
		inline float world_modulate_intensity = 1.0f;
		inline bool custom_shaders = false;
		inline bool fullbright;
		inline bool dlight = false;
		inline float dlight_clr[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool gravity_ragdoll = false;
		inline bool custom_sun = false;
		inline int  custom_sun_x = 50;
		inline int  custom_sun_y = 5;
		inline int  custom_sun_dist = 6000;
		inline int  custom_sun_mode = 0;
		inline float custom_sun_speed = 18.0f;
		inline float custom_sun_orbit_range = 35.0f;

		inline bool pixelsurf_db_enable = false;
		inline bool pixelsurf_db_draw_active = true;
		inline bool pixelsurf_db_draw_path = true;
		inline bool pixelsurf_db_load_csgomate = true;
		inline int pixelsurf_db_max_paths = 24;
		inline float pixelsurf_db_point_spacing = 6.f;
		inline float pixelsurf_db_line_thickness = 1.0f;
		inline float pixelsurf_db_clr[4]{ 0.1f, 0.8f, 1.0f, 1.0f };
	}

	namespace chams {

		inline bool enable_custom_shaders;
		inline bool disable_phong;

		inline bool weapon_chams = false;
		inline bool agent_chams = false;
		inline bool old_shaders = false;
		inline bool backtrack_chams = false;
		inline bool backtrack_chams_gradient = false;
		inline bool backtrack_chams_invisible = false;
		inline bool backtrack_chams_draw_all_ticks = true;
		inline float backtrack_chams_clr1[4]{ 1.f, 1.f, 1.f, 1.f };
		inline float backtrack_chams_clr2[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool visible_chams = false;
		inline bool visible_wireframe = false; //
		inline float visible_chams_clr[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool visible_chams_ov = false; //
		inline bool visible_wireframe_ov = false; //
		inline float visible_chams_clr_ov[4]{ 1.f, 1.f, 1.f, 1.f }; //
		inline bool invisible_chams = false;
		inline float invisible_chams_clr[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool invisible_wireframe = false; //
		inline bool sleeve_chams = false; //
		inline float sleeve_chams_clr[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool sleeve_wireframe = false; //
		inline bool arms_chams = false; //
		inline float arms_chams_clr[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool arms_wireframe = false; //
		inline bool sleeve_chams_ov = false;
		inline float sleeve_chams_clr_ov[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool sleeve_wireframe_ov = false; //
		inline bool arms_chams_ov = false;
		inline float arms_chams_clr_ov[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool arms_wireframe_ov = false; //
		inline bool wpn_chams = false; //
		inline float wpn_chams_clr[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool wpn_wireframe = false; //
		inline bool wpn_chams_ov = false; //
		inline float wpn_chams_clr_ov[4]{ 1.f, 1.f, 1.f, 1.f };
		inline bool wpn_wireframe_ov = false; //
		inline int cham_type_bt = 0;
		inline int cham_type = 0;
		inline int cham_type_overlay = 0; //
		inline int cham_type_sleeve = 0;
		inline int cham_type_sleeve_ov = 0;
		inline int cham_type_wpn = 0;
		inline int cham_type_wpn_ov = 0;
		inline int cham_type_arms = 0; //
		inline int cham_type_arms_ov = 0; //
		inline int chams_backrack_ticks = 0;
		inline int localtype = 0;
		inline int type_p = 0;
	}

	namespace fonts {

		inline int indi_size = 28;
		inline int indi_font = 0;
		inline bool indi_font_flag[11] = { false, false, false, false, false, false, false, false, false, true, false };

		inline int sub_indi_size = 28;
		inline int sub_indi_font = 0;
		inline bool sub_indi_font_flag[11] = { false, false, false, false, false, false, false, false, false, true, false };

		inline int esp_hp_font = 0;
		inline int esp_hp_size = 11;
		inline bool esp_font_flag[11] = { false, false, false, true, false, false, true, false, false, true, false };

		inline int sc_logs_font = 0;
		inline int sc_logs_size = 11;
		inline bool sc_logs_flag[11] = { false, false, false, true, false, false, true, false, false, true, false };

		inline int esp_name_font = 0;
		inline int esp_name_size = 11;
		inline bool esp_name_font_flag[11] = { false, false, false, true, false, false, true, false, false, true, false };

		inline int esp_wpn_font = 0;
		inline int esp_wpn_size = 11;
		inline bool esp_wpn_font_flag[11] = { false, false, false, true, false, false, true, false, false, true, false };

		inline int esp_dropped_wpn_font = 0;
		inline int esp_dropped_wpn_size = 11;
		inline bool esp_dropped_wpn_font_flag[11] = { false, false, false, true, false, false, true, false, false, true, false };

		inline int esp_font = 0;

		inline int spec_font = 0;
		inline int spec_size = 12;
		inline bool spec_font_flag[9] = { false, false, false, true, false, false, true, true, false };

		inline int watermark_font = 0;
		inline int watermark_size = 13;
		inline bool watermark_font_flag[11] = { false, false, false, true, false, false, true, false, false, true, false };

		inline int sub_spec_font = 0;
		inline bool sub_spec_font_flag[9] = { false, false, false, true, false, false, true, true, false };
	}

	namespace sfui {
		inline bool sfui_on = false;
	}

	namespace calculator {
		inline int  toggle_wireframe_key = 0;
		inline int  toggle_wireframe_key_s = 2;
		inline int  toggle_clipbrushe_key = 0;
		inline int  toggle_clipbrushe_key_s = 2;
		inline int  search_wall_key = 0;
		inline int  search_wall_key_s = 1;
		inline int  set_point_key = 0;
		inline int  set_point_key_s = 1;
		inline int  remove_point_key = 0;
		inline int  remove_point_key_s = 1;
		inline int  clear_point_key = 0;
		inline int  clear_point_key_s = 1;
		inline int  calculate_point_key = 0;
		inline int  calculate_point_key_s = 1;

	}

	namespace route_node {
		inline float similar_route_time = 1000.f;

		//Route Node Main
		inline bool enable_route_node = false;
		inline float route_node_scan_distance = 150.f;

		//Node_Eye
		inline float route_node_best_dot = 0.98f;

		//Node_align
		inline float route_node_align_distance = 2.f;
		inline float route_node_align_speed = 500.f;

		//Node_Surf
		inline int route_node_teleport_surf_ticks = 3;

		//Additional 
		inline float route_node_end_lift = 2.f;


		//UI
		inline bool route_node_ui = false;
		inline bool route_node_draw = false;

		//Binds
		inline int route_node_scan_key = 0;
		inline int route_node_scan_key_s = 1;

	}

	namespace c_strafe {

		inline bool strafe_ui_enable = false;
		inline bool strafe_enable = true;
		inline float strafe_smooth = 0.178f;
		inline float strafe_angle = 12.f;
		inline float strafe_deaccel = 17.f;
		inline float correct_move_val = 5.f;
		inline float calculated_speed_gain;

	}

	inline std::vector<std::string> configs;
}

struct keybind_t {
	bool enabled;
	int id;
	int type;
};

enum keybind_type {
	off,
	hold,
	toggle,
	always
};

struct cvar_t;
struct cvars_t;
