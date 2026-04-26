#define NOMINMAX
#include "movement.hpp"
#include "../../menu/menu.hpp"
#include "../../menu/config/config.hpp"
#include "../movement/prediction/prediction.hpp"
#include "../misc/misc.hpp"
#include "moveutil.h"

auto interpolate(const color_t first_color, const color_t& second_color, const float multiplier) {
	return color_t(first_color.r() + std::clamp(multiplier, 0.f, 1.f) * (second_color.r() - first_color.r()),
		first_color.g() + std::clamp(multiplier, 0.f, 1.f) * (second_color.g() - first_color.g()),
		first_color.b() + std::clamp(multiplier, 0.f, 1.f) * (second_color.b() - first_color.b()),
		first_color.a() + std::clamp(multiplier, 0.f, 1.f) * (second_color.a() - first_color.a()));
}

color_t velocity_color(int velocity_delta, float alpha = 1.0f) {
	if (velocity_delta > 0) {
		return color_t(
			static_cast<int>(c::movement::velocity_indicator_positive_clr[0] * 255),
			static_cast<int>(c::movement::velocity_indicator_positive_clr[1] * 255),
			static_cast<int>(c::movement::velocity_indicator_positive_clr[2] * 255),
			static_cast<int>(alpha * 255)
		);
	}
	else if (velocity_delta < 0) {
		return color_t(
			static_cast<int>(c::movement::velocity_indicator_negative_clr[0] * 255),
			static_cast<int>(c::movement::velocity_indicator_negative_clr[1] * 255),
			static_cast<int>(c::movement::velocity_indicator_negative_clr[2] * 255),
			static_cast<int>(alpha * 255)
		);
	}
	else {
		return color_t(
			static_cast<int>(c::movement::velocity_indicator_neutral_clr[0] * 255),
			static_cast<int>(c::movement::velocity_indicator_neutral_clr[1] * 255),
			static_cast<int>(c::movement::velocity_indicator_neutral_clr[2] * 255),
			static_cast<int>(alpha * 255)
		);
	}
}

bool indicator_grabbed = false;
void features::movement::velocity_indicator_anim() {
	if (!g::local || !g::local->is_alive() || !interfaces::engine->is_in_game())
		return;

	if (!c::movement::velocity_indicator)
		return;

	if (!(c::movement::velocity_indicator_type == 1))
		return;

	int screen_x, screen_y;
	interfaces::engine->get_screen_size(screen_x, screen_y);

	if (menu::open) {
		static bool is_dragging = false;
		static ImVec2 drag_offset;
		auto mouse_pos = get_mouse_position();

		int indicator_y = screen_y - c::movement::velocity_indicator_position;
		int center_x = screen_x / 2;
		bool hovering = mouse_pos.x >= center_x - 35 && mouse_pos.x <= center_x + 35 && mouse_pos.y >= indicator_y && mouse_pos.y <= indicator_y + 30;
		bool holding = GetAsyncKeyState(VK_LBUTTON) & 0x8000;

		if (hovering && holding && !indicator_grabbed) {
			if (!is_dragging)
				drag_offset = ImVec2(mouse_pos.x - center_x, mouse_pos.y - indicator_y);
			is_dragging = true;
			indicator_grabbed = true;
		}
		else if (!holding) {
			is_dragging = false;
			indicator_grabbed = false;
		}

		if (is_dragging) {
			im_render.drawline(center_x, screen_y, center_x, 0, color_t(255, 255, 255, 255));
			im_render.drawrect(center_x - 35, indicator_y, 70, 30, color_t(255, 255, 255, 255));
			c::movement::velocity_indicator_position = std::clamp(screen_y - static_cast<int>(mouse_pos.y - drag_offset.y), 0, screen_y);
		}
	}

	int y = screen_y - c::movement::velocity_indicator_position;

	static bool on_ground = false;
	static bool last_on_ground = false;
	static int take_off_velocity = 0;
	static float take_off_time = 0.0f;
	static float last_velocity = 0.0f;
	static float lerped_velocity = 0.0f;
	static float lerped_ghost_offset = 0.0f;
	static float lerped_alpha = 0.5f;

	int current_velocity = round(g::local->velocity().length_2d());
	on_ground = g::local->flags() & fl_onground;

	static float ghost_start_time = 0.0f;
	static bool ghost_active = false;

	if (last_on_ground && !on_ground) {
		take_off_velocity = current_velocity;
		ghost_start_time = interfaces::globals->cur_time;
		ghost_active = true;
	}
	last_on_ground = on_ground;

	// --- Rise timing ---
	const float anim_duration = 0.4f;   // how long ghost stays visible total
	float elapsed = interfaces::globals->cur_time - ghost_start_time;
	// deactivate after full anim
	if (elapsed > anim_duration) {
		ghost_active = false;
	}

	// --- Animation timing ---
	float t = std::clamp(elapsed / anim_duration, 0.f, 1.f);

	// Rise ease
	const float max_rise = 10.0f;
	float ghost_offset = max_rise * t; // simple linear rise (you can ease if you want)

	// Fade: fully visible then fade out at end
	float fade_out_start = 0.5f;
	float alpha = (t < fade_out_start)
		? 1.0f
		: 1.0f - (t - fade_out_start) / (1.0f - fade_out_start);
	alpha = std::clamp(alpha, 0.f, 1.f);

	// --- Smooth interpolation ---
	lerped_ghost_offset = std::lerp(lerped_ghost_offset, ghost_offset, 0.4f);
	lerped_alpha = std::lerp(lerped_alpha, alpha, 0.4f);

	// Smoothing interpolation (text blending)
	lerped_velocity = std::lerp(lerped_velocity, static_cast<float>(current_velocity), 0.15f);

	int vel_diff = current_velocity - static_cast<int>(last_velocity);
	//std::string vel_text = std::to_string(static_cast<int>(lerped_velocity));
	std::string vel_text = std::to_string(static_cast<int>(current_velocity));
	std::string shadow_text = std::to_string(take_off_velocity);

	color_t velocity_clr_int = color_t(c::movement::velocity_indicator_fade_clr3[0], c::movement::velocity_indicator_fade_clr3[1], c::movement::velocity_indicator_fade_clr3[2], c::movement::velocity_indicator_fade_clr3[3]);
	color_t velocity_clr_custom = color_t(c::movement::velocity_indicator_custom_clr[0], c::movement::velocity_indicator_custom_clr[1], c::movement::velocity_indicator_custom_clr[2], c::movement::velocity_indicator_custom_clr[3]);

	if (menu::experimental_femboy_theme_active) {
		velocity_clr_custom = color_t(255, 182, 193, 255);
		velocity_clr_int = color_t(255, 105, 180, 255);
	}
	else if (menu::experimental_fight_club_mode) {
		velocity_clr_custom = color_t(150, 0, 0, 255);
		velocity_clr_int = color_t(80, 0, 0, 255);
	}

	if (menu::experimental_femboy_theme_active) {
		velocity_clr_custom = color_t(255, 182, 193, 255);
		velocity_clr_int = color_t(255, 105, 180, 255);
	}
	color_t shadow_clr_int = color_t(c::movement::shadow_indicator_custom_clr[0], c::movement::shadow_indicator_custom_clr[1], c::movement::shadow_indicator_custom_clr[2], c::movement::shadow_indicator_custom_clr[3]);
	color_t shadow_clr_custom = color_t(c::movement::shadow_indicator_custom_clr2[0], c::movement::shadow_indicator_custom_clr2[1], c::movement::shadow_indicator_custom_clr2[2], c::movement::shadow_indicator_custom_clr2[3]);

	auto velocity_color = [](int velocity) -> color_t {
		if (velocity > 0)
			return color_t(c::movement::velocity_indicator_positive_clr[0], c::movement::velocity_indicator_positive_clr[1], c::movement::velocity_indicator_positive_clr[2], 255.f);
		else if (velocity < 0)
			return color_t(c::movement::velocity_indicator_negative_clr[0], c::movement::velocity_indicator_negative_clr[1], c::movement::velocity_indicator_negative_clr[2], 255.f);
		else
			return color_t(c::movement::velocity_indicator_neutral_clr[0], c::movement::velocity_indicator_neutral_clr[1], c::movement::velocity_indicator_neutral_clr[2], 255.f);
		};

	color_t render_clr = interpolate(velocity_clr_int, velocity_clr_custom, std::min(1.f, lerped_velocity / 255.f));

	color_t shadow1 = color_t(c::movement::shadow_indicator_custom_clr[0], c::movement::shadow_indicator_custom_clr[1], c::movement::shadow_indicator_custom_clr[2], c::movement::shadow_indicator_custom_clr[3]);
	color_t shadow2 = color_t(c::movement::shadow_indicator_custom_clr2[0], c::movement::shadow_indicator_custom_clr2[1], c::movement::shadow_indicator_custom_clr2[2], c::movement::shadow_indicator_custom_clr2[3]);
	color_t final_shadow = interpolate(shadow1, c::movement::velocity_indicator_sync_shadow ? velocity_color(vel_diff) : shadow2, lerped_alpha);

	final_shadow = color_t(
		static_cast<int>(final_shadow.r()),
		static_cast<int>(final_shadow.g()),
		static_cast<int>(final_shadow.b()),
		static_cast<int>(final_shadow.a() * 0.15f)
	);

	color_t fade_col = interpolate(shadow_clr_int, shadow_clr_custom, lerped_alpha);

	color_t render_clr2 = interpolate(shadow_clr_custom, shadow_clr_int, std::min(1.f, lerped_velocity / 255.f));

	// --- Only draw while ghost_active ---
	if (ghost_active && c::movement::velocity_indicator_show_ghost) {
		int ghost_y = y - static_cast<int>(lerped_ghost_offset);

		color_t ghost_color = color_t(
			shadow_clr_int.r(),
			shadow_clr_int.g(),
			shadow_clr_int.b(),
			static_cast<float>(shadow_clr_int.a() * lerped_alpha)
		);

		im_render.text(screen_x / 2, ghost_y, c::fonts::indi_size,
			fonts::indicator_font, std::to_string(take_off_velocity).c_str(),
			true, ghost_color, false);
	}

	// Smooth current velocity
	im_render.text(screen_x / 2 + 1, y + 1, c::fonts::indi_size, fonts::indicator_font, vel_text.c_str(), true, render_clr2, false, c::fonts::indi_font_flag[10]);
	im_render.text(screen_x / 2, y, c::fonts::indi_size, fonts::indicator_font, vel_text.c_str(), true, render_clr, false, c::fonts::indi_font_flag[10]);

	last_velocity = static_cast<float>(current_velocity);
}

