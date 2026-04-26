#pragma once
#include "../../game/sdk/classes/c_color.h"
#include "config.h"
#include "structs/font_setting_t.h"

// TODO: add this
enum e_player_flags {
	player_flag_money = 0,
	player_flag_armor,
	player_flag_reloading,
	player_flag_bomb,
	player_flags_max
};

enum e_free_type_font_flags : int {
	font_flag_nohinting = 0,
	font_flag_noautohint,
	font_flag_forceautohint,
	font_flag_lighthinting,
	font_flag_monohinting,
	font_flag_bold,
	font_flag_oblique,
	font_flag_monochrome,
	font_flag_max
};

enum e_log_types {
	log_type_hit_enemy,
	log_type_hit_teammate,
	log_type_purchase,
	log_type_votes,
	log_type_max,
};

enum e_keybind_indicators {
	key_eb = 0,
	key_ps,
	key_ej,
	key_lj,
	key_mj,
	key_jb,
	key_ast,
	key_bast,
	key_fr,
	key_lb,
	key_max
};

namespace n_variables
{
	struct impl_t {
		/* menu */
		ADD_VARIABLE( c_color, m_accent, c_color( 129, 99, 251, 255 ) );

		ADD_VARIABLE( bool, m_aimbot_enable, false );
		ADD_VARIABLE( bool, m_backtrack_enable, false );
		ADD_VARIABLE( bool, m_backtrack_extend, false );
		ADD_VARIABLE( int, m_backtrack_time_limit, 200 );
		ADD_VARIABLE( int, fakeLatencyAmount, 200 );
		/* visuals - players */
		ADD_VARIABLE( bool, m_players, false );
		ADD_VARIABLE( bool, m_players_box, false );
		ADD_VARIABLE( bool, m_players_box_corner, false );
		ADD_VARIABLE( bool, m_players_box_outline, false );
		ADD_VARIABLE( c_color, m_players_box_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_players_box_outline_color, c_color( 0, 0, 0, 255 ) );
		ADD_VARIABLE( c_color, m_players_box_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_players_box_outline_color_in, c_color( 0, 0, 0, 255 ) );
		ADD_VARIABLE( bool, m_players_health_bar, false );
		ADD_VARIABLE( bool, m_players_health_bar_custom_color, false );
		ADD_VARIABLE( bool, m_players_health_text, false );
		ADD_VARIABLE( bool, m_players_health_suffix, false );
		ADD_VARIABLE( int, m_players_health_text_style, 0 );
		ADD_VARIABLE( c_color, m_players_health_bar_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_players_health_bar_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_players_name, false );
		ADD_VARIABLE( c_color, m_players_name_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_players_name_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_weapon_name, false );
		ADD_VARIABLE( c_color, m_weapon_name_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_weapon_name_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_weapon_icon, false );
		ADD_VARIABLE( c_color, m_weapon_icon_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_weapon_icon_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_player_ammo_bar, false );
		ADD_VARIABLE( c_color, m_player_ammo_bar_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_player_ammo_bar_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_players_skeleton, false );
		ADD_VARIABLE( c_color, m_players_skeleton_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_players_skeleton_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( int, m_players_skeleton_type, 0 );
		ADD_VARIABLE( bool, m_glow_enable, false );
		ADD_VARIABLE( c_color, m_glow_vis_color, c_color( 224, 175, 86, 153 ) );
		ADD_VARIABLE( c_color, m_glow_invis_color, c_color( 114, 155, 221, 153 ) );
		ADD_VARIABLE( bool, m_players_backtrack_trail, false );
		ADD_VARIABLE( bool, m_players_avatar, false );
		ADD_VARIABLE( bool, m_out_of_fov_arrows, false );
		ADD_VARIABLE( c_color, m_out_of_fov_arrows_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( float, m_out_of_fov_arrows_size, 20.f );
		ADD_VARIABLE( int, m_out_of_fov_arrows_distance, 200 );

		/* visuals - chams*/

		// TODO ADD OVERLAYS USING g_ctx.m_chams.m_vis_layer_amt

		// vis
		ADD_VARIABLE( bool, m_player_visible_chams, false );
		ADD_VARIABLE( int, m_player_visible_chams_material, 0 );
		ADD_VARIABLE( c_color, m_player_visible_chams_color, c_color( 114, 0, 221, 153 ) );

		// invis
		ADD_VARIABLE( bool, m_player_invisible_chams, false );
		ADD_VARIABLE( int, m_player_invisible_chams_material, 0 );
		ADD_VARIABLE( c_color, m_player_invisible_chams_color, c_color( 0, 200, 221, 153 ) );

		// lag
		ADD_VARIABLE( bool, m_player_lag_chams, false );
		ADD_VARIABLE( bool, m_player_lag_chams_xqz, false );
		ADD_VARIABLE( int, m_player_lag_chams_type, 0 );
		ADD_VARIABLE( int, m_player_lag_chams_material, 0 );
		ADD_VARIABLE( c_color, m_player_lag_chams_color, c_color( 114, 155, 221, 153 ) );

		/* visuals - edicts */
		ADD_VARIABLE( bool, m_dropped_weapons, false );
		ADD_VARIABLE( bool, m_dropped_weapons_box, false );
		ADD_VARIABLE( bool, m_dropped_weapons_box_corner, false );
		ADD_VARIABLE( bool, m_dropped_weapons_box_outline, false );
		ADD_VARIABLE( c_color, m_dropped_weapons_box_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_dropped_weapons_box_outline_color, c_color( 0, 0, 0, 255 ) );
		ADD_VARIABLE( c_color, m_dropped_weapons_box_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_dropped_weapons_box_outline_color_in, c_color( 0, 0, 0, 255 ) );
		ADD_VARIABLE( bool, m_dropped_weapons_name, false );
		ADD_VARIABLE( c_color, m_dropped_weapons_name_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_dropped_weapons_name_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_dropped_weapons_icon, false );
		ADD_VARIABLE( c_color, m_dropped_weapons_icon_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_dropped_weapons_icon_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_hit_sound, false );
		ADD_VARIABLE( bool, m_thrown_objects, false );
		ADD_VARIABLE( bool, m_thrown_objects_name, false );
		ADD_VARIABLE( c_color, m_thrown_objects_name_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_thrown_objects_name_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_thrown_objects_icon, false );
		ADD_VARIABLE( c_color, m_thrown_objects_icon_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_thrown_objects_icon_color_in, c_color( 255, 255, 255, 255 ) );
		/* visuals - world */
		ADD_VARIABLE( bool, m_sun_set, false );
		ADD_VARIABLE( float, m_sun_set_rot_x, 0.f );
		ADD_VARIABLE( float, m_sun_set_rot_y, 0.f );
		ADD_VARIABLE( float, m_sun_set_rot_z, 0.f );
		ADD_VARIABLE( bool, m_precipitation, false );
		ADD_VARIABLE( int, m_precipitation_type, 0 );
		ADD_VARIABLE( c_color, m_precipitation_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_fog, false );
		ADD_VARIABLE( float, m_fog_start, 0.f );
		ADD_VARIABLE( float, m_fog_end, 0.f );
		ADD_VARIABLE( c_color, m_fog_color, c_color( 255, 255, 255, 255 ) );

		ADD_VARIABLE( bool, m_custom_smoke, false );
		ADD_VARIABLE( c_color, m_custom_smoke_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_custom_molotov, false );
		ADD_VARIABLE( c_color, m_custom_molotov_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_custom_blood, false );
		ADD_VARIABLE( c_color, m_custom_blood_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_custom_precipitation, false );
		ADD_VARIABLE( c_color, m_custom_precipitation_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_disable_post_processing, false );
		ADD_VARIABLE( bool, m_remove_panorama_blur, false );

		/* visuals - screen */
		ADD_VARIABLE( bool, m_motion_blur, false );
		ADD_VARIABLE(bool,  m_motion_forward_move_blur  , false);
		ADD_VARIABLE(float, m_motion_falling_minimum   , 10.0f);
		ADD_VARIABLE(float, m_motion_falling_maximum   , 20.0f);
		ADD_VARIABLE(float, m_motion_falling_intensity , 1.0f);
		ADD_VARIABLE(float, m_motion_rotate_intensity  , 1.0f);
		ADD_VARIABLE(float, m_motion_strength          , 1.0f);


		/* misc - movement */
		ADD_VARIABLE( bool, m_bunny_hop, false );
		ADD_VARIABLE( bool, m_no_crouch_cooldown, false );
		ADD_VARIABLE( bool, m_edge_jump, false );
		ADD_VARIABLE( key_bind_t, m_edge_jump_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( bool, m_edge_jump_ladder, false );
		ADD_VARIABLE( bool, m_long_jump, false );
		ADD_VARIABLE( key_bind_t, m_long_jump_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( bool, m_mini_jump, false );
		ADD_VARIABLE( key_bind_t, m_mini_jump_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( bool, m_mini_jump_hold_duck, false );
		ADD_VARIABLE( bool, m_jump_bug, false );
		ADD_VARIABLE( key_bind_t, m_jump_bug_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( bool, m_pixel_surf, false );
		ADD_VARIABLE( key_bind_t, m_pixel_surf_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( bool, m_pixel_surf_fix, false );
		ADD_VARIABLE( bool, m_auto_align, false );
		ADD_VARIABLE( bool, m_auto_duck, false );

	
		ADD_VARIABLE( key_bind_t, edge_bug_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( int, EdgeBugTicks, 1 );
		ADD_VARIABLE( bool, edge_bug, false );
		ADD_VARIABLE(bool, EdgeBugAdvanceSearch,false);
		ADD_VARIABLE( bool, EdgeBugDetectChat, false );
		ADD_VARIABLE(float, EdgeBugMouseLock,0.f);
		ADD_VARIABLE( float, deltaScaler, 0.f );
		ADD_VARIABLE(int, DeltaType,0);
		ADD_VARIABLE( bool, MegaEdgeBug, false );
		ADD_VARIABLE(bool, AutoStrafeEdgeBug, false );
		ADD_VARIABLE(bool, SiletEdgeBug     , false );

		ADD_VARIABLE( bool, m_velocity_indicator, false );
		ADD_VARIABLE( bool, m_velocity_indicator_show_pre_speed, false );
		ADD_VARIABLE( bool, m_velocity_indicator_fade_alpha, false );
		ADD_VARIABLE( bool, m_velocity_indicator_custom_color, false );
		ADD_VARIABLE( int, m_velocity_indicator_padding, 125 );

		ADD_VARIABLE( bool, m_fire_man, false );
		ADD_VARIABLE( key_bind_t, m_fire_man_key, key_bind_t( 0, 1 ) );

		ADD_VARIABLE( key_bind_t, m_pixel_finder_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( bool, m_hit_marker, false );
		ADD_VARIABLE( int, m_sky_box, 0 );
		ADD_VARIABLE( bool, m_pixel_surf_assist, false );
		ADD_VARIABLE( bool, m_pixel_surf_assist_brokehop, false );
		ADD_VARIABLE( bool, m_pixel_surf_assist_render, false );
		ADD_VARIABLE( float, m_pixel_surf_assist_radius, 300.f );
		ADD_VARIABLE( key_bind_t, m_pixel_surf_assist_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( key_bind_t, m_pixel_surf_assist_point_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( key_bind_t, m_pixel_jump_point_key, key_bind_t( 0, 1 ) );

		ADD_VARIABLE( bool, m_bouncee_assist, false );
		ADD_VARIABLE( bool, m_bouncee_assist_brokehop, false );
		ADD_VARIABLE( bool, m_bouncee_assist_render, false );
		ADD_VARIABLE( float, m_bouncee_assist_radius, 300.f );
		ADD_VARIABLE( key_bind_t, m_bounce_assist_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( key_bind_t, m_bounce_assist_point_key, key_bind_t( 0, 1 ) );

		ADD_VARIABLE( key_bind_t, m_ladder_bug_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( bool, m_ladder_bug, false );


		ADD_VARIABLE( key_bind_t, m_air_stuck_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( bool, m_air_stuck, false );
		float test_air_stuck = 0.f;

		/* misc - movement - indicators */
		ADD_VARIABLE( c_color, m_velocity_indicator_color1, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_velocity_indicator_color2, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_velocity_indicator_color3, c_color( 255, 199, 89, 255 ) );
		ADD_VARIABLE( c_color, m_velocity_indicator_color4, c_color( 255, 119, 119, 255 ) );
		ADD_VARIABLE( c_color, m_velocity_indicator_color5, c_color( 30, 255, 109, 255 ) );
		ADD_VARIABLE( float, m_sun_set_rotation_speed, 1.0f );

		ADD_VARIABLE( bool, m_stamina_indicator, false );
		ADD_VARIABLE( bool, m_stamina_indicator_show_pre_speed, false );
		ADD_VARIABLE( c_color, m_stamina_indicator_color1, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_stamina_indicator_color2, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_stamina_indicator_fade_alpha, false );
		ADD_VARIABLE( int, m_stamina_indicator_padding, 125 );

		ADD_VARIABLE( c_color, m_key_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_key_color_success, c_color( 0, 255, 0, 255 ) );
		ADD_VARIABLE( bool, m_key_indicators_enable, false );
		ADD_VARIABLE( int, m_key_indicators_type, 0 );
		ADD_VARIABLE( int, m_key_indicators_position, 100 );
		ADD_VARIABLE_VECTOR( bool, e_keybind_indicators::key_max, m_key_indicators, false );

		/* misc - game */
		ADD_VARIABLE( bool, m_spectators_list, false );
		ADD_VARIABLE( c_color, m_spectators_list_text_color_one, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_spectators_list_text_color_two, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( int, m_spectators_list_type, 0 );
		ADD_VARIABLE( bool, m_spectators_avatar, false );
		ADD_VARIABLE( bool, m_scaleform, false );

		ADD_VARIABLE( bool, m_sniper_crosshair, false );

		ADD_VARIABLE( bool, m_practice_window, false );
		ADD_VARIABLE( key_bind_t, m_practice_cp_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( key_bind_t, m_practice_tp_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( float, m_pixel_surf_assist_lockfactor, 0.f );
		ADD_VARIABLE( int, m_pixel_surf_assist_ticks, 12 );
		ADD_VARIABLE( int, m_pixel_surf_assist_type, 0 );
		ADD_VARIABLE( float, m_pixelsurf_assist_delta_scale, 0.f );
		ADD_VARIABLE_VECTOR( bool, e_log_types::log_type_max, m_log_types, false );

		/* fonts */
		ADD_VARIABLE_VECTOR( bool, e_free_type_font_flags::font_flag_max, m_indicator_font_flags, false );
		ADD_VARIABLE( font_setting_t, m_indicator_font_settings, font_setting_t( "Verdanab", 29 ) );

		// NEW: ESP font selection
		ADD_VARIABLE_VECTOR( bool, e_free_type_font_flags::font_flag_max, m_esp_font_flags, false );
		ADD_VARIABLE( font_setting_t, m_esp_font_settings, font_setting_t( "Verdanab", 13 ) );

		// NEW: Render Assistant (addwindow) font selection
		ADD_VARIABLE_VECTOR( bool, e_free_type_font_flags::font_flag_max, m_addwindow_font_flags, false );
		ADD_VARIABLE( font_setting_t, m_addwindow_font_settings, font_setting_t( "Verdanab", 11 ) );
		ADD_VARIABLE( bool, m_debugger_visual, true );
		ADD_VARIABLE( bool, m_watermark, false );
		ADD_VARIABLE( bool, m_disable_interp, false );
		ADD_VARIABLE( bool, trackdispay, false );
		ADD_VARIABLE( bool, esp_enable, false );
		ADD_VARIABLE( bool,    colors_custom, false );
		
		ADD_VARIABLE( c_color, colors_health_bar				, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, colors_health_bar_upper			, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, colors_health_bar_lower			, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, colors_health_bar_outline		, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, colors_health_bar_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, colors_health_bar_upper_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, colors_health_bar_lower_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, colors_health_bar_outline_in, c_color( 255, 255, 255, 255 ) );
		
		ADD_VARIABLE( int, m_visible_alpha, 255 );
		ADD_VARIABLE( int, m_invisuals_alpha, 255 );
		
			ADD_VARIABLE( bool, health_bar_enable       , false);
			ADD_VARIABLE( bool, health_bar_baseonhealth , false);
			ADD_VARIABLE( bool, health_bar_gradient     , false);
			ADD_VARIABLE( bool, health_bar_background   , false);
			ADD_VARIABLE( bool, health_bar_text         , false);
			ADD_VARIABLE( int,  health_bar_size          ,    1);
		

		
			
				
		

		std::wstring artist, title;
		bool pause    = false;
		bool next     = false;
		bool pervious = false;
		std::string albumArtPath;
		int64_t trackPosition;
		int64_t trackDuration = 0;
		std::string m_custom_sky_box{ };

	

	

	};
}; // namespace n_variables

inline n_variables::impl_t g_variables{ };