void features::movement::velocity_indicator() {
	if (!g::local || !g::local->is_alive() || !interfaces::engine->is_in_game()) {
		return;
	}

	if (!c::movement::velocity_indicator) {
		return;
	}

	if (!(c::movement::velocity_indicator_type == 0))
		return;

	int screen_x, screen_y;
	interfaces::engine->get_screen_size(screen_x, screen_y);

	if (menu::open) {
		static bool is_dragging = false;
		static ImVec2 drag_offset;
		auto mouse_pos = get_mouse_position();

		int indicator_y = screen_y - c::movement::velocity_indicator_position;
		int center_x = screen_x / 2;
		bool hovering = mouse_pos.x >= center_x - 35 && mouse_pos.x <= center_x + 35 && mouse_pos.y >= indicator_y && mouse_pos.y <= indicator_y + 30;
		bool holding = GetAsyncKeyState(VK_LBUTTON) & 0x8000;

		if (hovering && holding && !indicator_grabbed) {
			if (!is_dragging)
				drag_offset = ImVec2(mouse_pos.x - center_x, mouse_pos.y - indicator_y);
			is_dragging = true;
			indicator_grabbed = true;
		}
		else if (!holding) {
			is_dragging = false;
			indicator_grabbed = false;
		}

		if (is_dragging) {
			im_render.drawline(center_x, screen_y, center_x, 0, color_t(255, 255, 255, 255));
			im_render.drawrect(center_x - 35, indicator_y, 70, 30, color_t(255, 255, 255, 255));
			c::movement::velocity_indicator_position = std::clamp(screen_y - static_cast<int>(mouse_pos.y - drag_offset.y), 0, screen_y);
		}
	}

	int y = screen_y - c::movement::velocity_indicator_position;

	static bool on_ground = false;
	static auto take_off = 0;
	static auto take_off_time = 0.0f;
	static auto last_on_ground = false;

	int vel = round(g::local->velocity().length_2d());
	on_ground = g::local->flags() & fl_onground;

	if (last_on_ground && !on_ground) {
		take_off = vel;
		take_off_time = interfaces::globals->cur_time + (c::movement::velocity_indicator_disable_ong_show_pre ? 0.0f : 3.5f);
	}

	last_on_ground = on_ground;

	static float velocity_alpha = 0.0f;
	velocity_alpha = 1.f * (float(round(vel)) / 255.f);

	auto velocity_color = [](int velocity) -> color_t {
		if (velocity > 0)
			return color_t(c::movement::velocity_indicator_positive_clr[0], c::movement::velocity_indicator_positive_clr[1], c::movement::velocity_indicator_positive_clr[2], 255.f);
		else if (velocity < 0)
			return color_t(c::movement::velocity_indicator_negative_clr[0], c::movement::velocity_indicator_negative_clr[1], c::movement::velocity_indicator_negative_clr[2], 255.f);
		else if (velocity > -1 && velocity < 1)
			return color_t(c::movement::velocity_indicator_neutral_clr[0], c::movement::velocity_indicator_neutral_clr[1], c::movement::velocity_indicator_neutral_clr[2], 255.f);
		};

	color_t velocity_clr_int = color_t(c::movement::velocity_indicator_fade_clr3[0], c::movement::velocity_indicator_fade_clr3[1], c::movement::velocity_indicator_fade_clr3[2], c::movement::velocity_indicator_fade_clr3[3]);
	color_t velocity_clr_custom = color_t(c::movement::velocity_indicator_custom_clr[0], c::movement::velocity_indicator_custom_clr[1], c::movement::velocity_indicator_custom_clr[2], c::movement::velocity_indicator_custom_clr[3]);

	if (menu::experimental_femboy_theme_active) {
		velocity_clr_custom = color_t(255, 182, 193, 255);
		velocity_clr_int = color_t(255, 105, 180, 255);
	}
	else if (menu::experimental_fight_club_mode) {
		velocity_clr_custom = color_t(150, 0, 0, 255);
		velocity_clr_int = color_t(80, 0, 0, 255);
	}

	int tick_count = interfaces::globals->tick_count;
	last_delta = vel - last_vel;

	const auto should_draw_takeoff =
		(!on_ground || (take_off_time > interfaces::globals->cur_time)) && c::movement::velocity_indicator_show_pre;

	std::string place_holder = "";

	if (should_draw_takeoff)
		place_holder = std::to_string(vel) + " (" + std::to_string(take_off) + ")";
	else
		place_holder = std::to_string(vel);

	im_render.text(screen_x / 2, y, c::fonts::indi_size, fonts::indicator_font_tahoma, place_holder.c_str(), true, c::movement::velocity_indicator_custom_color ? interpolate(velocity_clr_int, velocity_clr_custom, velocity_alpha) : velocity_color(last_delta), c::fonts::indi_font_flag[9], c::fonts::indi_font_flag[10]);

	if (fabs(tick_count - last_tick) > 5) {
		last_tick = tick_count;
		last_vel = vel;
	}
}

void features::movement::stamina_indicator() {
	if (!g::local || !g::local->is_alive() || !interfaces::engine->is_in_game()) {
		return;
	}

	if (!c::movement::stamina_indicator) {
		return;
	}

	int x, y;
	interfaces::engine->get_screen_size(x, y);

	if (menu::open) {
		static bool is_dragging = false;
		static ImVec2 drag_offset;

		auto mouse_pos = get_mouse_position();

		int width, height;
		interfaces::engine->get_screen_size(width, height);

		int indicator_position = height - c::movement::stamina_indicator_position;

		int middle_x = width / 2;
		bool is_pos = (mouse_pos.x >= middle_x - 35 && mouse_pos.x <= middle_x + 35) &&
			(mouse_pos.y >= indicator_position && mouse_pos.y <= indicator_position + 30);

		bool is_press = GetAsyncKeyState(VK_LBUTTON) & 0x8000;

		if (is_pos && is_press && !indicator_grabbed) {
			if (!is_dragging) {
				drag_offset = ImVec2(mouse_pos.x - middle_x, mouse_pos.y - indicator_position);
			}
			is_dragging = true;
			indicator_grabbed = true;
		}
		else if (!is_press) {
			is_dragging = false;
			indicator_grabbed = false;
		}

		if (is_dragging) {
			im_render.drawline(x / 2, y, x / 2, 0, color_t(255, 255, 255, 255));
			im_render.drawrect(x / 2 - 35, indicator_position, 70, 30, color_t(255, 255, 255, 255));

			c::movement::stamina_indicator_position = std::clamp(height - static_cast<int>(mouse_pos.y - drag_offset.y), 0, y);
		}
	}

	y -= c::movement::stamina_indicator_position;

	static float last_speed = 0, last_jump_speed = 0;
	static float last_vel_update = 0.0f;
	static bool last_onground = false;
	static auto take_off_time = 0.0f;
	bool current_onground = g::local->flags() & fl_onground;
	float stamina = g::local->stamina();
	float current_speed = stamina;

	if (last_onground && !current_onground) {
		last_jump_speed = current_speed;
		take_off_time = interfaces::globals->cur_time + (c::movement::stamina_indicator_disable_ong_show_pre ? 0.0f : 3.5f);
	}

	static float stamina_alpha = 0.f;
	stamina_alpha = 1.f * (float(stamina) / 25.f);

	color_t stamina_clr = current_speed < last_speed ? color_t(c::movement::stamina_indicator_clr[0], c::movement::stamina_indicator_clr[1], c::movement::stamina_indicator_clr[2], c::movement::stamina_indicator_clr[3]) : color_t(c::movement::stamina_indicator_clr[0], c::movement::stamina_indicator_clr[1], c::movement::stamina_indicator_clr[2], c::movement::stamina_indicator_clr[3]);
	color_t stamina_clr_int = current_speed < last_speed ? color_t(c::movement::stamina_fade_clr[0], c::movement::stamina_fade_clr[1], c::movement::stamina_fade_clr[2], c::movement::stamina_fade_clr[3]) : color_t(c::movement::stamina_fade_clr[0], c::movement::stamina_fade_clr[1], c::movement::stamina_fade_clr[2], c::movement::stamina_fade_clr[3]);

	if (menu::experimental_femboy_theme_active) {
		stamina_clr = color_t(255, 182, 193, 255);
		stamina_clr_int = color_t(255, 105, 180, 255);
	}
	else if (menu::experimental_fight_club_mode) {
		stamina_clr = color_t(150, 0, 0, 255);
		stamina_clr_int = color_t(80, 0, 0, 255);
	}

	std::string value_str;

	std::stringstream ss;
	ss << std::fixed << std::setprecision(current_speed > 0.0f ? 1 : 1) << current_speed;
	value_str = ss.str();

	std::string str = value_str;

	std::stringstream ss1;
	ss1 << std::fixed << std::setprecision(1) << last_jump_speed;
	std::string value_str2 = ss1.str();

	const auto should_draw_takeoff =
		(!current_onground || (take_off_time > interfaces::globals->cur_time)) && c::movement::stamina_indicator_show_pre;

	if (should_draw_takeoff)
		str += " (" + value_str2 + ")";
	else
		str = value_str;

	im_render.text(x / 2, y, c::fonts::indi_size, fonts::indicator_font, str.c_str(), true, c::movement::stamina_indicator_fade ? interpolate(stamina_clr_int, stamina_clr, stamina_alpha) : stamina_clr, c::fonts::indi_font_flag[9], c::fonts::indi_font_flag[10]);

	if (interfaces::globals->cur_time > last_vel_update) {
		last_speed = current_speed;
		last_vel_update = interfaces::globals->cur_time + 0.05f;
	}
	last_onground = current_onground;
}

#define F2B(f) ((f) >= 1.0 ? 255.f : (int)((f)*256.0))

void render_indicator(
	int key, 
	int key_s, 
	int& alpha, 
	int& secondary_alpha, 
	color_t& clr, 
	const char* text, 
	bool tick_count, 
	int should_detect, 
	bool allow_detection_clr, 
	int& position, 
	int saved_tick = 0
) {
	int x, y;
	interfaces::engine->get_screen_size(x, y);

	if (menu::open) {
		static bool is_dragging = false;
		static ImVec2 drag_offset;

		auto mouse_pos = get_mouse_position();

		int width, height;
		interfaces::engine->get_screen_size(width, height);

		int indicator_position = height - c::movement::indicators_position;

		int middle_x = width / 2;
		bool is_pos = (mouse_pos.x >= middle_x - 35 && mouse_pos.x <= middle_x + 35) &&
			(mouse_pos.y >= indicator_position && mouse_pos.y <= indicator_position + 30);

		bool is_press = GetAsyncKeyState(VK_LBUTTON) & 0x8000;

		if (is_pos && is_press && !indicator_grabbed) {
			if (!is_dragging) {
				drag_offset = ImVec2(mouse_pos.x - middle_x, mouse_pos.y - indicator_position);
			}
			is_dragging = true;
			indicator_grabbed = true;
		}
		else if (!is_press) {
			is_dragging = false;
			indicator_grabbed = false;
		}

		if (is_dragging) {
			im_render.drawline(x / 2, y, x / 2, 0, color_t(255, 255, 255, 255));
			im_render.drawrect(x / 2 - 35, indicator_position, 70, 30, color_t(255, 255, 255, 255));

			c::movement::indicators_position = std::clamp(height - static_cast<int>(mouse_pos.y - drag_offset.y), 0, y);
		}
	}

	y -= c::movement::indicators_position;

	color_t default_clr(c::movement::velocity_indicator_custom_clr2[0], c::movement::velocity_indicator_custom_clr2[1], c::movement::velocity_indicator_custom_clr2[2], c::movement::velocity_indicator_custom_clr2[3]);

	if (allow_detection_clr) {
		if (tick_count)
			clr = interfaces::globals->tick_count - saved_tick < c::movement::detection_saved_tick ? interpolate(default_clr, color_t(c::movement::indicator_detect_clr[0], c::movement::indicator_detect_clr[1], c::movement::indicator_detect_clr[2], c::movement::indicator_detect_clr[3]), 1.f) : interpolate(color_t(c::movement::indicator_detect_clr[0], c::movement::indicator_detect_clr[1], c::movement::indicator_detect_clr[2], c::movement::indicator_detect_clr[3]), default_clr, 1.f);
		else
			clr = should_detect ? interpolate(default_clr, color_t(c::movement::indicator_detect_clr[0], c::movement::indicator_detect_clr[1], c::movement::indicator_detect_clr[2], c::movement::indicator_detect_clr[3]), 1.f) : interpolate(color_t(c::movement::indicator_detect_clr[0], c::movement::indicator_detect_clr[1], c::movement::indicator_detect_clr[2], c::movement::indicator_detect_clr[3]), default_clr, 1.f);
	}
	else
		clr = default_clr;

	// ill fix later, wanna make the fade match the default color alpha + color so Yeahg

	// Existing fading speed
	const float fading_speed = 255.f * (35.f / 35.f) * interfaces::globals->frame_time * c::movement::indicators_fading_speed;

	// Handle detection-based secondary fade
	bool is_detected = tick_count
		? interfaces::globals->tick_count - saved_tick < c::movement::detection_saved_tick
		: should_detect;

	if (menu::checkkey(key, key_s)) {
		alpha = std::clamp(alpha + fading_speed, 0.f, 255.f);
	}
	else {
		alpha = std::clamp(alpha - fading_speed, 0.f, 255.f);
		if (tick_count)
			should_detect = false;
	}

	// [NEW] Fade in/out secondary alpha
	if (is_detected)
		secondary_alpha = std::clamp(secondary_alpha + fading_speed, 0.f, 255.f);
	else
		secondary_alpha = std::clamp(secondary_alpha - fading_speed, 0.f, 255.f);

	// Primary indicator rendering
	if (alpha >= 1) {
		if (c::movement::indicators_allow_animation && c::movement::indicators_allow_move) {
			int animation_position = position + (alpha / 255.0f) * c::movement::indicators_move_pos;
			im_render.text(x / 2, y + animation_position, c::fonts::sub_indi_size, fonts::sub_indicator_font, text, true, clr.get_color_edit_alpha_int(alpha), c::fonts::sub_indi_font_flag[9], c::fonts::sub_indi_font_flag[10]);
			position += (24 + c::movement::indicators_gap);
		}
		else {
			im_render.text(x / 2, y + position, c::fonts::sub_indi_size, fonts::sub_indicator_font, text, true, clr.get_color_edit_alpha_int(alpha), c::fonts::sub_indi_font_flag[9], c::fonts::sub_indi_font_flag[10]);
			position += (24 + c::movement::indicators_gap);
		}
	}
}

void render_indicator_jb(int key, int key_s, int& alpha, int& secondary_alpha, color_t& clr, const char* text, bool tick_count, int should_detect, bool allow_detection_clr, int& position, int saved_tick = 0) {
	int x, y;
	interfaces::engine->get_screen_size(x, y);
	y -= c::movement::indicators_position;

	color_t default_clr(c::movement::velocity_indicator_custom_clr2[0], c::movement::velocity_indicator_custom_clr2[1], c::movement::velocity_indicator_custom_clr2[2]);

	if (menu::experimental_femboy_theme_active) {
		default_clr = color_t(255, 182, 193, 255);
	}

	if (menu::experimental_femboy_theme_active) {
		default_clr = color_t(255, 182, 193, 255);
	}

	if (allow_detection_clr) {
		if (tick_count)
			clr = interfaces::globals->tick_count - saved_tick < c::movement::detection_saved_tick ? interpolate(color_t(220, 0, 0), default_clr, 1.f) : interpolate(default_clr, color_t(220, 0, 0), 1.f);
		else
			clr = should_detect ? interpolate(color_t(220, 0, 0), default_clr, 1.f) : interpolate(default_clr, color_t(220, 0, 0), 1.f);
	}
	else
		clr = default_clr;

	// Existing fading speed
	const float fading_speed = 255.f * (35.f / 35.f) * interfaces::globals->frame_time * c::movement::indicators_fading_speed;

	// Handle detection-based secondary fade
	bool is_detected = tick_count
		? interfaces::globals->tick_count - saved_tick < c::movement::detection_saved_tick
		: should_detect;

	if (menu::checkkey(key, key_s)) {
		alpha = std::clamp(alpha + fading_speed, 0.f, 255.f);
	}
	else {
		alpha = std::clamp(alpha - fading_speed, 0.f, 255.f);
		if (tick_count)
			should_detect = false;
	}

	// [NEW] Fade in/out secondary alpha
	if (is_detected)
		secondary_alpha = std::clamp(secondary_alpha + fading_speed, 0.f, 255.f);
	else
		secondary_alpha = std::clamp(secondary_alpha - fading_speed, 0.f, 255.f);

	// Primary indicator rendering
	if (alpha >= 1) {
		if (c::movement::indicators_allow_animation && c::movement::indicators_allow_move) {
			int animation_position = position + (alpha / 255.0f) * c::movement::indicators_move_pos;
			im_render.text(x / 2, y + animation_position, c::fonts::sub_indi_size, fonts::sub_indicator_font, text, true, clr.get_color_edit_alpha_int(alpha), c::fonts::sub_indi_font_flag[9], c::fonts::sub_indi_font_flag[10]);
			position += (24 + c::movement::indicators_gap);
		}
		else {
			im_render.text(x / 2, y + position, c::fonts::sub_indi_size, fonts::sub_indicator_font, text, true, clr.get_color_edit_alpha_int(alpha), c::fonts::sub_indi_font_flag[9], c::fonts::sub_indi_font_flag[10]);
			position += (24 + c::movement::indicators_gap);
		}
	}
}

void render_indicator_ps(int key, int key_s, int& alpha, int& secondary_alpha, color_t& clr, const char* text, bool tick_count, int should_detect, bool allow_detection_clr, int& position, int saved_tick = 0) {
	int x, y;
	interfaces::engine->get_screen_size(x, y);
	y -= c::movement::indicators_position;

	color_t default_clr(c::movement::velocity_indicator_custom_clr2[0],
		c::movement::velocity_indicator_custom_clr2[1],
		c::movement::velocity_indicator_custom_clr2[2]);

	if (menu::experimental_femboy_theme_active) {
		default_clr = color_t(255, 182, 193, 255);
	}

	// Color logic
	if (allow_detection_clr) {
		if (tick_count)
			clr = (interfaces::globals->tick_count - saved_tick < c::movement::detection_saved_tick)
			? interpolate(default_clr, color_t(c::movement::indicator_detect_clr[0], c::movement::indicator_detect_clr[1], c::movement::indicator_detect_clr[2], c::movement::indicator_detect_clr[3]), 1.f)
			: interpolate(color_t(c::movement::indicator_detect_clr[0], c::movement::indicator_detect_clr[1], c::movement::indicator_detect_clr[2], c::movement::indicator_detect_clr[3]), default_clr, 1.f);
		else
			clr = should_detect
			? interpolate(default_clr, color_t(c::movement::indicator_detect_clr[0], c::movement::indicator_detect_clr[1], c::movement::indicator_detect_clr[2], c::movement::indicator_detect_clr[3]), 1.f)
			: interpolate(color_t(c::movement::indicator_detect_clr[0], c::movement::indicator_detect_clr[1], c::movement::indicator_detect_clr[2], c::movement::indicator_detect_clr[3]), default_clr, 1.f);
	}
	else {
		clr = default_clr;
	}

	const float fading_speed = 255.f * (35.f / 35.f) * interfaces::globals->frame_time * c::movement::indicators_fading_speed;

	// Detection status
	bool is_detected = tick_count
		? interfaces::globals->tick_count - saved_tick < c::movement::detection_saved_tick
		: should_detect;

	// Fade logic for primary
	const bool should_fade_in = menu::checkkey(key, key_s) && features::movement::should_ps;

	if (should_fade_in) {
		alpha = std::clamp(alpha + fading_speed, 0.f, 255.f);
	}
	else {
		alpha = std::clamp(alpha - fading_speed, 0.f, 255.f);
		if (tick_count)
			should_detect = false;
	}

	// [NEW] Fade secondary text
	if (is_detected)
		secondary_alpha = std::clamp(secondary_alpha + fading_speed, 0.f, 255.f);
	else
		secondary_alpha = std::clamp(secondary_alpha - fading_speed, 0.f, 255.f);

	// Main indicator draw
	if (alpha >= 1) {
		int draw_y = y + position;

		if (c::movement::indicators_allow_animation && c::movement::indicators_allow_move)
			draw_y = y + position + (alpha / 255.0f) * c::movement::indicators_gap;

		im_render.text(x / 2, draw_y, c::fonts::sub_indi_size, fonts::sub_indicator_font, text, true,
			clr.get_color_edit_alpha_int(alpha), c::fonts::sub_indi_font_flag[9], c::fonts::sub_indi_font_flag[10]);
		position += (24 + c::movement::indicators_gap);
	}
}

void render_indicator_glow(
	int key, int key_s,
	int& alpha, int& secondary_alpha,
	const char* text,
	bool tick_count, int should_detect,
	int& x_offset,
	int saved_tick = 0,
	int total_width = 0
) {
	int screen_x, screen_y;
	interfaces::engine->get_screen_size(screen_x, screen_y);
	screen_y -= c::movement::indicators_position;

	// Per-indicator state storage
	struct IndicatorState {
		float fade_alpha = 0.0f;
		float detection_alpha = 0.0f;
		bool was_detected = false;
		float last_detection_time = -999.0f;
		float ghost_alpha = 0.0f;
		float ghost_offset = 0.0f;
		float ghost_start_time = -999.0f;
		float spacing_multiplier = 0.0f;
	};

	static std::map<std::string, IndicatorState> states;
	static bool was_in_game = false;

	/*bool in_game_now =
		interfaces::engine->is_connected() &&
		interfaces::engine->is_in_game() &&
		g::local &&
		g::local->is_alive();

	if (in_game_now && !was_in_game) {
		states.clear();
	}

	if (!in_game_now) {
		was_in_game = false;
		return;
	}

	was_in_game = true;*/

	std::string key_str = std::string(text);
	IndicatorState& state = states[key_str];

	float dt = interfaces::globals->frame_time;
	float cur_time = interfaces::globals->cur_time;

	// ==================== INPUT STATE ====================
	bool key_down = menu::checkkey(key, key_s);

	bool is_detected = false;
	if (key_down) {
		if (tick_count) {
			is_detected = (interfaces::globals->tick_count - saved_tick) < c::movement::detection_saved_tick;
		}
		else {
			is_detected = should_detect;
		}
	}

	// ==================== MAIN FADE ANIMATION ====================
	float fade_speed = 2.0f * c::movement::indicators_fading_speed;
	if (key_down) {
		state.fade_alpha = std::lerp(state.fade_alpha, 1.0f, dt * fade_speed);
	}
	else {
		float release_speed = fade_speed * 4.f;
		state.fade_alpha = std::lerp(state.fade_alpha, 0.0f, dt * release_speed);
	}

	// ==================== DETECTION FADE ====================
	float target_detection = is_detected ? 1.0f : 0.0f;
	state.detection_alpha = std::lerp(state.detection_alpha, target_detection, dt * fade_speed * 1.2f);
	state.detection_alpha = std::clamp(state.detection_alpha, 0.0f, 1.0f);

	// ==================== GHOST ANIMATION ====================
	const float ghost_cooldown = 0.3f;
	bool should_trigger_ghost = is_detected && !state.was_detected &&
		(cur_time - state.last_detection_time) > ghost_cooldown;

	if (should_trigger_ghost) {
		state.ghost_start_time = cur_time;
		state.ghost_alpha = 1.0f;
		state.ghost_offset = 0.0f;
		state.last_detection_time = cur_time;
	}

	const float ghost_duration = 0.3f;
	const float ghost_max_rise = 10.0f;

	if (state.ghost_start_time > -999.0f) {
		float ghost_elapsed = cur_time - state.ghost_start_time;
		float ghost_progress = std::clamp(ghost_elapsed / ghost_duration, 0.0f, 1.0f);

		float eased_progress = 1.0f - (1.0f - ghost_progress) * (1.0f - ghost_progress);
		float target_offset = eased_progress * ghost_max_rise;

		float target_alpha = 0.0f;
		if (ghost_progress < 0.5f) {
			target_alpha = 1.0f;
		}
		else {
			target_alpha = 1.0f - ((ghost_progress - 0.5f) / 0.5f);
		}

		state.ghost_offset = std::lerp(state.ghost_offset, target_offset, 0.3f);
		state.ghost_alpha = std::lerp(state.ghost_alpha, target_alpha, 0.3f);

		if (ghost_progress >= 1.0f && state.ghost_alpha < 0.01f) {
			state.ghost_start_time = -999.0f;
			state.ghost_alpha = 0.0f;
			state.ghost_offset = 0.0f;
		}
	}

	// ==================== SPACING ANIMATION ====================
	int text_width = im_render.measure_text(text, fonts::sub_indicator_font, c::fonts::sub_indi_size).x;
	float target_spacing = key_down ? 1.0f : 0.0f;
	float spacing_speed = key_down ? fade_speed * 2.0f : fade_speed * 12.0f;
	state.spacing_multiplier = std::lerp(state.spacing_multiplier, target_spacing, dt * spacing_speed);
	state.spacing_multiplier = std::clamp(state.spacing_multiplier, 0.0f, 1.0f);

	// ==================== RENDERING ====================
	// Calculate the actual current width being used by visible indicators
	int draw_x = (screen_x / 2) - (total_width / 2) + x_offset + 4;
	int draw_y = screen_y;

	float final_alpha = state.fade_alpha * 255.0f;

	color_t accent_color(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], final_alpha);
	color_t white_color(255.f, 255.f, 255.f, final_alpha);
	color_t main_text_color = is_detected ? white_color : accent_color;

	// Draw ghost (rising when detected)
	if (state.ghost_alpha > 0.01f) {
		int ghost_y = draw_y - (int)state.ghost_offset;
		float ghost_alpha = state.ghost_alpha * state.fade_alpha * 255.0f;
		color_t ghost_color(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], ghost_alpha);

		im_render.text(
			draw_x,
			ghost_y,
			c::fonts::sub_indi_size,
			fonts::sub_indicator_font,
			text,
			false,
			ghost_color,
			false,
			c::fonts::sub_indi_font_flag[10]
		);
	}

	// Shadow
	im_render.text(
		draw_x - 1,
		draw_y - 1,
		c::fonts::sub_indi_size,
		fonts::sub_indicator_font,
		text,
		false,
		is_detected ? white_color : accent_color,
		c::fonts::sub_indi_font_flag[10]
	);

	// Glow effect when detected
	if (is_detected && state.detection_alpha > 0.01f) {
		im_render.text_glow(
			draw_x - 1,
			draw_y - 1,
			c::fonts::sub_indi_size,
			fonts::sub_indicator_font,
			text,
			false,
			white_color,
			c::fonts::sub_indi_font_flag[10]
		);
	}

	// Main text (double render for effect)
	im_render.text(
		draw_x - 1,
		draw_y - 1,
		c::fonts::sub_indi_size,
		fonts::sub_indicator_font,
		text,
		false,
		main_text_color,
		false,
		c::fonts::sub_indi_font_flag[10]
	);

	im_render.text(
		draw_x,
		draw_y,
		c::fonts::sub_indi_size,
		fonts::sub_indicator_font,
		text,
		false,
		white_color,
		false,
		c::fonts::sub_indi_font_flag[10]
	);

	// ==================== UPDATE OFFSET ====================
	int spacing_gap = (int)((text_width + c::movement::indicators_gap) * state.spacing_multiplier);
	x_offset += spacing_gap;

	// ==================== SAVE STATE ====================
	state.was_detected = is_detected;

	// Update output alpha values
	alpha = (int)final_alpha;
	secondary_alpha = (int)(state.detection_alpha * 255.0f);
}

void features::movement::indicators() {
	static bool was_in_game = false;

	bool in_game_now =
		interfaces::engine->is_connected() &&
		interfaces::engine->is_in_game() &&
		g::local &&
		g::local->is_alive();

	// ==============================
	// GAME TRANSITION RESET
	// ==============================
	static float lerped_total_width = 0.0f;

	static int p_alpha = 0;
	static int al_alpha = 0;
	static int sh_alpha = 0;
	static int eb_alpha = 0;
	static int jb_alpha = 0;
	static int ej_alpha = 0;
	static int lj_alpha = 0;
	static int mj_alpha = 0;
	static int lb_alpha = 0;
	static int fm_alpha = 0;
	static int ast_alpha = 0;
	static int bst_alpha = 0;
	static int air_alpha = 0;

	if (in_game_now && !was_in_game) {
		// Reset saved ticks
		saved_tick_jb = -1;
		saved_tick_lj = -1;
		saved_tick_ej = -1;
		saved_tick_mj = -1;
		saved_tick_fm = -1;
		saved_tick_ast = -1;
		saved_tick_bst = -1;
		saved_tick_air = -1;

		// Reset layout & animation state
		lerped_total_width = 0.0f;

		p_alpha = al_alpha = sh_alpha = eb_alpha = jb_alpha = 0;
		ej_alpha = lj_alpha = mj_alpha = lb_alpha = 0;
		fm_alpha = ast_alpha = bst_alpha = air_alpha = 0;
	}

	if (!in_game_now) {
		was_in_game = false;
		lerped_total_width = 0.0f;
		return;
	}

	was_in_game = true;

	if (!c::movehelp::indicatorEnable)
		return;

	// ==============================
	// UPDATE SAVED TICKS
	// ==============================
	if (detected_normal_jump_bug) saved_tick_jb = interfaces::globals->tick_count;
	if (should_ej)  saved_tick_ej = interfaces::globals->tick_count;
	if (should_lj)  saved_tick_lj = interfaces::globals->tick_count;
	if (should_mj)  saved_tick_mj = interfaces::globals->tick_count;
	if (should_fireman) saved_tick_fm = interfaces::globals->tick_count;
	if (should_ast) saved_tick_ast = interfaces::globals->tick_count;
	if (should_bst) saved_tick_bst = interfaces::globals->tick_count;
	if (should_air) saved_tick_air = interfaces::globals->tick_count;

	// ==============================
	// WIDTH CALCULATION
	// ==============================
	float dt = interfaces::globals->frame_time;
	int target_total_width = 0;

	auto add_width = [&](const char* text) {
		target_total_width +=
			im_render.measure_text(text, fonts::sub_indicator_font, c::fonts::sub_indi_size).x +
			c::movement::indicators_gap;
		};

	if (c::movehelp::pixelsurf_indicator && menu::checkkey(c::movement::pixel_surf_key, c::movement::pixel_surf_key_s)) add_width("px");
	if (c::movehelp::edgebug_indicator && menu::checkkey(c::movement::edge_bug_key, c::movement::edge_bug_key_s)) add_width("eb");
	if (c::movehelp::edgejump_indicator && menu::checkkey(c::movement::edgejump_key, c::movement::edgejump_key_s)) add_width("ej");
	if (c::movehelp::longjump_indicator && menu::checkkey(c::movement::long_jump_key, c::movement::long_jump_key_s)) add_width("lj");
	if (c::movehelp::minijump_indicator && menu::checkkey(c::movement::mini_jump_key, c::movement::mini_jump_key_s)) add_width("mj");
	if (c::movehelp::jumpbug_indicator && menu::checkkey(c::movement::jump_bug_key, c::movement::jump_bug_key_s)) add_width("jb");
	if (c::movehelp::fireman_indicator && menu::checkkey(c::movement::fireman_key, c::movement::fireman_key_s)) add_width("fm");
	if (c::movehelp::air_indicator && menu::checkkey(c::movement::airstuck_key, c::movement::airstuck_key_s)) add_width("air");
	if (c::movehelp::ast_indicator && menu::checkkey(c::assist::pixelsurf_assist_key, c::assist::pixelsurf_assist_key_s)) add_width("ast");

	float width_lerp_speed = 15.0f * c::movement::indicators_fading_speed;
	lerped_total_width = std::lerp(lerped_total_width, (float)target_total_width, dt * width_lerp_speed);

	int total_width = (int)lerped_total_width;
	int position = 0;

	// ==============================
	// RENDER
	// ==============================
	if (c::movehelp::pixelsurf_indicator)
		render_indicator_glow(c::movement::pixel_surf_key, c::movement::pixel_surf_key_s,
			p_alpha, p_alpha, "px", false,
			should_ps && !should_edge_bug || should_ps && !should_air,
			position, 0, total_width);

	if (c::movehelp::edgebug_indicator)
		render_indicator_glow(c::movement::edge_bug_key, c::movement::edge_bug_key_s,
			eb_alpha, eb_alpha, "eb", false,
			should_edge_bug,
			position, 0, total_width);

	if (c::movehelp::edgejump_indicator)
		render_indicator_glow(c::movement::edgejump_key, c::movement::edgejump_key_s,
			ej_alpha, ej_alpha, "ej", true,
			should_ej,
			position, saved_tick_ej, total_width);

	if (c::movehelp::longjump_indicator)
		render_indicator_glow(c::movement::long_jump_key, c::movement::long_jump_key_s,
			lj_alpha, lj_alpha, "lj", true,
			should_lj,
			position, saved_tick_lj, total_width);

	if (c::movehelp::minijump_indicator)
		render_indicator_glow(c::movement::mini_jump_key, c::movement::mini_jump_key_s,
			mj_alpha, mj_alpha, "mj", true,
			should_mj,
			position, saved_tick_mj, total_width);

	if (c::movehelp::jumpbug_indicator)
		render_indicator_glow(c::movement::jump_bug_key, c::movement::jump_bug_key_s,
			jb_alpha, jb_alpha, "jb", true,
			detected_normal_jump_bug || fake_jb,
			position, saved_tick_jb, total_width);

	if (c::movehelp::fireman_indicator)
		render_indicator_glow(c::movement::fireman_key, c::movement::fireman_key_s,
			fm_alpha, fm_alpha, "fm", true,
			should_fireman,
			position, saved_tick_fm, total_width);

	if (c::movehelp::air_indicator)
		render_indicator_glow(c::movement::airstuck_key, c::movement::airstuck_key_s,
			air_alpha, air_alpha, "air", true,
			should_air,
			position, saved_tick_air, total_width);

	if (c::movehelp::ast_indicator)
		render_indicator_glow(c::assist::pixelsurf_assist_key, c::assist::pixelsurf_assist_key_s,
			ast_alpha, ast_alpha, "ast", true,
			should_ast,
			position, saved_tick_ast, total_width);
}

features::movement::velocity_data_t current_vel_data;
static std::deque<std::pair<float, bool>> stamina_data;
void features::movement::gather_vel_graph_data() {
	if (!g::local || !g::local->is_alive() || !c::movement::velocity_graph)
		return;

	if (!interfaces::engine->is_connected())
		return;

	float speed = g::local->velocity().length_2d();
	int flags = g::local->flags();
	bool on_ground = flags & fl_onground;

	current_vel_data.speed = speed;
	current_vel_data.on_ground = on_ground;
	current_vel_data.edge_bug = detected_normal_edge_bug;
	current_vel_data.jump_bug = detected_normal_jump_bug;
	current_vel_data.pixel_surf = detected_normal_pixel_surf;

	velocity_data.insert(velocity_data.begin(), current_vel_data);

	if (velocity_data.size() > 185)
		velocity_data.pop_back();
}

void features::movement::gather_stam_graph_data() {
	if (!g::local || !g::local->is_alive() || !c::movement::stamina_graph)
		return;

	if (!interfaces::engine->is_connected())
		return;

	stamina_data.push_front({ g::local->stamina(), !(g::local->flags() & fl_onground) });

	if (stamina_data.size() > 185)
		stamina_data.pop_back();
}

void features::movement::velocity_graph_draw() {
	if (!g::local || !g::local->is_alive())
		return;

	if (!interfaces::engine->is_in_game() || !interfaces::engine->is_connected()) {
		velocity_data.clear();
		return;
	}

	if (c::movement::velocity_graph) {
		int x, y;
		interfaces::engine->get_screen_size(x, y);

		if (velocity_data.size() == 185) {
			for (auto i = 0; i < velocity_data.size() - 2; i++) {
				auto& curdata = velocity_data[i];//
				auto& nextdata = velocity_data[i + 1];
				auto cur = curdata.speed;
				auto next = nextdata.speed;
				auto landed = !curdata.on_ground && nextdata.on_ground;
				auto eb = curdata.edge_bug;
				auto jb = curdata.jump_bug;
				auto ps = curdata.pixel_surf;
				int alpha = 255;

				switch (c::movement::graph_fade) {
				case 0:
					//no faded
					break;
				case 1:
					//faded start and end
					if (i < 70) {
						float a = ((i + 1) / 70.f) * 255.f;
						alpha = (int)a;
					}
					else if (i > velocity_data.size() - 71) {
						int todivide = velocity_data.size() - i;
						float a = (todivide / 70.f) * 255.f;
						alpha = (int)a;
					}
					break;
				case 2:
					//faded by vel
					if (i < 65) {
						float a = ((i + 1) / 65.f) * 255.f;
						alpha = (int)a;
					}
					else if (i > velocity_data.size() - 66) {
						int todivide = velocity_data.size() - i;
						float a = (todivide / 65.f) * 255.f;
						alpha = (int)a;
					}

					alpha *= std::clamp(((cur + next) / 2) / c::movement::velocity_tofade, 0.f, 1.f);
					break;
				case 3:
					//faded end
					if (i > velocity_data.size() - 66) {
						int todivide = velocity_data.size() - i;
						float a = (todivide / 65.f) * 255.f;
						alpha = (int)a;
					}
					break;
				case 4:
					//faded start
					if (i < 65) {
						float a = ((i + 1) / 65.f) * 255.f;
						alpha = (int)a;
					}
					break;
				}

				alpha = std::clamp(alpha, 0, 255);

				color_t linecolor = color_t(c::movement::velocity_graph_color[0], c::movement::velocity_graph_color[1], c::movement::velocity_graph_color[2], (alpha / 255.f));

				im_render.drawline(
					((x / 2) + (((velocity_data.size() / 2) * c::movement::velocity_linewidth))) - ((i - 1) * c::movement::velocity_linewidth),
					y * c::movement::graph_xscreenpos - 15 - ((std::clamp(cur, 0.f, 450.f) * 75 / 320) * c::movement::velocity_height),
					x / 2 + (((velocity_data.size() / 2) * c::movement::velocity_linewidth)) - i * c::movement::velocity_linewidth,
					y * c::movement::graph_xscreenpos - 15 - ((std::clamp(next, 0.f, 450.f) * 75 / 320) * c::movement::velocity_height),
					linecolor, 0);

				if (landed && c::movement::velocity_graph_show_landed_speed && !eb)
					im_render.text(((x / 2) + (((velocity_data.size() / 2) * c::movement::velocity_linewidth))) - ((i + 1) * c::movement::velocity_linewidth), y * c::movement::graph_xscreenpos - 30 - ((std::clamp(next, 0.f, 450.f) * 75 / 320) * c::movement::velocity_height), 12, fonts::esp_misc, std::to_string((int)round(next)).c_str(), true, color_t(1.f, 1.f, 1.f, (alpha / 255.f)), true);

				if (eb && c::movement::velocity_graph_show_edge_bug)
					im_render.text(((x / 2) + (((velocity_data.size() / 2) * c::movement::velocity_linewidth))) - ((i + 1) * c::movement::velocity_linewidth), y * c::movement::graph_xscreenpos - 10 - ((std::clamp(next, 0.f, 450.f) * 75 / 320) * c::movement::velocity_height), 12, fonts::esp_misc, "eb", true, color_t(1.f, 1.f, 1.f, (alpha / 255.f)), true);

				if (jb && c::movement::velocity_graph_show_jump_bug)
					im_render.text(((x / 2) + (((velocity_data.size() / 2) * c::movement::velocity_linewidth))) - ((i + 1) * c::movement::velocity_linewidth), y * c::movement::graph_xscreenpos - 10 - ((std::clamp(next, 0.f, 450.f) * 75 / 320) * c::movement::velocity_height), 12, fonts::esp_misc, "jb", true, color_t(1.f, 1.f, 1.f, (alpha / 255.f)), true);

				if (ps && c::movement::velocity_graph_show_pixel_surf)
					im_render.text(((x / 2) + (((velocity_data.size() / 2) * c::movement::velocity_linewidth))) - ((i + 1) * c::movement::velocity_linewidth), y * c::movement::graph_xscreenpos - 10 - ((std::clamp(next, 0.f, 450.f) * 75 / 320) * c::movement::velocity_height), 12, fonts::esp_misc, "ps", true, color_t(1.f, 1.f, 1.f, (alpha / 255.f)), true);
			}
		}
	}
}

void features::movement::stamina_graph_draw() {
	if (!g::local || !g::local->is_alive())
		return;

	if (!interfaces::engine->is_in_game() || !interfaces::engine->is_connected()) {
		stamina_data.clear();
		return;
	}

	if (c::movement::stamina_graph) {
		int x, y;
		interfaces::engine->get_screen_size(x, y);

		if (stamina_data.size() == 185) {
			for (auto i = 0; i < stamina_data.size() - 2; i++) {
				auto& curdata = stamina_data[i];//
				auto& nextdata = stamina_data[i + 1];
				auto cur = curdata.first;
				auto next = nextdata.first;
				auto landed = curdata.second && !nextdata.second;
				int alpha = 255;

				switch (c::movement::graph_fade) {
				case 0:
					//no faded
					break;
				case 1:
					//faded start and end
					if (i < 65) {
						float a = ((i + 1) / 65.f) * 255.f;
						alpha = (int)a;
					}
					else if (i > stamina_data.size() - 66) {
						int todivide = stamina_data.size() - i;
						float a = (todivide / 65.f) * 255.f;
						alpha = (int)a;
					}
					break;
				case 2:
					//faded by vel
					if (i < 65) {
						float a = ((i + 1) / 65.f) * 255.f;
						alpha = (int)a;
					}
					else if (i > stamina_data.size() - 66) {
						int todivide = stamina_data.size() - i;
						float a = (todivide / 65.f) * 255.f;
						alpha = (int)a;
					}

					alpha *= std::clamp(((cur + next) / 2) / 1.f, 0.f, 1.f);//ee
					break;
				case 3:
					//faded end
					if (i > velocity_data.size() - 66) {
						int todivide = velocity_data.size() - i;
						float a = (todivide / 65.f) * 255.f;
						alpha = (int)a;
					}
					break;
				case 4:
					//faded start
					if (i < 65) {
						float a = ((i + 1) / 65.f) * 255.f;
						alpha = (int)a;
					}
					break;
				}

				alpha = std::clamp(alpha, 0, 255);

				color_t linecolor = color_t(c::movement::stamina_graph_color[0], c::movement::stamina_graph_color[1], c::movement::stamina_graph_color[2], (alpha / 255.f));

				im_render.drawline(
					((x / 2) + (((velocity_data.size() / 2) * c::movement::velocity_linewidth))) - ((i - 1) * c::movement::velocity_linewidth),
					y * c::movement::graph_xscreenpos - 15 - ((std::clamp(cur, 0.f, 450.f) * 75 / 320) * c::movement::velocity_height),
					x / 2 + (((velocity_data.size() / 2) * c::movement::velocity_linewidth)) - i * c::movement::velocity_linewidth,
					y * c::movement::graph_xscreenpos - 15 - ((std::clamp(next, 0.f, 450.f) * 75 / 320) * c::movement::velocity_height),
					linecolor, 0);

			}
		}
	}
}

static auto world_circle = [](vec3_t location, float radius, color_t col = color_t(255, 255, 255, 255), bool filled = false) {
	static constexpr float Step = m_pi * 2.0f / 60;
	std::vector<ImVec2> points;
	for (float lat = 0.f; lat <= m_pi * 2.0f; lat += Step) {
		const auto& point3d = vec3_t(sin(lat), cos(lat), 0.f) * radius;
		vec3_t point2d;

		if (interfaces::debug_overlay->world_to_screen(location + point3d, point2d))
			points.push_back(ImVec2(point2d.x, point2d.y));
	}

	im_render.drawpolyline(points, col, true, 2.f);
	};