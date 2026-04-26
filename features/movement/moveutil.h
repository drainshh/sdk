#include "movement.hpp"
#include <json/json.h>
#include <filesystem>
#include <vector>
#include <deque>
#include <fstream>
#include <mutex>
#include <nlohmann/json.hpp> // JSON library
#include "../misc/misc.hpp"
#include "pixelsurf_calculator.hpp"
#include "pixelsurf_finder.hpp"
#include "../features/panorama/scaleform.hpp"
using json = nlohmann::json;
inline std::string jump_to_marker;

inline static bool do_fucking_strafe = false;
inline bool alert_mini_crouch_hop = false;
inline bool bounce_alert_mini_crouch_hop = false;
inline bool seccheckcros = false;
inline bool float_ground = false;
inline bool bounce_set_point = false;
inline bool seccheckcros2 = false;

inline static int saved_tick_count;
inline static int cmd_i = 0;
inline int gloabal_settings_timer = 0;
inline int index_continue = -1;

inline vec3_t aim_1{ };
inline vec3_t back_ang = { };
inline vec3_t save_delta{ };
inline vec3_t save_cur{ };

static constexpr float NORMAL_FADEIN_DURATION = 0.25f;
static constexpr float QUICK_FADEIN_DURATION = 0.15f;
static constexpr float VISIBLE_DURATION = 2.5f;
static constexpr float NORMAL_FADEOUT_DURATION = 0.2f;
static constexpr float QUICK_FADEOUT_DURATION = 0.12f;
static constexpr float WINDOW_SPACING = 5.0f / 1080.0f;

class WindowDetect
{
public:
	enum WindowState {
		FADE_IN,
		VISIBLE,
		FADE_OUT
	};

	struct AnimatedWindow {
		int type;
		std::string info;
		float elapsed;
		WindowState state;
		float fadeInDuration;
		float visibleDuration;
		float fadeOutDuration;
		float totalDuration;
		float alpha;

		ImVec2 targetPosition;
		ImVec2 currentPosition;

		float accentProgress;
		float accentWidth;
		bool startFadeOut;
	};

	void AddWindow(int type, const std::string& info)
	{
		std::lock_guard<std::mutex> lock(window_mutex);
		const float baseYPosition = 0.8f;

		constexpr int MAX_VISIBLE_WINDOWS = 5;


		ImVec2 infoTextSize = fonts::esp_name_font->CalcTextSizeA(12.f, FLT_MAX, NULL, info.c_str());
		float heightPadding = 22.0f;
		float windowHeight = (infoTextSize.y + heightPadding) / 1080.0f;

		AnimatedWindow newWin;
		newWin.type = type;
		newWin.info = info;
		newWin.elapsed = 0.f;
		newWin.state = FADE_IN;
		newWin.fadeInDuration = NORMAL_FADEIN_DURATION;
		newWin.visibleDuration = VISIBLE_DURATION;
		newWin.fadeOutDuration = NORMAL_FADEOUT_DURATION;
		newWin.totalDuration = NORMAL_FADEIN_DURATION + VISIBLE_DURATION + NORMAL_FADEOUT_DURATION;
		newWin.alpha = 0.f;
		newWin.accentProgress = 0.f;
		newWin.accentWidth = 0.f;
		newWin.startFadeOut = false;

		int visibleCount = 0;
		for (auto& win : windows) {
			if (win.state != FADE_OUT) {
				visibleCount++;
			}
		}

		for (auto& win : windows) {
			if (win.state != FADE_OUT) {
				win.targetPosition.y -= (windowHeight + WINDOW_SPACING);
				win.currentPosition.y -= (windowHeight + WINDOW_SPACING);
				if (visibleCount >= MAX_VISIBLE_WINDOWS &&
					win.targetPosition.y < baseYPosition - (MAX_VISIBLE_WINDOWS - 1) * (windowHeight + WINDOW_SPACING)) {
					win.state = FADE_OUT;
					win.elapsed = 0.f;
					win.fadeOutDuration = NORMAL_FADEOUT_DURATION;
				}
			}
		}
		newWin.targetPosition = ImVec2(0.5f, baseYPosition);
		newWin.currentPosition = ImVec2(0.5f, baseYPosition);
		windows.push_back(newWin);
	}
	float Lerp(float a, float b, float t)
	{
		return a + t * (b - a);
	}
	float EaseInOutCubic(float t)
	{
		return t < 0.5f ? 4.0f * t * t * t : 1.0f - pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
	}
	void Update(float deltaTime)
	{
		std::lock_guard<std::mutex> lock(window_mutex);
		CleanupOldWindows();
		std::sort(windows.begin(), windows.end(),
			[](const AnimatedWindow& a, const AnimatedWindow& b) { return a.targetPosition.y > b.targetPosition.y; });

		for (auto it = windows.begin(); it != windows.end(); ) {
			it->elapsed += deltaTime;

			float totalElapsed = 0.0f;
			if (it->state == FADE_IN) {
				totalElapsed = it->elapsed;
			}
			else if (it->state == VISIBLE) {
				totalElapsed = it->fadeInDuration + it->elapsed;
			}
			else if (it->state == FADE_OUT) {
				totalElapsed = it->fadeInDuration + it->visibleDuration + it->elapsed;
			}
			it->accentProgress = totalElapsed / it->totalDuration;
			if (it->accentProgress > 1.0f)
				it->accentProgress = 1.0f;

			switch (it->state) {
			case FADE_IN:

				it->alpha = it->elapsed / it->fadeInDuration;
				if (it->alpha > 1.0f)
					it->alpha = 1.0f;

				if (it->elapsed >= it->fadeInDuration) {
					it->state = VISIBLE;
					it->elapsed = 0.f;
					it->alpha = 1.f;
				}
				break;
			case VISIBLE:
				it->alpha = 1.f;

				if (it->elapsed >= it->visibleDuration && !it->startFadeOut) {
					it->startFadeOut = true;
					it->state = FADE_OUT;
					it->elapsed = 0.f;
				}
				break;
			case FADE_OUT:

				it->alpha = 1.0f - (it->elapsed / it->fadeOutDuration);
				if (it->alpha < 0.0f)
					it->alpha = 0.0f;

				if (it->elapsed >= it->fadeOutDuration) {

					it = windows.erase(it);
					continue;
				}
				break;
			}
			++it;
		}
	}

	void Draw(ImDrawList* draw, const ImVec2& displaySize)
	{
		std::lock_guard<std::mutex> lock(window_mutex);
		float uiScale = displaySize.y / 1080.0f;   // scale relative to 1080p
		uiScale = std::clamp<float>(uiScale, 0.7f, 2.5f);    // optional safety clamp

		float fontSize = 12.f * uiScale;
		float widthPadding = 18.0f * uiScale;
		float heightPadding = 18.0f * uiScale;
		float cornerRadius = 4.0f * uiScale;
		float accentHeight = 1.0f * uiScale;

		for (const auto& win : windows)
		{
			if (win.state == FADE_OUT && win.alpha <= 0.0f)
				continue;

			ImVec2 infoTextSize = fonts::points_font->CalcTextSizeA(
				fontSize, FLT_MAX, NULL, win.info.c_str()
			);

			ImVec2 size(
				infoTextSize.x + widthPadding,
				infoTextSize.y + heightPadding
			);

			ImVec2 pos(
				displaySize.x * win.currentPosition.x - size.x * 0.5f,
				displaySize.y * win.currentPosition.y - size.y * 0.5f
			);

			int a = int(win.alpha * 255);
			ImU32 bgColor = IM_COL32(14, 14, 14, a);
			ImU32 textColor = IM_COL32(255, 255, 255, a);

			if (draw) {
				draw->AddRectFilled(
					pos,
					ImVec2(pos.x + size.x, pos.y + size.y),
					bgColor,
					cornerRadius
				);
			}

			// Accent bar
			ImVec4 accentColor(
				menu::menu_accent[0],
				menu::menu_accent[1],
				menu::menu_accent[2],
				win.alpha
			);

			float maxWidth = size.x - 4.f * uiScale;
			float margin = 2.f * uiScale;

			float currentWidth = maxWidth * (1.0f - EaseInOutCubic(win.accentProgress));
			currentWidth = std::max<float>(currentWidth, 0.0f);

			if (currentWidth > 0)
			{
				float centerX = pos.x + size.x * 0.5f;
				float halfWidth = currentWidth * 0.5f;
				float lineY = pos.y;

				if (draw) {
					draw->AddRectFilled(
						ImVec2(centerX - halfWidth, lineY),
						ImVec2(centerX + halfWidth, lineY + accentHeight),
						ImGui::GetColorU32(accentColor)
					);
				}
			}

			// Text centered
			ImVec2 textPos(
				pos.x + (size.x - infoTextSize.x) * 0.5f,
				pos.y + (size.y - infoTextSize.y) * 0.5f
			);

			if (draw) {
				draw->AddText(
					fonts::points_font,
					fontSize,
					ImVec2(textPos.x + 1, textPos.y + 1),
					IM_COL32(0, 0, 0, a / 4),
					win.info.c_str()
				);

				draw->AddText(
					fonts::points_font,
					fontSize,
					textPos,
					textColor,
					win.info.c_str()
				);
			}
		}
	}


private:
	std::mutex window_mutex;
	std::vector< AnimatedWindow > windows; // Vector of animated windows

	// Method for programmatically removing old windows when there are too many
	void CleanupOldWindows()
	{
		constexpr int MAX_WINDOWS = 10; // Maximum total number of windows in collection

		// If there are more windows than the maximum
		if (windows.size() > MAX_WINDOWS) {
			// Find oldest windows (highest ones) and transition them to fade-out mode
			int windowsToRemove = windows.size() - MAX_WINDOWS;

			// Sort by Y position (topmost/oldest first)
			std::sort(windows.begin(), windows.end(),
				[](const AnimatedWindow& a, const AnimatedWindow& b) { return a.targetPosition.y < b.targetPosition.y; });

			// Transition oldest windows to fade-out mode
			for (int i = 0; i < windowsToRemove && i < windows.size(); i++) {
				if (windows[i].state != FADE_OUT) {
					windows[i].state = FADE_OUT;
					windows[i].elapsed = 0.f;
					windows[i].fadeOutDuration = QUICK_FADEOUT_DURATION; // Use quick fadeout for cleanup
				}
			}
		}
	}
};
inline WindowDetect myWindowDetect;

struct NewCmd3 {
	vec3_t viewangle;
	float forwardmove;
	float sideMove;
	int buttons;
	vec3_t origin;
};
inline NewCmd3 NewCmd_assist[128];

class UtilityAssist {
private:

	struct auto_duck_data_t {
		int duck_ticks = 0; // countdown of how many ticks to hold duck
	} auto_duck_data;

	vec3_t origin;
	float pressdelay = 0.f;
	int currentcheckpoint = 0;
	std::deque<std::pair<vec3_t, vec3_t>> checkpoints;

	void handle_practice(c_usercmd* cmd) {
		auto local_player = reinterpret_cast<player_t*>(interfaces::ent_list->get_client_entity(interfaces::engine->get_local_player()));
		if (!g::local || !interfaces::engine->is_connected() || !interfaces::engine->is_in_game() || !c::misc::practice) {
			checkpoints.clear();
			currentcheckpoint = 0;
			pressdelay = 0;
			return;
		}

		float j = interfaces::console->get_convar("sv_cheats")->get_float();
		if (j == 1) {
			if (g::local->is_alive() && pressdelay < interfaces::globals->realtime) {
				std::stringstream ss;
				if (GetAsyncKeyState(c::misc::savepos) && (g::local->flags() & fl_onground)) {
					checkpoints.push_front({ vec3_t(cmd->view_angles.x, cmd->view_angles.y, 0.f), local_player->origin() });
					currentcheckpoint = 0;
					if (checkpoints.size() > 99)
						checkpoints.pop_back();
					pressdelay = interfaces::globals->realtime + 0.2;
					for (int i = 1; i <= 1; ++i) {


					}
				}
				else if (GetAsyncKeyState(c::misc::savepos) && !(g::local->flags() & fl_onground)) {
					pressdelay = interfaces::globals->realtime + 0.2;
					for (int i = 1; i == 1; ++i) {

					}
				}

				if (!checkpoints.empty()) {
					if (menu::checkkey(c::misc::loadpos, c::misc::loadpos_s)) {
						auto& cur = checkpoints.at(currentcheckpoint);
						std::string cmd = ("setpos ") + std::to_string(cur.second.x) + (" ") + std::to_string(cur.second.y) + (" ") + std::to_string(cur.second.z) + (";");
						interfaces::engine->execute_cmd(cmd.c_str());
						cmd = ("setang ") + std::to_string(cur.first.x) + (" ") + std::to_string(cur.first.y) + (" ") + std::to_string(cur.first.z) + (";");
						interfaces::engine->execute_cmd(cmd.c_str());
						pressdelay = interfaces::globals->realtime + 0.2;
						for (int i = 1; i <= 1; ++i) {

						}
					}
					if (menu::checkkey(c::misc::nextkey, c::misc::nextkey_s)) {
						if (currentcheckpoint > 0)
							currentcheckpoint--;
						pressdelay = interfaces::globals->realtime + 0.2;
						for (int i = 1; i <= 1; ++i) {

						}
					}
					if (menu::checkkey(c::misc::prevkey, c::misc::prevkey_s)) {
						if (currentcheckpoint + 1 < checkpoints.size())
							currentcheckpoint++;
						pressdelay = interfaces::globals->realtime + 0.2;
						for (int i = 1; i <= 1; ++i) {

						}
					}
					if (menu::checkkey(c::misc::undokey, c::misc::undokey_s)) {
						checkpoints.erase(checkpoints.begin() + currentcheckpoint);
						if (currentcheckpoint >= checkpoints.size())
							currentcheckpoint = std::max<int>(0, checkpoints.size() - 1);
						pressdelay = interfaces::globals->realtime + 0.2;
						for (int i = 1; i <= 1; ++i) {

						}
					}
				}
			}
		}
	}

public:
	void handle_fix(c_usercmd* cmd, vec3_t& angle) {
		if (!g::local || !cmd)
			return;

		vec3_t move, dir;
		vec3_t move_angle;
		float delta, len;

		if (!(g::local->flags() & fl_onground) && cmd->view_angles.z != 0 && cmd->buttons & in_attack)
			cmd->side_move = 0;

		move = { cmd->forward_move, cmd->side_move, 0 };

		len = move.normalize_movement();

		if (!len)
			return;

		math::vectorangles(move, move_angle);

		delta = (cmd->view_angles.y - angle.y);

		move_angle.y += delta;

		math::angle_vectors(move_angle, &dir);

		dir *= len;

		if (g::local->move_type() == movetype_ladder) {
			if (cmd->view_angles.x >= 45 && angle.x < 45 && std::abs(delta) <= 65)
				dir.x = -dir.x;

			cmd->forward_move = dir.x;
			cmd->side_move = dir.y;

			if (cmd->forward_move > 200)
				cmd->buttons |= in_forward;

			else if (cmd->forward_move < -200)
				cmd->buttons |= in_back;

			if (cmd->side_move > 200)
				cmd->buttons |= in_moveright;

			else if (cmd->side_move < -200)
				cmd->buttons |= in_moveleft;
		}
		else {
			if (cmd->view_angles.x < -90 || cmd->view_angles.x > 90)
				dir.x = -dir.x;

			cmd->forward_move = dir.x;
			cmd->side_move = dir.y;
		}

		cmd->forward_move = std::clamp(cmd->forward_move, -450.f, 450.f);
		cmd->side_move = std::clamp(cmd->side_move, -450.f, 450.f);
		cmd->up_move = std::clamp(cmd->up_move, -320.f, 320.f);

		if (g::local->move_type() != movetype_ladder)
			cmd->buttons &= ~(in_forward | in_back | in_moveright | in_moveleft);
	}
private:
	void handle_restore(c_usercmd* cmd) {
		if (c::misc::anti_untrusted) {
			cmd->view_angles.x = std::clamp(cmd->view_angles.x, -90.f + FLT_EPSILON, 90.f - FLT_EPSILON);
			cmd->view_angles.y = math::normalize_yaw(cmd->view_angles.y);
			cmd->view_angles.z = 0.f;
			features::movement::last_tick_y = cmd->view_angles.y;
		}
	}

	void handle_mouse_delta(c_usercmd* cmd) {
		if (!c::misc::mouse_fix)
			return;

		static vec3_t delta_viewangles{ };
		vec3_t delta = cmd->view_angles - delta_viewangles;
		delta.clamp();

		static convar* sensitivity = interfaces::console->get_convar("sensitivity");

		if (!sensitivity)
			return;

		if (delta.x != 0.f) {
			static convar* m_pitch = interfaces::console->get_convar("m_pitch");

			if (!m_pitch)
				return;

			int final_dy = static_cast<int>((delta.x / m_pitch->get_float()) / sensitivity->get_float());
			if (final_dy <= 32767) {
				if (final_dy >= -32768) {
					if (final_dy >= 1 || final_dy < 0) {
						if (final_dy <= -1 || final_dy > 0)
							final_dy = final_dy;
						else
							final_dy = -1;
					}
					else {
						final_dy = 1;
					}
				}
				else {
					final_dy = 32768;
				}
			}
			else {
				final_dy = 32767;
			}

			cmd->mouse_dy = static_cast<short>(final_dy);
		}

		if (delta.y != 0.f) {
			static convar* m_yaw = interfaces::console->get_convar("m_yaw");

			if (!m_yaw)
				return;

			int final_dx = static_cast<int>((delta.y / m_yaw->get_float()) / sensitivity->get_float());
			if (final_dx <= 32767) {
				if (final_dx >= -32768) {
					if (final_dx >= 1 || final_dx < 0) {
						if (final_dx <= -1 || final_dx > 0)
							final_dx = final_dx;
						else
							final_dx = -1;
					}
					else {
						final_dx = 1;
					}
				}
				else {
					final_dx = 32768;
				}
			}
			else {
				final_dx = 32767;
			}

			cmd->mouse_dx = static_cast<short>(final_dx);
		}

		delta_viewangles = cmd->view_angles;
	}

	void handle_perfect_sensitivity(c_usercmd* cmd) {
		if (!c::misc::mouse_fix || !g::local || !g::local->is_alive())
			return;

		static convar* sensitivity = interfaces::console->get_convar("sensitivity");
		static convar* m_yaw = interfaces::console->get_convar("m_yaw");
		static convar* m_pitch = interfaces::console->get_convar("m_pitch");
		static convar* cl_updaterate = interfaces::console->get_convar("cl_updaterate");

		if (!sensitivity || !m_yaw || !m_pitch || !cl_updaterate)
			return;

		// --- Get current tickrate ---
		const float tickrate = std::clamp(cl_updaterate->get_float(), 32.f, 256.f);
		const float tickscale = 64.f / tickrate; // normalize 64 tick as baseline

		// --- Get velocity and normalize ---
		vec3_t vel = g::local->velocity();
		float speed = vel.length_2d();
		if (speed < 5.f)
			return;

		vec3_t view_dir;
		math::angle_vectors(cmd->view_angles, &view_dir);

		// --- Calculate desired alignment angle between view and velocity ---
		float vel_yaw = math::rad2deg(std::atan2(vel.y, vel.x));
		float delta_yaw = math::normalize_yaw(cmd->view_angles.y - vel_yaw);

		// Ideal strafe alignment (perpendicular to velocity)
		float target_yaw_offset = (delta_yaw > 0.f ? -40.f : 40.f);
		float target_yaw = vel_yaw + target_yaw_offset;

		float diff = math::normalize_yaw(target_yaw - cmd->view_angles.y);

		// --- Sensitivity + tickrate based correction speed ---
		float sens_factor = std::clamp(sensitivity->get_float() / 2.0f, 0.5f, 2.0f);
		float correction_speed = (0.5f / sens_factor) * tickscale;

		// --- Smooth correction ---
		float adjusted_yaw = diff * correction_speed;

		// Convert yaw delta to mouse_dx scaled by tickrate
		float tick_sens_scale = math::deg2rad(tickscale / g::local->velocity().length_2d() * math::deg2rad(atan2f(250.f, 500.f)) * g::local->max_speed()); // faster tick = higher precision
		int mouse_dx = static_cast<int>((adjusted_yaw / m_yaw->get_float()) / (sensitivity->get_float() * tick_sens_scale));

		mouse_dx = std::clamp(mouse_dx, -32768, 32767);

		cmd->mouse_dx = static_cast<short>(mouse_dx);
		cmd->mouse_dy = 0;
		vec3_t ang = cmd->view_angles;
		ang.clamp();
		interfaces::engine->set_view_angles(ang);
	}

	void handle_auto_strafe(c_usercmd* cmd, vec3_t& view_angles)
	{
		if (!c::movement::gain_helper)
			return;

		if (!g::local || !g::local->is_alive())
			return;

		const int move_type = g::local->move_type();
		if (move_type == movetype_ladder || move_type == movetype_noclip)
			return;

		if (g::local->flags() & fl_onground)
			return;

		const vec3_t vel = g::local->get_velocity();
		float speed = vel.length_2d();
		if (speed < 5.f)
			return;

		// manual movement cancels
		if (!(cmd->buttons & (in_forward | in_back | in_moveleft | in_moveright)))
			return;

		cmd->forward_move = 0.f;
		cmd->side_move = 0.f;

		// ----------------------------------------------------
		// 🧭 1. Predict surface ahead for landing assist
		// ----------------------------------------------------
		vec3_t src = g::local->get_eye_pos();
		vec3_t dir;
		math::angle_vectors(view_angles, &dir);

		float look_ahead = std::clamp(speed * 0.7f, 150.f, 800.f);
		vec3_t end = src + dir * look_ahead - vec3_t(0, 0, 32.f);

		ray_t ray;
		trace_t trace;
		trace_world_only filter;
		filter.p_skip1 = g::local;

		ray.initialize(src, end);
		interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID_BRUSHONLY, &filter, &trace);

		bool will_land = trace.did_hit() && trace.plane.normal.z > 0.65f;

		// ----------------------------------------------------
		// 💨 2. Optimize yaw to velocity direction
		// ----------------------------------------------------
		const float vel_yaw = math::rad2deg(std::atan2(vel.y, vel.x));
		float yaw_delta = math::normalize_yaw(view_angles.y - vel_yaw);

		static bool flip = false;
		flip = !flip;

		// higher turn speed = faster strafing reaction
		float turn_speed = std::clamp(speed / 90.f, 1.5f, 6.f);
		float add_yaw = flip ? turn_speed : -turn_speed;

		// simulate realistic mouse input
		cmd->mouse_dx = static_cast<int>(add_yaw * 10.f); // scales movement
		view_angles.y += add_yaw;
		cmd->view_angles.y = view_angles.y;

		cmd->side_move = flip ? -450.f : 450.f;

		// ----------------------------------------------------
		// 🚀 3. Add speed gain bias (forward movement)
		// ----------------------------------------------------
		float gain_scale = std::clamp((1000.f - speed) / 1000.f, 0.05f, 0.45f);
		cmd->forward_move += 30.f + gain_scale * 100.f;

		// ----------------------------------------------------
		// 🪂 4. Landing assist (pitch control)
		// ----------------------------------------------------
		if (will_land)
		{
			float target_pitch = -math::rad2deg(std::asin(trace.plane.normal.z)) * 0.7f;
			float pitch_delta = math::normalize_pitch(target_pitch - view_angles.x);

			// smooth mouse dy simulation for pitch adjust
			cmd->mouse_dy = static_cast<int>(pitch_delta * 12.f);
			view_angles.x += pitch_delta * 0.4f;
			cmd->view_angles.x = view_angles.x;
		}
		else
		{
			cmd->mouse_dy = 0;
		}

		// normalize
		view_angles.normalize();
		cmd->view_angles = view_angles;
	}

	void null_strafing(c_usercmd* cmd) {
		if (!g::local || g::local->flags() & fl_onground) {
			return;
		}

		if (c::movement::null_strafing) {
			if (cmd->mouse_dx > 0 && cmd->buttons & in_moveright && cmd->buttons & in_moveleft)
				cmd->side_move = -450.f;
			else if (cmd->mouse_dx < 0 && cmd->buttons & in_moveleft && cmd->buttons & in_moveright)
				cmd->side_move = 450.f;

			if (cmd->buttons & in_moveright)
				cmd->buttons &= ~in_moveleft;

			if (cmd->buttons & in_moveleft)
				cmd->buttons &= ~in_moveright;
		}
	}

	void auto_duck_on_landing(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive())
			return;

		if (!(g::local->flags() & fl_onground))
			return;

		// --- Continue holding crouch for N ticks if active ---
		if (auto_duck_data.duck_ticks > 0) {
			cmd->buttons |= in_duck;
			auto_duck_data.duck_ticks--;
			return;
		}

		// --- Predict next position ---
		vec3_t start = g::local->get_eye_pos();
		vec3_t velocity = g::local->velocity();
		vec3_t predicted = start + velocity * interfaces::globals->interval_per_tick;

		trace_t tr_down, tr_up;
		ray_t ray;
		trace_world_only filter;
		filter.p_skip1 = g::local;

		// --- Trace DOWN to detect landing surfaces ---
		ray.initialize(predicted, predicted - vec3_t(0, 0, 100.0f));
		interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &filter, &tr_down);

		// --- Trace UP to detect low ceilings ---
		ray.initialize(predicted, predicted + vec3_t(0, 0, 72.0f));
		interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &filter, &tr_up);

		// --- Low ceiling check ---
		if (tr_up.flFraction < 1.0f) {
			// There's a ceiling close above your head
			cmd->buttons |= in_duck;
			auto_duck_data.duck_ticks = 2; // hold crouch for 2 ticks
			return;
		}

		// --- Landing check ---
		if (tr_down.flFraction == 1.0f)
			return; // no ground below

		float distance = start.z - tr_down.end.z;
		if (distance <= 0.0f)
			return;

		constexpr float normal_step = 18.0f;
		constexpr float crouch_extra = 36.0f;

		if (distance > normal_step && distance < (normal_step + crouch_extra)) {
			// Check if standing clips but crouched fits
			vec3_t stand_maxs(16, 16, 72);
			vec3_t duck_maxs(16, 16, 54);

			trace_t stand_tr, duck_tr;
			ray.initialize(tr_down.end + vec3_t(0, 0, 1), tr_down.end + vec3_t(0, 0, 72));
			interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &filter, &stand_tr);
			ray.initialize(tr_down.end + vec3_t(0, 0, 1), tr_down.end + vec3_t(0, 0, 54));
			interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &filter, &duck_tr);

			if (stand_tr.flFraction < 1.0f && duck_tr.flFraction == 1.0f) {
				cmd->buttons |= in_duck;
				auto_duck_data.duck_ticks = 2;
			}
		}
	}

	void AutoStafe(c_usercmd* cmd)
	{
		if (g::local->flags() & fl_onground)
			return;

		// detect manual strafing (player holding A/D or giving manual sidemove)
		bool manual_strafe = false;
		if (cmd->buttons & in_moveleft)  manual_strafe = true;
		if (cmd->buttons & in_moveright) manual_strafe = true;
		if (fabsf(cmd->side_move) > 1.f) manual_strafe = true;

		static float side = 1.f;
		side = -side;

		const vec3_t velocity = g::local->velocity();
		vec3_t wish_angle = cmd->view_angles;

		float speed = velocity.length_2d();
		float ideal_strafe = std::clamp<float>(math::rad2deg(atan(15.f / speed)), 0.f, 90.f);

		cmd->forward_move = 0.f;

		static auto cl_sidespeed = interfaces::console->get_convar("cl_sidespeed");
		float cl_sidespeed_value = cl_sidespeed->get_float();

		static float old_yaw = 0.f;
		float yaw_delta = std::remainderf(wish_angle.y - old_yaw, 360.f);
		float abs_yaw_delta = fabsf(yaw_delta);
		old_yaw = wish_angle.y;

		// ------ MANUAL STRAFING PROTECTION ------
		if (manual_strafe)
		{
			// Only apply tiny micro-corrections to prevent slowdown
			vec3_t vel_dir;
			math::vectorangles(velocity, vel_dir);

			float delta_to_vel = std::remainderf(wish_angle.y - vel_dir.y, 360.f);

			// if player is fighting air acceleration weirdly, softly help them
			if (fabsf(delta_to_vel) > ideal_strafe * 1.25f)
			{
				float correction = std::clamp(delta_to_vel, -ideal_strafe, ideal_strafe);
				wish_angle.y -= correction * 0.25f;  // light correction only
				this->handle_fix(cmd, wish_angle);
			}

			// leave cmd->side_move alone so manual strafing stays perfect
			return;
		}
		// ----------------------------------------


		// ---------- AUTO STRAFE (unchanged behavior) ----------
		if (abs_yaw_delta <= ideal_strafe || abs_yaw_delta >= 30.f)
		{
			vec3_t velocity_direction;
			math::vectorangles(velocity, velocity_direction);

			float velocity_delta = std::remainderf(wish_angle.y - velocity_direction.y, 360.f);
			float retrack = std::clamp<float>(math::rad2deg(atan(30.f / speed)), 0.f, 90.f) * 2.f;

			if (velocity_delta <= retrack || speed <= 15.f)
			{
				if (-retrack <= velocity_delta || speed <= 15.f)
				{
					wish_angle.y += side * ideal_strafe;
					cmd->side_move = cl_sidespeed_value * side;
				}
				else
				{
					wish_angle.y = velocity_direction.y - retrack;
					cmd->side_move = cl_sidespeed_value;
				}
			}
			else
			{
				wish_angle.y = velocity_direction.y + retrack;
				cmd->side_move = -cl_sidespeed_value;
			}

			this->handle_fix(cmd, wish_angle);
		}
		else
		{
			cmd->side_move = (yaw_delta > 0.f) ? -cl_sidespeed_value : cl_sidespeed_value;
		}
	}

	void apply_changed_convars() {
		// Access convar linked list
		convar* head = **reinterpret_cast<convar***>(reinterpret_cast<std::uintptr_t>(interfaces::console) + 0x34);
		if (!head)
			return;

		for (auto& entry : c::changed_convars) {
			convar* c = head;
			while (c) {
				if (c->name && _stricmp(c->name, entry.name.c_str()) == 0) {
					// Determine if numeric
					bool is_number = true;
					for (const char* ch = entry.value.c_str(); *ch; ++ch) {
						if ((*ch < '0' || *ch > '9') && *ch != '.' && *ch != '-') {
							is_number = false;
							break;
						}
					}

					if (is_number) {
						float f = static_cast<float>(atof(entry.value.c_str()));
						c->set_value(f);
					}
					else {
						c->set_value(entry.value.c_str());
					}
					break;
				}
				c = c->next;
			}
		}
	}

	void fast_stop(c_usercmd* m_pcmd)
	{
		if (!c::movement::fast_stop)
			return;

		if (!g::local)
			return;

		if (!(g::local->flags() & fl_onground && prediction_backup::flags & fl_onground))
			return;

		auto pressed_move_key = m_pcmd->buttons & in_forward || m_pcmd->buttons & in_moveleft || m_pcmd->buttons & in_back || m_pcmd->buttons & in_moveright || m_pcmd->buttons & in_jump;

		if (pressed_move_key)
			return;

		auto velocity = g::local->velocity();

		if (velocity.length_2d() > 20.0f)
		{
			vec3_t direction;
			vec3_t real_view;

			math::vector_angles(velocity, direction);
			interfaces::engine->get_view_angles(real_view);

			direction.y = real_view.y - direction.y;

			vec3_t forward;
			math::angle_vectors(direction, forward);

			static auto cl_sidespeed = interfaces::console->get_convar("cl_sidespeed");

			auto negative_side_speed = -cl_sidespeed->get_float();
			auto negative_direction = forward * negative_side_speed;

			m_pcmd->forward_move = negative_direction.x;
			m_pcmd->side_move = negative_direction.y;
		}
		else
		{
			auto speed = 1.01f;

			if (m_pcmd->buttons & in_duck)
				speed *= 2.94117647f;

			if (m_pcmd->command_number & 1)
				m_pcmd->side_move += speed;
			else
				m_pcmd->side_move -= speed;
		}
	}

	void auto_hsw(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive())
			return;

		constexpr float DEG2RAD = M_PI_F / 180.0f;
		constexpr float MOVE_SPEED_THRESHOLD = 10000.f;
		constexpr float HALF_SIDESWAYS_THRESHOLD = 45.0f;
		constexpr float STRAFE_FACTOR = 45.f; // Adjust as needed

		// Calculate perfect strafe movement based on velocity and view angles
		float move_angle = atan2(g::local->velocity().y, g::local->velocity().x) * (180.0f / M_PI_F);
		float ideal_strafe_angle = cmd->view_angles.y - move_angle;

		float move_direction = (g::local->velocity().y < 0.0f) ? -1.0f : 1.0f;
		float move_speed = g::local->velocity().length_2d();

		float cos_view_y = cos(cmd->view_angles.y * DEG2RAD);
		float sin_view_y = sin(cmd->view_angles.y * DEG2RAD);

		float move_direction_x = cos(ideal_strafe_angle * DEG2RAD);
		float move_direction_y = sin(ideal_strafe_angle * DEG2RAD);

		float move_direction_rotated_x = move_direction_x * cos_view_y - move_direction_y * sin_view_y;
		float move_direction_rotated_y = move_direction_x * sin_view_y + move_direction_y * cos_view_y;

		float strafe_angle = move_angle + 90.0f * move_direction;

		float velocity_delta = fmaxf(0.0f, move_speed - MOVE_SPEED_THRESHOLD);
		float final_strafe_speed = fminf(velocity_delta, move_speed);

		float strafe_velocity_x = move_direction_rotated_x * final_strafe_speed * STRAFE_FACTOR;
		float strafe_velocity_y = move_direction_rotated_y * final_strafe_speed * STRAFE_FACTOR;

		float angle_difference = fabsf(ideal_strafe_angle - cmd->view_angles.y);
		if (angle_difference < HALF_SIDESWAYS_THRESHOLD) {
			constexpr float HALF_SIDESWAYS_FACTOR = 45.f;
			strafe_velocity_x *= HALF_SIDESWAYS_FACTOR;
			strafe_velocity_y *= HALF_SIDESWAYS_FACTOR;
		}

		cmd->mouse_dx = strafe_velocity_x;
		cmd->mouse_dy = strafe_velocity_y;
	}

public:

	void pre(c_usercmd* cmd) {
		features::misc::clantag_spammer();
		this->handle_mouse_delta(cmd);
		this->null_strafing(cmd);
		this->fast_stop(cmd);
		this->apply_changed_convars();
	}

	void run(c_usercmd* cmd) {
		this->handle_fix(cmd, cmd->view_angles);
		this->handle_restore(cmd);
		this->handle_practice(cmd);
	}
};
inline UtilityAssist UtilityAssistant;

class BhopAssist {
private:

	void handle_fastduck(c_usercmd* cmd) {
		if (c::movement::fastduck && g::local->is_alive() && g::local && interfaces::engine->is_in_game()) cmd->buttons |= in_bullrush;
		if (c::movement::fastduck && g::local->is_alive() && g::local && interfaces::engine->is_in_game() && cmd->buttons & in_duck) cmd->buttons |= in_bullrush;
	}

	void handle_bhop(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive()) {
			return;
		}

		if (c::movement::bhop) {
			if (!GetAsyncKeyState(c::movement::jump_bug_key) && g::local->move_type() != movetype_ladder) {
				if (!(g::local->flags() & fl_onground) && cmd->buttons & (in_jump)) {
					cmd->buttons &= ~(in_jump);
				}
			}
		}
	}

public:

	void run(c_usercmd* cmd) {
		this->handle_bhop(cmd);
		this->handle_fastduck(cmd);
	}

};
inline BhopAssist BhopAssistant;

class JumpTypeAssist {
private:

	bool should_ej;
	bool should_mj;
	bool should_lj;
	bool do_long_jump;

	void handle_edgejump(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive()) {
			return;
		}

		const int move_type = g::local->move_type();
		if (move_type == movetype_ladder || move_type == movetype_noclip || move_type == movetype_observer) {
			return;
		}

		if (c::movement::edgejump && menu::checkkey(c::movement::edgejump_key, c::movement::edgejump_key_s)) {
			if (prediction_backup::flags & (fl_onground) && !(g::local->flags() & fl_onground)) {

				if (c::movement::ej_252pre) {
					cmd->side_move = -450.f;
					cmd->up_move = 450.f;
					cmd->forward_move = 0.f;
				}

				if (c::movement::ej_adaptive_nulls) {
					if (cmd->buttons | in_moveleft)
						cmd->buttons &= ~in_moveleft;
					if (cmd->buttons | in_moveright)
						cmd->buttons &= ~in_moveright;
					if (cmd->buttons | in_forward)
						cmd->buttons &= ~in_forward;
					if (cmd->buttons | in_back)
						cmd->buttons &= ~in_back;
				}

				should_ej = true;
				cmd->buttons |= in_jump;
			}
			else {
				should_ej = false;
			}
		}
	}

	void handle_longjump(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive()) {
			return;
		}

		if (c::movement::long_jump_on_edge) {
			const int move_type = g::local->move_type();
			if (move_type == movetype_ladder || move_type == movetype_noclip || move_type == movetype_observer) {
				return;
			}
			if (g::local->flags() & fl_onground) {
				saved_tick_count = interfaces::globals->tick_count;
			}
			if (interfaces::globals->tick_count - saved_tick_count > 2) {
				this->do_long_jump = false;
			}
			else {
				this->do_long_jump = true;
			}
			if (menu::checkkey(c::movement::long_jump_key, c::movement::long_jump_key_s)) {
				if (this->do_long_jump && !(g::local->flags() & fl_onground)) {
					if (this->do_long_jump) {

						if (c::movement::lj_252pre) {
							cmd->side_move = -450.f;
							cmd->up_move = 450.f;
							cmd->forward_move = 0.f;
						}

						if (c::movement::lj_adaptive_nulls) {
							if (cmd->buttons | in_moveleft)
								cmd->buttons &= ~in_moveleft;
							if (cmd->buttons | in_moveright)
								cmd->buttons &= ~in_moveright;
							if (cmd->buttons | in_forward)
								cmd->buttons &= ~in_forward;
							if (cmd->buttons | in_back)
								cmd->buttons &= ~in_back;
						}

					}
					cmd->buttons |= in_duck;
					if (c::movement::unduck_after_lj && (interfaces::globals->tick_count - saved_tick_count > 3)) {
						cmd->buttons &= ~in_duck;
					}
					this->should_lj = true;
				}
				else {
					this->should_lj = false;
				}
			}
		}
	}

	void handle_minijump(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive()) {
			return;
		}

		const int move_type = g::local->move_type();
		if (move_type == movetype_ladder || move_type == movetype_noclip || move_type == movetype_observer) {
			return;
		}

		if (c::movement::mini_jump && menu::checkkey(c::movement::mini_jump_key, c::movement::mini_jump_key_s)) {
			if (prediction_backup::flags & (fl_onground) && !(g::local->flags() & fl_onground)) {

				if (c::movement::mj_252pre) {
					cmd->side_move = -450.f;
					cmd->up_move = 450.f;
					cmd->forward_move = 0.f;
				}

				if (c::movement::mj_adaptive_nulls) {
					if (cmd->buttons | in_moveleft)
						cmd->buttons &= ~in_moveleft;
					if (cmd->buttons | in_moveright)
						cmd->buttons &= ~in_moveright;
					if (cmd->buttons | in_forward)
						cmd->buttons &= ~in_forward;
					if (cmd->buttons | in_back)
						cmd->buttons &= ~in_back;
				}

				this->should_mj = true;

				if (c::movement::mini_jump_ej)
					cmd->buttons |= in_jump;

				cmd->buttons |= in_duck;
				if (c::movement::unduck_after_mj && (interfaces::globals->tick_count - saved_tick_count > 2)) {
					cmd->buttons &= ~in_duck;
				}
			}
			else {
				this->should_mj = false;
			}
		}
	}

	void handle_ladderjump(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive()) {
			return;
		}

		static int saved_tick;

		if (c::movement::ladderjump_ej && c::movement::edgejump && menu::checkkey(c::movement::edgejump_key, c::movement::edgejump_key_s) || c::movement::ladderjump_lj && c::movement::long_jump_on_edge && menu::checkkey(c::movement::long_jump_key, c::movement::long_jump_key_s) || c::movement::ladderjump_mj && c::movement::mini_jump && menu::checkkey(c::movement::mini_jump_key, c::movement::mini_jump_key_s)) {
			if (c::movement::edge_jump_on_ladder) {
				const int pre = g::local->move_type();

				prediction::begin(cmd);
				prediction::end();

				const int post = g::local->move_type();
				if (pre == movetype_ladder) {
					if (post != movetype_ladder) {
						saved_tick = interfaces::globals->tick_count;
						cmd->buttons |= in_jump;
						cmd->forward_move = 0.f;
						cmd->side_move = 0.f;
						cmd->buttons = cmd->buttons & ~(in_forward | in_back | in_moveright | in_moveleft);
					}
				}

				if (interfaces::globals->tick_count - saved_tick > 1 && interfaces::globals->tick_count - saved_tick < 15) {
					cmd->forward_move = 0.f;
					cmd->side_move = 0.f;
					cmd->buttons = cmd->buttons & ~(in_forward | in_back | in_moveright | in_moveleft);
					cmd->buttons |= in_duck;
				}
			}
		}
	}

	void handle_stamina_hop(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive()) {
			return;
		}

		if (c::movement::delay_hop && menu::checkkey(c::movement::delay_hop_key, c::movement::delay_hop_key_s)) {

			static int ticks = 0;
			const int ticks_to_wait = c::movement::dh_tick;

			const int move_type = g::local->move_type();
			if (move_type == movetype_ladder || move_type == movetype_noclip || move_type == movetype_observer)
				return;

			if (g::local->flags() & fl_onground) {
				if (cmd->tick_count > ticks + ticks_to_wait) {
					cmd->buttons |= in_jump;
					ticks = cmd->tick_count;
				}
			}
			else {
				ticks = cmd->tick_count;
			}
		}
	}

public:
	void pre(c_usercmd* cmd) {
		this->handle_stamina_hop(cmd);
	}

	void run(c_usercmd* cmd) {
		this->handle_edgejump(cmd);
		this->handle_longjump(cmd);
		this->handle_minijump(cmd);
		this->handle_ladderjump(cmd);
	}
};
inline JumpTypeAssist JumpTypeAssistant;

class JumpbugAssist {
private:
	bool should_predict = false;
	float gravity = 0.f;
	float upmove = 0.f;
	vec3_t velocity{ 0,0,0 };

	struct scan_result_t {
		bool valid = false;
		vec3_t hit_pos;
		vec3_t normal;
		float distance = FLT_MAX;
		float score; // optional, e.g., how likely it is to produce a jumpbug
	};

	scan_result_t scan_for_jumpbug(c_usercmd* cmd, const vec3_t& origin, const vec3_t& start_vel) {
		scan_result_t best{};

		const int max_ticks_ahead = 64; // ~0.4s, tune this for longer falls
		const float step_time = interfaces::globals->interval_per_tick;
		const float step_down = 32.f;   // how far to trace down per prediction step

		vec3_t pos = origin;
		vec3_t vel = start_vel;

		for (int t = 0; t < time_to_ticks(c::movement::jumpbug_scan); t++) {
			// apply gravity
			vel.z -= gravity * step_time * 0.5f;
			pos += vel * step_time;

			// trace downward from predicted pos
			vec3_t end = pos - vec3_t(0, 0, step_down);

			trace_t trace;
			ray_t ray;
			ray.initialize(pos, end);

			trace_world_only filter;
			filter.p_skip1 = g::local;

			interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &filter, &trace);

			if (trace.flFraction < 1.0f && trace.plane.normal.z > 0.7f) {

				float dist = (trace.end - origin).length();

				if (dist < best.distance) {

					best.valid = true;
					best.hit_pos = trace.end;
					best.normal = trace.plane.normal;
					best.distance = dist;
				}
			}

			// early exit if we already found something very close
			if (best.valid && best.distance < 9.f)
				break;
		}

		return best;
	}

	std::vector<scan_result_t> scan_for_jumpbug_multi(const vec3_t& origin, const vec3_t& velocity) {
		std::vector<scan_result_t> results;

		// Define multiple offsets or directions to check
		std::vector<vec3_t> offsets = {
			{0, 0, 0},
			{16, 0, 0},
			{-16, 0, 0},
			{0, 16, 0},
			{0, -16, 0},
			{16,16,72 },
			{-16,-16,-72},
			{8, 8, 0},
			{-8, -8, 0},
			// Add more offsets for finer coverage
		};

		for (auto& offset : offsets) {
			auto res = scan_for_jumpbug(nullptr, origin + offset, velocity); // reuse your current scan
			if (res.valid) {
				res.hit_pos = res.hit_pos + offset; // adjust for offset if necessary
				results.push_back(res);
			}
		}

		return results;
	}

	struct jumpbug_crouch
	{
		vec3_t position_ground;

		bool detected = false;
		bool should_uncrouch = false;
		bool ground_found = false;
		int jb_tick = 0;

		//values from csgo scr
		float JUMPBUG_MAX_HEIGHT = 16.f;
		static constexpr float ducked_bbox_height = 54.f;
		static constexpr float unducked_bbox_height = 72.f;
		static constexpr float duck_pos_shift_amt = 0.5f * (unducked_bbox_height - ducked_bbox_height);

		//traceray to find ground
		bool find_ground(float end_trace_distance)
		{
			const auto mins = g::local->collideable()->mins();
			const auto maxs = g::local->collideable()->maxs();

			vec3_t start_pos = g::local->origin();
			vec3_t end_pos = start_pos;
			end_pos.z -= end_trace_distance;

			trace_t trace;
			ray_t ray;
			trace_filter filter(g::local);

			ray.initialize(start_pos, end_pos, mins, maxs);
			interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &filter, &trace);

			if (trace.did_hit())
			{
				this->position_ground = trace.end;
				return true;
			}
			return false;
		}

		void set_detected()
		{
			this->detected = true;
		}

		void set_undetected()
		{
			this->detected = false;
			this->ground_found = false;
			this->jb_tick = 0;
		}

		//can jumpbug or nah
		void check_condition(vec3_t& position_player, vec3_t& ground_position)
		{

			float uncrouch_position = position_player.z - duck_pos_shift_amt;

			if (ground_position.z + JUMPBUG_MAX_HEIGHT < uncrouch_position)
			{
				set_undetected();
				return;
			}

			//csgo src
			float corrected_ground_postion = ground_position.z - 0.03125f;
			float z_check = uncrouch_position - 2.f;
			if (z_check < corrected_ground_postion)
			{
				set_detected();
			}
			else {
				set_undetected();
			}

		}

		//pred
		bool can_we_jumpbug(c_usercmd* cmd, vec3_t& ground_position)
		{
			auto movetype = g::local->move_type();
			vec3_t velocity = g::local->velocity();
			if (movetype == movetype_ladder || movetype == movetype_noclip || velocity.z > 0.f)
			{
				set_undetected();
				return false;
			}

			if (detected)
				return true;

			set_undetected();

			//we dont need to predict more (so we dont need to have a cycle here)
			c_usercmd fakecmd;
			fakecmd = *cmd;

			prediction::begin(&fakecmd);
			prediction::end();

			check_condition(g::local->origin(), ground_position);

			prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);

			if (detected)
				return true;


			return false;
		}

	} jumpbug_data;

	void surf_jump_bug_crouch(c_usercmd* cmd)
	{
		if (!g::local || !g::local->is_alive()) {
			return;
		}

		if (!features::movement::AlertJB && !features::movement::AlertJB2)
		{
			jumpbug_data.set_undetected();
			return;
		}

		//is playa crouched
		int duck_pressed = (cmd->buttons & in_duck);

		//if ducked - we uncrouch
		if (duck_pressed)
		{
			if (!jumpbug_data.detected)
			{
				if (!jumpbug_data.ground_found && !jumpbug_data.find_ground(16.f))
				{
					jumpbug_data.set_undetected();
					return;
				}

				if (!jumpbug_data.detected)
				{
					jumpbug_data.can_we_jumpbug(cmd, jumpbug_data.position_ground);
				}

			}
			if (jumpbug_data.detected)
			{
				cmd->buttons &= ~in_duck;
			}
			else
			{
				jumpbug_data.set_undetected();
			}
		}
		else
		{
			jumpbug_data.set_undetected();
			return;
		}
	}

	void surf_jump_bug(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive()) {
			return;
		}

		if (!features::movement::AlertJB && !features::movement::AlertJB2)
		{
			jumpbug_data.set_undetected();
			return;
		}

		//is playa crouched
		int duck_pressed = (cmd->buttons & in_duck);

		//if ducked - we uncrouch
		//if not - normal jb
		if (!duck_pressed)
		{
			if (!(prediction_backup::flags & (fl_onground)) && g::local->flags() & (fl_onground)) {
				cmd->buttons |= in_duck;
			}

		}
		else
		{
			if (jumpbug_data.detected)
				cmd->buttons &= ~in_duck;

			jumpbug_data.set_undetected();
		}

		//jump on ground
		if (g::local->flags() & fl_onground) {
			cmd->buttons &= ~in_jump;
			jumpbug_data.set_undetected();
		}

		//print shit (kinda works so havent changed anything)
		features::movement::detected_normal_jump_bug = (c::movement::jump_bug
			&& !features::movement::should_edge_bug
			&& g::local->velocity().z > prediction_backup::velocity.z
			&& !(prediction_backup::flags & fl_onground)
			&& !(g::local->flags() & 1)
			&& g::local->move_type() != movetype_ladder
			&& g::local->move_type() != movetype_noclip
			&& g::local->move_type() != movetype_observer) ? true : false;
	}

	void handle_prediction(c_usercmd* cmd) {
		if (this->should_predict) {
			prediction::begin(cmd);
			prediction::end();
		}
	}

	void handle_jumpbug(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive())
			return;

		this->gravity = interfaces::console->get_convar("sv_gravity")->get_float();
		this->velocity = g::local->velocity();
		this->upmove = cmd->up_move;
		this->should_predict = false;

		if (!c::movement::jump_bug)
			return;

		if (!menu::checkkey(c::movement::jump_bug_key, c::movement::jump_bug_key_s))
			return;

		auto results = scan_for_jumpbug_multi(g::local->origin(), this->velocity);
		this->handle_prediction(cmd);

		if (!results.empty()) {
			// Pick the best one (closest, highest score, etc.)
			scan_result_t best = results[0];
			for (auto& r : results) {
				if (r.score > best.score) best = r; // or some other heuristic
			}

			this->should_predict = true;
			vec3_t at = best.hit_pos - g::local->origin();
			float yaw_to_target = math::vector_to_yaw(at);
			float diff = math::angle_diff(cmd->view_angles.y, yaw_to_target);
			cmd->view_angles.y += math::deg2rad(diff * c::movement::jumpbug_angle);
			UtilityAssistant.handle_fix(cmd, cmd->view_angles);
		}

			if (!(prediction_backup::flags & fl_onground) && g::local->flags() & fl_onground) {
				this->should_predict = true;
				cmd->buttons |= in_duck;
			}
			else if (!(prediction_backup::flags & fl_onground) && g::local->flags() & fl_onground && cmd->buttons & in_duck) {
				this->should_predict = true;
				cmd->buttons &= ~in_duck;
			}

			vec3_t backup_pos = g::local->origin();

			if (g::local->flags() & fl_onground) {
				this->should_predict = true;
				cmd->buttons &= ~in_jump;
			}

			if (g::local->move_type() == movetype_ladder) {
				cmd->buttons |= in_jump;
				cmd->up_move = 0.5f * interfaces::console->get_convar("sv_gravity")->get_float() * interfaces::globals->interval_per_tick;
				g::local->velocity().z = 0.5f * interfaces::console->get_convar("sv_gravity")->get_float() * interfaces::globals->interval_per_tick;
			}

			g::local->velocity() = this->velocity;
			g::local->origin() = backup_pos;

			static bool detected_once = false;

			// Check if the jumpbug condition is true THIS tick
			bool condition =
				(
					c::movement::jump_bug &&
					!features::movement::should_edge_bug &&
					g::local->velocity().z > prediction_backup::velocity.z &&
					!(prediction_backup::flags & fl_onground) &&
					!(g::local->flags() & fl_onground) &&
					!(g::local->flags() & fl_ducking) &&
					g::local->move_type() != movetype_ladder &&
					g::local->move_type() != movetype_noclip &&
					g::local->move_type() != movetype_observer
					);

			// FIRST-TICK ONLY TRIGGER
			if (condition && !detected_once)
			{
				features::movement::detected_normal_jump_bug = true;
				detected_once = true;            // lock it
			}
			else
			{
				features::movement::detected_normal_jump_bug = false;
			}

			// Reset latch when airborne again
			if (!(g::local->flags() & fl_onground))
			{
				detected_once = false;
			}

		if (features::movement::detected_normal_jump_bug && c::movement::jump_bug_detection_printf) {
			interfaces::chat_element->chatprintf("#dna#_print_jumpbugged");
		}
	}

public:

	void pre(c_usercmd* cmd) {
		this->surf_jump_bug_crouch(cmd);
	}

	void run(c_usercmd* cmd) {
		this->handle_jumpbug(cmd);
		this->surf_jump_bug(cmd);
	}
};
inline JumpbugAssist JumpbugAssistant;

class EdgebugAssist {
public:

	std::vector<vec3_t> ebpos;
	vec3_t backup_origin = vec3_t(0, 0, 0);
	vec3_t backup_velocity = vec3_t(0, 0, 0);
	vec3_t origin = vec3_t(0, 0, 0);
	vec3_t angles = vec3_t(0, 0, 0);
	bool should_predict = false;
	bool should_edge_bug = false;
	bool detected_normal_edge_bug = false;
	bool crouched = false;
	bool is_double_eb = false;
	int detecttick = 0;
	int edgebugtick = 0;
	short mousedx;
	bool strafing = false;
	float yawdelta = 0.f;
	float yaw = 0.f;
	float forwardmove, upmove, sidemove = 0.f;
	float startingyaw = 0.f;
	int eblength = 0;
	int ticks_left = 0;
	int did_edgebug = 0;
	int did_edgebug_strafe = 0;
	int buttons = 0;
	int second_eb_tick = 0;
	float highest_ground = 0.f;
	float saved_velocity_z = 0.f;
	float saved_origin_z = 0.f;
	vec3_t velocity_backup;
	vec3_t old_viewangles;
	int flags_backup;

	enum ducktype_t {
		standing,
		crouching
	};
	ducktype_t ducktype;

public:

	bool check_edge_bug(c_usercmd* cmd, bool& brk) {
		if (!g::local)
			return false;
		if (const auto mt = g::local->move_type(); mt == movetype_ladder || mt == movetype_noclip) {
			brk = true;
			return false;
		}
		vec3_t unpredicted_velocity = prediction_backup::velocity;
		vec3_t predicted_velocity = g::local->velocity();

		vec3_t unpredicted_origin = prediction_backup::origin;
		vec3_t predicted_origin = g::local->origin();

		int predicted_flags = g::local->flags();

		static convar* Sv_gravity = interfaces::console->get_convar(("sv_gravity"));
		auto sv_gravity = Sv_gravity->get_float();
		float fTickInterval = interfaces::globals->interval_per_tick;
		float fTickRate = (fTickInterval > 0) ? (1.0f / fTickInterval) : 0.0f;
		float gravityvelo = ((sv_gravity / 2) / fTickRate) * -1.f;

		//calculating for all ticks
		//k it doesnt work ...
		//unpredicted_target = 5.62895 * gravityvelo / 6.25f;
		//predicted_target = 8.293333 * gravityvelo / 6.25f;

		//hope will fix soon

		float z_vel_pred = round(predicted_velocity.z);
		float l2d_vel_pred = round(g::local->velocity().length_2d());

		//there is no point in edgebugging while having 0 velocity
		if (z_vel_pred >= 0.f || (predicted_flags & fl_onground) || l2d_vel_pred == 0.f) {
			brk = true;
			return false;
		}
		else if (unpredicted_velocity.z < 0.f && predicted_velocity.z > unpredicted_velocity.z && predicted_velocity.z < 0.f) {
			//we dont need to calc edgebugs which we cant hit by player origin :D (ig its better to use abs_origin, but we will get ~same result)
			if (unpredicted_origin.z < predicted_origin.z) {
				return false;
			}

			int z_vel = predicted_velocity.z;

			prediction::begin(cmd);
			prediction::end();

			float rounded_vel = round(-sv_gravity * interfaces::globals->interval_per_tick) + z_vel;
			float unpredicted_vel = round(g::local->velocity().z);

			if (rounded_vel == unpredicted_vel || (unpredicted_vel == 0.f && (g::local->flags() & fl_onground))) {
				return true;
			}
			else {
				brk = true;
				return false;
			}
		}
		else if (unpredicted_velocity.z < gravityvelo && round(g::local->velocity().z) == round(gravityvelo) && g::local->move_type() != movetype_ladder) {
			return true;
		}
		return false;
	}

private:
	void handle_pre_edgebug(c_usercmd* cmd) {
		prediction_backup::origin = g::local->origin();
		prediction_backup::velocity = g::local->velocity();
		prediction_backup::flags = g::local->flags();

		if (this->should_edge_bug && this->crouched && this->ticks_left) {
			g::local->velocity() = this->backup_velocity;
			cmd->buttons |= in_duck;
		}

		if (this->should_edge_bug && this->ticks_left && this->strafing) {
			cmd->forward_move = this->forwardmove;
			cmd->side_move = this->sidemove;
		}
	}

	void handle_edgebug(c_usercmd* cmd) {
		if (!c::movement::edge_bug || !menu::checkkey(c::movement::edge_bug_key, c::movement::edge_bug_key_s) || !g::local) {
			this->detecttick = 0;
			this->edgebugtick = 0;
			this->ticks_left = 0;
			this->should_edge_bug = false;
			features::movement::should_edge_bug = false;
			return;
		}

		const int move_type = g::local->move_type();
		if ((prediction_backup::flags & fl_onground) || move_type != movetype_walk || move_type == movetype_noclip || move_type == movetype_observer || move_type == movetype_ladder) {
			this->detecttick = 0;
			this->edgebugtick = 0;
			this->ticks_left = 0;
			this->should_edge_bug = false;
			features::movement::should_edge_bug = false;
			return;
		}

		features::movement::detect_data.ticks_left = this->ticks_left;

		vec3_t originalangles = cmd->view_angles;
		float originalfmove = cmd->forward_move;
		float originalsmove = cmd->side_move;
		float originalupmove = cmd->up_move;
		vec3_t originalvelocity = g::local->velocity();

		if (!this->ticks_left) {
			convar* get_yaw = interfaces::console->get_convar("m_yaw");
			convar* get_sens = interfaces::console->get_convar("sensitivity");
			const float m_yaw = get_yaw->get_float();
			const float sensitivity = get_sens->get_float();
			float yawdelta = std::clamp(cmd->mouse_dx * m_yaw * sensitivity, -30.f, 30.f);
			int ticklimit = time_to_ticks(c::movehelp::edgebug_scan);
			int pred_rounds = (c::movehelp::advanced_edgebug && yawdelta != 0.f) ? 4 : 2;
			this->startingyaw = originalangles.y;
			this->highest_ground = 0.f;
			std::vector<vec3_t> positions;

			vec3_t originalpos = prediction_backup::origin;
			vec3_t originalvel = prediction_backup::velocity;
			int originalflags = prediction_backup::flags;
			int cmd_pred = interfaces::prediction->split->commands_predicted - 1;

			ebpos.clear();
			ebpos.push_back(g::local->origin());

			const float max_radias = m_pi * 2.f;
			const float step = max_radias / 16.f;

			const float velocity_threshold = 0.f; // Minimum velocity for edgebug
			const float ground_distance_threshold = 2.f; // Distance from ground to check
			int consecutive_ground_checks = 0;
			bool potential_double_eb = false;
			int first_eb_tick = 0;

			for (int round = 0; round < pred_rounds; round++) {
				if (round > 1) {
					float max_delta = yawdelta;
					// Increased precision for yaw scanning
					int divisor = c::movehelp::advanced_edgebug ? c::movement::advanced_radius : 2;
					for (yawdelta = max_delta / divisor; abs(yawdelta) <= abs(max_delta); yawdelta += max_delta / divisor) {

						c_usercmd predictcmd = *cmd;
						if (round == 2) {
							this->crouched = true;
							predictcmd.buttons |= in_duck;
							ducktype = crouching;
							this->strafing = true;
							predictcmd.view_angles.y = originalangles.y;
							predictcmd.forward_move = originalfmove;
							predictcmd.side_move = predictcmd.view_angles.y > 0 ? originalsmove * 450.f : originalsmove * -450.f;
							predictcmd.up_move = originalupmove;
						}
						else if (round == 3) {
							this->crouched = false;
							predictcmd.buttons &= ~in_duck;
							ducktype = standing;
							this->strafing = true;
							predictcmd.view_angles.y = originalangles.y;
							predictcmd.forward_move = originalfmove;
							predictcmd.side_move = originalsmove * 450.f;
							predictcmd.up_move = originalupmove;
						}

						prediction::restore_ent_to_predicted_frame(cmd_pred);

						for (int t = 1; t <= ticklimit; t++) {
							if (round > 1)
								predictcmd.view_angles.y = math::normalize_yaw(originalangles.y + (yawdelta * t));

							if (c::movement::advanced_radius == 0 || c::movement::edge_bug_angle_limit == 0)
								break;

							if (abs(predictcmd.view_angles.y - this->startingyaw) > c::movement::enable_angle_limit ? c::movement::edgebug_angle_limit : 1)
								break;

							prediction::begin(&predictcmd);
							prediction::end();

							vec3_t current_vel = g::local->velocity();
							vec3_t current_pos = g::local->origin();

							// Enhanced velocity checks
							if (originalvelocity.z > 0.f || originalvelocity.length_2d() < velocity_threshold || prediction_backup::velocity.z > 0.f || prediction_backup::velocity.length_2d() < velocity_threshold ||
								g::local->move_type() == movetype_ladder)
								break;

							ebpos.push_back(current_pos);

							bool br = false;

							// More accurate edge detection with multiple angle checks
							if (this->check_edge_bug(&predictcmd, br)) {
								this->ticks_left = t;
								this->eblength = t;
								this->edgebugtick = interfaces::globals->tick_count + t;
								this->detecttick = interfaces::globals->tick_count;
								this->forwardmove = predictcmd.forward_move;
								this->sidemove = predictcmd.side_move;
								this->upmove = predictcmd.up_move;
								this->yawdelta = yawdelta;
								this->yaw = predictcmd.view_angles.y;
								this->backup_velocity = originalvel;
								this->backup_velocity.z = 0;
								this->mousedx = cmd->mouse_dx;
								break;
							}

							ebpos.push_back(current_pos);

							if (br)
								break;
						}
					}
				}
				else {
					c_usercmd predictcmd = *cmd;
					if (round == 0) {
						this->crouched = true;
						predictcmd.buttons |= in_duck;
						ducktype = crouching;
						this->strafing = false;
						predictcmd.forward_move = 0.f;
						predictcmd.side_move = 0.f;
						predictcmd.up_move = 0.f;
					}
					else if (round == 1) {
						this->crouched = false;
						predictcmd.buttons &= ~in_duck;
						ducktype = standing;
						this->strafing = false;
						predictcmd.forward_move = 0.f;
						predictcmd.side_move = 0.f;
						predictcmd.up_move = 0.f;
					}

					prediction::restore_ent_to_predicted_frame(cmd_pred);

					for (int t = 1; t <= ticklimit; ++t) {

						prediction::begin(&predictcmd);
						prediction::end();

						vec3_t current_vel = g::local->velocity();
						vec3_t current_pos = g::local->origin();

						// Enhanced velocity checks
						if (originalvelocity.z > 0.f || originalvelocity.length_2d() < velocity_threshold || prediction_backup::velocity.z > 0.f || prediction_backup::velocity.length_2d() < velocity_threshold ||
							g::local->move_type() == movetype_ladder)
							break;

						ebpos.push_back(current_pos);

						bool br = false;

						if (this->check_edge_bug(&predictcmd, br)) {
							this->ticks_left = t;
							this->eblength = t;
							this->edgebugtick = interfaces::globals->tick_count + t;
							this->detecttick = interfaces::globals->tick_count;
							this->forwardmove = predictcmd.forward_move;
							this->sidemove = predictcmd.side_move;
							this->upmove = predictcmd.up_move;
							this->yawdelta = yawdelta;
							this->backup_velocity = originalvel;
							this->backup_velocity.z = 0;
							this->mousedx = cmd->mouse_dx;
							break;
						}

						ebpos.push_back(current_pos);

						if (br)
							break;
					}
				}

				if (this->ticks_left)
					break;
			}
		}

		if (this->ticks_left) {
			if (this->ticks_left == 1) {
				if (c::movement::edge_bug_detection_printf) {
					interfaces::chat_element->chatprintf("#dna#_print_edgebugged");
				}
				detected_normal_edge_bug = true;
			}

			this->should_edge_bug = true;
			features::movement::should_edge_bug = true;

			if (this->crouched) {
				cmd->buttons |= in_bullrush;
				cmd->buttons |= in_duck;
			}
			else {
				cmd->buttons &= ~in_duck;
			}

			if (this->strafing) {
				cmd->forward_move = this->forwardmove;
				cmd->side_move = this->sidemove;
				cmd->mouse_dx = this->mousedx;
				vec3_t ang = vec3_t(originalangles.x + 18.f, math::normalize_yaw(startingyaw + (yawdelta * (eblength - (ticks_left - 1)))), 0);
				cmd->view_angles.y = ang.y;
				UtilityAssistant.handle_fix(cmd, ang);
			}
			else {
				cmd->forward_move = 0.f;
				cmd->side_move = 0.f;
				UtilityAssistant.handle_fix(cmd, originalangles);
			}

			this->ticks_left--;
		}
		else {
			should_edge_bug = false;
			features::movement::should_edge_bug = false;
			detected_normal_edge_bug = false;
			features::movement::detected_normal_edge_bug = false;
			this->is_double_eb = false;
			this->second_eb_tick = 0;
		}
	}

	void handle_visualize() {
		if (!g::local || !g::local->is_alive())
			return;

		if (!interfaces::engine->is_in_game() || !interfaces::engine->is_connected())
			return;

		static float alpha = 0.0f; // Current alpha

		float target_alpha = (ticks_left > 0) ? 1.0f : 0.0f;
		float lerp_speed = 8.0f; // Higher = faster, Lower = slower fade

		// Smoothly interpolate alpha towards target
		float frame_time = interfaces::globals->frame_time;
		alpha = alpha + (target_alpha - alpha) * lerp_speed * frame_time;

		if (c::movement::visualize_edge_bug && alpha > 0.0f) {

			// Draw path lines
			for (int i = 0; i < ebpos.size() - 1; i++) {
				vec3_t cur, next;
				if (interfaces::debug_overlay->world_to_screen(ebpos[i], cur) &&
					interfaces::debug_overlay->world_to_screen(ebpos[i + 1], next)) {

					im_render.drawline(
						cur.x, cur.y, next.x, next.y,
						color_t(
							c::movement::visualize_edge_bug_clr[0],
							c::movement::visualize_edge_bug_clr[1],
							c::movement::visualize_edge_bug_clr[2],
							alpha // float alpha 0.0f-1.0f
						)
					);
				}
			}

			// Draw square at last position
			vec3_t endpoints[4];
			if (interfaces::debug_overlay->world_to_screen(vec3_t{ ebpos.back().x - 16.f, ebpos.back().y - 16.f, ebpos.back().z }, endpoints[0]) &&
				interfaces::debug_overlay->world_to_screen(vec3_t{ ebpos.back().x - 16.f, ebpos.back().y + 16.f, ebpos.back().z }, endpoints[1]) &&
				interfaces::debug_overlay->world_to_screen(vec3_t{ ebpos.back().x + 16.f, ebpos.back().y - 16.f, ebpos.back().z }, endpoints[2]) &&
				interfaces::debug_overlay->world_to_screen(vec3_t{ ebpos.back().x + 16.f, ebpos.back().y + 16.f, ebpos.back().z }, endpoints[3]))
			{
				im_render.drawline(endpoints[0].x, endpoints[0].y, endpoints[1].x, endpoints[1].y,
					color_t(c::movement::visualize_edge_bug_clr[0], c::movement::visualize_edge_bug_clr[1], c::movement::visualize_edge_bug_clr[2], alpha));
				im_render.drawline(endpoints[1].x, endpoints[1].y, endpoints[3].x, endpoints[3].y,
					color_t(c::movement::visualize_edge_bug_clr[0], c::movement::visualize_edge_bug_clr[1], c::movement::visualize_edge_bug_clr[2], alpha));
				im_render.drawline(endpoints[3].x, endpoints[3].y, endpoints[2].x, endpoints[2].y,
					color_t(c::movement::visualize_edge_bug_clr[0], c::movement::visualize_edge_bug_clr[1], c::movement::visualize_edge_bug_clr[2], alpha));
				im_render.drawline(endpoints[2].x, endpoints[2].y, endpoints[0].x, endpoints[0].y,
					color_t(c::movement::visualize_edge_bug_clr[0], c::movement::visualize_edge_bug_clr[1], c::movement::visualize_edge_bug_clr[2], alpha));
			}
		}
	}

public:

	void pre(c_usercmd* cmd) {
		this->handle_pre_edgebug(cmd);
	}

	void run(c_usercmd* cmd) {
		this->handle_edgebug(cmd);
	}

	void draw() {
		this->handle_visualize();
	}
};
inline EdgebugAssist EdgebugAssistant;

class AlignmentAssist {
public:

	struct alignment_data_t {
		vec3_t velocity;
		vec3_t origin;
		vec3_t mins;
		vec3_t maxs;
		vec3_t predicted_origin;
		vec3_t start;
		vec3_t end;
		vec3_t angles;
		vec3_t wall_normal;
		float height_amount;
		float gravity;
		float forward;
		float side;
		float up;
		bool should_predict;
		bool has_predicted;
		bool is_bugging;
		bool is_aligned;
		bool should_ps_standing;
		bool wall_detected;
		bool test;
	};
	alignment_data_t alignment_data;

private:
	void handle_pre_alignment(c_usercmd* cmd) {
		if (!c::movement::auto_align) {
			return;
		}

		if (!g::local || !g::local->is_alive() || g::local->flags() & fl_onground) {
			return;
		}

		if (const auto mt = g::local->move_type(); mt == movetype_ladder || mt == movetype_noclip) {
			return;
		}

		float max_radias = m_pi * 2.f;
		float step = max_radias / 16.f;
		vec3_t start_pos = g::local->abs_origin();
		const auto mins = g::local->collideable()->mins();
		const auto maxs = g::local->collideable()->maxs();
		trace_world_only fil;

		for (float a = 0.f; a < max_radias; a += step) {
			vec3_t end_pos;
			end_pos.x = cos(a) + start_pos.x;
			end_pos.y = sin(a) + start_pos.y;
			end_pos.z = start_pos.z;
			trace_t trace;
			ray_t ray;
			ray.initialize(start_pos, end_pos, mins, maxs);

			// traceraying the whole circle around us trying to find a wall
			interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &fil, &trace);
			if ((trace.flFraction < 1.f) && (trace.plane.normal.z == 0.f)) {
				// found a wall
				//interfaces::console->console_printf(std::string("wall").append(" \n").c_str());
				vec3_t angles{ trace.plane.normal.x * -0.005f, trace.plane.normal.y * -0.005f, 0.f };
				const vec3_t end_pos2 = start_pos + angles;
				trace_t trace2;
				// traceraying towards the wall we found to see if we're aligned or not
				ray_t ray2;
				ray2.initialize(start_pos, end_pos2, mins, maxs);
				interfaces::trace_ray->trace_ray(ray2, MASK_PLAYERSOLID, &fil, &trace2);
				if (trace2.flFraction == 1.f) {
					vec3_t to_wall = angles.to_angle();
					to_wall.normalize();
					vec3_t velo = g::local->velocity();
					velo.z = 0.f;
					// checking if your velocity if not zero
					if (velo.length_2d() > 0.f) {
						vec3_t velo_ang = velo.to_angle();
						vec3_t delta = velo_ang - to_wall;
						delta.normalize();
						// comparing our velocity with a vector towards the wall to see if we actually want to align
						if (fabsf(delta.z) > 90.f)
							break;
					}
					// calculating the direction towards the wall to forward and sidemove
					float rotation = math::deg2rad(to_wall.y - cmd->view_angles.y);
					float cos_rot = cos(rotation);
					float sin_rot = sin(rotation);
					bool should_align = false;

					for (int i = 1; i <= 5; i++) {
						// try different forward and sidemove variations to find one that alignes in 1 tick
						float forwardmove = cos_rot * i * 9;
						float sidemove = -sin_rot * i * 9;
						c_usercmd fakecmd = *cmd;
						fakecmd.forward_move = forwardmove;
						fakecmd.side_move = sidemove;
						prediction::begin(&fakecmd);
						prediction::end();

						vec3_t start_pos2 = g::local->abs_origin();
						const vec3_t end_pos3 = start_pos2 + angles;
						trace_t trace3;
						ray_t ray3;
						ray3.initialize(start_pos2, end_pos3, mins, maxs);
						interfaces::trace_ray->trace_ray(ray3, MASK_PLAYERSOLID, &fil, &trace3);
						if (trace3.flFraction < 1.f) {
							cmd->forward_move = forwardmove;
							cmd->side_move = sidemove;
							should_align = true;
							prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
							break;
						}
						prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
					}
					if (!should_align) {
						for (int i = 1; i <= 5; i++) {
							// same shit here but in 2 ticks bc it might not be able to align in one
							float forwardmove = cos_rot * i * 9;
							float sidemove = -sin_rot * i * 9;
							vec3_t absorigin = g::local->abs_origin();
							c_usercmd fakecmd2 = *cmd;
							fakecmd2.forward_move = forwardmove;
							fakecmd2.side_move = sidemove;
							prediction::begin(&fakecmd2);
							prediction::end();
							prediction::begin(&fakecmd2);
							prediction::end();

							vec3_t start_pos2 = g::local->abs_origin();
							start_pos2.z = absorigin.z;
							const vec3_t end_pos3 = start_pos2 + angles;
							trace_t trace3;
							ray_t ray4;
							ray4.initialize(start_pos2, end_pos3, mins, maxs);
							interfaces::trace_ray->trace_ray(ray4, MASK_PLAYERSOLID, &fil, &trace3);
							if (trace3.flFraction < 1.f) {
								cmd->forward_move = forwardmove;
								cmd->side_move = sidemove;
								should_align = true;
								prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
								break;
							}
							prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
						}
					}
				}
				else {
					// this part of code works if we are already aligned
					for (int i = 45.f; i > 0.f; i -= 5.f) {
						// finding good forward and sidemove values to keep us stable on the surf
						float forwardmove = 0.f;
						float sidemove = 0.f;
						// direct it towards the direction we're pressing
						if (cmd->buttons & in_forward)
							forwardmove = i;
						if (cmd->buttons & in_back)
							forwardmove = -i;
						if (cmd->buttons & in_moveleft)
							sidemove = -i;
						if (cmd->buttons & in_moveright)
							sidemove = i;

						if (!forwardmove && !sidemove && c::movehelp::free_angle
							&& menu::checkkey(c::movement::pixel_surf_key, c::movement::pixel_surf_key_s)) {
							// OR direct it towards the wall when we're not pressing anything and freelook is enabled
							vec3_t to_wall = angles.to_angle();
							to_wall.normalize();
							float rotation = math::deg2rad(to_wall.y - cmd->view_angles.y);
							float cos_rot = cos(rotation);
							float sin_rot = sin(rotation);
							forwardmove = cos_rot * i;
							sidemove = -sin_rot * i;
						}

						// checking if the value is good to keep stable and using it
						c_usercmd fakecmd5 = *cmd;
						fakecmd5.forward_move = forwardmove;
						fakecmd5.side_move = sidemove;
						prediction::begin(&fakecmd5);
						prediction::end();

						float zvelo = g::local->velocity().z;
						if (zvelo == -6.25f) {
							prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
							cmd->forward_move = forwardmove;
							cmd->side_move = sidemove;
							break;
						}
						prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
					}

					if (!features::movement::should_ps) {
						prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);

						// Calculate target Z velocity for pixelsurf/bounce detection
						float sv_gravity = interfaces::console->get_convar("sv_gravity")->get_float();
						float tick_rate = (tick_interval > 0.f) ? (1.f / tick_interval) : 0.f;
						float target_z_velo = ((sv_gravity / 2.f) / tick_rate) * -1.f;
						int cmd_pred = interfaces::prediction->split->commands_predicted - 1;

						// Use the current wall's normal for scanning
						vec3_t scan_wall_normal = trace.plane.normal;
						vec3_t wall_angles{ scan_wall_normal.x * -1.f, scan_wall_normal.y * -1.f, 0.f };
						vec3_t to_wall_angle = wall_angles.to_angle();
						to_wall_angle.normalize();

						// BOUNCE/PIXELSURF SCAN with freelook and headbounce support
						for (int scan_attempt = 0; scan_attempt < 3; scan_attempt++) {
							prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
							for (int move_mult = 1; move_mult <= 5; move_mult++) {
								// Test different movement intensities (for freelook input-based control)
								float forwardmove = 0.f;
								float sidemove = 0.f;

								// Direct movement based on button inputs (allows moving off surf)
								if (cmd->buttons & in_forward)
									forwardmove = move_mult;
								if (cmd->buttons & in_back)
									forwardmove = -move_mult;
								if (cmd->buttons & in_moveleft)
									sidemove = -move_mult;
								if (cmd->buttons & in_moveright)
									sidemove = move_mult;

								// Freelook mode: auto-align to wall when no input
								if (!forwardmove && !sidemove && c::movehelp::free_angle
									&& menu::checkkey(c::movement::pixel_surf_key, c::movement::pixel_surf_key_s)) {
									float rotation = math::deg2rad(to_wall_angle.y - cmd->view_angles.y);
									float cos_rot = cos(rotation);
									float sin_rot = sin(rotation);
									forwardmove = cos_rot * move_mult;
									sidemove = -sin_rot * move_mult;
								}

								// If we have input-based movement, test it
								if (forwardmove != 0.f || sidemove != 0.f) {
									c_usercmd test_cmd = *cmd;
									test_cmd.forward_move = forwardmove;
									test_cmd.side_move = sidemove;
									test_cmd.buttons |= in_jump;
									test_cmd.buttons |= in_duck;

									prediction::begin(&test_cmd);
									prediction::end();

									vec3_t pred_velo = g::local->velocity();
									vec3_t pred_origin = g::local->abs_origin();

									// Check if we hit pixelsurf velocity
									if (pred_velo.z == target_z_velo) {
										trace_t verify_trace;
										ray_t verify_ray;
										verify_ray.initialize(pred_origin, pred_origin + (scan_wall_normal * -5.f), mins, maxs);
										interfaces::trace_ray->trace_ray(verify_ray, MASK_PLAYERSOLID, &fil, &verify_trace);

										if (verify_trace.flFraction < 1.f && verify_trace.plane.normal.z == 0.f) {
											prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
											cmd->forward_move = forwardmove;
											cmd->side_move = sidemove;
											cmd->buttons |= in_jump;
											cmd->buttons |= in_duck;
											goto found_bounce;
										}
									}
									prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
								}
								prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
							}
							prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
						found_bounce:;
						}
						break;
					}
				}
				break;
			}
		}
	}

	void handle_bounce_alignment(c_usercmd* cmd) {
		//if (features::movement::should_ps) {
		//	prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);

		//	// Calculate target Z velocity for pixelsurf/bounce detection
		//	float sv_gravity = interfaces::console->get_convar("sv_gravity")->get_float();
		//	float tick_rate = (tick_interval > 0.f) ? (1.f / tick_interval) : 0.f;
		//	float target_z_velo = ((sv_gravity / 2.f) / tick_rate) * -1.f;
		//	int cmd_pred = interfaces::prediction->split->commands_predicted - 1;

		//	// Use the current wall's normal for scanning
		//	vec3_t scan_wall_normal = trace.plane.normal;
		//	vec3_t wall_angles{ scan_wall_normal.x * -1.f, scan_wall_normal.y * -1.f, 0.f };
		//	vec3_t to_wall_angle = wall_angles.to_angle();
		//	to_wall_angle.normalize();

		//	// BOUNCE/PIXELSURF SCAN with freelook and headbounce support
		//	for (int scan_attempt = 0; scan_attempt < 3; scan_attempt++) {
		//		prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
		//		for (int move_mult = 1; move_mult <= 5; move_mult++) {
		//			// Test different movement intensities (for freelook input-based control)
		//			float forwardmove = 0.f;
		//			float sidemove = 0.f;

		//			// Direct movement based on button inputs (allows moving off surf)
		//			if (cmd->buttons & in_forward)
		//				forwardmove = move_mult;
		//			if (cmd->buttons & in_back)
		//				forwardmove = -move_mult;
		//			if (cmd->buttons & in_moveleft)
		//				sidemove = -move_mult;
		//			if (cmd->buttons & in_moveright)
		//				sidemove = move_mult;

		//			// Freelook mode: auto-align to wall when no input
		//			if (!forwardmove && !sidemove && c::movehelp::free_angle
		//				&& menu::checkkey(c::movement::pixel_surf_key, c::movement::pixel_surf_key_s)) {
		//				float rotation = math::deg2rad(to_wall_angle.y - cmd->view_angles.y);
		//				float cos_rot = cos(rotation);
		//				float sin_rot = sin(rotation);
		//				forwardmove = cos_rot * move_mult;
		//				sidemove = -sin_rot * move_mult;
		//			}

		//			// If we have input-based movement, test it
		//			if (forwardmove != 0.f || sidemove != 0.f) {
		//				c_usercmd test_cmd = *cmd;
		//				test_cmd.forward_move = forwardmove;
		//				test_cmd.side_move = sidemove;
		//				test_cmd.buttons |= in_jump;
		//				test_cmd.buttons |= in_duck;

		//				prediction::begin(&test_cmd);
		//				prediction::end();

		//				vec3_t pred_velo = g::local->velocity();
		//				vec3_t pred_origin = g::local->abs_origin();

		//				// Check if we hit pixelsurf velocity
		//				if (pred_velo.z == target_z_velo) {
		//					trace_t verify_trace;
		//					ray_t verify_ray;
		//					verify_ray.initialize(pred_origin, pred_origin + (scan_wall_normal * -5.f), mins, maxs);
		//					interfaces::trace_ray->trace_ray(verify_ray, MASK_PLAYERSOLID, &fil, &verify_trace);

		//					if (verify_trace.flFraction < 1.f && verify_trace.plane.normal.z == 0.f) {
		//						prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
		//						cmd->forward_move = forwardmove;
		//						cmd->side_move = sidemove;
		//						cmd->buttons |= in_jump;
		//						cmd->buttons |= in_duck;
		//						goto found_bounce;
		//					}
		//				}
		//				prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
		//			}
		//			prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
		//		}
		//		prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
		//	found_bounce:;
		//	}
		//	break;
		//}
	}

	// Separate function to find and align to displacements in walls
	void handle_displacement_alignment(c_usercmd* cmd) {
		if (!c::movement::auto_align)
			return;

		if (!g::local || !g::local->is_alive() || g::local->flags() & fl_onground) {
			return;
		}

		const auto mt = g::local->move_type();
		if (mt == movetype_ladder || mt == movetype_noclip) {
			return;
		}

		const float max_radias = m_pi * 2.f;
		const float step = max_radias / 16.f; // More samples for displacements
		vec3_t start_pos = g::local->abs_origin();
		const auto mins = g::local->collideable()->mins();
		const auto maxs = g::local->collideable()->maxs();
		trace_world_only fil;

		// First pass: Find displacement walls
		for (float a = 0.f; a < max_radias; a += step) {
			vec3_t end_pos{ start_pos.x + cos(a), start_pos.y + sin(a), start_pos.z };
			trace_t trace;
			ray_t ray;
			ray.initialize(start_pos, end_pos, mins, maxs);

			interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &fil, &trace);

			// Check if it's a displacement wall (vertical surface + displacement name)
			bool is_displacement = false;
			if (trace.surface.name) {
				is_displacement = (strstr(trace.surface.name, "displacement") != nullptr);
			}

			if (trace.flFraction < 1.f && trace.plane.normal.z == 0.f && is_displacement) {
				// Found a displacement wall
				vec3_t displacement_normal = trace.plane.normal;
				vec3_t displacement_pos = trace.end;

				// Calculate alignment direction (perpendicular to wall normal)
				vec3_t align_dir{ displacement_normal.x * -0.005f, displacement_normal.y * -0.005f, 0.f };
				vec3_t align_dir2{ displacement_normal.x * -0.01f, displacement_normal.y * -0.01f, 0.f };

				// Test if we can move into alignment
				vec3_t test_pos = start_pos + align_dir;
				trace_t align_trace;
				ray_t align_ray;
				align_ray.initialize(start_pos, test_pos, mins, maxs);
				interfaces::trace_ray->trace_ray(align_ray, MASK_PLAYERSOLID, &fil, &align_trace);

				if (align_trace.flFraction == 1.f) {
					// Calculate movement towards displacement
					vec3_t to_displacement = align_dir.to_angle();
					to_displacement.normalize();

					// Check velocity direction compatibility
					vec3_t velo = g::local->velocity();
					velo.z = 0.f;

					if (velo.length_2d() > 0.f) {
						vec3_t velo_ang = velo.to_angle();
						vec3_t delta = velo_ang - to_displacement;
						delta.normalize();

						// Skip if moving away from displacement
						if (fabsf(delta.z) > 90.f)
							continue;
					}

					float rotation = math::deg2rad(to_displacement.y - cmd->view_angles.y);
					float cos_rot = cos(rotation);
					float sin_rot = sin(rotation);

					// Backup state for prediction
					vec3_t backup_origin = g::local->origin();
					vec3_t backup_velocity = g::local->velocity();
					float backup_stamina = g::local->stamina();
					float backup_sim = g::local->simulation_time();
					int backup_predicted = interfaces::prediction->split->commands_predicted;

					bool aligned = false;

					// === ATTEMPT 1: Single tick alignment ===
					for (int i = 1; i <= 8; i++) {
						float forwardmove = cos_rot * i * 9.f; // Slightly stronger for displacements
						float sidemove = -sin_rot * i * 9.f;

						c_usercmd fakecmd = *cmd;
						fakecmd.forward_move = forwardmove;
						fakecmd.side_move = sidemove;

						prediction::begin(&fakecmd);
						prediction::end();

						vec3_t predicted_pos = g::local->abs_origin();
						vec3_t test_end = predicted_pos + align_dir;
						trace_t test_trace;
						ray_t test_ray;
						test_ray.initialize(predicted_pos, test_end, mins, maxs);
						interfaces::trace_ray->trace_ray(test_ray, MASK_PLAYERSOLID, &fil, &test_trace);

						// Check if we hit displacement
						bool hit_displacement = false;
						if (test_trace.surface.name) {
							hit_displacement = (strstr(test_trace.surface.name, "displacement") != nullptr);
						}

						if (test_trace.flFraction < 1.f && hit_displacement) {
							g::local->origin() = backup_origin;
							g::local->velocity() = backup_velocity;
							g::local->stamina() = backup_stamina;
							g::local->simulation_time() = backup_sim;
							prediction::restore_ent_to_predicted_frame(backup_predicted - 1);

							cmd->forward_move = forwardmove;
							cmd->side_move = sidemove;
							aligned = true;
							break;
						}

						g::local->origin() = backup_origin;
						g::local->velocity() = backup_velocity;
						g::local->stamina() = backup_stamina;
						g::local->simulation_time() = backup_sim;
						prediction::restore_ent_to_predicted_frame(backup_predicted - 1);
					}

					// === ATTEMPT 2: Two tick alignment ===
					if (!aligned) {
						for (int i = 1; i <= 8; i++) {
							float forwardmove = cos_rot * i * 9.f;
							float sidemove = -sin_rot * i * 9.f;

							c_usercmd fakecmd2 = *cmd;
							fakecmd2.forward_move = forwardmove;
							fakecmd2.side_move = sidemove;

							// Run 2 ticks
							prediction::begin(&fakecmd2);
							prediction::end();
							prediction::begin(&fakecmd2);
							prediction::end();

							vec3_t predicted_pos = g::local->abs_origin();
							vec3_t test_end = predicted_pos + align_dir2;
							trace_t test_trace;
							ray_t test_ray;
							test_ray.initialize(predicted_pos, test_end, mins, maxs);
							interfaces::trace_ray->trace_ray(test_ray, MASK_PLAYERSOLID, &fil, &test_trace);

							bool hit_displacement = false;
							if (test_trace.surface.name) {
								hit_displacement = (strstr(test_trace.surface.name, "displacement") != nullptr);
							}

							if (test_trace.flFraction < 1.f && hit_displacement) {
								g::local->origin() = backup_origin;
								g::local->velocity() = backup_velocity;
								g::local->stamina() = backup_stamina;
								g::local->simulation_time() = backup_sim;
								prediction::restore_ent_to_predicted_frame(backup_predicted - 1);

								cmd->forward_move = forwardmove;
								cmd->side_move = sidemove;
								aligned = true;
								break;
							}

							g::local->origin() = backup_origin;
							g::local->velocity() = backup_velocity;
							g::local->stamina() = backup_stamina;
							g::local->simulation_time() = backup_sim;
							prediction::restore_ent_to_predicted_frame(backup_predicted - 1);
						}
					}

					// === ATTEMPT 3: Detect surf/headbang on displacements ===
					if (!aligned) {
						for (int i = 1; i <= 8; i++) {
							float forwardmove = cos_rot * i * 9.f;
							float sidemove = -sin_rot * i * 9.f;

							c_usercmd fakecmd3 = *cmd;
							fakecmd3.forward_move = forwardmove;
							fakecmd3.side_move = sidemove;

							prediction::begin(&fakecmd3);
							prediction::end();

							vec3_t predicted_pos = g::local->abs_origin();

							// Check for pixelsurf on displacement
							vec3_t prevel = backup_velocity;
							vec3_t postvel = g::local->velocity();

							// Detect pixelsurf velocity change
							if ((prevel.z == -6.25f && postvel.z != -6.25f) ||
								(prevel.z != -6.25f && postvel.z == -6.25f)) {

								// Verify it's a displacement
								ray_t verify_ray;
								trace_t verify_trace;
								vec3_t verify_start = predicted_pos;
								vec3_t verify_end = predicted_pos + vec3_t(0.f, 0.f, -16.f);
								verify_ray.initialize(verify_start, verify_end);
								interfaces::trace_ray->trace_ray(verify_ray, MASK_PLAYERSOLID, &fil, &verify_trace);

								bool is_displacement_surf = false;
								if (verify_trace.surface.name) {
									is_displacement_surf = (strstr(verify_trace.surface.name, "displacement") != nullptr);
								}

								if (is_displacement_surf) {
									g::local->origin() = backup_origin;
									g::local->velocity() = backup_velocity;
									g::local->stamina() = backup_stamina;
									g::local->simulation_time() = backup_sim;
									prediction::restore_ent_to_predicted_frame(backup_predicted - 1);

									cmd->forward_move = forwardmove;
									cmd->side_move = sidemove;
									aligned = true;
									break;
								}
							}

							// Check for headbang on displacement
							ray_t head_ray;
							trace_t head_trace;
							vec3_t head_start = predicted_pos + vec3_t(0.f, 0.f, 16.f);
							vec3_t head_end = predicted_pos + vec3_t(0.f, 0.f, 36.f);
							head_ray.initialize(head_start, head_end);
							interfaces::trace_ray->trace_ray(head_ray, MASK_PLAYERSOLID, &fil, &head_trace);

							bool is_displacement_ceiling = false;
							if (head_trace.surface.name) {
								is_displacement_ceiling = (strstr(head_trace.surface.name, "displacement") != nullptr);
							}

							if (head_trace.flFraction < 0.65f && is_displacement_ceiling) {
								g::local->origin() = backup_origin;
								g::local->velocity() = backup_velocity;
								g::local->stamina() = backup_stamina;
								g::local->simulation_time() = backup_sim;
								prediction::restore_ent_to_predicted_frame(backup_predicted - 1);

								cmd->forward_move = forwardmove;
								cmd->side_move = sidemove;
								aligned = true;
								break;
							}

							// Check for bounce on displacement
							ray_t bounce_ray;
							trace_t bounce_trace;
							vec3_t bounce_start = predicted_pos + vec3_t(16.f, 16.f, 0.f);
							vec3_t bounce_end = bounce_start + vec3_t(-16.f, -16.f, -72.f);
							bounce_ray.initialize(bounce_start, bounce_end);
							interfaces::trace_ray->trace_ray(bounce_ray, MASK_PLAYERSOLID, &fil, &bounce_trace);

							bool is_displacement_bounce = false;
							if (bounce_trace.surface.name) {
								is_displacement_bounce = (strstr(bounce_trace.surface.name, "displacement") != nullptr);
							}

							if (bounce_trace.flFraction < 0.65f && is_displacement_bounce) {
								g::local->origin() = backup_origin;
								g::local->velocity() = backup_velocity;
								g::local->stamina() = backup_stamina;
								g::local->simulation_time() = backup_sim;
								prediction::restore_ent_to_predicted_frame(backup_predicted - 1);

								cmd->forward_move = forwardmove;
								cmd->side_move = sidemove;
								aligned = true;
								break;
							}

							g::local->origin() = backup_origin;
							g::local->velocity() = backup_velocity;
							g::local->stamina() = backup_stamina;
							g::local->simulation_time() = backup_sim;
							prediction::restore_ent_to_predicted_frame(backup_predicted - 1);
						}
					}

					// === ATTEMPT 4: Input-based displacement alignment ===
					if (!aligned && (cmd->buttons & (in_forward | in_back | in_moveleft | in_moveright))) {
						for (int i = 45; i > 0; i -= 5) {
							float forwardmove = 0.f;
							float sidemove = 0.f;

							if (cmd->buttons & in_forward)  forwardmove = i;
							if (cmd->buttons & in_back)     forwardmove = -i;
							if (cmd->buttons & in_moveleft) sidemove = -i;
							if (cmd->buttons & in_moveright) sidemove = i;

							c_usercmd fakecmd5 = *cmd;
							fakecmd5.forward_move = forwardmove;
							fakecmd5.side_move = sidemove;

							prediction::begin(&fakecmd5);
							prediction::end();

							vec3_t post_vel = g::local->velocity();

							// Check if we hit pixelsurf on displacement
							if (post_vel.z == -6.25f) {
								// Verify displacement
								ray_t verify_ray;
								trace_t verify_trace;
								vec3_t verify_start = g::local->abs_origin();
								vec3_t verify_end = verify_start + vec3_t(0.f, 0.f, -16.f);
								verify_ray.initialize(verify_start, verify_end);
								interfaces::trace_ray->trace_ray(verify_ray, MASK_PLAYERSOLID, &fil, &verify_trace);

								bool is_displacement_surf = false;
								if (verify_trace.surface.name) {
									is_displacement_surf = (strstr(verify_trace.surface.name, "displacement") != nullptr);
								}

								if (is_displacement_surf) {
									g::local->origin() = backup_origin;
									g::local->velocity() = backup_velocity;
									g::local->stamina() = backup_stamina;
									g::local->simulation_time() = backup_sim;
									prediction::restore_ent_to_predicted_frame(backup_predicted - 1);

									cmd->forward_move = forwardmove;
									cmd->side_move = sidemove;
									aligned = true;
									break;
								}
							}

							g::local->origin() = backup_origin;
							g::local->velocity() = backup_velocity;
							g::local->stamina() = backup_stamina;
							g::local->simulation_time() = backup_sim;
							prediction::restore_ent_to_predicted_frame(backup_predicted - 1);
						}
					}

					// If aligned to displacement, exit (only align to one displacement per tick)
					if (aligned) {
						return;
					}
				}
			}
		}
	}

	float difference(float a, float b)
	{
		return std::max<float>(abs(a), abs(b)) - std::min<float>(abs(a), abs(b));
	}

	void auto_align(c_usercmd* cmd) {
	if (const auto mt = g::local->move_type(); mt == movetype_ladder || mt == movetype_noclip) {
		this->alignment_data.should_ps_standing = false;
		return;
	}

	if (!c::movement::auto_align) {
		return;
	}

	if (!g::local || !g::local->is_alive()) {
		return;
	}

	if (c::movement::airstuck_enabled && menu::checkkey(c::movement::airstuck_key, c::movement::airstuck_key_s)) {
		return;
	}

	float max_radias = m_pi * 2.f;
	float step = max_radias / 16.f;
	vec3_t start_pos = g::local->abs_origin();
	const auto mins = g::local->collideable()->mins();
	const auto maxs = g::local->collideable()->maxs();
	trace_world_only fil;

	float sv_gravity = interfaces::console->get_convar(("sv_gravity"))->get_float();
	float fTickInterval = interfaces::globals->interval_per_tick;
	float fTickRate = (fTickInterval > 0) ? (1.0f / fTickInterval) : 0.0f;
	float targetZvelo = ((sv_gravity / 2) / fTickRate) * -1.f;

	static float start_circle = 0.f;
	this->alignment_data.wall_detected = false;
	trace_t trace;
	vec3_t save_start_pos = { };
	vec3_t save_end_pos = { };
	float saved_cos = 0.f;
	float saved_sin = 0.f;
	for (float a = start_circle; a < max_radias; a += step) {
		vec3_t end_pos;
		end_pos.x = cos(a) + start_pos.x;
		end_pos.y = sin(a) + start_pos.y;
		end_pos.z = start_pos.z;
		saved_cos = cos(a) * 64.f;
		saved_sin = sin(a) * 64.f;
		ray_t ray;
		ray.initialize(start_pos, end_pos, mins, maxs);

		// traceraying the whole circle around us trying to find a wall
		interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &fil, &trace);
		if (trace.flFraction != 1.f && trace.plane.normal.z == 0.f) {
			this->alignment_data.wall_detected = true;
			start_circle = a;
			save_start_pos = start_pos;
			save_end_pos = end_pos;
			break;
		}
	}
	if (!this->alignment_data.wall_detected) {
		start_circle = 0.f;
		return;
	}

	//this part is only needed to avoid being "pushed" to the wall while not strafing to the wall
	//better recode it to find "shape" of the wall
	this->alignment_data.test = false;
	vec3_t angles{ trace.plane.normal.x * -0.005f, trace.plane.normal.y * -0.005f, 0.f };
	const vec3_t end_pos2 = start_pos + angles;
	trace_t trace228;
	ray_t ray228;
	ray228.initialize(start_pos, end_pos2, mins, maxs);
	interfaces::trace_ray->trace_ray(ray228, MASK_PLAYERSOLID, &fil, &trace228);

	if (trace228.flFraction == 1.f) {
		vec3_t to_wall = angles.to_angle();
		to_wall.normalize();
		vec3_t velo = g::local->velocity();
		velo.z = 0.f;

		if (velo.length_2d() > 0.f) {
			vec3_t velo_ang = velo.to_angle();
			vec3_t delta = velo_ang - to_wall;
			delta.normalize();
			// comparing our velocity with a vector towards the wall to see if we actually want to align
			//u can do 90, but it will fail on some spots
			if (fabsf(delta.y) > 92.5f) {
				this->alignment_data.test = true;
			}
		}
	}

	if (this->alignment_data.test) {
		return;
	}

	vec3_t wall_angle = angles.to_angle();
	wall_angle.normalize();

	const vec3_t end_pos2_lb = vec3_t(save_start_pos.x, save_start_pos.y, trace.end.z) + vec3_t(saved_cos, saved_sin, 0.f);
	float rotation = deg2rad(wall_angle.y - cmd->view_angles.y);
	float cos_rot = cos(rotation);
	float sin_rot = sin(rotation);
	bool detect = false;
	trace_t trace_2;
	ray_t ray_2;
	ray_2.initialize(vec3_t(save_start_pos.x, save_start_pos.y, trace.end.z), end_pos2_lb);
	interfaces::trace_ray->trace_ray(ray_2, MASK_PLAYERSOLID, &fil, &trace_2);

	float backup_forward_move = cmd->forward_move;
	float backup_side_move = cmd->side_move;

	//cp from og align
	for (float multiplayer = 10.f; multiplayer < 100.f; multiplayer += 10.f) {
		prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);

		float forwardmove = cos_rot * multiplayer;
		float sidemove = -sin_rot * multiplayer;
		cmd->forward_move = forwardmove;
		cmd->side_move = sidemove;
		float backup_zspeed = g::local->get_velocity().z;

		prediction::begin(cmd);
		prediction::end();

		//we dont need to waste time on this shit
		if (g::local->flags() & fl_onground || g::local->move_type() & movetype_ladder) {
			continue;
		}

		float new_zspeed = g::local->get_velocity().z;
		if (new_zspeed == targetZvelo) {
			cmd->forward_move = forwardmove;
			cmd->side_move = sidemove;
			detect = true;
			break;
		}
	}

	//part from delusional
	if (!detect) {
		for (int i = 1; i <= 5; i++) {
			// try different forward and sidemove variations to find one that alignes in 1 tick
			prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
			float forwardmove = cos_rot * i * 9;
			float sidemove = -sin_rot * i * 9;

			for (int seph = 0; seph < 10; seph++) {
				c_usercmd fakecmd = *cmd;
				fakecmd.forward_move = forwardmove;
				fakecmd.side_move = sidemove;
				prediction::begin(&fakecmd);
				prediction::end();

				if (g::local->move_type() & movetype_ladder) {
					continue;
				}

				vec3_t start_pos2 = g::local->abs_origin();
				const vec3_t end_pos3 = start_pos2 + angles;
				trace_t trace3;
				ray_t ray3;
				ray3.initialize(start_pos2, end_pos3, mins, maxs);
				interfaces::trace_ray->trace_ray(ray3, MASK_PLAYERSOLID, &fil, &trace3);
				if (trace3.flFraction < 1.f) {
					cmd->forward_move = forwardmove;
					cmd->side_move = sidemove;
					detect = true;
					break;
				}
			}
		}
	}

	//added buttons as a condition to fix that "sticky" effect when using align
	//TODO: RECODE ALL THIS SHIT
	if (prediction_backup::velocity.z == targetZvelo || (cmd->buttons & in_forward) || (cmd->buttons & in_back) || (cmd->buttons & in_moveleft) || (cmd->buttons & in_moveright)) {
		vec3_t wishdir;

		bool done = false;

		if ((cmd->buttons & in_forward) && !(cmd->buttons & in_back) && !(cmd->buttons & in_moveleft) &&
			!(cmd->buttons & in_moveright)) {
			wishdir = { std::cos(deg2rad(cmd->view_angles.y)) * 128.f, std::sin(deg2rad(cmd->view_angles.y)) * 128.f, 0.f };
			done = true;
		}
		if ((cmd->buttons & in_back) && !(cmd->buttons & in_forward) && !(cmd->buttons & in_moveleft) &&
			!(cmd->buttons & in_moveright)) {
			wishdir = { std::cos(deg2rad(cmd->view_angles.y + 180.f)) * 128.f, std::sin(deg2rad(cmd->view_angles.y + 180.f)) * 128.f,
						0.f };
			done = true;
		}
		if ((cmd->buttons & in_moveleft) && !(cmd->buttons & in_back) && !(cmd->buttons & in_forward) &&
			!(cmd->buttons & in_moveright)) {
			wishdir = { std::cos(deg2rad(cmd->view_angles.y + 90.f)) * 128.f, std::sin(deg2rad(cmd->view_angles.y + 90.f)) * 128.f,
						0.f };
			done = true;
		}
		if ((cmd->buttons & in_moveright) && !(cmd->buttons & in_back) && !(cmd->buttons & in_moveleft) &&
			!(cmd->buttons & in_forward)) {
			wishdir = { std::cos(deg2rad(cmd->view_angles.y - 90.f)) * 128.f, std::sin(deg2rad(cmd->view_angles.y - 90.f)) * 128.f,
						0.f };
			done = true;
		}
		if (done) {
			trace_t trace_4;
			vec3_t st = vec3_t(save_start_pos.x, save_start_pos.y, trace.end.z);
			ray_t ray_4;
			ray_4.initialize(st, st + wishdir);
			interfaces::trace_ray->trace_ray(ray_4, MASK_PLAYERSOLID, &fil, &trace_4);
			if (trace_4.flFraction < 1.f) {
				int buttons_2 = cmd->buttons;
				float forwardmove_2 = cmd->forward_move;
				float sidemove_2 = cmd->side_move;
				int i_backup_velo = g::local->get_velocity().length_2d();
				bool do_surf_detected = false;

				if (prediction_backup::velocity.z == targetZvelo || g::local->velocity().z == targetZvelo) {
					if (prediction_backup::velocity.length_2d() != 0.f && g::local->velocity().length_2d() != 0.f) {

						//better than og impl imO
						for (int i = 450; i > 0; i -= 45) {
							prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
							if (buttons_2 & in_forward)
								cmd->forward_move = i;
							if (buttons_2 & in_back)
								cmd->forward_move = -i;
							if (buttons_2 & in_moveleft)
								cmd->side_move = -i;
							if (buttons_2 & in_moveright)
								cmd->side_move = i;

							prediction::begin(cmd);
							prediction::end();

							float zvelo = g::local->get_velocity().z;
							if (zvelo == targetZvelo) {
								forwardmove_2 = cmd->forward_move;
								sidemove_2 = cmd->side_move;
								break;
							}
						}

						int i_preed_velo = g::local->get_velocity().length_2d();

						cmd->forward_move = forwardmove_2;
						cmd->side_move = sidemove_2;

						//finding the best angle to accelerate if not alr
						//tbh u should run it be4 first prediction and then compare
						std::vector< vec2_t > direction{ };
						std::vector< float > max_speed{ };
						if (difference(i_backup_velo, i_preed_velo) < 5) {
							for (float angle = 15.f; angle < 30.f; angle += 5.f) {
								prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
								float mVel = hypotf(prediction_backup::velocity.x, prediction_backup::velocity.y);
								float ideal = math::rad2deg(atanf(angle / mVel));
								vec3_t dvelo = g::local->get_velocity();
								dvelo.z = 0.f;
								vec3_t velo_angle = dvelo.to_angle();
								vec3_t delta =
									velo_angle - vec3_t(trace.plane.normal.x * -1.f, trace.plane.normal.y * -1.f, 0.f).to_angle();
								delta.normalize();
								if (delta.y >= 0.f)
									wall_angle.y += ideal;
								else
									wall_angle.y -= ideal;
								float rotation2 = deg2rad(wall_angle.y - cmd->view_angles.y);
								float cos_rot2 = cos(rotation2);
								float sin_rot2 = sin(rotation2);
								float forwardmove2 = cos_rot2 * 450.f;
								float sidemove2 = -sin_rot2 * 450.f;
								cmd->forward_move = forwardmove2;
								cmd->side_move = sidemove2;
								vec3_t b_velo = g::local->get_velocity();
								prediction::begin(cmd);
								prediction::end();
								vec3_t p_velo = g::local->get_velocity();
								if (b_velo.z == targetZvelo && p_velo.z == targetZvelo) {
									if (p_velo.length_2d() > b_velo.length_2d()) {
										direction.emplace_back(vec2_t(forwardmove2, sidemove2));
										max_speed.emplace_back(p_velo.length_2d() - b_velo.length_2d());
									}
								}
							}

							//backup
							cmd->forward_move = forwardmove_2;
							cmd->side_move = sidemove_2;

							//comapare
							//can be done a lot better tho
							float mxsp = 0;
							int index_max_speed = -1;
							for (int k = 0; k < max_speed.size(); k++) {
								if (max_speed.at(k) > mxsp) {
									index_max_speed = k;
									mxsp = max_speed.at(k);
								}
							}
							if (index_max_speed != -1) {
								cmd->forward_move = direction.at(index_max_speed).x;
								cmd->side_move = direction.at(index_max_speed).y;
							}
						}
					}
				}

				if (difference(trace_4.end.x, trace_2.end.x) > 1.f && difference(trace_4.end.y, trace_2.end.y) > 1.f) {
					if (cmd->forward_move < 0.f && cmd->buttons & in_forward)
						cmd->forward_move = 450.f;
					if (cmd->forward_move > 0.f && cmd->buttons & in_back)
						cmd->forward_move = -450.f;
					if (cmd->side_move < 0.f && cmd->buttons & in_moveright)
						cmd->side_move = 450.f;
					if (cmd->side_move > 0.f && cmd->buttons & in_moveleft)
						cmd->side_move = -450.f;
				}

			}
			else {
				if (cmd->forward_move < 0.f && cmd->buttons & in_forward)
					cmd->forward_move = 450.f;
				if (cmd->forward_move > 0.f && cmd->buttons & in_back)
					cmd->forward_move = -450.f;
				if (cmd->side_move < 0.f && cmd->buttons & in_moveright)
					cmd->side_move = 450.f;
				if (cmd->side_move > 0.f && cmd->buttons & in_moveleft)
					cmd->side_move = -450.f;
			}
		}
	}

	if (!c::movement::bhopfix) {
		prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
	}
}

public:

	void pre(c_usercmd* cmd) {
		this->handle_pre_alignment(cmd);
		//this->handle_displacement_alignment(cmd);
		//this->handle_pre_surf(cmd);
	}

	void run(c_usercmd* cmd) {
	}
};
inline AlignmentAssist AlignmentAssistant;

class PixelSurfAssist {
public:
	struct ps_data_t {
		// Pack bools into bitfield to reduce memory footprint
		uint8_t predicted_ps : 1;
		uint8_t pixelsurfing : 1;
		uint8_t pixelstuck : 1;
		uint8_t pixelducking : 1;
		uint8_t valid : 1;
		uint8_t was_ducking : 1;
		uint8_t _padding : 2;

		int counter = 0;
		int pixeltick = 0;
		int old_tick = 0;
		float last_set = 0.f;
		float timestamp = 0.f;

		c_usercmd* pixelsurf_cmd = nullptr;

		// Group vectors together for cache efficiency
		vec3_t view_point{};
		vec3_t backup_velocity_surf{};
		vec3_t backup_origin_surf{};
		vec3_t backup_velocity_headbang{};
		vec3_t backup_origin_headbang{};
		vec3_t pre_velocity{};
		vec3_t post_velocity{};
		vec3_t origin{};
		vec3_t angles{};
		vec3_t backup_angles{};
		vec3_t surf_pos{};
		vec3_t normal{};
		vec3_t surf_start{};
		vec3_t surf_end{};
		vec3_t surf_center{};

		float forward_move = 0.f;
		float side_move = 0.f;
		float up_move = 0.f;
		int pre_flags = 0;
		int post_flags = 0;
	};

	ps_data_t ps_data{};
	std::vector<ps_data_t> pixelsurf_database;

private:
	// Precomputed constants
	static constexpr float PS_Z_TOLERANCE = 0.5f;
	static constexpr float PS_Z_TARGET_1 = -6.25f;
	static constexpr float PS_Z_TARGET_2 = -6.0f;
	static constexpr float PS_Z_TARGET_3 = -3.125f;
	static constexpr float PS_SPEED_THRESHOLD = 285.92f;
	static constexpr float MIN_DATABASE_DISTANCE = 50.f;
	static constexpr float SURF_LINE_LENGTH = 200.f;

	// Inline hot path for pixelsurf Z-velocity detection - EXACT original logic
	__forceinline bool is_pixelsurf_z(float vel_z, float prev_z) const {
		return vel_z == -6.25f && prev_z == -6.25f;
	}

	// Optimized validation checks
	__forceinline bool should_skip_processing() const {
		if (!g::local) return true;
		if (!g::local->is_alive()) return true;

		const auto mt = g::local->move_type();
		return (mt == movetype_ladder || mt == movetype_noclip);
	}

	__forceinline bool is_edgebug_active() const {
		return features::movement::detect_data.ticks_left || features::movement::should_edge_bug;
	}

public:
	bool handle_pixelsurf_detection(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive())
			return false;

		if (is_edgebug_active())
			return false;

			const vec3_t pre_vel = g::local->velocity();

			ps_data.pre_velocity = pre_vel;
			ps_data.pixelsurf_cmd = cmd;

			prediction::begin(cmd);
			prediction::end();

			// Original detection logic - check current velocity against pre_vel
			if (is_pixelsurf_z(g::local->velocity().z, pre_vel.z)) {
				ps_data.post_velocity = g::local->velocity();
				features::movement::should_ps = true;
				return true;
			}
			else {
				features::movement::should_ps = false;
				return false;
			}
			return false;
	}

	void handle_pixelsurf_duck(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive())
			return;

		if (!c::movement::pixel_surf ||
			!menu::checkkey(c::movement::pixel_surf_key, c::movement::pixel_surf_key_s)) {
			features::movement::should_ps = false;
			ps_data.pixelsurfing = false;
			return;
		}

		if (g::local->flags() & fl_onground) {
			features::movement::should_ps = false;
			ps_data.pixelsurfing = false;
			return;
		}

			if (should_skip_processing() || is_edgebug_active())
				return;

				int cmds_pred = interfaces::prediction->split->commands_predicted - 1;

				if (!ps_data.pixelsurfing)
					return;

					if (!ps_data.predicted_ps) {

						prediction::restore_ent_to_predicted_frame(
							cmds_pred
						);

						if (ps_data.pixeltick < cmd->tick_count) {

							if (c::movement::pixel_surf_detection_printf) {
								interfaces::chat_element->chatprintf("#dna#_print_pixelsurfed");
								}

								// Save the surf location to database
							//ps_data.valid = true;
							//ps_data.surf_pos = g::local->origin();
							//ps_data.timestamp = interfaces::globals->cur_time;
							//ps_data.was_ducking = ps_data.pixelducking;

							//// Save the UserCmd that triggered the surf
							//ps_data.angles = ps_data.pixelsurf_cmd->view_angles;
							//ps_data.pre_velocity = ps_data.pre_velocity;
							//ps_data.post_velocity = ps_data.post_velocity;

							//// Reserve capacity if needed to avoid reallocations
							//if (pixelsurf_database.capacity() == pixelsurf_database.size()) {
							//	pixelsurf_database.reserve(pixelsurf_database.size() + 32);
							//	}
							//pixelsurf_database.push_back(ps_data);

							//// Trace for normal
							//trace_t trace;
							//ray_t ray;
							//trace_world_only filter;
							//const vec3_t start = g::local->origin();
							//const vec3_t end = start + ps_data.surf_pos;

							//ray.initialize(start, end);
							//interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &filter, &trace);

							//ps_data.normal = trace.plane.normal;
							//ps_data.surf_start = g::local->origin();
							//ps_data.surf_end = trace.end;

							features::movement::detected_normal_pixel_surf = true;
							ps_data.predicted_ps = true;
						}
						else {
							features::movement::detected_normal_pixel_surf = false;
						}

						cmd->buttons = ps_data.pixelsurf_cmd->buttons;
						cmd->up_move = 0;
					}

					prediction::begin(cmd);
					prediction::end();

					// Apply duck state
					if (ps_data.pixelducking) {
						cmd->buttons |= in_bullrush;
						cmd->buttons |= in_duck;
					}
					else
					{
						cmd->buttons &= ~in_duck;
					}


					// Original validation check - maintain surf state properly
					if (ps_data.predicted_ps) {
						const float current_z = g::local->velocity().z;

						// Check if we lost the pixelsurf
						bool lost_surf = !handle_pixelsurf_detection(ps_data.pixelsurf_cmd);
						bool wrong_velocity = !(fabsf(current_z + 6.25f) < PS_Z_TOLERANCE);

						if (lost_surf || (!(g::local->velocity().z == -6.25f))) {
							features::movement::should_ps = false;
							ps_data.pixelsurfing = false;
							ps_data.predicted_ps = false;
							return;
							}
					}
	}

	void handle_pixelsurf(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive())
			return;

		if (!c::movement::pixel_surf ||
			!menu::checkkey(c::movement::pixel_surf_key, c::movement::pixel_surf_key_s))
			return;

			if (should_skip_processing())
				return;

				if (g::local->flags() & fl_onground) 
					return;

					if (ps_data.pixelsurfing || is_edgebug_active())
						return;

						const int max_ticks = time_to_ticks(c::movement::ps_ticks);
						int cmds_pred = interfaces::prediction->split->commands_predicted - 1;

						prediction::restore_ent_to_predicted_frame(
							cmds_pred
						);

						// Try both duck states (0 = ducked, 1 = not ducked)
						for (int s = 0; s < 2 && !ps_data.pixelsurfing; ++s) {

							if (ps_data.pixelsurfing)
								break;

								prediction::restore_ent_to_predicted_frame(
									cmds_pred
								);

								int flags = g::local->flags();
								vec3_t velocity = g::local->velocity();

								// Scan ahead for pixelsurf
								for (int i = 0; i < max_ticks && !ps_data.pixelsurfing; ++i) {

									// Allocate command (original uses new)
									ps_data.pixelsurf_cmd = new c_usercmd(*cmd);

									if (s == 0)
										ps_data.pixelsurf_cmd->buttons |= in_duck;
									else
										ps_data.pixelsurf_cmd->buttons &= ~in_duck;

									// Early exit if grounded
									if (flags & fl_onground)
										break;

										// Check for pixelsurf
										if (handle_pixelsurf_detection(ps_data.pixelsurf_cmd)) {
											prediction::restore_ent_to_predicted_frame(
												cmds_pred
											);
											ps_data.pixeltick = cmd->tick_count + i;
											ps_data.pixelsurf_cmd = new c_usercmd(*ps_data.pixelsurf_cmd);
											ps_data.pixelducking = (s == 0);
											ps_data.pixelsurfing = true;
											ps_data.backup_velocity_surf = g::local->velocity();
											UtilityAssistant.handle_fix(
												ps_data.pixelsurf_cmd,
												cmd->view_angles
											);
											break;
										}

										// Update state for next iteration
										flags = g::local->flags();
										velocity = g::local->velocity();

										if (ps_data.pixelsurfing)
											return;
								}
								prediction::restore_ent_to_predicted_frame(
									cmds_pred
								);
						}
	}

	void handle_pixelsurf_fix(c_usercmd* cmd) {
		if (!c::movement::pixel_surf_fix || !g::local || !g::local->is_alive())
			return;

				const auto mt = g::local->move_type();
				if (mt == movetype_ladder || mt == movetype_noclip)
					return;

					if (cmd->buttons & in_duck)
						return;

						vec3_t velo = g::local->velocity();
						const float speed_2d = velo.length_2d();
						bool once_fix = false;

						// Early exit if conditions not met
						if (speed_2d <= PS_SPEED_THRESHOLD || velo.z >= 0.f)
							return;

							// Single prediction pass
							prediction::begin(cmd);
							prediction::end();

							if (g::local->flags() & fl_onground) {
								// Precompute values
								const int tickrate = static_cast<int>(1.0f / interfaces::globals->interval_per_tick);
								const float airaccelerate = interfaces::console->get_convar("sv_airaccelerate")->get_float();
								const float wishdelta = (speed_2d - PS_SPEED_THRESHOLD) * tickrate / airaccelerate;

								// Compute rotation angle
								velo = velo * -1.f;
								vec3_t velo_ang = velo.to_angle();
								velo_ang.normalize();

								const float rotation = math::deg2rad(velo_ang.y - cmd->view_angles.y);
								const float cos_rot = cosf(rotation);
								const float sin_rot = sinf(rotation);

								cmd->forward_move = cos_rot * wishdelta;
								cmd->side_move = -sin_rot * wishdelta;

								// Special case for exact pixelsurf velocity
								if (fabsf(g::local->velocity().z + 6.25f) < PS_Z_TOLERANCE &&
									fabsf(velo.z + 6.25f) < PS_Z_TOLERANCE) {
										// Movement already set correctly above
									}
							}

							prediction::restore_ent_to_predicted_frame(
								interfaces::prediction->split->commands_predicted - 1
							);
	}

	// Optimized database rendering with spatial culling
	void handle_pixelsurf_database() {
		if (pixelsurf_database.empty()) [[unlikely]]
			return;

			// Use static to avoid reallocation every frame
			static std::vector<vec3_t> drawn_positions;
			drawn_positions.clear();
			drawn_positions.reserve(pixelsurf_database.size() / 2);

			for (const auto& surf : pixelsurf_database) {
				if (!surf.valid) [[unlikely]]
					continue;

					// Spatial deduplication check
					bool too_close = false;
					for (const auto& pos : drawn_positions) {
						const float dist_sq = (surf.surf_pos - pos).length_sqr();
						if (dist_sq < (MIN_DATABASE_DISTANCE * MIN_DATABASE_DISTANCE)) {
							too_close = true;
							break;
						}
					}

					if (too_close) [[unlikely]]
						continue;

						vec3_t normal = surf.normal;

						// Compute robust perpendicular vector
						vec3_t up = (fabsf(normal.z) > 0.9f) ? vec3_t(1, 0, 0) : vec3_t(0, 0, 1);
						vec3_t right = up.cross(normal);

						// Fallback for near-zero cross product
						if (right.length_sqr() < 0.000001f) [[unlikely]]
							right = vec3_t(1, 0, 0).cross(normal);

							right.normalize();

							// Compute line endpoints
							const vec3_t p1 = surf.surf_pos + right * SURF_LINE_LENGTH;
							const vec3_t p2 = surf.surf_pos - right * SURF_LINE_LENGTH;

							// Convert to screen space
							vec3_t s1, s2;
							if (interfaces::debug_overlay->world_to_screen(p1, s1) &&
								interfaces::debug_overlay->world_to_screen(p2, s2)) [[likely]] {

									im_render.drawline(
										s1.x, s1.y, s2.x, s2.y,
										color_t(0, 255, 255, 255),
										2.f
									);

									drawn_positions.push_back(surf.surf_pos);
								}
			}
	}

public:
	void pre(c_usercmd* cmd) {
		handle_pixelsurf_fix(cmd);
	}

	void run(c_usercmd* cmd) {
		handle_pixelsurf_duck(cmd);
		handle_pixelsurf(cmd);
	}

	void notify() {
		
	}

	void draw() {
		// Uncomment to enable database visualization
		// handle_pixelsurf_database();
	}
};
inline PixelSurfAssist PixelSurfAssistant;

class FiremanAssist {
private:
	enum fireman_state_t {
		FIREMAN_NONE,
		FIREMAN_SLIDING,
		FIREMAN_GRABBING,
		FIREMAN_GRABBED,
		FIREMAN_FLINGING
	};

	fireman_state_t current_state = FIREMAN_NONE;
	int state_enter_tick = 0;
	vec3_t saved_ladder_normal = vec3_t(0, 0, 0);
	float saved_distance_to_floor = 0.f;
	bool fling_executed = false;

	// Constants
	static constexpr float GRAB_HEIGHT = 50.f;        // Start grab 30 units from floor
	static constexpr float FLING_HEIGHT = 25.f;       // Fling at 10 units from floor
	static constexpr int GRAB_DELAY_TICKS = 1;        // 8 tick delay before grab (~0.12s)
	static constexpr int FLING_HOLD_TICKS = 15;       // Hold crouch for 15 ticks after fling
	static constexpr float SLIDE_SPEED = -420.f;      // Downward speed while sliding
	static constexpr float GRAB_PUSH_STRENGTH = 450.f; // Push strength to hug ladder
	static constexpr float FLING_STRENGTH = 450.f;    // Fling away from ladder strength

	void find_ladder_normal(vec3_t& out_normal, float& out_fraction) {
		trace_world_only filter;
		out_fraction = 1.f;
		out_normal = vec3_t(0, 0, 0);

		const int steps = 24; // More samples for better accuracy
		const float step_angle = (2.0f * m_pi) / steps;
		vec3_t eye = g::local->get_eye_pos();

		for (int i = 0; i < steps; i++) {
			float a = i * step_angle;
			vec3_t dir = vec3_t(cosf(a), sinf(a), 0.f);

			trace_t tr;
			ray_t ray;
			ray.initialize(eye, eye + dir * 50.f); // Increased range
			interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &filter, &tr);

			// Only consider surfaces that are vertical (ladder surfaces)
			if (tr.flFraction < out_fraction && fabsf(tr.plane.normal.z) < 0.1f) {
				out_fraction = tr.flFraction;
				out_normal = tr.plane.normal;
			}
		}
	}

	float get_distance_to_floor() {
		trace_world_only filter;
		trace_t floor_tr;
		ray_t floor_ray;

		vec3_t start = g::local->abs_origin();
		vec3_t end = start - vec3_t(0, 0, 100.f); // Look down 100 units

		floor_ray.initialize(start, end);
		interfaces::trace_ray->trace_ray(floor_ray, MASK_PLAYERSOLID, &filter, &floor_tr);

		if (floor_tr.did_hit()) {
			return (start.z - floor_tr.end.z);
		}

		return 999.f; // No floor found
	}

	void handle_fireman(c_usercmd* cmd) {
		if (!c::movement::fireman || !g::local || !g::local->is_alive())
			return;

		if (!menu::checkkey(c::movement::fireman_key, c::movement::fireman_key_s)) {
			current_state = FIREMAN_NONE;
			fling_executed = false;
			features::movement::should_fireman = false;
			return;  // ← return here so NOTHING runs unless key is held
		}

		// Backup state for prediction
		vec3_t backup_origin = g::local->origin();
		vec3_t backup_velocity = g::local->velocity();
		float backup_stamina = g::local->stamina();
		float backup_sim = g::local->simulation_time();

		 //Reset state if not on ladder
		if (menu::checkkey(c::movement::fireman_key, c::movement::fireman_key_s) && g::local->move_type() != movetype_ladder) {
			if (current_state == FIREMAN_FLINGING) {
				// Maintain fling state briefly after leaving ladder
				int ticks_since_fling = interfaces::globals->tick_count - state_enter_tick;
				if (ticks_since_fling >= FLING_HOLD_TICKS) {
					cmd->buttons |= in_duck;
					cmd->forward_move = 0.f;
					cmd->side_move = 0.f;
				}
				else {
					current_state = FIREMAN_NONE;
					fling_executed = false;
				}
			}
			else {
				current_state = FIREMAN_NONE;
				fling_executed = false;
			}
			prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
			return;
		}

		// Get current state info
		float dist_to_floor = get_distance_to_floor();
		vec3_t ladder_normal;
		float ladder_fraction;
		find_ladder_normal(ladder_normal, ladder_fraction);

		bool ladder_found = (ladder_fraction < 1.f);

		features::movement::should_fireman = false;

		// === STATE MACHINE ===
		switch (current_state) {
		case FIREMAN_NONE:
		case FIREMAN_SLIDING: {
			// SLIDING DOWN LADDER
			if (current_state != FIREMAN_SLIDING) {
				current_state = FIREMAN_SLIDING;
				state_enter_tick = interfaces::globals->tick_count;
				fling_executed = false;
			}

			// Slide down fast
			cmd->forward_move = 0.f;
			cmd->side_move = 0.f;
			cmd->up_move = SLIDE_SPEED;

			// Predict where we'll be next tick
			prediction::begin(cmd);
			prediction::end();

			vec3_t predicted_pos = g::local->origin();
			float predicted_dist_to_floor = get_distance_to_floor();

			// Restore state
			g::local->origin() = backup_origin;
			g::local->velocity() = backup_velocity;
			g::local->stamina() = backup_stamina;
			g::local->simulation_time() = backup_sim;

			// Transition to GRABBING when we're at grab height
			if (dist_to_floor <= GRAB_HEIGHT && ladder_found) {
				current_state = FIREMAN_GRABBING;
				state_enter_tick = interfaces::globals->tick_count;
				saved_ladder_normal = ladder_normal;
				saved_distance_to_floor = dist_to_floor;
			}
			break;
		}

		case FIREMAN_GRABBING: {
			// GRABBING DELAY - wait before actually grabbing
			int ticks_in_grab = interfaces::globals->tick_count - state_enter_tick;

			// Continue sliding during delay
			cmd->forward_move = 0.f;
			cmd->side_move = 0.f;
			cmd->up_move = SLIDE_SPEED;

			// Update ladder normal
			if (ladder_found) {
				saved_ladder_normal = ladder_normal;
			}

			// Predict if we're getting close to fling height
			prediction::begin(cmd);
			prediction::end();

			float predicted_dist = get_distance_to_floor();

			g::local->origin() = backup_origin;
			g::local->velocity() = backup_velocity;
			g::local->stamina() = backup_stamina;
			g::local->simulation_time() = backup_sim;

			// Transition to GRABBED after delay
			if (ticks_in_grab >= GRAB_DELAY_TICKS) {
				current_state = FIREMAN_GRABBED;
				state_enter_tick = interfaces::globals->tick_count;
			}
			break;
		}

		case FIREMAN_GRABBED: {
			features::movement::should_fireman = true;
			// GRABBED - hug ladder and prepare for fling

			// Stop sliding, hug ladder
			cmd->up_move = 0.f;

			// Push into ladder using saved normal
			if (saved_ladder_normal.length() > 0.1f) {
				vec3_t push_dir = saved_ladder_normal * -1.f; // Push opposite to normal

				// Convert to view-space movement
				float rot = math::deg2rad(cmd->view_angles.y);
				vec3_t forward_dir = vec3_t(cosf(rot), sinf(rot), 0.f);
				vec3_t right_dir = vec3_t(-sinf(rot), cosf(rot), 0.f);

				float forward_component = push_dir.dot(forward_dir);
				float right_component = push_dir.dot(right_dir);

				cmd->forward_move = forward_component * GRAB_PUSH_STRENGTH;
				cmd->side_move = right_component * GRAB_PUSH_STRENGTH;
			}

			// Predict position with grab
			prediction::begin(cmd);
			prediction::end();

			vec3_t predicted_pos = g::local->origin();
			float predicted_dist = get_distance_to_floor();

			g::local->origin() = backup_origin;
			g::local->velocity() = backup_velocity;
			g::local->stamina() = backup_stamina;
			g::local->simulation_time() = backup_sim;

			// Check if we're at fling height
			if (dist_to_floor <= FLING_HEIGHT && !fling_executed) {
				current_state = FIREMAN_FLINGING;
				state_enter_tick = interfaces::globals->tick_count;
				fling_executed = true;
			}
			break;
		}

		case FIREMAN_FLINGING: {
			features::movement::should_fireman = false;
			// FLINGING - crouch jump away from ladder
			int ticks_since_fling = interfaces::globals->tick_count - state_enter_tick;

			if (ticks_since_fling == 0) {
				// First tick of fling - apply all inputs
				cmd->buttons |= in_duck;
				cmd->buttons |= in_jump;

				// Fling away from ladder using saved normal
				if (saved_ladder_normal.length() > 0.1f) {
					vec3_t fling_dir = saved_ladder_normal; // Fling direction is away from ladder
					fling_dir.normalize();

					// Convert to view-space movement
					float rot = math::deg2rad(cmd->view_angles.y);
					vec3_t forward_dir = vec3_t(cosf(rot), sinf(rot), 0.f);
					vec3_t right_dir = vec3_t(-sinf(rot), cosf(rot), 0.f);

					float forward_component = fling_dir.dot(forward_dir);
					float right_component = fling_dir.dot(right_dir);

					cmd->forward_move = forward_component * FLING_STRENGTH;
					cmd->side_move = right_component * FLING_STRENGTH;
					cmd->up_move = 420.f; // Max upward
				}

				// Predict fling result
				prediction::begin(cmd);
				prediction::end();

				vec3_t fling_velocity = g::local->velocity();
				vec3_t fling_position = g::local->origin();

				g::local->origin() = backup_origin;
				g::local->velocity() = backup_velocity;
				g::local->stamina() = backup_stamina;
				g::local->simulation_time() = backup_sim;

				// Optional: Apply velocity boost if prediction shows we need it
				if (fling_velocity.length_2d() < 200.f) {
					// Boost velocity directly
					prediction::begin(cmd);
					prediction::end();

					vec3_t boost = saved_ladder_normal * 100.f;
					g::local->velocity().x += boost.x;
					g::local->velocity().y += boost.y;
					g::local->velocity().z += 50.f;

					g::local->origin() = backup_origin;
					g::local->velocity().x += boost.x;
					g::local->velocity().y += boost.y;
					g::local->velocity().z += 50.f;
					g::local->stamina() = backup_stamina;
					g::local->simulation_time() = backup_sim;
				}
			}
			else if (ticks_since_fling >= FLING_HOLD_TICKS) {
				// Hold crouch during fling
				cmd->buttons |= in_duck;
				cmd->forward_move = 0.f;
				cmd->side_move = 0.f;
				features::movement::should_fireman = false;
			}
			else {
				// Fling complete
				current_state = FIREMAN_NONE;
				fling_executed = false;
				features::movement::should_fireman = false;
			}
			break;
		}
		}

		// Reset on key release
		if (!menu::checkkey(c::movement::fireman_key, c::movement::fireman_key_s)) {
			current_state = FIREMAN_NONE;
			fling_executed = false;
			features::movement::should_fireman = false;
		}

		prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
	}

	struct fireman_data_t {
		bool is_ladder = false;
		bool fr_hit_1 = false;
		bool fr_hit = false;
		bool awall = false;
	}; fireman_data_t m_fireman_data;

	void fire_man(c_usercmd* cmd)
	{
		m_fireman_data.is_ladder = false;

		if (!g::local || !g::local->is_alive()) {
			return;
		}

		if (!g::local->move_type()) {
			return;
		}

		if (c::movement::airstuck_enabled && menu::checkkey(c::movement::airstuck_key, c::movement::airstuck_key_s)) {
			return;
		}

		if (!c::movement::fireman) {
			m_fireman_data.fr_hit = false;
			m_fireman_data.fr_hit_1 = false;
			return;
		}

		if (!menu::checkkey(c::movement::fireman_key, c::movement::fireman_key_s)) {
			m_fireman_data.fr_hit = false;
			m_fireman_data.fr_hit_1 = false;
			features::movement::should_fireman = false;
			return;
		}

		if (m_fireman_data.fr_hit_1) {
			if (prediction_backup::movetype == movetype_ladder) {
				cmd->buttons |= in_jump;
				cmd->forward_move = 0.f;
				cmd->side_move = 0.f;
			}
		}

		auto colidable = g::local->collideable();
		if (!colidable)
			return;

		prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
		m_fireman_data.awall = false;
		trace_t trace;
		float step = std::numbers::pi_v< float > *2.0f / 16.f;
		static float start_circle = 0.f;

		if (g::local->get_velocity().z != -6.25f && prediction_backup::velocity.z != -6.25f)
			start_circle = 0.f;

		bool foundWall = false;
		float foundAngle = 0.f;
		vec3_t wall_position;

		for (float a = start_circle; a < std::numbers::pi_v< float > *2.0f; a += step) {
			vec3_t wishdir(cosf(a), sinf(a), 0.f);
			auto start_pos = g::local->abs_origin();
			auto end_pos = start_pos + wishdir;
			trace_filter flt(g::local);
			ray_t ray;
			ray.initialize(start_pos, end_pos, colidable->mins(), colidable->maxs());

			interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &flt, &trace);

			if (trace.flFraction < 1.f && trace.plane.normal.z < 0.4f) {
				wall_position = trace.end;
				foundWall = true;
				foundAngle = a;
				break;
			}
		}

		if (foundWall) {
			m_fireman_data.awall = true;
			int commands_predicted = interfaces::prediction->split->commands_predicted;

			vec3_t normal_plane(trace.plane.normal.x * -1.f, trace.plane.normal.y * -1.f, 0.f);
			vec3_t wall_angle = normal_plane.to_angle();
			vec3_t d_velo = g::local->get_velocity();
			d_velo.z = 0.f;
			vec3_t velo_angle = d_velo.to_angle();
			vec3_t delta = velo_angle - wall_angle;
			delta.normalize();
			float rotation = deg2rad(wall_angle.y - cmd->view_angles.y);
			float cos_rot = cos(rotation);
			float sin_rot = sin(rotation);
			float multiplayer = 450.f;
			float backup_forward_move = cmd->forward_move;
			float backup_side_move = cmd->side_move;
			float forward_move = cos_rot * multiplayer;
			float side_move = -sin_rot * multiplayer;
			int backup_buttons = cmd->buttons;

			prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
			cmd->forward_move = forward_move;
			cmd->side_move = side_move;
			cmd->buttons &= ~in_jump;

			prediction::begin(cmd);
			prediction::end();

			if (!m_fireman_data.is_ladder) {
				if ((prediction_backup::movetype != movetype_ladder && g::local->move_type() == movetype_ladder) ||
					prediction_backup::movetype == movetype_ladder) {
					m_fireman_data.is_ladder = true;
					start_circle = foundAngle;
				}
				else {
					cmd->buttons = backup_buttons;
					cmd->forward_move = backup_forward_move;
					cmd->side_move = backup_side_move;
				}
			}

			if (m_fireman_data.is_ladder) {
				prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
				int old_flags = g::local->flags();
				int old_move_type = g::local->move_type();
				cmd->forward_move = 0.f;
				cmd->side_move = 0.f;

				if (!m_fireman_data.fr_hit && !m_fireman_data.fr_hit_1) {
					for (int i = 0; i < 12; i++) {
						prediction::begin(cmd);
						prediction::end();
						if (!(old_flags & 1) && (g::local->flags() & 1)) {
							if (i < 10) {
								m_fireman_data.fr_hit_1 = true;
								break;
							}
							m_fireman_data.fr_hit = true;
						}
					}
				}
				if (!m_fireman_data.fr_hit_1) {
					if (!m_fireman_data.fr_hit) {
						cmd->buttons &= ~in_jump;
						cmd->forward_move = 0.f;
						cmd->side_move = 0.f;
					}
					else {
						if (prediction_backup::movetype != movetype_ladder) {
							cmd->forward_move = forward_move;
							cmd->side_move = side_move;
							cmd->buttons |= in_jump;
							features::movement::should_fireman = true;
						}
						else {
							cmd->forward_move = 0.f;
							cmd->side_move = 0.f;
							cmd->buttons |= in_jump;
							features::movement::should_fireman = false;
						}
					}
				}
				else {
					prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
					int oldflags = g::local->flags();
					int oldMoveType = g::local->move_type();
					cmd->forward_move = 0.f;
					cmd->side_move = 0.f;

					prediction::begin(cmd);
					prediction::end();

					if (!(oldflags & 1) && (g::local->flags() & 1)) {
						cmd->forward_move = forward_move;
						cmd->side_move = side_move;
						cmd->buttons &= ~in_jump;
						features::movement::should_fireman = true;
					}
					else {
						cmd->buttons |= in_jump;
						cmd->forward_move = 0.f;
						cmd->side_move = 0.f;
						features::movement::should_fireman = false;
					}
				}
			}
			else {
				m_fireman_data.fr_hit = false;
				m_fireman_data.fr_hit_1 = false;
				features::movement::should_fireman = false;
			}
		}
	}

public:
	void run(c_usercmd* cmd) {
		this->fire_man(cmd);
	}
};
inline FiremanAssist FiremanAssistant;

class RouteAssist {
public:

	bool is_longjump;
	vec3_t surf_velocity = vec3_t(0, 0, 0);
	vec3_t headbang_velocity = vec3_t(0, 0, 0);
	float saved_fraction;
	float saved_normal_z;
	float saved_velz;
	bool alert_mini_crouch_hop = false;
	bool bounce_alert_mini_crouch_hop = false;
	vec3_t scan_StartPos;
	vec3_t scan_EndPos;
	vec3_t scan_WallNormal;
	bool scan_is_displacement;
	bool scan_FirstPoint;

	enum jumptype_t {
		floor,
		ceiling,
		bounce,
		pixelsurf,
		headbang
	};
	jumptype_t jumptype = floor;

	struct jump_marker_t {
		vec3_t position;
		vec3_t velocity;
		vec3_t surface_normal{};
		jumptype_t type;
		std::string name;
		int chain_id = -1;
		bool is_chain_start = false;
		bool is_chain_end = false;
		std::string map;
		float currentScale = 1.f;
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
		bool stand = true;
		bool duck = true;
		bool transient = false;
		float radius = 300.f;
		float delta_strafe = 0.5f;
		jump_marker_t(const vec3_t& Pos = { }, const std::string& Map = "") : position(Pos), map(Map) {}
	};
	std::vector<jump_marker_t> route_markers;
	std::vector<vec3_t> Point_surf{ };        // pixelsurf points
	std::vector<vec3_t> Point_bounce{ };      // bounce points
	std::vector<vec3_t> Point_floor{ };       // ADD THIS - floor contact points
	std::vector<vec3_t> Point_ceiling{ };     // ADD THIS - ceiling contact points
	std::vector<vec3_t> Point_headbang{ };    // ADD THIS - headbang points
	int current_marker_index = -1;
	std::string path;

	RouteAssist() {
		route_markers.clear();
		route_markers.reserve(128); // Pre-allocate some space
		current_marker_index = -1;
	}

	std::string get_current_map_name() const {
		const char* map = interfaces::engine->get_level_name();
		if (!map || !*map) return "unknown_map";
		std::string map_str(map);
		size_t last_slash = map_str.find_last_of('/');
		if (last_slash != std::string::npos)
			map_str = map_str.substr(last_slash + 1);
		return map_str;
	}

	void save(const std::size_t index = 0) {
		if (index >= c::configs.size())
			return;

		path.clear();
		path.append(c::configs.at(index));
		path.append("_");
		path.append(get_current_map_name());
		path.append(".json");

		nlohmann::json json_array = nlohmann::json::array();

		for (const auto& marker : route_markers) {
			if (marker.transient)
				continue;

			nlohmann::json j;
			j["position"] = { {"x", marker.position.x}, {"y", marker.position.y}, {"z", marker.position.z} };
			j["velocity"] = { {"x", marker.velocity.x}, {"y", marker.velocity.y}, {"z", marker.velocity.z} };
			j["type"] = static_cast<int>(marker.type);
			j["name"] = marker.name;
			j["map"] = marker.map;  // ✅ SAVE MAP
			json_array.push_back(j);
		}

		std::ofstream output_file(path);
		if (output_file.is_open()) {
			output_file << json_array.dump(4);
			interfaces::chat_element->chatprintf("RouteStudio: saved markers", (int)route_markers.size());
		}
	}

	void load(const std::size_t index = 0) {
		if (index >= c::configs.size())
			return;

		path.clear();
		path.append(c::configs.at(index));
		path.append("_");
		path.append(get_current_map_name());
		path.append(".json");

		// Always clear old markers
		route_markers.clear();
		current_marker_index = -1;

		if (std::ifstream input_file{ path }; input_file.good()) {
			nlohmann::json json;
			try {
				input_file >> json;
			}
			catch (...) {
				interfaces::chat_element->chatprintf("RouteStudio: invalid JSON");
				return;
			}

			for (const auto& item : json) {
				jump_marker_t marker;
				marker.position = vec3_t(
					item.at("position").at("x").get<float>(),
					item.at("position").at("y").get<float>(),
					item.at("position").at("z").get<float>()
				);
				marker.velocity = vec3_t(
					item.at("velocity").at("x").get<float>(),
					item.at("velocity").at("y").get<float>(),
					item.at("velocity").at("z").get<float>()
				);
				marker.type = static_cast<jumptype_t>(item.at("type").get<int>());
				marker.name = item.at("name").get<std::string>();
				marker.transient = false;
				marker.stand = true;
				marker.duck = true;

				// ✅ LOAD MAP - handle old JSON files that don't have map field
				if (item.contains("map")) {
					marker.map = item.at("map").get<std::string>();
				}
				else {
					// If old JSON doesn't have map, use current map
					marker.map = get_current_map_name();
				}

				route_markers.push_back(marker);
			}

			interfaces::chat_element->chatprintf("RouteStudio: loaded", (int)route_markers.size());
		}
	}

	void update_current_marker_state() {
		if (route_markers.empty() || !g::local || !g::local->is_alive())
			return;

		vec3_t player_pos = g::local->origin();
		float closest_dist = FLT_MAX;
		int closest_index = current_marker_index;

		for (int i = 0; i < (int)route_markers.size(); ++i) {
			float dist = player_pos.distance(route_markers[i].position);
			if (dist < closest_dist) {
				closest_dist = dist;
				closest_index = i;
			}
		}

		if (closest_index != current_marker_index && closest_index != -1) {
			current_marker_index = closest_index;
		}
	}

	bool is_planner_marker(const jump_marker_t& marker) const {
		return marker.active && (marker.type == floor || marker.type == pixelsurf);
	}

	int nearest_planner_marker_index(bool include_transient = true) const {
		if (!g::local || !g::local->is_alive())
			return -1;

		const std::string current_map = get_current_map_name();
		const vec3_t player_pos = g::local->origin();
		float closest_dist = FLT_MAX;
		int closest_index = -1;

		for (int i = 0; i < static_cast<int>(route_markers.size()); ++i) {
			const auto& marker = route_markers[i];
			if (!is_planner_marker(marker) || marker.map != current_map)
				continue;

			if (!include_transient && marker.transient)
				continue;

			const float dist = player_pos.distance_to(marker.position);
			if (dist < closest_dist) {
				closest_dist = dist;
				closest_index = i;
			}
		}

		return closest_index;
	}

	void sync_finder_markers() {
		if (!interfaces::engine || !interfaces::engine->is_in_game())
			return;

		const std::string current_map = get_current_map_name();

		if (current_marker_index >= 0 &&
			current_marker_index < static_cast<int>(route_markers.size()) &&
			route_markers[current_marker_index].transient) {
			current_marker_index = -1;
		}

		route_markers.erase(std::remove_if(route_markers.begin(), route_markers.end(), [&](const jump_marker_t& marker) {
			return marker.transient && marker.map == current_map;
		}), route_markers.end());

		if (current_marker_index >= static_cast<int>(route_markers.size()))
			current_marker_index = -1;

		if (!c::movement::pscalc_finder_enable)
			return;

		const auto& finder_points = features::movement::pixelsurf_finder::last_points();

		for (const auto& point : finder_points) {
			jump_marker_t marker;
			marker.position = point.pos;
			marker.velocity = {};
			marker.surface_normal = point.normal;
			marker.type = pixelsurf;
			marker.name = "[pixelsurf]";
			marker.map = current_map;
			marker.active = true;
			marker.transient = true;
			marker.stand = point.stand;
			marker.duck = point.duck;
			marker.radius = 24.0f;
			marker.jump = point.stand;
			marker.c_jump = point.duck;
			marker.minijump = point.stand;
			marker.c_minijump = point.duck;
			marker.longjump = point.stand;
			marker.c_longjump = point.duck;
			route_markers.push_back(marker);
		}
	}

	void handle_marker_placement(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive())
			return;

		static float last_placement_time = 0.f;
		const float cur_time = interfaces::globals->realtime;
		const float debounce_time = 0.25f;

		if (cur_time - last_placement_time < debounce_time)
			return;

		// Default keys if not set
		int floor_key = c::movement::add_floor_key == 0 ? 0x70 : c::movement::add_floor_key; // F1
		int ps_key = c::movement::add_pixelsurf_key == 0 ? 0x71 : c::movement::add_pixelsurf_key; // F2
		int del_key = c::movement::delete_point_key == 0 ? 0x08 : c::movement::delete_point_key; // Backspace

		auto add_marker = [&](jumptype_t type, const std::string& name) {
			vec3_t view_angles = cmd->view_angles;
			vec3_t forward;
			math::angle_vectors(view_angles, &forward);
			
			vec3_t start = g::local->get_eye_pos();
			vec3_t end = start + (forward * 8000.f);

			trace_t trace;
			ray_t ray;
			ray.initialize(start, end);
			trace_filter filter(g::local);
			interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &filter, &trace);

			if (trace.flFraction < 1.0f) {
				jump_marker_t marker;
				marker.position = trace.end;
				marker.type = type;
				marker.name = name;
				marker.map = get_current_map_name();
				marker.transient = false;
				route_markers.push_back(marker);
				interfaces::chat_element->chatprintf(std::format("RouteStudio: added {} marker at {:.0f} {:.0f} {:.0f}", name, marker.position.x, marker.position.y, marker.position.z).c_str());
				last_placement_time = cur_time;
			}
			else {
				interfaces::chat_element->chatprintf("RouteStudio: trace failed (look at a surface!)");
				last_placement_time = cur_time;
			}
		};

		if (menu::checkkey(floor_key, c::movement::add_floor_key_s)) {
			add_marker(floor, "floor");
		}
		else if (menu::checkkey(ps_key, c::movement::add_pixelsurf_key_s)) {
			add_marker(pixelsurf, "pixelsurf");
		}
		else if (menu::checkkey(del_key, c::movement::delete_point_key_s)) {
			if (!route_markers.empty()) {
				route_markers.pop_back();
				interfaces::chat_element->chatprintf("RouteStudio: removed last marker");
				last_placement_time = cur_time;
			}
		}
	}

	void handle_marker_clearing() {
		if (!g::local || !g::local->is_alive())
			return;

		if (menu::open)
			return;

		const int marker_count = static_cast<int>(route_markers.size());
		if (marker_count == 0)
			return;

		static float last_action_time = 0.f;
		const float cur_time = interfaces::globals->realtime;
		const float debounce_time = 0.25f;

		if (menu::checkkey(c::movement::pscalc_clear_all_key, c::movement::pscalc_clear_all_key_s)) {
			if (cur_time - last_action_time > debounce_time) {
				interfaces::chat_element->chatprintf("RouteStudio: cleared all markers");
				route_markers.clear();
				current_marker_index = -1;
				last_action_time = cur_time;
			}
		}
	}

	void draw_route_markers() {
		if (!g::local || !g::local->is_alive())
			return;

		update_current_marker_state();

		const float max_render = static_cast<float>(c::movement::max_render_distance);
		const vec3_t player_origin = g::local->origin();

		for (int i = 0; i < (int)route_markers.size(); ++i) {
			const auto& marker = route_markers[i];
			if (marker.transient && !c::movement::pscalc_finder_draw)
				continue;
			if (player_origin.distance_to(marker.position) > max_render)
				continue;

			vec3_t screen_pos;
			if (!interfaces::debug_overlay->world_to_screen(marker.position, screen_pos))
				continue;

			color_t clr;
			switch (marker.type) {
			case pixelsurf:  clr = color_t(0, 230, 0, 180); break;
			case headbang:   clr = color_t(255, 255, 0, 180); break;
			case bounce:     clr = color_t(255, 0, 0, 180); break;
			case floor:      clr = color_t(255, 140, 0, 180); break;   // Orange
			case ceiling:    clr = color_t(138, 43, 226, 180); break; // Purple
			default:         clr = color_t(200, 200, 200, 180); break;
			}

			if (marker.transient)
				clr = color_t(90, 215, 255, 190);

			if (i == current_marker_index) {
				clr = color_t(255, 255, 255, 255);
				im_render.drawcircle(screen_pos.x, screen_pos.y, 7.f, 48, clr, 3.f);
			}
			else {
				im_render.drawcircle(screen_pos.x, screen_pos.y, 6.f, 32, clr, 2.f);
			}

			im_render.text(
				screen_pos.x,
				screen_pos.y - 6.f,
				12,
				fonts::esp_name_font,
				std::to_string(i + 1).c_str(),
				true,
				color_t(255, 255, 255, 255),
				true
			);

			const float text_offset_y = (i == current_marker_index) ? -20.f : -19.f;
			im_render.text(
				screen_pos.x,
				screen_pos.y + text_offset_y,
				12,
				fonts::esp_name_font,
				marker.transient ? "[pixelsurf]" : marker.name.c_str(),
				true,
				color_t(255, 255, 255, 220),
				true
			);

			// Draw line to next marker
			if (!marker.transient && i < (int)route_markers.size() - 1) {
				const auto& next_marker = route_markers[i + 1];
				if (next_marker.transient)
					continue;
				if (player_origin.distance_to(next_marker.position) > max_render)
					continue;
				vec3_t next_screen_pos;
				if (interfaces::debug_overlay->world_to_screen(next_marker.position, next_screen_pos)) {
					im_render.drawline(screen_pos.x, screen_pos.y, next_screen_pos.x, next_screen_pos.y, color_t(255, 255, 255, 100), 1.f);
				}
			}
		}
	}
	
	void draw_scan_line(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive())
			return;

		if (cmd != nullptr) {
			vec3_t ViewAngles = vec3_t(cmd->view_angles.x, cmd->view_angles.y, 0.f);

			if (menu::checkkey(c::movement::pscalc_drawpoint_key, c::movement::pscalc_drawpoint_key_s)) {
				if (!scan_FirstPoint) {
					const auto endPos = g::local->get_eye_pos() + vec3_t::fromAngle(ViewAngles) * 6000;

					trace_t trace = { };
					ray_t ray;
					ray.initialize(g::local->get_eye_pos(), endPos);

					trace_filter flt(g::local);

					interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &flt, &trace);

					scan_WallNormal = trace.plane.normal;
					scan_is_displacement = strstr(trace.surface.name, "displacement");

					scan_StartPos = g::local->get_eye_pos() + (endPos - g::local->get_eye_pos()) * trace.flFraction;
					scan_EndPos = g::local->get_eye_pos() + (endPos - g::local->get_eye_pos()) * trace.flFraction;
					scan_FirstPoint = true;
				}
				else {
					const auto endPos = g::local->get_eye_pos() + vec3_t::fromAngle(ViewAngles) * 6000;
					trace_t trace = { };
					ray_t ray;
					ray.initialize(g::local->get_eye_pos(), endPos);

					trace_filter flt(g::local);

					interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &flt, &trace);

					scan_EndPos = vec3_t(scan_StartPos.x, scan_StartPos.y, vec3_t(g::local->get_eye_pos() + (endPos - g::local->get_eye_pos()) * trace.flFraction).z);
				}
			}
		}
		else {
			if (menu::checkkey(c::movement::pscalc_drawpoint_key, c::movement::pscalc_drawpoint_key_s)) {
				if (!scan_StartPos.is_zero() && !scan_EndPos.is_zero()) {
					ImVec2 start2D, end2D;
					if (im_render.world_to_screen(scan_StartPos, &start2D) && im_render.world_to_screen(scan_EndPos, &end2D)) {
						im_render.drawline(start2D.x, start2D.y, end2D.x, end2D.y, color_t(200, 215, 215, 255 / 1.25), 1.5f);
					}
				}
			}
		}
	}

	void scan_markers_from_line(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive())
			return;

		if (cmd != nullptr)
		{
			float sv_gravity = interfaces::console->get_convar(("sv_gravity"))->get_float();
			float fTickInterval = interfaces::globals->interval_per_tick;
			float fTickRate = (fTickInterval > 0) ? (1.0f / fTickInterval) : 0.0f;
			float targetZvelo = ((sv_gravity / 2) / fTickRate) * -1.f;

			// Trigger the legacy scanline calculation only on release of the draw key.
			bool release_calc = !menu::checkkey(c::movement::pscalc_drawpoint_key, c::movement::pscalc_drawpoint_key_s);

			if (release_calc && scan_FirstPoint)
			{
				int buttons = cmd->buttons;
				float ForwardMove = cmd->forward_move;
				float Lerp = 0;
				int PredictionFrames = interfaces::prediction->split->commands_predicted - 1;
				float test_al = 15.97803f;

				if (scan_is_displacement)
					test_al = 16.001f;

				// BOUNCE SCAN (going UP)
				for (int v = 0; v < 4; v++) {
					prediction::restore_ent_to_predicted_frame(PredictionFrames);
					bool done_and_break = false;

					for (int i = 0; i < 128; i++)
					{
						cmd->buttons |= in_jump;
						cmd->buttons |= in_duck;

						vec3_t Angles{ scan_WallNormal.x * -1.f, scan_WallNormal.y * -1.f, 0.f };
						vec3_t to_wall = Angles.to_angle();
						float rotation = math::deg2rad(to_wall.y - cmd->view_angles.y);
						float cos_rot = cos(rotation);
						float sin_rot = sin(rotation);
						float forwardmove = cos_rot * 10.f;
						float sidemove = -sin_rot * 10.f;
						cmd->forward_move = forwardmove;
						cmd->side_move = sidemove;

						if (g::local->get_velocity().z > 0)
						{
							static float Height = 0.f;
							if (scan_StartPos.z > scan_StartPos.z) {
								if (scan_StartPos.z < 0.f)
									Height = (int)scan_StartPos.z - 0.02f - Lerp - 54.f;
								else
									Height = (int)scan_StartPos.z + 0.02f + Lerp - 54.f;
							}
							else {
								if (scan_EndPos.z < 0.f)
									Height = (int)scan_EndPos.z - 0.02f - Lerp - 54.f;
								else
									Height = (int)scan_EndPos.z + 0.02f + Lerp - 54.f;
							}

							int maxLerp;
							if (scan_StartPos.z > scan_EndPos.z) {
								maxLerp = scan_StartPos.z - scan_EndPos.z;
							}
							else {
								maxLerp = scan_EndPos.z - scan_StartPos.z;
							}

							if (Lerp > maxLerp) {
								cmd->buttons = buttons;
								cmd->forward_move = ForwardMove;
								scan_FirstPoint = false;
								done_and_break = true;
								break;
							}
							else {
								Lerp += 1.f;
							}

							// Position player based on wall normal
							if (scan_WallNormal.x < 0 && scan_WallNormal.y < 0.f) {
								g::local->origin() = vec3_t(scan_StartPos.x - test_al, scan_StartPos.y - test_al, Height);
								g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
							}
							else if (scan_WallNormal.x < 0 && scan_WallNormal.y > 0.f) {
								g::local->origin() = vec3_t(scan_StartPos.x - test_al, scan_StartPos.y + test_al, Height);
								g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
							}
							else if (scan_WallNormal.x > 0 && scan_WallNormal.y < 0.f) {
								g::local->origin() = vec3_t(scan_StartPos.x + test_al, scan_StartPos.y - test_al, Height);
								g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
							}
							else if (scan_WallNormal.x > 0 && scan_WallNormal.y > 0.f) {
								g::local->origin() = vec3_t(scan_StartPos.x + test_al, scan_StartPos.y + test_al, Height);
								g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
							}
							else if (scan_WallNormal.x == 0.f && scan_WallNormal.y > 0.f) {
								g::local->origin() = vec3_t(scan_StartPos.x, scan_StartPos.y + test_al, Height);
								g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
							}
							else if (scan_WallNormal.x == 0.f && scan_WallNormal.y < 0.f) {
								g::local->origin() = vec3_t(scan_StartPos.x, scan_StartPos.y - test_al, Height);
								g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
							}
							else if (scan_WallNormal.x < 0 && scan_WallNormal.y == 0.f) {
								g::local->origin() = vec3_t(scan_StartPos.x - test_al, scan_StartPos.y, Height);
								g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
							}
							else if (scan_WallNormal.x > 0 && scan_WallNormal.y == 0.f) {
								g::local->origin() = vec3_t(scan_StartPos.x + test_al, scan_StartPos.y, Height);
								g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
							}

							// Check if valid bounce position
							trace_t trace;
							trace_filter flt(g::local);
							ray_t ray;
							ray.initialize(vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z + 54.f),
								vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z - 1000.f));

							interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &flt, &trace);
							vec3_t down_point = trace.end;

							trace_t trace2;
							trace_filter flt2(g::local);
							ray_t ray2;
							ray2.initialize(vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z + 54.f),
								vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z + 1000.f));

							interfaces::trace_ray->trace_ray(ray2, MASK_PLAYERSOLID, &flt2, &trace2);
							vec3_t upper_point = trace2.end;

							if (g::local->origin().z + 1.f < down_point.z)
								continue;
							if (g::local->origin().z + 1.f > upper_point.z)
								continue;
						}

						vec3_t un_pred_velo = g::local->get_velocity();

						prediction::begin(cmd);
						prediction::end();

						vec3_t pred_velo = g::local->get_velocity();

						if (g::local->get_velocity().z == targetZvelo) {
							// Add bounce point
							if (scan_WallNormal.x < 0 && scan_WallNormal.y < 0.f) {
								Point_bounce.emplace_back(vec3_t(g::local->origin().x + test_al,
									g::local->origin().y + test_al,
									g::local->origin().z + 54.f));
								scan_FirstPoint = false;
							}
							else if (scan_WallNormal.x < 0 && scan_WallNormal.y > 0.f) {
								Point_bounce.emplace_back(vec3_t(g::local->origin().x + test_al,
									g::local->origin().y - test_al,
									g::local->origin().z + 54.f));
								scan_FirstPoint = false;
							}
							else if (scan_WallNormal.x > 0 && scan_WallNormal.y < 0.f) {
								Point_bounce.emplace_back(vec3_t(g::local->origin().x - test_al,
									g::local->origin().y + test_al,
									g::local->origin().z + 54.f));
								scan_FirstPoint = false;
							}
							else if (scan_WallNormal.x > 0 && scan_WallNormal.y > 0.f) {
								Point_bounce.emplace_back(vec3_t(g::local->origin().x - test_al,
									g::local->origin().y - test_al,
									g::local->origin().z + 54.f));
								scan_FirstPoint = false;
							}
							else if (scan_WallNormal.x == 0.f && scan_WallNormal.y > 0.f) {
								Point_bounce.emplace_back(vec3_t(g::local->origin().x, g::local->origin().y - test_al,
									g::local->origin().z + 54.f));
								scan_FirstPoint = false;
							}
							else if (scan_WallNormal.x == 0.f && scan_WallNormal.y < 0.f) {
								Point_bounce.emplace_back(vec3_t(g::local->origin().x, g::local->origin().y + test_al,
									g::local->origin().z + 54.f));
								scan_FirstPoint = false;
							}
							else if (scan_WallNormal.x < 0 && scan_WallNormal.y == 0.f) {
								Point_bounce.emplace_back(vec3_t(g::local->origin().x + test_al, g::local->origin().y,
									g::local->origin().z + 54.f));
								scan_FirstPoint = false;
							}
							else if (scan_WallNormal.x > 0 && scan_WallNormal.y == 0.f) {
								Point_bounce.emplace_back(vec3_t(g::local->origin().x - test_al, g::local->origin().y,
									g::local->origin().z + 54.f));
								scan_FirstPoint = false;
							}
						}

						if (un_pred_velo.z < 0.f && pred_velo.z < 0.f)
							break;
					}
					if (done_and_break)
						break;
				}

				cmd->buttons = buttons;
				cmd->forward_move = ForwardMove;

				// PIXELSURF SCAN (going DOWN)
				prediction::restore_ent_to_predicted_frame(PredictionFrames);
				Lerp = 0.f;

				for (int i = 0; i < 128; i++)
				{
					cmd->buttons |= in_jump;
					cmd->buttons |= in_duck;

					vec3_t Angles{ scan_WallNormal.x * -1.f, scan_WallNormal.y * -1.f, 0.f };
					vec3_t to_wall = Angles.to_angle();
					float rotation = math::deg2rad(to_wall.y - cmd->view_angles.y);
					float cos_rot = cos(rotation);
					float sin_rot = sin(rotation);
					float forwardmove = cos_rot * 10.f;
					float sidemove = -sin_rot * 10.f;
					cmd->forward_move = forwardmove;
					cmd->side_move = sidemove;

					if (g::local->get_velocity().z < 0) {
						static float Height = 0.f;
						if (scan_StartPos.z > scan_EndPos.z) {
							if (scan_StartPos.z < 0.f)
								Height = (int)scan_StartPos.z - 0.972092f - Lerp;
							else
								Height = (int)scan_StartPos.z + 0.0287018f - Lerp;
						}
						else {
							if (scan_EndPos.z < 0.f)
								Height = (int)scan_EndPos.z - 0.972092f - Lerp;
							else
								Height = (int)scan_EndPos.z + 0.0287018f - Lerp;
						}

						int maxLerp;
						if (scan_StartPos.z > scan_EndPos.z) {
							maxLerp = scan_StartPos.z - scan_EndPos.z;
						}
						else {
							maxLerp = scan_EndPos.z - scan_StartPos.z;
						}

						if (Lerp > maxLerp) {
							cmd->buttons = buttons;
							cmd->forward_move = ForwardMove;
							scan_FirstPoint = false;
							break;
						}
						else {
							Lerp += 1.f;
						}

						// Position player
						if (scan_WallNormal.x < 0 && scan_WallNormal.y < 0.f) {
							g::local->origin() = vec3_t(scan_StartPos.x - test_al, scan_StartPos.y - test_al, Height);
							g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
						}
						else if (scan_WallNormal.x < 0 && scan_WallNormal.y > 0.f) {
							g::local->origin() = vec3_t(scan_StartPos.x - test_al, scan_StartPos.y + test_al, Height);
							g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
						}
						else if (scan_WallNormal.x > 0 && scan_WallNormal.y < 0.f) {
							g::local->origin() = vec3_t(scan_StartPos.x + test_al, scan_StartPos.y - test_al, Height);
							g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
						}
						else if (scan_WallNormal.x > 0 && scan_WallNormal.y > 0.f) {
							g::local->origin() = vec3_t(scan_StartPos.x + test_al, scan_StartPos.y + test_al, Height);
							g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
						}
						else if (scan_WallNormal.x == 0.f && scan_WallNormal.y > 0.f) {
							g::local->origin() = vec3_t(scan_StartPos.x, scan_StartPos.y + test_al, Height);
							g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
						}
						else if (scan_WallNormal.x == 0.f && scan_WallNormal.y < 0.f) {
							g::local->origin() = vec3_t(scan_StartPos.x, scan_StartPos.y - test_al, Height);
							g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
						}
						else if (scan_WallNormal.x < 0 && scan_WallNormal.y == 0.f) {
							g::local->origin() = vec3_t(scan_StartPos.x - test_al, scan_StartPos.y, Height);
							g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
						}
						else if (scan_WallNormal.x > 0 && scan_WallNormal.y == 0.f) {
							g::local->origin() = vec3_t(scan_StartPos.x + test_al, scan_StartPos.y, Height);
							g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
						}

						// Check valid pixelsurf position
						trace_t trace;
						trace_filter flt(g::local);
						ray_t ray;
						ray.initialize(vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z + 54.f),
							vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z - 1000.f));

						interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &flt, &trace);
						vec3_t down_point = trace.end;

						trace_t trace2;
						trace_filter flt2(g::local);
						ray_t ray2;
						ray2.initialize(vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z + 54.f),
							vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z + 1000.f));

						interfaces::trace_ray->trace_ray(ray2, MASK_PLAYERSOLID, &flt2, &trace2);
						vec3_t upper_point = trace2.end;

						if (g::local->origin().z + 1.f < down_point.z)
							continue;
						if (g::local->origin().z + 1.f > upper_point.z)
							continue;
					}

					vec3_t un_pred_velo = g::local->get_velocity();

					prediction::begin(cmd);
					prediction::end();

					vec3_t pred_velo = g::local->get_velocity();

					if (g::local->get_velocity().z == targetZvelo) {
						// Add pixelsurf point
						if (scan_WallNormal.x < 0 && scan_WallNormal.y < 0.f) {
							Point_surf.emplace_back(vec3_t(g::local->origin().x + test_al, g::local->origin().y + test_al,
								g::local->origin().z));
							scan_FirstPoint = false;
						}
						else if (scan_WallNormal.x < 0 && scan_WallNormal.y > 0.f) {
							Point_surf.emplace_back(vec3_t(g::local->origin().x + test_al, g::local->origin().y - test_al,
								g::local->origin().z));
							scan_FirstPoint = false;
						}
						else if (scan_WallNormal.x > 0 && scan_WallNormal.y < 0.f) {
							Point_surf.emplace_back(vec3_t(g::local->origin().x - test_al, g::local->origin().y + test_al,
								g::local->origin().z));
							scan_FirstPoint = false;
						}
						else if (scan_WallNormal.x > 0 && scan_WallNormal.y > 0.f) {
							Point_surf.emplace_back(vec3_t(g::local->origin().x - test_al, g::local->origin().y - test_al,
								g::local->origin().z));
							scan_FirstPoint = false;
						}
						else if (scan_WallNormal.x == 0.f && scan_WallNormal.y > 0.f) {
							Point_surf.emplace_back(vec3_t(g::local->origin().x, g::local->origin().y - test_al,
								g::local->origin().z));
							scan_FirstPoint = false;
						}
						else if (scan_WallNormal.x == 0.f && scan_WallNormal.y < 0.f) {
							Point_surf.emplace_back(vec3_t(g::local->origin().x, g::local->origin().y + test_al,
								g::local->origin().z));
							scan_FirstPoint = false;
						}
						else if (scan_WallNormal.x < 0 && scan_WallNormal.y == 0.f) {
							Point_surf.emplace_back(vec3_t(g::local->origin().x + test_al, g::local->origin().y,
								g::local->origin().z));
							scan_FirstPoint = false;
						}
						else if (scan_WallNormal.x > 0 && scan_WallNormal.y == 0.f) {
							Point_surf.emplace_back(vec3_t(g::local->origin().x - test_al, g::local->origin().y,
								g::local->origin().z));
							scan_FirstPoint = false;
						}
					}
				}

				cmd->buttons = buttons;
				cmd->forward_move = ForwardMove;
				scan_FirstPoint = false;

				// After the existing pixelsurf scan, add these new scans:

// FLOOR SCAN - scan downward along the line
				prediction::restore_ent_to_predicted_frame(PredictionFrames);
				Lerp = 0.f;

				for (int i = 0; i < 128; i++)
				{
					float Height = 0.f;
					if (scan_StartPos.z > scan_EndPos.z) {
						Height = scan_StartPos.z - Lerp;
					}
					else {
						Height = scan_EndPos.z - Lerp;
					}

					int maxLerp = std::abs((int)(scan_StartPos.z - scan_EndPos.z));
					if (Lerp > maxLerp) break;

					Lerp += 1.f;

					// Trace downward to find floor
					trace_t trace;
					trace_filter flt(g::local);
					ray_t ray;
					vec3_t scan_pos = vec3_t(scan_StartPos.x, scan_StartPos.y, Height);
					ray.initialize(scan_pos, scan_pos + vec3_t(0, 0, -1000.f));

					interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &flt, &trace);

					if (trace.flFraction < 1.f && trace.plane.normal.z > 0.7f) {
						// Found floor contact point
						Point_floor.emplace_back(trace.end);
					}
				}

				// CEILING SCAN - scan upward along the line  
				prediction::restore_ent_to_predicted_frame(PredictionFrames);
				Lerp = 0.f;

				for (int i = 0; i < 128; i++)
				{
					float Height = 0.f;
					if (scan_StartPos.z > scan_EndPos.z) {
						Height = scan_StartPos.z - Lerp + 64.f; // Player eye height
					}
					else {
						Height = scan_EndPos.z - Lerp + 64.f;
					}

					int maxLerp = std::abs((int)(scan_StartPos.z - scan_EndPos.z));
					if (Lerp > maxLerp) break;

					Lerp += 1.f;

					// Trace upward to find ceiling
					trace_t trace;
					trace_filter flt(g::local);
					ray_t ray;
					vec3_t scan_pos = vec3_t(scan_StartPos.x, scan_StartPos.y, Height);
					ray.initialize(scan_pos, scan_pos + vec3_t(0, 0, 1000.f));

					interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &flt, &trace);

					if (trace.flFraction < 1.f && trace.plane.normal.z < -0.7f) {
						// Found ceiling contact point
						Point_ceiling.emplace_back(trace.end);
					}
				}
				// HEADBANG SCAN (going UP) - hits ceiling with head
				for (int v = 0; v < 4; v++) {
					prediction::restore_ent_to_predicted_frame(PredictionFrames);
					bool done_and_break = false;

					for (int i = 0; i < 128; i++)
					{
						cmd->buttons |= in_jump;
						cmd->buttons |= in_duck;

						vec3_t Angles{ scan_WallNormal.x * -1.f, scan_WallNormal.y * -1.f, 0.f };
						vec3_t to_wall = Angles.to_angle();
						float rotation = math::deg2rad(to_wall.y - cmd->view_angles.y);
						float cos_rot = cos(rotation);
						float sin_rot = sin(rotation);
						float forwardmove = cos_rot * 10.f;
						float sidemove = -sin_rot * 10.f;
						cmd->forward_move = forwardmove;
						cmd->side_move = sidemove;

						if (g::local->get_velocity().z > 0)
						{
							static float Height = 0.f;
							if (scan_StartPos.z > scan_EndPos.z) {
								if (scan_StartPos.z < 0.f)
									Height = (int)scan_StartPos.z - 0.02f - Lerp - 54.f;
								else
									Height = (int)scan_StartPos.z + 0.02f + Lerp - 54.f;
							}
							else {
								if (scan_EndPos.z < 0.f)
									Height = (int)scan_EndPos.z - 0.02f - Lerp - 54.f;
								else
									Height = (int)scan_EndPos.z + 0.02f + Lerp - 54.f;
							}

							int maxLerp;
							if (scan_StartPos.z > scan_EndPos.z) {
								maxLerp = scan_StartPos.z - scan_EndPos.z;
							}
							else {
								maxLerp = scan_EndPos.z - scan_StartPos.z;
							}

							if (Lerp > maxLerp) {
								cmd->buttons = buttons;
								cmd->forward_move = ForwardMove;
								scan_FirstPoint = false;
								done_and_break = true;
								break;
							}
							else {
								Lerp += 1.f;
							}

							// Position player based on wall normal
							if (scan_WallNormal.x < 0 && scan_WallNormal.y < 0.f) {
								g::local->origin() = vec3_t(scan_StartPos.x - test_al, scan_StartPos.y - test_al, Height);
								g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
							}
							else if (scan_WallNormal.x < 0 && scan_WallNormal.y > 0.f) {
								g::local->origin() = vec3_t(scan_StartPos.x - test_al, scan_StartPos.y + test_al, Height);
								g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
							}
							else if (scan_WallNormal.x > 0 && scan_WallNormal.y < 0.f) {
								g::local->origin() = vec3_t(scan_StartPos.x + test_al, scan_StartPos.y - test_al, Height);
								g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
							}
							else if (scan_WallNormal.x > 0 && scan_WallNormal.y > 0.f) {
								g::local->origin() = vec3_t(scan_StartPos.x + test_al, scan_StartPos.y + test_al, Height);
								g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
							}
							else if (scan_WallNormal.x == 0.f && scan_WallNormal.y > 0.f) {
								g::local->origin() = vec3_t(scan_StartPos.x, scan_StartPos.y + test_al, Height);
								g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
							}
							else if (scan_WallNormal.x == 0.f && scan_WallNormal.y < 0.f) {
								g::local->origin() = vec3_t(scan_StartPos.x, scan_StartPos.y - test_al, Height);
								g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
							}
							else if (scan_WallNormal.x < 0 && scan_WallNormal.y == 0.f) {
								g::local->origin() = vec3_t(scan_StartPos.x - test_al, scan_StartPos.y, Height);
								g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
							}
							else if (scan_WallNormal.x > 0 && scan_WallNormal.y == 0.f) {
								g::local->origin() = vec3_t(scan_StartPos.x + test_al, scan_StartPos.y, Height);
								g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
							}

							// Check if valid headbang position (crouched height + head clearance)
							// Crouched player is 54 units tall, check from top of crouch hitbox
							trace_t trace;
							trace_filter flt(g::local);
							ray_t ray;
							ray.initialize(vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z + 54.f),
								vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z - 1000.f));

							interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &flt, &trace);
							vec3_t down_point = trace.end;

							// Check CEILING collision - this is the key difference from bounce scan
							trace_t trace2;
							trace_filter flt2(g::local);
							ray_t ray2;
							// Ray from crouch head position upward to check for ceiling
							ray2.initialize(vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z + 54.f),
								vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z + 1000.f)); // Only check 46 units up (54 crouch + 46 = ~100 standing height)

							interfaces::trace_ray->trace_ray(ray2, MASK_PLAYERSOLID, &flt2, &trace2);
							vec3_t upper_point = trace2.end;

							if (g::local->origin().z + 1.f < down_point.z)
								continue;
							if (g::local->origin().z + 1.f > upper_point.z)
								continue;
						}

						vec3_t un_pred_velo = g::local->get_velocity();

						prediction::begin(cmd);
						prediction::end();

						vec3_t pred_velo = g::local->get_velocity();


						// Detect headbang position
						if (pred_velo.z == targetZvelo) {
							vec3_t final_pos;
							if (scan_WallNormal.x < 0 && scan_WallNormal.y < 0.f) {
								final_pos = vec3_t(g::local->origin().x + test_al, g::local->origin().y + test_al, g::local->origin().z + 54.f);
							}
							else if (scan_WallNormal.x < 0 && scan_WallNormal.y > 0.f) {
								final_pos = vec3_t(g::local->origin().x + test_al, g::local->origin().y - test_al, g::local->origin().z + 54.f);
							}
							else if (scan_WallNormal.x > 0 && scan_WallNormal.y < 0.f) {
								final_pos = vec3_t(g::local->origin().x - test_al, g::local->origin().y + test_al, g::local->origin().z + 54.f);
							}
							else if (scan_WallNormal.x > 0 && scan_WallNormal.y > 0.f) {
								final_pos = vec3_t(g::local->origin().x - test_al, g::local->origin().y - test_al, g::local->origin().z + 54.f);
							}
							else if (scan_WallNormal.x == 0.f && scan_WallNormal.y > 0.f) {
								final_pos = vec3_t(g::local->origin().x, g::local->origin().y - test_al, g::local->origin().z + 54.f);
							}
							else if (scan_WallNormal.x == 0.f && scan_WallNormal.y < 0.f) {
								final_pos = vec3_t(g::local->origin().x, g::local->origin().y + test_al, g::local->origin().z + 54.f);
							}
							else if (scan_WallNormal.x < 0 && scan_WallNormal.y == 0.f) {
								final_pos = vec3_t(g::local->origin().x + test_al, g::local->origin().y, g::local->origin().z + 54.f);
							}
							else if (scan_WallNormal.x > 0 && scan_WallNormal.y == 0.f) {
								final_pos = vec3_t(g::local->origin().x - test_al, g::local->origin().y, g::local->origin().z + 54.f);
							}

							Point_headbang.emplace_back(final_pos);
						}

						if (un_pred_velo.z < 0.f && pred_velo.z < 0.f)
							break;
				}
				if (done_and_break)
					break;
			}

			cmd->buttons = buttons;
			cmd->forward_move = ForwardMove;

				// Clean up pixelsurf points (remove duplicates)
				if (Point_surf.size() > 1) {
					for (int i = 0; i < Point_surf.size() - 1; ) {
						if ((int)Point_surf[i].z - (int)Point_surf[i + 1].z == 1) {
							Point_surf.erase(Point_surf.begin() + i);
						}
						else {
							++i;
						}
					}
				}

				// Convert scanned points to route markers
				convert_scanned_points_to_markers(Point_surf, Point_bounce, Point_floor, Point_ceiling, Point_headbang);

				// Clear temporary scan data
				Point_surf.clear();
				Point_bounce.clear();
				Point_floor.clear();      // ADD THIS
				Point_ceiling.clear();    // ADD THIS
				Point_headbang.clear();   // ADD THIS
			}
		}
	}

	bool is_too_close(const vec3_t& p, jumptype_t type, float min_dist = 20.0f)
	{
		for (const auto& m : route_markers) {
			if (m.type == type) {
				if ((m.position - p).length() < min_dist)
					return true;
			}
		}
		return false;
	}

	void convert_scanned_points_to_markers(
		const std::vector<vec3_t>& pixelsurf_points,
		const std::vector<vec3_t>& bounce_points,
		const std::vector<vec3_t>& floor_points,      // ADD THIS
		const std::vector<vec3_t>& ceiling_points,    // ADD THIS  
		const std::vector<vec3_t>& headbang_points)   // ADD THIS
	{
		int added = 0;
		std::string current_map = interfaces::engine->get_level_name();

		if (c::assist::assist_enable_pixelsurf_points) {
			for (const auto& point : pixelsurf_points)
			{
				if (!is_too_close(point, pixelsurf, 20.0f))
				{
					jump_marker_t marker;
					marker.position = point;
					marker.velocity = point;
					marker.type = pixelsurf;
					marker.name = "pixelsurf";
					marker.map = current_map;  // ASSIGN MAP

					route_markers.push_back(marker);
					added++;
				}
			}
		}

		if (c::assist::assist_enable_bounce_points) {
			for (const auto& point : bounce_points)
			{
				if (!is_too_close(point, bounce, 20.0f))
				{
					jump_marker_t marker;
					marker.position = point;
					marker.velocity = point;
					marker.type = bounce;
					marker.name = "bounce";
					marker.map = current_map;  // ASSIGN MAP

					route_markers.push_back(marker);
					added++;
				}
			}
		}

		if (c::assist::assist_enable_floor_points) {
			for (const auto& point : floor_points) {
				if (!is_too_close(point, floor, 20.0f)) {
					jump_marker_t marker;
					marker.position = point;
					marker.velocity = point;
					marker.type = floor;
					marker.name = "floor";
					marker.map = current_map;
					route_markers.push_back(marker);
					added++;
				}
			}
		}

		// ADD CEILING POINTS
		if (c::assist::assist_enable_ceiling_points) {
			for (const auto& point : ceiling_points) {
				if (!is_too_close(point, ceiling, 20.0f)) {
					jump_marker_t marker;
					marker.position = point;
					marker.velocity = point;
					marker.type = ceiling;
					marker.name = "ceiling";
					marker.map = current_map;
					route_markers.push_back(marker);
					added++;
				}
			}
		}

		// ADD HEADBANG POINTS
		if (c::assist::assist_enable_headbang_points) {
			for (const auto& point : headbang_points) {
				if (!is_too_close(point, headbang, 20.0f)) {
					jump_marker_t marker;
					marker.position = point;
					marker.velocity = point;
					marker.type = headbang;
					marker.name = "headbang";
					marker.map = current_map;
					route_markers.push_back(marker);
					added++;
				}
			}
		}


		if (added > 0) {
			interfaces::chat_element->chatprintf("RouteStudio: scanned", added);
			current_marker_index = static_cast<int>(route_markers.size()) - 1;
		}
	}

	void detect_multisurf_chains(std::vector<jump_marker_t>& markers, c_usercmd* cmd, float max_gap = FLT_MAX) {
		if (markers.empty() || !g::local || !g::local->is_alive())
			return;

		if (g::local->move_type() == movetype_ladder || g::local->move_type() == movetype_noclip)
			return;

		int next_chain_id = 0;
		for (auto& m : markers) {
			m.chain_id = -1;
			m.is_chain_start = false;
			m.is_chain_end = false;
		}

		bool in_chain = false;
		int current_chain_id = -1;

		for (size_t i = 0; i < markers.size(); ++i) {
			bool this_is_surf = (markers[i].type == pixelsurf || markers[i].type == headbang || markers[i].type == bounce);
			bool next_is_surf = false;
			if (i + 1 < markers.size()) {
				next_is_surf = (markers[i + 1].type == pixelsurf || markers[i + 1].type == headbang || markers[i + 1].type == bounce);
			}

			float gap = FLT_MAX;
			if (i + 1 < markers.size())
				gap = (markers[i + 1].position - markers[i].position).length();

			if (this_is_surf && next_is_surf && gap <= max_gap) {
				if (!in_chain) {
					in_chain = true;
					current_chain_id = next_chain_id++;
					markers[i].is_chain_start = true;
					markers[i].chain_id = current_chain_id;
				}
				markers[i].chain_id = current_chain_id;
				markers[i + 1].chain_id = current_chain_id;
			}
			else {
				if (in_chain) {
					size_t end_index = i;
					if (!this_is_surf && i > 0)
						end_index = i - 1;

					markers[end_index].is_chain_end = true;
					in_chain = false;
					current_chain_id = -1;
				}
			}
		}

		if (in_chain) {
			int last_id = next_chain_id - 1;
			for (int i = (int)markers.size() - 1; i >= 0; --i) {
				if (markers[i].chain_id == last_id) {
					markers[i].is_chain_end = true;
					break;
				}
			}
		}
	}

	void chain_velocity_assist(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive())
			return;

		if (!c::movement::chain_assist) // Add this config option
			return;

		if (g::local->flags() & fl_onground)
			return;

		if (!interfaces::engine->is_in_game())
			return;

		if (PixelSurfAssistant.ps_data.pixelsurfing)
			return;

		if (route_markers.empty())
			return;

		// Physics constants
		float sv_gravity = interfaces::console->get_convar("sv_gravity")->get_float();
		float tick_rate = (tick_interval > 0) ? (1.0f / tick_interval) : 0.0f;
		float target_z_velo = ((sv_gravity / 2.0f) / tick_rate) * -1.0f;

		// Don't assist if already at perfect Z velocity
		if (fabsf(prediction_backup::velocity.z - target_z_velo) < 0.1f)
			return;

		vec3_t player_pos = g::local->origin();
		vec3_t player_vel = g::local->get_velocity();

		// Find nearby markers and sort by distance
		std::vector<std::pair<float, int>> nearby_markers;
		for (int i = 0; i < route_markers.size(); i++) {
			if (interfaces::engine->get_level_name() != route_markers[i].map)
				continue;
			if (!route_markers[i].active)
				continue;

			float distance = player_pos.distance_to(route_markers[i].position);
			float search_radius = route_markers[i].radius * 2.0f; // Wider search

			if (distance <= search_radius) {
				nearby_markers.push_back(std::make_pair(distance, i));
			}
		}

		if (nearby_markers.empty())
			return;

		// Sort by distance (closest first)
		std::sort(nearby_markers.begin(), nearby_markers.end());

		// Get closest marker
		int target_index = nearby_markers[0].second;
		const auto& target_marker = route_markers[target_index];
		vec3_t target_pos = target_marker.position;

		// Calculate trajectory
		vec3_t to_target = target_pos - player_pos;
		float horizontal_dist = vec3_t(to_target.x, to_target.y, 0).length();
		float vertical_dist = to_target.z;

		// Don't assist if too close horizontally (avoid jitter)
		if (horizontal_dist < 16.0f)
			return;

		// Calculate time to reach target based on current horizontal velocity
		float horizontal_vel = sqrtf(player_vel.x * player_vel.x + player_vel.y * player_vel.y);
		if (horizontal_vel < 50.0f) // Minimum velocity threshold
			return;

		float time_to_target = horizontal_dist / horizontal_vel;
		time_to_target = std::clamp(time_to_target, 0.05f, 2.0f); // Clamp to reasonable range

		// Calculate required Z velocity to reach target
		// Using: z = z0 + vz*t - 0.5*g*t^2
		// Solving for vz: vz = (z - z0 + 0.5*g*t^2) / t
		float gravity_drop = 0.5f * sv_gravity * time_to_target * time_to_target;
		float required_z_vel = (vertical_dist + gravity_drop) / time_to_target;

		// Clamp to reasonable velocity limits
		required_z_vel = std::clamp(required_z_vel, -450.0f, 450.0f);

		// Calculate assist strength based on distance and alignment
		float distance_factor = 1.0f - std::min<float>(1.0f, horizontal_dist / (target_marker.radius * 2.0f));

		// Check velocity alignment with target direction
		vec3_t vel_dir = vec3_t(player_vel.x, player_vel.y, 0).normalized();
		vec3_t target_dir = vec3_t(to_target.x, to_target.y, 0).normalized();
		float alignment = vel_dir.dot(target_dir);

		// Only assist if moving towards target
		if (alignment < 0.3f)
			return;

		// Smooth assist strength
		float assist_strength = distance_factor * alignment * 1.f; // Add config: 0-1 range
		assist_strength = std::clamp(assist_strength, 0.0f, 1.0f);

		// Type-specific multipliers
		float type_multiplier = 1.0f;
		switch (target_marker.type) {
		case pixelsurf:
			type_multiplier = 1.5f;
			break;
		case bounce:
			type_multiplier = 1.8f;
			break;
		case headbang:
			type_multiplier = 1.3f;
			break;
		default:
			type_multiplier = 1.0f;
			break;
		}

		assist_strength *= type_multiplier;

		// Backup state for prediction
		int backup_predicted = interfaces::prediction->split->commands_predicted;
		vec3_t backup_vel = g::local->get_velocity();
		vec3_t backup_origin = g::local->origin();

		// Predict trajectory with new velocity
		prediction::restore_ent_to_predicted_frame(backup_predicted - 1);

		// Calculate smooth velocity adjustment
		float current_z_vel = player_vel.z;
		float velocity_diff = required_z_vel - current_z_vel;
		float velocity_adjustment = velocity_diff * assist_strength;

		// Apply velocity adjustment
		vec3_t new_velocity = g::local->get_velocity();
		new_velocity.z += velocity_adjustment;
		g::local->velocity() = new_velocity;

		// Also apply to cmd->up_move for smoother control
		cmd->up_move = velocity_adjustment * 1.5f;

		// Strafe correction towards marker
		if (horizontal_dist > 32.0f && c::movement::chain_strafe_correction) { // Add config option
			vec3_t desired_dir = target_dir;
			vec3_t current_dir = vel_dir;

			// Calculate strafe adjustment
			float strafe_correction_strength = 0.1f * assist_strength;

			// Apply gentle steering towards marker
			vec3_t velocity_2d(player_vel.x, player_vel.y, 0);
			vec3_t desired_velocity = desired_dir * horizontal_vel;

			vec3_t corrected_velocity = math::lerp_vec(velocity_2d, desired_velocity, strafe_correction_strength);

			cmd->forward_move = corrected_velocity.x;
			cmd->side_move = corrected_velocity.y;
		}
	}

	void smooth_chain_movement(c_usercmd* cmd) {
		if (!c::movement::chain_assist) // Add this config option
			return;

		if (!g::local || !g::local->is_alive())
			return;

		if (g::local->flags() & fl_onground)
			return;

		if (route_markers.empty())
			return;

		if (current_marker_index == -1 || current_marker_index >= route_markers.size())
			return;

		const auto& current_marker = route_markers[current_marker_index];
		vec3_t player_pos = g::local->origin();

		// Check if we've reached current marker
		float dist_to_current = player_pos.distance_to(current_marker.position);
		float horizontal_dist = vec3_t(
			player_pos.x - current_marker.position.x,
			player_pos.y - current_marker.position.y,
			0
		).length();

		// Marker completion detection
		if (horizontal_dist < 24.0f && fabsf(player_pos.z - current_marker.position.z) < 16.0f) {
			// Move to next marker
			current_marker_index++;
			if (current_marker_index >= route_markers.size()) {
				current_marker_index = -1; // Loop or end
			}
		}
	}

	void calculate_perfect_z_velocity(c_usercmd* cmd, const jump_marker_t& marker) {
		if (!c::movement::chain_assist) // Add this config option
			return;

		if (!g::local || !g::local->is_alive())
			return;

		if (g::local->flags() & fl_onground)
			return;

		vec3_t player_pos = g::local->origin();
		vec3_t player_vel = g::local->velocity();

		// Calculate trajectory
		float dist_to_marker = player_pos.distance_to(marker.position);
		if (dist_to_marker > 128.0f)
			return; // Too far

		float horizontal_dist = vec3_t(
			marker.position.x - player_pos.x,
			marker.position.y - player_pos.y,
			0
		).length();

		float horizontal_speed = sqrtf(player_vel.x * player_vel.x + player_vel.y * player_vel.y);
		if (horizontal_speed < 1.0f)
			return;

		float time_to_marker = horizontal_dist / horizontal_speed;

		float gravity = interfaces::console->get_convar("sv_gravity")->get_float();
		float height_diff = marker.position.z - player_pos.z;

		// Perfect Z velocity calculation
		float required_z_velocity = (height_diff / time_to_marker) + (0.5f * gravity * time_to_marker);

		// Smooth application
		float current_z = player_vel.z;
		float velocity_adjustment = (required_z_velocity - current_z) * 0.3f; // Smooth factor

		if (cmd->buttons & in_jump) {
			g::local->velocity().z += velocity_adjustment;
			//cmd->up_move = velocity_adjustment * 1.2f;
		}
	}

	void movement_update(c_usercmd* cmd) {
		// Calculate perfect Z velocity for current marker
		if (current_marker_index != -1 && current_marker_index < route_markers.size()) {
			const auto& marker = route_markers[current_marker_index];
			calculate_perfect_z_velocity(cmd, marker);
		}
	}

	features::movement::pixelsurf_calculator::request_t build_route_request() {
		features::movement::pixelsurf_calculator::request_t request;

		if (!g::local || !g::local->is_alive() || route_markers.empty())
			return request;

		const std::string current_map = get_current_map_name();
		std::vector<int> planner_indices;
		planner_indices.reserve(route_markers.size());

		for (int i = 0; i < static_cast<int>(route_markers.size()); ++i) {
			if (is_planner_marker(route_markers[i]) &&
				!route_markers[i].transient &&
				route_markers[i].map == current_map)
				planner_indices.push_back(i);
		}

		request.placed_points = static_cast<int>(planner_indices.size());
		for (const int index : planner_indices) {
			if (route_markers[index].type == floor)
				request.placed_floor_points++;
			else
				request.placed_pixelsurf_points++;
		}
		request.chained_mode = true;

		if (planner_indices.empty())
			return request;

		auto planner_visible_index = [&](int marker_index) -> int {
			const auto it = std::find(planner_indices.begin(), planner_indices.end(), marker_index);
			if (it == planner_indices.end())
				return -1;
			return static_cast<int>(std::distance(planner_indices.begin(), it)) + 1;
		};

		auto marker_type_name = [](const jump_marker_t& marker) -> const char* {
			return marker.type == floor ? "floor" : "pixelsurf";
		};

		auto append_marker = [&](const jump_marker_t& marker, int marker_index) {
			features::movement::pixelsurf_calculator::target_t target;
			target.pos = marker.position;
			target.normal = marker.surface_normal;
			target.kind = marker.type == floor
				? features::movement::pixelsurf_calculator::target_kind_t::floor_point
				: features::movement::pixelsurf_calculator::target_kind_t::pixelsurf_point;
			target.marker_index = marker_index;
			target.radius = marker.transient
				? 24.0f
				: std::clamp(c::movement::pscalc_scan_distance, 12.0f, (std::max)(12.0f, marker.radius));
			target.allow_stand = marker.stand;
			target.allow_duck = marker.duck;
			target.jump_stand = marker.jump;
			target.jump_crouch = marker.c_jump;
			target.minijump_stand = marker.minijump;
			target.minijump_crouch = marker.c_minijump;
			target.longjump_stand = marker.longjump;
			target.longjump_crouch = marker.c_longjump;
			target.crouch_hop_stand = marker.crouch_hop;
			target.crouch_hop_crouch = marker.c_crouch_hop;
			target.jumpbug_stand = marker.jumpbug;
			target.jumpbug_crouch = marker.c_jumpbug;
			target.mini_crouch_hop_stand = marker.mini_crouch_hop;
			target.mini_crouch_hop_crouch = marker.c_mini_crouch_hop;

			const bool has_any_capability =
				target.jump_stand ||
				target.jump_crouch ||
				target.minijump_stand ||
				target.minijump_crouch ||
				target.longjump_stand ||
				target.longjump_crouch ||
				target.crouch_hop_stand ||
				target.crouch_hop_crouch ||
				target.jumpbug_stand ||
				target.jumpbug_crouch ||
				target.mini_crouch_hop_stand ||
				target.mini_crouch_hop_crouch;

			if (!has_any_capability) {
				target.jump_stand = true;
				target.jump_crouch = false;
				target.minijump_stand = true;
				target.minijump_crouch = false;
				target.longjump_stand = true;
				target.longjump_crouch = false;
				target.crouch_hop_stand = false;
				target.crouch_hop_crouch = false;
				target.jumpbug_stand = false;
				target.jumpbug_crouch = false;
				target.mini_crouch_hop_stand = false;
				target.mini_crouch_hop_crouch = false;
			}

			request.targets.push_back(target);

			const int visible_index = planner_visible_index(marker_index);
			if (!request.target_order.empty())
				request.target_order.append(" -> ");
			request.target_order.append("#");
			request.target_order.append(std::to_string(visible_index > 0 ? visible_index : marker_index + 1));
			request.target_order.push_back(' ');
			request.target_order.append(marker_type_name(marker));
		};

		int final_pixelsurf_index = -1;
		for (const int marker_index : planner_indices) {
			if (route_markers[marker_index].type != floor)
				final_pixelsurf_index = marker_index;
		}

		if (final_pixelsurf_index == -1)
			return request;

		for (const int marker_index : planner_indices) {
			append_marker(route_markers[marker_index], marker_index);
			if (marker_index == final_pixelsurf_index)
				break;
		}

		return request;
	}

	bool issimilarlowfractionpair(double a, double b)
	{
		if (a > 0 && b > 0) {
			double frac_a = a - std::floor(a);
			double frac_b = b - std::floor(b);
			if (frac_a > 0 && frac_a < 0.3 && frac_b > 0 && frac_b < 0.3) {
				return true;
			}
			else {
				return false;
			}
		}
		else if (a < 0 && b < 0) {
			int i_a = (int)a;
			int i_b = (int)b;
			if (i_a != i_b) {
				return false;
			}
			float outofpoint_a = a - i_a;
			float outofpoint_b = b - i_b;
			int full_a = std::abs(outofpoint_a * 1000000.f);
			int full_b = std::abs(outofpoint_b * 1000000.f);
			if (full_b <= full_a && full_a < 990000) {
				return true;
			}
			else {
				return false;
			}
		}
		else {
			return false;
		}
	}

	bool calc(float a, float b)
	{
		int a1 = (int)a;
		int b1 = (int)b;
		if (b < 0) {
			if (a1 == b1) {
				int a2 = (a - a1) * 100;
				int b2 = (b - b1) * 100;
				int a3 = (a - a1) * 1000;
				if (b2 == a2 || a2 + 1 == b2 || a2 + 2 == b2)
					return true;
				else
					return false;
			}
		}
		else {
			if (a1 == b1) {
				int a2 = (a - a1) * 100;
				int b2 = (b - b1) * 100;
				int a3 = (a - a1) * 1000;
				if (b2 == a2 || a2 == b2 - 1 || a2 == b2 - 2)
					return true;
				else
					return false;
			}
		}

		return false;
	}

	void jump_to_marker(c_usercmd* cmd) {
		static bool HitJump{ false };
		static bool HitMiniJump{ false };
		static bool HitLongJump{ false };
		static bool HitCHop{ false };
		static bool HitMiniChop{ false };
		static bool HitJumpBug{ false };

		// ============================================================
		// CACHED SCAN RESULTS
		// ============================================================
		struct ScanResult {
			bool valid = false;
			int jump_type = -1; // 0=jump, 1=mini, 2=chop, 3=jb
			int tick_offset = 0;
			int marker_index = -1;
			float target_z = 0.f;
			uint64_t scan_time = 0;
		};
		static std::map<int, ScanResult> cached_scans; // marker_index -> result
		static uint64_t last_cache_clear = 0;
		const uint64_t CACHE_LIFETIME_MS = 1; // Cache results for 500ms
		const int MAX_SCAN_TICKS_PER_FRAME = 127; // Limit scan iterations per frame

		if (!g::local)
			return;

		if (!c::assist::pixelsurf_assist)
			return;

		if (!interfaces::engine->is_in_game())
			return;

		if (!g::local->is_alive())
			return;

		if (PixelSurfAssistant.ps_data.pixelsurfing || features::movement::detect_data.ticks_left)
			return;

		float sv_gravity2 = interfaces::console->get_convar(("sv_gravity"))->get_float();
		float fTickInterval = interfaces::globals->interval_per_tick;
		float fTickRate = (fTickInterval > 0) ? (1.0f / fTickInterval) : 0.0f;
		float veloZoftick = (((sv_gravity2 / 2) / fTickRate) * -1.f);
		vec3_t ray_shit = g::local->abs_origin() + vec3_t(0.f, 0.f, 64.f);

		static int toggletime = 0;
		static int ticks = 0;
		static int ljticks = 0;
		static int TicksOut = 0;

		if (!menu::checkkey(c::assist::pixelsurf_assist_key, c::assist::pixelsurf_assist_key_s)) {
			features::movement::AlertJB = false;
			features::movement::HITGODA = false;
			HitJump = false;
			HitMiniJump = false;
			HitLongJump = false;
			HitCHop = false;
			HitJumpBug = false;
			HitMiniChop = false;
			float_ground = false;
			cmd_i = 0;
			do_fucking_strafe = false;
			index_continue = -1;
			TicksOut = 0;  // ADD THIS - Reset cooldown timer
			ticks = 0;     // ADD THIS - Reset tick counter
			cached_scans.clear(); // Clear cache when key released
			return;
		}

		if (prediction_backup::velocity.z == veloZoftick)
			return;

		// Clear old cache periodically
		uint64_t current_time = GetTickCount64();
		if (current_time - last_cache_clear > CACHE_LIFETIME_MS) {
			for (auto it = cached_scans.begin(); it != cached_scans.end(); ) {
				if (current_time - it->second.scan_time > CACHE_LIFETIME_MS) {
					it = cached_scans.erase(it);
				}
				else {
					++it;
				}
			}
			last_cache_clear = current_time;
		}

		if (route_markers.size() > 0) {
			std::vector<std::pair<float, int>> points;

			for (int i = 0; i < route_markers.size(); i++) {
				if (interfaces::engine->get_level_name() == route_markers.at(i).map && route_markers.at(i).active) {
					float Distance = g::local->origin().distance_to(route_markers.at(i).position);
					if (Distance <= route_markers.at(i).radius) {
						points.push_back(std::make_pair(Distance, i));
					}
				}
			}

			std::sort(points.begin(), points.end());

			int max = std::min<float>(c::assist::pixelsurf_max_points, (int)points.size());

			for (int point_index = 0; point_index < max; point_index++) {
				int index = points[point_index].second;

				if (index < 0 || index >= (int)route_markers.size())
					continue;

				if (index != -1)
				{
					vec3_t Surf = route_markers.at(index).position;

					auto isLargeAngleDifference = [](float yaw1, float yaw2) -> bool {
						float yawDifference = std::abs(yaw1 - yaw2);
						if (yawDifference > 180.0f)
							yawDifference = 360.0f - yawDifference;
						// CHANGED: Reduced from 90° to 45° for wider angle acceptance
						return yawDifference >= 45.f;
						};

					int BackupButtons = cmd->buttons;
					float ForwaMove = cmd->forward_move;
					float SideMove = cmd->side_move;

					vec3_t BackupVelo = g::local->get_velocity();
					vec3_t Origin = g::local->origin();
					float stamina = g::local->stamina();
					int BackupPredicted = interfaces::prediction->split->commands_predicted;

					if (!c::assist::assist_broke_hop) {
						if (cmd->tick_count > ticks && prediction_backup::flags & 1 && g::local->stamina() != 0.f)
							return;
					}

					if (c::assist::assist_broke_hop) {
						if (cmd->tick_count > ticks && prediction_backup::flags & 1 && g::local->stamina() > c::assist::assist_stamina_value)
							return;
					}

					if (prediction_backup::velocity.z < 0.f)
						ljticks = 0;

					int g = (prediction_backup::flags & 1) ? 2 : 4;
					int hitgroudtick{ };
					vec3_t ChatPositions{ g::local->origin() };
					bool ChatStand{ };
					bool ChatDuck{ };

					static vec3_t last{ };

					float BackupForwardMove = cmd->forward_move;
					float BackupSideMove = cmd->side_move;
					vec3_t angViewPointBackup = cmd->view_angles;
					float DeltaSalier = route_markers.at(index).delta_strafe * 45.f; // 1.5x multiplier
					vec3_t DeltaAngle = (angViewPointBackup - last) * route_markers.at(index).delta_strafe;
					last = cmd->view_angles;

					// ============================================================
					// CHECK CACHE FIRST
					// ============================================================
					bool use_cached = false;
					ScanResult cached_result;
					if (cached_scans.find(index) != cached_scans.end()) {
						cached_result = cached_scans[index];
						if (cached_result.valid && (current_time - cached_result.scan_time < CACHE_LIFETIME_MS)) {
							use_cached = true;

							// Apply cached result
							switch (cached_result.jump_type) {
							case 0: HitJump = true; break;
							case 1: HitMiniJump = true; break;
							case 2: HitCHop = true; break;
							case 3: HitJumpBug = true; break;
							case 4: HitLongJump = true; break;
							case 5: HitMiniChop = true; break;
							}
							ticks = cmd->tick_count + cached_result.tick_offset;
						}
					}

					// ============================================================
					// OPTIMIZED SCAN (ONLY IF NOT CACHED)
					// ============================================================
					if (!use_cached) {
						for (int v = 0; v < g; v++) {
							prediction::restore_ent_to_predicted_frame(BackupPredicted - 1);
							cmd->side_move = BackupSideMove;
							cmd->forward_move = BackupForwardMove;
							cmd->buttons = BackupButtons;
							cmd->view_angles = angViewPointBackup;
							bool jb = false;
							int wait = 0;
							int once = 0;
							vec3_t Calculate{ };
							float iCalcilate{ };
							int once_fix = 0;
							vec3_t currentAngle = cmd->view_angles;

							if (!HitJump && !HitMiniJump && !HitLongJump && !HitCHop && !HitJumpBug && !HitMiniChop) {
								int max_scan_ticks = time_to_ticks(c::assist::pixelsurf_assist_ticks);

								// Adaptive scan step - skip ticks when far from target
								int scan_step = 1;
								float dist_to_marker = g::local->origin().distance_to(Surf);
								if (dist_to_marker > 200.f) {
									scan_step = 3; // Check every 3rd tick when far
								}
								else if (dist_to_marker > 100.f) {
									scan_step = 2; // Check every 2nd tick when medium distance
								}

								int scanned_this_frame = 0;

								for (int i = 0; i < max_scan_ticks; i += scan_step) {
									// Limit iterations per frame to prevent lag
									if (scanned_this_frame >= MAX_SCAN_TICKS_PER_FRAME) {
										break;
									}
									scanned_this_frame++;

									int max_tick_on_ground = 0;
									cmd->side_move = BackupSideMove;
									cmd->forward_move = BackupForwardMove;

									if (!(g::local->flags() & 1)) {
										if (!isLargeAngleDifference(currentAngle.y, angViewPointBackup.y)) {
											// Increase angle delta application for better sweep
											vec3_t extendedDelta = DeltaAngle * (float)scan_step * 45.f;
											currentAngle = (currentAngle + extendedDelta).normalized().clamped();
										}
										else {
											// If angle difference is too large, blend back towards target
											vec3_t toTarget = (Surf - g::local->origin()).normalized();
											float targetYaw = toTarget.to_angle().y;
											float yawDiff = targetYaw - currentAngle.y;

											// Normalize angle difference to -180 to 180
											while (yawDiff > 180.f) yawDiff -= 360.f;
											while (yawDiff < -180.f) yawDiff += 360.f;

											// Blend towards target (25% per tick)
											currentAngle.y += yawDiff * 0.25f;
											currentAngle = currentAngle.normalized().clamped();
										}

										cmd->view_angles = currentAngle;
										UtilityAssistant.handle_fix(cmd, currentAngle);
										cmd->view_angles = angViewPointBackup;
										UtilityAssistant.handle_fix(cmd, angViewPointBackup);
									}


									if (c::movement::pixel_surf_fix) {
										if (prediction_backup::velocity.length_2d() >= 285.91f && once_fix == 0) {
											auto sv_airAcelerate = interfaces::console->get_convar("sv_airaccelerate");
											float Razn = ((prediction_backup::velocity.length_2d() + 2.f - 285.91f) / 12.f * 64.f);
											vec3_t velocity = prediction_backup::velocity * -1.f;
											velocity.z = 0.f;
											float rotation = math::deg2rad(velocity.to_angle().y - cmd->view_angles.y);
											float cos_rot = cos(rotation);
											float sin_rot = sin(rotation);

											float forwardmove = cos_rot * Razn;
											float sidemove = -sin_rot * Razn;
											cmd->forward_move = forwardmove;
											cmd->side_move = sidemove;
											once_fix = 1;
										}
									}

									if (g::local->flags() & 1) {
										max_tick_on_ground += 1;
										once += 1;
										trace_t trace;
										ray_t ray_t;

										vec3_t start = g::local->abs_origin();

										ray_t.initialize(start, ray_shit);

										trace_world_only flt;

										interfaces::trace_ray->trace_ray(ray_t, MASK_PLAYERSOLID, &flt, &trace);
										if ((trace.flFraction < 1.f) && (trace.plane.normal.z == 1.f))
											float_ground = true;
									}

									if (once == 1)
										ChatPositions = g::local->origin();

									if (v == 0) {
										if (g::local->flags() & 1)
											cmd->buttons |= in_jump;
									}
									if (v == 1) {
										if (g::local->flags() & 1) {
											cmd->buttons |= in_jump;
											cmd->buttons |= in_duck;
										}
									}

									if (!(g::local->flags() & 1) && v != 2 && v != 3 && v != 4) {
										cmd->buttons &= ~in_duck;
										cmd->buttons &= ~in_jump;
									}
									if (v == 2) {
										if (g::local->flags() & 1) {
											cmd->buttons |= in_jump;
										}
										cmd->buttons |= in_duck;
									}
									if (v == 3) {
										if (i == hitgroudtick) {
											cmd->buttons |= in_duck;
											cmd->buttons &= ~in_jump;
											once += 1;
										}
										else {
											cmd->buttons &= ~in_duck;
											cmd->buttons |= in_jump;
										}
									}

									vec3_t BackupVelo2 = g::local->get_velocity();
									vec3_t Origin2 = g::local->origin();

									int backflags = g::local->flags();
									prediction::begin(cmd);
									prediction::end();

									if (v == 1) {
										if (g::local->flags() & 1 && !(backflags & 1)) {
											if (!jb)
												hitgroudtick = i;
											jb = true;
										}
									}

									if (!(g::local->flags() & 1) && v != 3 && v != 4) {
										cmd->buttons &= ~in_duck;
										cmd->buttons &= ~in_jump;
									}

									int preedflags = g::local->flags();

									if (preedflags & 1)
										max_tick_on_ground += 1;
									if (max_tick_on_ground > 1)
										break;

									vec3_t PredictedLocalPlayerOrigin = g::local->origin();
									vec3_t PredictedLocalPlayerVelocity = g::local->get_velocity();
									iCalcilate = 0.f;

									if (!(v == 2)) {
										if (!iCalcilate && once == 1 && !(backflags & fl_ducking) && !(preedflags & fl_ducking) &&
											BackupVelo2.z > 0.f && PredictedLocalPlayerVelocity.z > 0.f) {
											if (wait > 2) {
												Calculate = Origin2;
												iCalcilate = Origin2.z - PredictedLocalPlayerOrigin.z;
												once += 1;
											}
											else
												wait += 1;
										}
									}
									else {
										if (!iCalcilate && once == 1 && BackupVelo2.z > 0.f && PredictedLocalPlayerVelocity.z > 0.f) {
											if (wait > 2) {
												Calculate = Origin2;
												iCalcilate = Origin2.z - PredictedLocalPlayerOrigin.z;
												once += 1;
											}
											else
												wait += 1;
										}
									}

									NewCmd_assist[i].forwardmove = cmd->forward_move;
									NewCmd_assist[i].sideMove = cmd->side_move;
									NewCmd_assist[i].viewangle = cmd->view_angles;
									save_cur = angViewPointBackup;
									save_delta = DeltaAngle;

									if (iCalcilate) {
										float z = 0;
										for (int k = 0; k < 48; k++) {
											float o = z * (z + 1) / 2;
											float sv_gravity = interfaces::console->get_convar(("sv_gravity"))->get_float();
											float fTickInterval = interfaces::globals->interval_per_tick;
											float fTickRate = (fTickInterval > 0) ? (1.0f / fTickInterval) : 0.0f;
											auto sq_ticks = fTickRate * fTickRate;
											float p = iCalcilate + ((sv_gravity / sq_ticks) * z);
											if (p > 0) {
												if (route_markers.at(index).jump) {
													if (v == 0) {
														if (calc(Calculate.z, Surf.z)) {
															prediction::restore_ent_to_predicted_frame(BackupPredicted - 1);
															if (TicksOut < GetTickCount64()) {
																TicksOut = GetTickCount64() + 500;
																if (c::assist::assist_render)
																	c::assist::simple_display ? myWindowDetect.AddWindow(5, std::string("assisting to ").append(std::to_string((int)Surf.z))) : myWindowDetect.AddWindow(5, std::string("jump to surf ")
																	.append(std::to_string((int)Surf.z))
																	.append(" with ")
																	.append(std::to_string((int)prediction_backup::origin.z))
																	.append(" (stand)"));
															}
															HitJump = true;
															ticks = cmd->tick_count + i - 3;
															break;
														}
													}
												}

												if (route_markers.at(index).c_jump) {
													if (v == 0) {
														if (calc(Calculate.z + 9.f, Surf.z)) {
															prediction::restore_ent_to_predicted_frame(BackupPredicted - 1);
															if (TicksOut < GetTickCount64()) {
																TicksOut = GetTickCount64() + 500;
																if (c::assist::assist_render)
																	c::assist::simple_display ? myWindowDetect.AddWindow(5, std::string("assisting to ").append(std::to_string((int)Surf.z))) : myWindowDetect.AddWindow(
																		5, std::string("jump to surf ")
																		.append(std::to_string((int)Surf.z))
																		.append(" with ")
																		.append(std::to_string((int)prediction_backup::origin.z))
																		.append(" (crouch)"));
															}
															HitJump = true;
															ticks = cmd->tick_count + i - 3;
															break;
														}
													}
												}

												if (route_markers.at(index).minijump) {
													if (v == 1) {
														if (calc(Calculate.z, Surf.z)) {
															prediction::restore_ent_to_predicted_frame(BackupPredicted - 1);
															if (TicksOut < GetTickCount64()) {
																TicksOut = GetTickCount64() + 500;
																if (c::assist::assist_render)
																	c::assist::simple_display ? myWindowDetect.AddWindow(5, std::string("assisting to ").append(std::to_string((int)Surf.z))) : myWindowDetect.AddWindow(
																		5, std::string("mj to surf ")
																		.append(std::to_string((int)Surf.z))
																		.append(" with ")
																		.append(std::to_string((int)prediction_backup::origin.z))
																		.append(" (stand)"));
															}
															HitMiniJump = true;
															ticks = cmd->tick_count + i - 3;
															break;
														}
													}
												}

												if (route_markers.at(index).c_minijump) {
													if (v == 1) {
														if (calc(Calculate.z + 9.f, Surf.z)) {
															prediction::restore_ent_to_predicted_frame(BackupPredicted - 1);
															if (TicksOut < GetTickCount64()) {
																TicksOut = GetTickCount64() + 500;
																if (c::assist::assist_render)
																	c::assist::simple_display ? myWindowDetect.AddWindow(5, std::string("assisting to ").append(std::to_string((int)Surf.z))) : myWindowDetect.AddWindow(
																		5, std::string("mj to surf ")
																		.append(std::to_string((int)Surf.z))
																		.append(" with ")
																		.append(std::to_string((int)prediction_backup::origin.z))
																		.append(" (crouch)"));
															}
															HitMiniJump = true;
															ticks = cmd->tick_count + i - 3;
															break;
														}
													}
												}

												if (route_markers.at(index).longjump) {
													if (v == 1) {
														if (calc(Calculate.z + 8.9999704f, Surf.z)) {
															if (TicksOut < GetTickCount64()) {
																TicksOut = GetTickCount64() + 500;
																if (c::assist::assist_render)
																	c::assist::simple_display ? myWindowDetect.AddWindow(5, std::string("assisting to ").append(std::to_string((int)Surf.z))) : myWindowDetect.AddWindow(
																		5, std::string("lj to surf ")
																		.append(std::to_string((int)Surf.z))
																		.append(" with ")
																		.append(std::to_string((int)prediction_backup::origin.z))
																		.append(" (stand)"));
															}
															HitLongJump = true;
															ticks = cmd->tick_count + i - 3;
															break;
														}
													}
												}

												if (route_markers.at(index).c_longjump) {
													if (v == 1) {
														if (calc(Calculate.z + 9.f + 8.9999704f, Surf.z)) {
															prediction::restore_ent_to_predicted_frame(BackupPredicted - 1);
															if (TicksOut < GetTickCount64()) {
																TicksOut = GetTickCount64() + 500;
																if (c::assist::assist_render)
																	c::assist::simple_display ? myWindowDetect.AddWindow(5, std::string("assisting to ").append(std::to_string((int)Surf.z))) : myWindowDetect.AddWindow(
																		5, std::string("lj to surf ")
																		.append(std::to_string((int)Surf.z))
																		.append(" with ")
																		.append(std::to_string((int)prediction_backup::origin.z))
																		.append(" (crouch)"));
															}
															HitLongJump = true;
															ticks = cmd->tick_count + i - 3;
															break;
														}
													}
												}

												if (route_markers.at(index).crouch_hop) {
													if (v == 2) {
														if (calc(Calculate.z - 9.f, Surf.z)) {
															prediction::restore_ent_to_predicted_frame(BackupPredicted - 1);
															if (TicksOut < GetTickCount64()) {
																TicksOut = GetTickCount64() + 500;
																if (c::assist::assist_render)
																	c::assist::simple_display ? myWindowDetect.AddWindow(5, std::string("assisting to ").append(std::to_string((int)Surf.z))) : myWindowDetect.AddWindow(
																		5, std::string("crouch jump to surf ")
																		.append(std::to_string((int)Surf.z))
																		.append(" with ")
																		.append(std::to_string((int)prediction_backup::origin.z))
																		.append(" (stand)"));
															}
															HitCHop = true;
															ticks = cmd->tick_count + i - 3;
															break;
														}
													}
												}

												if (route_markers.at(index).c_crouch_hop) {
													if (v == 2) {
														if (calc(Calculate.z, Surf.z)) {
															prediction::restore_ent_to_predicted_frame(BackupPredicted - 1);
															if (TicksOut < GetTickCount64()) {
																TicksOut = GetTickCount64() + 500;
																if (c::assist::assist_render)
																	c::assist::simple_display ? myWindowDetect.AddWindow(5, std::string("assisting to ").append(std::to_string((int)Surf.z))) : myWindowDetect.AddWindow(
																		5, std::string("crouch jump to surf ")
																		.append(std::to_string((int)Surf.z))
																		.append(" with ")
																		.append(std::to_string((int)prediction_backup::origin.z))
																		.append(" (crouch)"));
															}
															HitCHop = true;
															ticks = cmd->tick_count + i - 3;
															break;
														}
													}
												}

												if (route_markers.at(index).jumpbug) {
													if (v == 3) {
														if (calc(Calculate.z, Surf.z)) {
															prediction::restore_ent_to_predicted_frame(BackupPredicted - 1);
															if (TicksOut < GetTickCount64()) {
																TicksOut = GetTickCount64() + 500;
																if (c::assist::assist_render)
																	c::assist::simple_display ? myWindowDetect.AddWindow(5, std::string("assisting to ").append(std::to_string((int)Surf.z))) : myWindowDetect.AddWindow(
																		5, std::string("jb to surf ")
																		.append(std::to_string((int)Surf.z))
																		.append(" with ")
																		.append(std::to_string((int)prediction_backup::origin.z))
																		.append(" (stand)"));
															}
															HitJumpBug = true;
															ticks = cmd->tick_count + i - 3;
															break;
														}
													}
												}

												if (route_markers.at(index).c_jumpbug) {
													if (v == 3) {
														if (calc(Calculate.z + 9.f, Surf.z)) {
															prediction::restore_ent_to_predicted_frame(BackupPredicted - 1);
															if (TicksOut < GetTickCount64()) {
																TicksOut = GetTickCount64() + 500;
																if (c::assist::assist_render)
																	c::assist::simple_display ? myWindowDetect.AddWindow(5, std::string("assisting to ").append(std::to_string((int)Surf.z))) : myWindowDetect.AddWindow(
																		5, std::string("jb to surf ")
																		.append(std::to_string((int)Surf.z))
																		.append(" with ")
																		.append(std::to_string((int)prediction_backup::origin.z))
																		.append(" (crouch)"));
															}
															HitJumpBug = true;
															ticks = cmd->tick_count + i - 3;
															break;
														}
													}
												}

												if (route_markers.at(index).c_mini_crouch_hop) {
													if (v == 2) {
														if (calc(Calculate.z + 9.f, Surf.z)) {
															prediction::restore_ent_to_predicted_frame(BackupPredicted - 1);
															if (TicksOut < GetTickCount64()) {
																TicksOut = GetTickCount64() + 500;
																if (c::assist::assist_render)
																	c::assist::simple_display ? myWindowDetect.AddWindow(5, std::string("assisting to ").append(std::to_string((int)Surf.z))) : myWindowDetect.AddWindow(
																		5, std::string("high crouch jump to surf ")
																		.append(std::to_string((int)Surf.z))
																		.append(" with ")
																		.append(std::to_string((int)prediction_backup::origin.z))
																		.append(" (crouch)"));
															}
															HitMiniChop = true;
															ticks = cmd->tick_count + i + 10;
															break;
														}
													}
												}
											}

											Calculate.z = Calculate.z - p;
											z += 1;
										}
										break;
									}
								}
							}
							else {
								break;
							}
							prediction::restore_ent_to_predicted_frame(BackupPredicted - 1);
							cmd->buttons = BackupButtons;
							cmd->forward_move = BackupForwardMove;
							cmd->side_move = BackupSideMove;
						}

						prediction::restore_ent_to_predicted_frame(BackupPredicted - 1);
						cmd->buttons = BackupButtons;
						cmd->forward_move = BackupForwardMove;
						cmd->side_move = BackupSideMove;
					
						if (cmd->tick_count > ticks) {
							ticks = 0;
							features::movement::HITGODA = false;
							float_ground = false;
							HitJump = false;
							HitMiniJump = false;
							HitLongJump = false;
							HitCHop = false;
							HitJumpBug = false;
							HitMiniChop = false;
							features::movement::AlertJB = false;
							cmd_i = 0;
							do_fucking_strafe = false;
						}

						if (cmd->tick_count < ticks) {
							features::movement::HITGODA = true;
							if (prediction_backup::velocity.z < 0.f || g::local->get_velocity().z < 0.f) {
								if (cmd_i > time_to_ticks(c::assist::pixelsurf_assist_ticks))
									cmd_i = 0;
								try {
									prediction::restore_ent_to_predicted_frame(BackupPredicted - 1);
									cmd->forward_move = NewCmd_assist[cmd_i].forwardmove;
									cmd->side_move = NewCmd_assist[cmd_i].sideMove;
									cmd->view_angles = NewCmd_assist[cmd_i].viewangle;
									UtilityAssistant.handle_fix(cmd, NewCmd_assist[cmd_i].viewangle);
									cmd->view_angles = angViewPointBackup;
									UtilityAssistant.handle_fix(cmd, angViewPointBackup);
									cmd_i += 1;
								}
								catch (...) {
									prediction::restore_ent_to_predicted_frame(BackupPredicted - 1);
									cmd->forward_move = 0.f;
									cmd->side_move = 0.f;
								}

								do_fucking_strafe = true;
								cmd->buttons = 0;
							}
							else {
								do_fucking_strafe = false;
							}

							if (prediction_backup::velocity.z > 0.f) {
								if (HitJumpBug) {}
								HitCHop = false;
								HitJumpBug = false;
							}

							if (HitCHop) {
								cmd->buttons |= in_duck;
								if (prediction_backup::flags & 1)
									cmd->buttons |= in_jump;
							}
							if (HitJump && prediction_backup::flags & 1) {
								cmd->buttons |= in_jump;
								HitJump = false;
							}
							if (HitMiniJump && prediction_backup::flags & 1) {
								cmd->buttons |= in_jump;
								cmd->buttons |= in_duck;
								HitMiniJump = false;
							}
							if (HitLongJump && prediction_backup::flags & 1) {
								cmd->buttons |= in_jump;
								cmd->buttons |= in_duck;
								ljticks = interfaces::globals->tick_count + 3;
								HitLongJump = false;
							}
							if (HitMiniChop) {
								alert_mini_crouch_hop = true;
							}
							if (ljticks > interfaces::globals->tick_count)
								cmd->buttons |= in_duck;
							else {
								if (prediction_backup::velocity.z > 0)
									ticks = 0;
							}
							features::movement::AlertJB = HitJumpBug;
						}
						else {
							features::movement::HITGODA = false;
						}
					}

					if (features::movement::HITGODA || HitJump || HitMiniJump || HitLongJump || HitCHop || HitJumpBug || HitMiniChop) {
						break;
					}

					if (!c::movement::bhopfix) {
						prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
					}
				}
				prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
			}
			prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
		}
		prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
	}

	void assist_createmove(c_usercmd* cmd)
	{
		if (g::local && g::local->is_alive()) {
			if (c::assist::assist) {
				
				int cmd_pred = interfaces::prediction->split->commands_predicted - 1;
				prediction::restore_ent_to_predicted_frame(cmd_pred);

				this->jump_to_marker(cmd);

				prediction::restore_ent_to_predicted_frame(cmd_pred);

				float sv_gravity2 = interfaces::console->get_convar(("sv_gravity"))->get_float();
				float fTickInterval = interfaces::globals->interval_per_tick;
				float fTickRate = (fTickInterval > 0) ? (1.0f / fTickInterval) : 0.0f;
				float veloZoftick = (((sv_gravity2 / 2) / fTickRate) * -1.f);
				static bool disable_pxs = false;
				if (menu::checkkey(c::assist::pixelsurf_assist_key, c::assist::pixelsurf_assist_key_s)) {
					if (alert_mini_crouch_hop) {
						if (disable_pxs) {
							if (prediction_backup::velocity.z == veloZoftick) {
								disable_pxs = false;
								alert_mini_crouch_hop = false;
							}
							if (g::local->flags() & fl_onground) {
								disable_pxs = false;
								alert_mini_crouch_hop = false;
							}
						}
						if (g::local->flags() & fl_onground) {
							prediction::restore_ent_to_predicted_frame(cmd_pred);
							cmd->buttons &= ~in_duck;
							cmd->buttons |= in_jump;
							disable_pxs = true;
						}
						else {
							cmd->buttons |= in_duck;
						}
					}

				}
				else {
					alert_mini_crouch_hop = false;
					disable_pxs = false;
				}
				static bool disable_bounce = false;
				if (menu::checkkey(c::assist::pixelsurf_assist_key, c::assist::pixelsurf_assist_key)) {
					if (bounce_alert_mini_crouch_hop) {
						if (disable_bounce) {
							if (prediction_backup::velocity.z == veloZoftick) {
								disable_bounce = false;
								bounce_alert_mini_crouch_hop = false;
							}
							if (g::local->flags() & fl_onground) {
								disable_bounce = false;
								bounce_alert_mini_crouch_hop = false;
							}
						}
						if (g::local->flags() & fl_onground) {
							prediction::restore_ent_to_predicted_frame(cmd_pred);
							cmd->buttons &= ~in_duck;
							cmd->buttons |= in_jump;
							disable_bounce = true;
						}
						else {
							cmd->buttons |= in_duck;
						}
					}
				}
				else {
					bounce_alert_mini_crouch_hop = false;
					disable_bounce = false;
				}

				if (features::movement::AlertJB || features::movement::AlertJB2)
				{
					cmd->buttons |= in_jump;
				}
			}

			if (!c::assist::assist) {
				features::movement::m_opened = false;
			}

			//...
			if (c::movement::pixel_surf) {
				if (PixelSurfAssistant.ps_data.pixelsurfing && PixelSurfAssistant.ps_data.pixelducking) {
					cmd->buttons |= in_bullrush;
					cmd->buttons |= in_duck;
				}
				if (g::local->flags() & fl_onground)
					PixelSurfAssistant.ps_data.pixelsurfing = false;
			}
		}
		else {
			features::movement::m_opened = false;
		}
	}

	struct HeadbangData {
		bool is_active = false;
		bool scanning = false;
		vec3_t wall_normal{ };
		vec3_t edge_position{ };
		vec3_t bounce_position{ };
		std::vector<vec3_t> bounce_points{ };
		bool is_displacement = false;
		int consecutive_hits = 0;
		float ceiling_z = 0.f;
		float ground_z = 0.f;
	};

	 HeadbangData g_headbang;

	 void auto_headbang(c_usercmd* cmd)
	 {
		 if (!c::movement::auto_headbang || !g::local || !g::local->is_alive())
			 return;

		 if (g::local->move_type() == movetype_ladder || g::local->move_type() == movetype_noclip)
			 return;

		 if (!(g::local->flags() & fl_onground)) {
			 g_headbang = HeadbangData(); // Reset on ground
			 return;
		 }

		 if (PixelSurfAssistant.ps_data.pixelsurfing || features::movement::should_ps || menu::checkkey(c::movement::jump_bug_key, c::movement::jump_bug_key_s)) {
			 g_headbang = HeadbangData();
			 return;
		 }

		 // Calculate headbang Z-velocity target
		 float sv_gravity = interfaces::console->get_convar("sv_gravity")->get_float();
		 float tickrate = (tick_interval > 0.f) ? (1.f / tick_interval) : 0.f;
		 float target_z_velo = ((sv_gravity / 2.f) / tickrate) * -1.f;
		 bool is_displacement{};

		 vec3_t player_velocity = g::local->get_velocity();
		 vec3_t StartPos = g::local->abs_origin();
		 vec3_t closest_hit_point(0, 0, 0);

		 auto colid = g::local->collideable();
		 if (!colid) return;

		 const vec3_t mins = colid->mins();
		 const vec3_t maxs = colid->maxs();

		 // Only activate when Z velocity is at headbang point
		 float z_velo_diff = fabsf(player_velocity.z - target_z_velo);
		 bool at_headbang_z = z_velo_diff < 1.0f;

		 if (!at_headbang_z)
		 {
			 g_headbang.is_active = false;
			 g_headbang.scanning = false;
			 return;
		 }

		 // We're at headbang Z, scan for walls if not already scanned
		 if (!g_headbang.scanning)
		 {
			 trace_world_only fil;

			 // Scan around player for walls
			 const float max_radius = m_pi * 2.f;
			 const float angle_step = max_radius / 16.f;

			 float closest_wall_dist = 999999.f;
			 vec3_t closest_wall_normal(0, 0, 0);
			 bool found_wall = false;

			 for (float angle = 0.f; angle < max_radius; angle += angle_step)
			 {
				 vec3_t direction(cosf(angle), sinf(angle), 0.f);
				 vec3_t end_pos = StartPos + direction * 100.f;

				 ray_t ray;
				 ray.initialize(StartPos, end_pos);

				 trace_t tr;
				 interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &fil, &tr);


				 if (tr.flFraction < 1.f && (tr.plane.normal.z) == 0.f)
				 {
					 float dist = (tr.end - StartPos).length();
					 if (dist < closest_wall_dist)
					 {
						 closest_wall_dist = dist;
						 closest_wall_normal = tr.plane.normal;
						 closest_hit_point = tr.end;
						 found_wall = true;
						 is_displacement = strstr(tr.surface.name, "displacement");

						 if (strstr(tr.surface.name, "displacement"))
							 g_headbang.is_displacement = true;
						 else
							 g_headbang.is_displacement = false;
					 }
					 prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
				 }

			 }

			 if (!found_wall)
				 return;

			 g_headbang.wall_normal = closest_wall_normal;
			 g_headbang.edge_position = closest_hit_point;

			 // Scan upward to find ceiling
			 ray_t up_ray;
			 vec3_t up_start = closest_hit_point;
			 vec3_t up_end = closest_hit_point + vec3_t(0, 0, 1000.f);
			 up_ray.initialize(up_start, up_end);

			 trace_t up_tr;
			 interfaces::trace_ray->trace_ray(up_ray, MASK_PLAYERSOLID, &fil, &up_tr);

			 if (up_tr.flFraction < 1.f)
			 {
				 g_headbang.ceiling_z = up_tr.end.z;
			 }
			 else
			 {
				 return; // No ceiling found
			 }

			 // Scan downward to find ground
			 ray_t down_ray;
			 vec3_t down_start = closest_hit_point;
			 vec3_t down_end = closest_hit_point + vec3_t(0, 0, -1000.f);
			 down_ray.initialize(down_start, down_end);

			 trace_t down_tr;
			 interfaces::trace_ray->trace_ray(down_ray, MASK_PLAYERSOLID, &fil, &down_tr);

			 if (down_tr.flFraction < 1.f)
			 {
				 g_headbang.ground_z = down_tr.end.z;
			 }

			 g_headbang.scanning = true;
		 }

		 // Now we have wall info, calculate bounce position
		 vec3_t wall_normal = g_headbang.wall_normal;
		 float offset_dist = g_headbang.is_displacement ? 16.001f : 15.97803f;

		 // Calculate offset direction
		 vec3_t offset_vec(0.f, 0.f, 0.f);

		 float Lerp = 0;
		 int PredictionFrames = interfaces::prediction->split->commands_predicted - 1;
		 float test_al = is_displacement ? 16.001f : 15.97803f;
		 for (int v = 0; v < 4; v++) {
			 prediction::restore_ent_to_predicted_frame(PredictionFrames);
			 bool done_and_break = false;
			 float align = 15.97803f;
			 vec3_t Angles{ wall_normal.x * -1.f, wall_normal.y * -1.f, 0.f };
			 vec3_t to_wall = Angles.to_angle();
			 float rotation = math::deg2rad(to_wall.y - cmd->view_angles.y);
			 float cos_rot = cos(rotation);
			 float sin_rot = sin(rotation);
			 float forwardmove = cos_rot * 10.f * v;
			 float sidemove = -sin_rot * 10.f * v;
			 cmd->forward_move = forwardmove;
			 cmd->side_move = sidemove;

			 if (g::local->get_velocity().z > 0)
			 {
				 static float Height = 0.f;
				 if (StartPos.z > closest_hit_point.z) {
					 if (StartPos.z < 0.f)
						 Height = (int)StartPos.z - 0.02f - Lerp - 54.f;
					 else
						 Height = (int)StartPos.z + 0.02f + Lerp - 54.f;
				 }
				 else {
					 if (closest_hit_point.z < 0.f)
						 Height = (int)closest_hit_point.z - 0.02f - Lerp - 54.f;
					 else
						 Height = (int)closest_hit_point.z + 0.02f + Lerp - 54.f;
				 }

				 int maxLerp;
				 if (StartPos.z > closest_hit_point.z) {
					 maxLerp = StartPos.z - closest_hit_point.z;
				 }
				 else {
					 maxLerp = closest_hit_point.z - StartPos.z;
				 }

				 if (Lerp > maxLerp) {
					 done_and_break = true;
					 break;
				 }
				 else {
					 Lerp += 1.f;
				 }
				 if (wall_normal.x < 0 && wall_normal.y < 0.f) {
					 g::local->origin() = vec3_t(StartPos.x - test_al, StartPos.y - test_al, Height);
					 g::local->set_abs_origin(vec3_t(StartPos.x, StartPos.y, Height));
					 offset_vec = vec3_t(StartPos.x - test_al, StartPos.y - test_al, Height);
				 }
				 else if (wall_normal.x < 0 && wall_normal.y > 0.f) {
					 g::local->origin() = vec3_t(StartPos.x - test_al, StartPos.y + test_al, Height);
					 g::local->set_abs_origin(vec3_t(StartPos.x, StartPos.y, Height));
					 offset_vec = vec3_t(StartPos.x - test_al, StartPos.y + test_al, Height);
				 }
				 else if (wall_normal.x > 0 && wall_normal.y < 0.f) {
					 g::local->origin() = vec3_t(StartPos.x + test_al, StartPos.y - test_al, Height);
					 g::local->set_abs_origin(vec3_t(StartPos.x, StartPos.y, Height));
					 offset_vec = vec3_t(StartPos.x + test_al, StartPos.y - test_al, Height);
				 }
				 else if (wall_normal.x > 0 && wall_normal.y > 0.f) {
					 g::local->origin() = vec3_t(StartPos.x + test_al, StartPos.y + test_al, Height);
					 g::local->set_abs_origin(vec3_t(StartPos.x, StartPos.y, Height));
					 offset_vec = vec3_t(StartPos.x + test_al, StartPos.y + test_al, Height);
				 }
				 else if (wall_normal.x == 0.f && wall_normal.y > 0.f) {
					 g::local->origin() = vec3_t(StartPos.x, StartPos.y + test_al, Height);
					 g::local->set_abs_origin(vec3_t(StartPos.x, StartPos.y, Height));
					 offset_vec = vec3_t(StartPos.x, StartPos.y + test_al, Height);
				 }
				 else if (wall_normal.x == 0.f && wall_normal.y < 0.f) {
					 g::local->origin() = vec3_t(StartPos.x, StartPos.y - test_al, Height);
					 g::local->set_abs_origin(vec3_t(StartPos.x, StartPos.y, Height));
					 offset_vec = vec3_t(StartPos.x, StartPos.y - test_al, Height);
				 }
				 else if (wall_normal.x < 0 && wall_normal.y == 0.f) {
					 g::local->origin() = vec3_t(StartPos.x - test_al, StartPos.y, Height);
					 g::local->set_abs_origin(vec3_t(StartPos.x, StartPos.y, Height));
					 offset_vec = vec3_t(StartPos.x - test_al, StartPos.y, Height);
				 }
				 else if (wall_normal.x > 0 && wall_normal.y == 0.f) {
					 g::local->origin() = vec3_t(StartPos.x + test_al, StartPos.y, Height);
					 g::local->set_abs_origin(vec3_t(StartPos.x, StartPos.y, Height));
					 offset_vec = vec3_t(StartPos.x + test_al, StartPos.y, Height);
				 }
				 trace_t trace;
				 trace_filter flt(g::local);
				 ray_t ray;
				 ray.initialize(vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z + 54.f), vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z - 1000.f));

				 interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &flt, &trace);
				 vec3_t down_point = trace.end;
				 trace_t trace2;
				 trace_filter flt2(g::local);
				 ray_t ray2;
				 ray2.initialize(vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z + 54.f), vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z + 1000.f));

				 interfaces::trace_ray->trace_ray(ray2, MASK_PLAYERSOLID, &flt2, &trace2);
				 vec3_t upper_point = trace2.end;

				 if (g::local->origin().z + 1.f < down_point.z)
					 continue;
				 if (g::local->origin().z + 1.f > upper_point.z)
					 continue;
			 }
			 vec3_t un_pred_velo = g::local->get_velocity();

			 prediction::begin(cmd);
			 prediction::end();

			 vec3_t pred_velo = g::local->get_velocity();
			 {
				 if (pred_velo.z == target_z_velo) {
					 if (wall_normal.x < 0 && wall_normal.y < 0.f) {
						 offset_vec = (vec3_t(g::local->origin().x + test_al,
							 g::local->origin().y + test_al,
							 g::local->origin().z + 54.f));
					 }
					 else if (wall_normal.x < 0 && wall_normal.y > 0.f) {
						 offset_vec = (vec3_t(g::local->origin().x + test_al,
							 g::local->origin().y - test_al,
							 g::local->origin().z + 54.f));
					 }
					 else if (wall_normal.x > 0 && wall_normal.y < 0.f) {
						 offset_vec = (vec3_t(g::local->origin().x - test_al,
							 g::local->origin().y + test_al,
							 g::local->origin().z + 54.f));
					 }
					 else if (wall_normal.x > 0 && wall_normal.y > 0.f) {
						 offset_vec = (vec3_t(g::local->origin().x - test_al,
							 g::local->origin().y - test_al,
							 g::local->origin().z + 54.f));
					 }
					 else if (wall_normal.x == 0.f && wall_normal.y > 0.f) {
						 offset_vec = (vec3_t(g::local->origin().x, g::local->origin().y - test_al,
							 g::local->origin().z + 54.f));
					 }
					 else if (wall_normal.x == 0.f && wall_normal.y < 0.f) {
						 offset_vec = (vec3_t(g::local->origin().x, g::local->origin().y + test_al,
							 g::local->origin().z + 54.f));
					 }
					 else if (wall_normal.x < 0 && wall_normal.y == 0.f) {
						 offset_vec = (vec3_t(g::local->origin().x + test_al, g::local->origin().y,
							 g::local->origin().z + 54.f));
					 }
					 else if (wall_normal.x > 0 && wall_normal.y == 0.f) {
						 offset_vec = (vec3_t(g::local->origin().x - test_al, g::local->origin().y,
							 g::local->origin().z + 54.f));
					 }
				 }
				 prediction::restore_ent_to_predicted_frame(PredictionFrames);

				 vec3_t move_angle = vec3_t(offset_vec.x * -test_al, offset_vec.y * -test_al, 0);
				 float rotation = math::deg2rad(move_angle.to_angle().y - cmd->view_angles.y);

				 prediction::begin(cmd);
				 prediction::end();

				 if (pred_velo.z == target_z_velo) {
					 // Apply movement input
					 cmd->forward_move = cosf(rotation) * 450.f;
					 cmd->side_move = -sinf(rotation) * 450.f;
					 cmd->buttons |= in_jump;
					 cmd->buttons |= in_duck;
				 }

				 // CRITICAL: Set view angle pitch to the bounce Z height
				 vec3_t bounce_point_eye = offset_vec;

				 // Set pitch to look at the ceiling contact point
				 vec3_t bounce_angle = vec3_t(bounce_point_eye.z, bounce_point_eye.z, 0).to_angle();
				 UtilityAssistant.handle_fix(cmd, bounce_angle);

				 g_headbang.is_active = true;
				 g_headbang.consecutive_hits++;

				 // Record bounce point
				 bool is_new = true;
				 for (const auto& pt : g_headbang.bounce_points)
				 {
					 if ((pt - bounce_point_eye).length() < 5.f) {
						 is_new = false;
						 break;
					 }
					 prediction::restore_ent_to_predicted_frame(PredictionFrames);
				 }

				 if (is_new && g_headbang.bounce_points.size() < 100)
					 g_headbang.bounce_points.push_back(bounce_point_eye);
			 }
		 }
	 }

	 void auto_wall_bounce(c_usercmd* cmd)
	 {
		 if (!c::movement::auto_bounce || !g::local || !g::local->is_alive())
			 return;

		 if (g::local->move_type() == movetype_ladder || g::local->move_type() == movetype_noclip)
			 return;

		 // Only trigger while on ground
		 if (!(g::local->flags() & fl_onground))
			 return;

		 // Skip conflicts
		 if (PixelSurfAssistant.ps_data.pixelsurfing ||
			 features::movement::should_ps ||
			 menu::checkkey(c::movement::jump_bug_key, c::movement::jump_bug_key_s))
			 return;

		 // Calculate the velocity.z “activation point” (same method as headbang)
		 float sv_gravity = interfaces::console->get_convar("sv_gravity")->get_float();
		 float tickrate = (tick_interval > 0.f) ? (1.f / tick_interval) : 0.f;
		 float target_z_velo = ((sv_gravity / 2.f) / tickrate) * -1.f;

		 float z_diff = fabsf(g::local->get_velocity().z - target_z_velo);
		 if (z_diff > 1.0f)
			 return;

		 // Scan forward for sloped walls
		 trace_world_only filter;

		 vec3_t start = g::local->abs_origin();
		 start.z += 34.0f;

		 const float scan_dist = 420.f;
		 const int rays = 15;
		 const float fov = math::deg2rad(90.f);
		 const float half = fov * 0.5f;

		 float base_yaw = cmd->view_angles.y;

		 bool found = false;
		 vec3_t best_normal;
		 vec3_t best_hit;
		 float best_dist = FLT_MAX;

		 for (int i = 0; i < rays; i++)
		 {
			 float t = (float)i / (float)(rays - 1);
			 float angle = -half + t * fov;
			 float yaw = math::deg2rad(base_yaw) + angle;

			 vec3_t dir(cosf(yaw), sinf(yaw), 0.f);
			 dir.normalize();

			 vec3_t end = start + dir * scan_dist;

			 ray_t ray;
			 ray.initialize(start, end);
			 trace_t tr;
			 interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &filter, &tr);

			 if (tr.flFraction < 1.f)
			 {
				 float nz = tr.plane.normal.z;

				 // Only accept slopes (not ground or walls)
				 if (nz > 0.2f && nz < 0.92f)
				 {
					 float dist = (tr.end - start).length();
					 if (dist < best_dist)
					 {
						 best_dist = dist;
						 best_normal = tr.plane.normal;
						 best_hit = tr.end;
						 found = true;
					 }
				 }
			 }
		 }

		 if (!found)
			 return;

		 // Compute reflection vector
		 vec3_t v = g::local->get_velocity();
		 float vlen = v.length();
		 vec3_t inc = v.normalized();

		 float d = inc.dot(best_normal);
		 vec3_t reflect = (inc - best_normal * (2.f * d)).normalized();

		 // Don't bounce into downward slopes
		 if (reflect.z < -0.15f)
			 return;

		 // --------------------------
		 //  MAIN ASSIST LOGIC
		 // --------------------------

		 // Uses ONLY up_move + button input (like your jetpack/headbang assist)
		 cmd->up_move = reflect.z * 450.f;

		 // The rest is jump + duck to make sure we "launch" off the slope
		 cmd->buttons |= in_jump;
		 cmd->buttons |= in_duck;

		 // Small directional push based on the reflect vector
		 // ***NOT*** setting forward_move/sidemove — instead apply velocity-based nudge
		 vec3_t flat = vec3_t(reflect.x, reflect.y, 0.f);
		 float flat_len = flat.length_2d();

		 if (flat_len > 0.01f)
		 {
			 flat.normalize();
			 // A small assist — not full sidemove/forwardmove
			 // This is effectively “velocity injection”
			 g::local->velocity().x += flat.x * 15.f;
			 g::local->velocity().y += flat.y * 15.f;
		 }

		 // Slight view pitch correction to align bounce
		 float new_pitch = -math::rad2deg(atan2f(reflect.z, 1.f));
		 new_pitch = std::clamp(new_pitch, -35.f, 35.f);

		 vec3_t new_ang = cmd->view_angles;
		 new_ang.x = new_pitch;
		 UtilityAssistant.handle_fix(cmd, new_ang);
	 }

	 void auto_headbang_align(c_usercmd* cmd) {
		 if (!c::movement::marker_align || !g::local || !g::local->is_alive())
			 return;

		 if (g::local->move_type() == movetype_ladder || g::local->move_type() == movetype_noclip)
			 return;

		 if (features::movement::should_ps || PixelSurfAssistant.ps_data.pixelsurfing)
			 return;

		 // Store original command state
		 int original_buttons = cmd->buttons;
		 float original_forward = cmd->forward_move;
		 float original_side = cmd->side_move;
		 float original_up = cmd->up_move;
		 vec3_t original_view = cmd->view_angles;

		 // Calculate target Z velocity for pixelsurf/bounce detection
		 float sv_gravity = interfaces::console->get_convar("sv_gravity")->get_float();
		 float tick_rate = (tick_interval > 0.f) ? (1.f / tick_interval) : 0.f;
		 float target_z_velo = ((sv_gravity / 2.f) / tick_rate) * -1.f;
		 int cmd_pred = interfaces::prediction->split->commands_predicted - 1;
		 float Lerp = 0;

		 vec3_t player_pos = g::local->abs_origin();
		 vec3_t player_velocity = g::local->get_velocity();

		 // Player height constants (from pixelsurf documentation)
		 const float PLAYER_POV_HEIGHT = 64.09f; // 64.062561 simplified
		 const float PLAYER_HITBOX_HEIGHT = 72.03f; // 72 + .03125 simplified
		 const float NORMAL_JUMP_HEIGHT = 57.0f;
		 const float CROUCH_JUMP_HEIGHT = 66.0f;
		 const float POSITION_PRECISION = 0.025497f; // Range for valid heights

		 // Check velocity state for headbounce
		 bool going_up = player_velocity.z > 0.f;
		 bool going_down = player_velocity.z < 0.f;
		 float z_velo_diff = fabsf(player_velocity.z - target_z_velo);
		 bool at_target_z = z_velo_diff < 1.0f;

		 // Enhanced detection for approaching headbounce
		 bool approaching_headbounce = going_up && player_velocity.z > 200.0f;
		 bool at_peak = fabsf(player_velocity.z) < 50.0f;

		 // Scan for wall near the marker to determine alignment
		 trace_world_only filter;
		 const float scan_distance = 4096.f;
		 const float max_radius = m_pi * 2.f;
		 const float angle_step = max_radius / 16.f;

		 float closest_wall_dist = 999999.f;
		 vec3_t closest_wall_normal(0, 0, 0);
		 vec3_t closest_wireframe_pos(0, 0, 0);
		 bool found_wall = false;
		 bool is_displacement = false;

		 // Scan around marker position for walls and wireframes
		 vec3_t direction(cosf(cmd->view_angles.x), sinf(cmd->view_angles.y), 0.f);
		 vec3_t scan_start = g::local->origin();
		 vec3_t target_pos = scan_start;
		 vec3_t scan_end = scan_start + direction * scan_distance;

		 ray_t ray;
		 ray.initialize(scan_start, scan_end);

		 trace_t tr;
		 interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &filter, &tr);

		 // Check for vertical wall and find wireframe height
		 if (tr.flFraction < 1.f)
		 {
			 float dist = (tr.end - scan_start).length();
			 if (dist < closest_wall_dist)
			 {
				 closest_wall_dist = dist;
				 closest_wall_normal = tr.plane.normal;
				 closest_wireframe_pos = tr.end;
				 found_wall = true;

				 // Check if displacement (pattern changes indicate pixelsurf points)
				 if (strstr(tr.surface.name, "displacement"))
					 is_displacement = true;
				 else
					 is_displacement = false;
			 }
		 }

		 if (!found_wall)
			 return;

		 // Calculate alignment offset based on wall type
		 vec3_t wall_normal = closest_wall_normal;

		 // Position must be .00 against wall (not .03) for pixelsurf
		 float align_offset = is_displacement ? 16.001f : 15.97803f;

		 // Get wireframe height from trace
		 float wireframe_height = closest_wireframe_pos.z;

		 // Calculate pixelsurf heights based on jump type
		 // Basic calculations: [Wireframe Height] + 64.09 - [Jump Height Variant]
		 float pixelsurf_heights[10];
		 int height_count = 0;

		 // Standing/Crouching variants
		 pixelsurf_heights[height_count++] = wireframe_height + PLAYER_POV_HEIGHT - NORMAL_JUMP_HEIGHT;
		 pixelsurf_heights[height_count++] = wireframe_height + PLAYER_POV_HEIGHT - (NORMAL_JUMP_HEIGHT - 0.33f);
		 pixelsurf_heights[height_count++] = wireframe_height + PLAYER_POV_HEIGHT - (NORMAL_JUMP_HEIGHT - 0.78f);
		 pixelsurf_heights[height_count++] = wireframe_height + PLAYER_POV_HEIGHT - (NORMAL_JUMP_HEIGHT - 6.0f);
		 pixelsurf_heights[height_count++] = wireframe_height + PLAYER_POV_HEIGHT - (NORMAL_JUMP_HEIGHT - 7.63f);

		 // Crouch Jump variants (for difficult bounces)
		 pixelsurf_heights[height_count++] = wireframe_height + PLAYER_POV_HEIGHT - CROUCH_JUMP_HEIGHT;
		 pixelsurf_heights[height_count++] = wireframe_height + PLAYER_POV_HEIGHT - (CROUCH_JUMP_HEIGHT - 0.33f);
		 pixelsurf_heights[height_count++] = wireframe_height + PLAYER_POV_HEIGHT - (CROUCH_JUMP_HEIGHT - 0.78f);
		 pixelsurf_heights[height_count++] = wireframe_height + PLAYER_POV_HEIGHT - (CROUCH_JUMP_HEIGHT - 6.0f);
		 pixelsurf_heights[height_count++] = wireframe_height + PLAYER_POV_HEIGHT - (CROUCH_JUMP_HEIGHT - 7.63f);

		 static float target_height = 0.f;
		 static bool attempting_bounce = false;
		 static int crouch_jump_variant = 0; // Track which variant to use

		 // Determine which pixelsurf height we're targeting based on current position
		 if (!attempting_bounce)
		 {
			 float current_height = player_pos.z + PLAYER_POV_HEIGHT;
			 float closest_height_diff = 999999.f;
			 int best_variant = 0;

			 for (int i = 0; i < height_count; i++)
			 {
				 float diff = fabsf(current_height - pixelsurf_heights[i]);
				 if (diff < closest_height_diff && diff < 10.0f) // Within reasonable range
				 {
					 closest_height_diff = diff;
					 target_height = pixelsurf_heights[i];
					 best_variant = i;
				 }
			 }

			 // Determine if we need crouch jump (variants 5-9 are crouch jumps)
			 crouch_jump_variant = best_variant;

			 if (approaching_headbounce)
				 attempting_bounce = true;
		 }

		 // Simulate to find best alignment
		 bool found_valid_alignment = false;
		 vec3_t best_aligned_origin;
		 vec3_t best_abs_origin;
		 float best_forward_move = 0.f;
		 float best_side_move = 0.f;
		 float best_up_move = 0.f;
		 int best_buttons = 0;
		 vec3_t best_view_angles = original_view;

		 // BOUNCE SCAN (going UP)
		 for (int v = 0; v < 4; v++) {
			 prediction::restore_ent_to_predicted_frame(cmd_pred);
			 bool done_and_break = false;

			 for (int i = 0; i < time_to_ticks(c::movement::alignment_rage / 16 / m_pi); i++)
			 {
				 cmd->buttons |= in_jump;
				 cmd->buttons |= in_duck;

				 vec3_t Angles{ scan_WallNormal.x * -1.f, scan_WallNormal.y * -1.f, 0.f };
				 vec3_t to_wall = Angles.to_angle();
				 float rotation = math::deg2rad(to_wall.y - cmd->view_angles.y);
				 float cos_rot = cos(rotation);
				 float sin_rot = sin(rotation);
				 float forwardmove = cos_rot * 10.f;
				 float sidemove = -sin_rot * 10.f;
				 cmd->forward_move = forwardmove;
				 cmd->side_move = sidemove;

				 if (g::local->get_velocity().z > 0)
				 {
					 static float Height = 0.f;
					 if (scan_StartPos.z > scan_StartPos.z) {
						 if (scan_StartPos.z < 0.f)
							 Height = (int)scan_StartPos.z - 0.02f - Lerp - 54.f;
						 else
							 Height = (int)scan_StartPos.z + 0.02f + Lerp - 54.f;
					 }
					 else {
						 if (scan_EndPos.z < 0.f)
							 Height = (int)scan_EndPos.z - 0.02f - Lerp - 54.f;
						 else
							 Height = (int)scan_EndPos.z + 0.02f + Lerp - 54.f;
					 }

					 int maxLerp;
					 if (scan_StartPos.z > scan_EndPos.z) {
						 maxLerp = scan_StartPos.z - scan_EndPos.z;
					 }
					 else {
						 maxLerp = scan_EndPos.z - scan_StartPos.z;
					 }

					 if (Lerp > maxLerp) {
						 cmd->buttons = original_buttons;
						 cmd->forward_move = original_forward;
						 scan_FirstPoint = false;
						 done_and_break = true;
						 break;
					 }
					 else {
						 Lerp += 1.f;
					 }

					 // Position player based on wall normal
					 if (scan_WallNormal.x < 0 && scan_WallNormal.y < 0.f) {
						 g::local->origin() = vec3_t(scan_StartPos.x - align_offset, scan_StartPos.y - align_offset, Height);
						 g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
					 }
					 else if (scan_WallNormal.x < 0 && scan_WallNormal.y > 0.f) {
						 g::local->origin() = vec3_t(scan_StartPos.x - align_offset, scan_StartPos.y + align_offset, Height);
						 g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
					 }
					 else if (scan_WallNormal.x > 0 && scan_WallNormal.y < 0.f) {
						 g::local->origin() = vec3_t(scan_StartPos.x + align_offset, scan_StartPos.y - align_offset, Height);
						 g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
					 }
					 else if (scan_WallNormal.x > 0 && scan_WallNormal.y > 0.f) {
						 g::local->origin() = vec3_t(scan_StartPos.x + align_offset, scan_StartPos.y + align_offset, Height);
						 g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
					 }
					 else if (scan_WallNormal.x == 0.f && scan_WallNormal.y > 0.f) {
						 g::local->origin() = vec3_t(scan_StartPos.x, scan_StartPos.y + align_offset, Height);
						 g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
					 }
					 else if (scan_WallNormal.x == 0.f && scan_WallNormal.y < 0.f) {
						 g::local->origin() = vec3_t(scan_StartPos.x, scan_StartPos.y - align_offset, Height);
						 g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
					 }
					 else if (scan_WallNormal.x < 0 && scan_WallNormal.y == 0.f) {
						 g::local->origin() = vec3_t(scan_StartPos.x - align_offset, scan_StartPos.y, Height);
						 g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
					 }
					 else if (scan_WallNormal.x > 0 && scan_WallNormal.y == 0.f) {
						 g::local->origin() = vec3_t(scan_StartPos.x + align_offset, scan_StartPos.y, Height);
						 g::local->set_abs_origin(vec3_t(scan_StartPos.x, scan_StartPos.y, Height));
					 }

					 // Check if valid bounce position
					 trace_t trace;
					 trace_filter flt(g::local);
					 ray_t ray;
					 ray.initialize(vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z + 54.f),
						 vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z - 1000.f));

					 interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &flt, &trace);
					 vec3_t down_point = trace.end;

					 trace_t trace2;
					 trace_filter flt2(g::local);
					 ray_t ray2;
					 ray2.initialize(vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z + 54.f),
						 vec3_t(g::local->origin().x, g::local->origin().y, g::local->origin().z + 1000.f));

					 interfaces::trace_ray->trace_ray(ray2, MASK_PLAYERSOLID, &flt2, &trace2);
					 vec3_t upper_point = trace2.end;

					 if (g::local->origin().z + 1.f < down_point.z)
						 continue;
					 if (g::local->origin().z + 1.f > upper_point.z)
						 continue;
				 }

				 vec3_t un_pred_velo = g::local->get_velocity();

				 prediction::restore_ent_to_predicted_frame(cmd_pred);
				 prediction::begin(cmd);
				 prediction::end();

				 vec3_t pred_velo = g::local->get_velocity();
				 vec3_t pred_origin = g::local->origin();

				 if (pred_velo.z == target_z_velo) {
					 // Add bounce point
					 if (scan_WallNormal.x < 0 && scan_WallNormal.y < 0.f) {
						 g::local->origin() = vec3_t(g::local->origin().x + align_offset,
							 g::local->origin().y + align_offset,
							 g::local->origin().z + 54.f);
						 pred_origin = vec3_t(g::local->origin().x + align_offset,
							 g::local->origin().y + align_offset,
							 g::local->origin().z + 54.f);
					 }
					 else if (scan_WallNormal.x < 0 && scan_WallNormal.y > 0.f) {
						 g::local->origin() = vec3_t(g::local->origin().x + align_offset,
							 g::local->origin().y - align_offset,
							 g::local->origin().z + 54.f);
						 pred_origin = vec3_t(g::local->origin().x + align_offset,
							 g::local->origin().y - align_offset,
							 g::local->origin().z + 54.f);
					 }
					 else if (scan_WallNormal.x > 0 && scan_WallNormal.y < 0.f) {
						 g::local->origin() = vec3_t(g::local->origin().x - align_offset,
							 g::local->origin().y + align_offset,
							 g::local->origin().z + 54.f);
					 }
					 else if (scan_WallNormal.x > 0 && scan_WallNormal.y > 0.f) {
						 g::local->origin() = vec3_t(g::local->origin().x - align_offset,
							 g::local->origin().y - align_offset,
							 g::local->origin().z + 54.f);
						 pred_origin = vec3_t(g::local->origin().x - align_offset,
							 g::local->origin().y - align_offset,
							 g::local->origin().z + 54.f);
					 }
					 else if (scan_WallNormal.x == 0.f && scan_WallNormal.y > 0.f) {
						 g::local->origin() = vec3_t(g::local->origin().x, g::local->origin().y - align_offset,
							 g::local->origin().z + 54.f);
						 pred_origin = vec3_t(g::local->origin().x, g::local->origin().y - align_offset,
							 g::local->origin().z + 54.f);
					 }
					 else if (scan_WallNormal.x == 0.f && scan_WallNormal.y < 0.f) {
						 g::local->origin() = vec3_t(g::local->origin().x, g::local->origin().y + align_offset,
							 g::local->origin().z + 54.f);
						 pred_origin = vec3_t(g::local->origin().x, g::local->origin().y + align_offset,
							 g::local->origin().z + 54.f);
					 }
					 else if (scan_WallNormal.x < 0 && scan_WallNormal.y == 0.f) {
						 g::local->origin() = vec3_t(g::local->origin().x + align_offset, g::local->origin().y,
							 g::local->origin().z + 54.f);
						 pred_origin = vec3_t(g::local->origin().x + align_offset, g::local->origin().y,
							 g::local->origin().z + 54.f);
					 }
					 else if (scan_WallNormal.x > 0 && scan_WallNormal.y == 0.f) {
						 g::local->origin() = vec3_t(g::local->origin().x - align_offset, g::local->origin().y,
							 g::local->origin().z + 54.f);
						 pred_origin = vec3_t(g::local->origin().x - align_offset, g::local->origin().y,
							 g::local->origin().z + 54.f);
					 }
				 }

				 cmd->buttons = original_buttons;
				 cmd->forward_move = original_forward;

				 // Calculate movement inputs
				 float move_yaw = pred_origin.to_angle().y;
				 float delta = math::deg2rad(move_yaw - cmd->view_angles.y);
				 float test_forward = cos(delta) * 450.f;
				 float test_side = sin(delta) * 450.f;
				 float test_upmove = prediction_backup::origin.z - pred_origin.z;
				 int test_buttons = cmd->buttons;

				 // Apply headbounce logic
				 if (pred_velo.z == target_z_velo) {
					 prediction::begin(cmd);
					 prediction::end();

					 // Use crouch jump for difficult bounces (variants 5-9)
					 if (crouch_jump_variant >= 5) {
						 cmd->buttons |= in_duck; // Hold crouch as per pixelsurf requirements
					 }

					 // Jump at precise moment of headbounce
					 cmd->buttons |= in_jump;
					 cmd->forward_move = test_forward;
					 cmd->side_move = test_side;
					 cmd->up_move = test_upmove;
					 prediction::restore_ent_to_predicted_frame(cmd_pred);

					 attempting_bounce = false; // Reset after bounce attempt
				 }

				 // Check if this achieves valid headbounce
				 if (pred_velo.z == target_z_velo)
				 {
					 found_valid_alignment = true;
					 best_aligned_origin = pred_origin;
					 best_abs_origin = pred_origin;
					 best_forward_move = test_forward;
					 best_side_move = test_side;
					 best_up_move = test_upmove;
					 best_buttons = test_buttons;

					 // Calculate view angle to face the wireframe
					 vec3_t to_marker = closest_wireframe_pos - pred_origin;
					 best_view_angles = to_marker.to_angle();
					 break;
				 }
			 }

			 if (cmd) {
				 // Apply alignment and bounce if found
				 if (found_valid_alignment)
				 {
					 prediction::begin(cmd);
					 prediction::end();

					 cmd->forward_move = best_forward_move;
					 cmd->side_move = best_side_move;
					 cmd->up_move = best_up_move;
					 cmd->buttons = best_buttons;

					 // Apply crouch if needed
					 if (crouch_jump_variant >= 5) {
						 cmd->buttons |= in_duck;
					 }

					 // Apply view angle correction
					 best_view_angles.x = math::deg2rad(cmd->view_angles.x - best_aligned_origin.z);
					 best_view_angles.y = math::deg2rad(cmd->view_angles.y - best_aligned_origin.z);
					 best_view_angles.z = 0;
					 cmd->view_angles = best_view_angles;
					 UtilityAssistant.handle_fix(cmd, best_view_angles);
					 prediction::restore_ent_to_predicted_frame(cmd_pred);
				 }
				 else
				 {
					 // Restore original input
					 cmd->buttons = original_buttons;
					 cmd->forward_move = original_forward;
					 cmd->side_move = original_side;
					 cmd->up_move = original_up;
					 cmd->view_angles = original_view;
					 prediction::restore_ent_to_predicted_frame(cmd_pred);
				 }
				 prediction::restore_ent_to_predicted_frame(cmd_pred);
			 }

			 // Reset state when landed
			 if (g::local->flags() & fl_onground) {
				 attempting_bounce = false;
				 target_height = 0.f;
			 }

			 prediction::restore_ent_to_predicted_frame(cmd_pred);
			 if (done_and_break)
				 break;
		 }
	 }

	 void jetpack_surf_simple(c_usercmd* cmd) {
		 if (!g::local || !g::local->is_alive())
			 return;

		 if (route_markers.empty())
			 return;

		 // Find closest active marker
		 int closest_marker = -1;
		 float closest_dist = FLT_MAX;

		 for (int i = 0; i < route_markers.size(); i++) {
			 if (interfaces::engine->get_level_name() == route_markers.at(i).map && route_markers.at(i).active) {
				 float dist = g::local->origin().distance_to(route_markers.at(i).position);
				 if (dist < closest_dist && dist <= route_markers.at(i).radius * 2.0f) {
					 closest_dist = dist;
					 closest_marker = i;
				 }
			 }
		 }

		 if (closest_marker == -1)
			 return;

		 vec3_t target = route_markers.at(closest_marker).position;
		 vec3_t current = g::local->origin();
		 vec3_t direction = target - current;

		 // Calculate required angle
		 vec3_t target_angle = direction.to_angle();

		 // Smooth angle transition
		 vec3_t current_angle = cmd->view_angles;
		 float yaw_diff = target_angle.y - current_angle.y;
		 while (yaw_diff > 180.f) yaw_diff -= 360.f;
		 while (yaw_diff < -180.f) yaw_diff += 360.f;

		 // Apply smoothed angles
		 cmd->view_angles.y += yaw_diff * 0.3f; // 30% blend
		 cmd->view_angles.x = target_angle.x * 0.5f; // 50% pitch
		 cmd->view_angles.normalized().clamped();

		 UtilityAssistant.handle_fix(cmd, cmd->view_angles);

		 // Activate jetpack
		 cmd->buttons |= in_speed; // Adjust based on your jetpack plugin binding

		 // Movement
		 cmd->forward_move = 450.0f;

		 // Strafe for better control
		 if (std::abs(yaw_diff) > 10.0f) {
			 cmd->side_move = (yaw_diff > 0) ? 450.0f : -450.0f;
		 }
	 }

	 void jetpack_to_marker(c_usercmd* cmd) {
		 static bool IsJetpacking{ false };
		 static int jetpack_ticks = 0;
		 static int marker_target = -1;

		 // Cache for scan results
		 struct JetpackScanResult {
			 bool valid = false;
			 int tick_offset = 0;
			 int marker_index = -1;
			 vec3_t target_position;
			 float target_z = 0.f;
			 uint64_t scan_time = 0;
		 };
		 static std::map<int, JetpackScanResult> jetpack_cache;
		 static uint64_t last_cache_clear = 0;
		 const uint64_t CACHE_LIFETIME_MS = 1000;
		 const int MAX_SCAN_TICKS_PER_FRAME = 64;

		 if (!g::local)
			 return;

		 if (!c::movement::jetpack_to_surf) // Add this config option
			 return;

		 if (!interfaces::engine->is_in_game())
			 return;

		 if (!g::local->is_alive())
			 return;

		 // Jetpack can work alongside pixelsurf
		 float sv_gravity = interfaces::console->get_convar("sv_gravity")->get_float();
		 float fTickInterval = interfaces::globals->interval_per_tick;
		 float fTickRate = (fTickInterval > 0) ? (1.0f / fTickInterval) : 0.0f;

		 uint64_t current_time = GetTickCount64();

		 // Clear old cache periodically
		 if (current_time - last_cache_clear > CACHE_LIFETIME_MS) {
			 for (auto it = jetpack_cache.begin(); it != jetpack_cache.end(); ) {
				 if (current_time - it->second.scan_time > CACHE_LIFETIME_MS) {
					 it = jetpack_cache.erase(it);
				 }
				 else {
					 ++it;
				 }
			 }
			 last_cache_clear = current_time;
		 }

		 // Key release handling
		 if (!menu::checkkey(c::movement::jetpack_to_surf_key, c::movement::jetpack_to_surf_key_s)) {
			 IsJetpacking = false;
			 jetpack_ticks = 0;
			 marker_target = -1;
			 jetpack_cache.clear();
			 return;
		 }

		 if (route_markers.size() == 0)
			 return;

		 // Find nearest valid marker
		 std::vector<std::pair<float, int>> points;
		 for (int i = 0; i < route_markers.size(); i++) {
			 if (interfaces::engine->get_level_name() == route_markers.at(i).map && route_markers.at(i).active) {
				 float Distance = g::local->origin().distance_to(route_markers.at(i).position);
				 if (Distance <= route_markers.at(i).radius) {
					 points.push_back(std::make_pair(Distance, i));
				 }
			 }
		 }

		 if (points.empty())
			 return;

		 std::sort(points.begin(), points.end());

		 int max_points = std::min<int>(c::assist::pixelsurf_max_points, (int)points.size());

		 for (int point_index = 0; point_index < max_points; point_index++) {
			 int index = points[point_index].second;

			 if (index < 0 || index >= (int)route_markers.size())
				 continue;

			 vec3_t target_pos = route_markers.at(index).position;
			 vec3_t current_pos = g::local->origin();
			 vec3_t current_vel = g::local->get_velocity();

			 // Calculate direction to target
			 vec3_t direction = target_pos - current_pos;
			 float horizontal_dist = direction.length_2d();
			 float vertical_dist = direction.z;

			 int BackupButtons = cmd->buttons;
			 float BackupForwardMove = cmd->forward_move;
			 float BackupSideMove = cmd->side_move;
			 vec3_t BackupViewAngles = cmd->view_angles;
			 int BackupPredicted = interfaces::prediction->split->commands_predicted;

			 // Check cache first
			 bool use_cached = false;
			 JetpackScanResult cached_result;
			 if (jetpack_cache.find(index) != jetpack_cache.end()) {
				 cached_result = jetpack_cache[index];
				 if (cached_result.valid && (current_time - cached_result.scan_time < CACHE_LIFETIME_MS)) {
					 use_cached = true;
					 IsJetpacking = true;
					 jetpack_ticks = cmd->tick_count + cached_result.tick_offset;
					 marker_target = index;
				 }
			 }

			 // Scan for jetpack trajectory if not cached
			 if (!use_cached && !IsJetpacking) {
				 // Calculate optimal jetpack angle and timing
				 vec3_t target_angle = direction.to_angle();

				 // Simulate jetpack thrust
				 const float JETPACK_THRUST = 300.0f; // Adjust based on jetpack plugin settings
				 const float JETPACK_FUEL_RATE = 10.0f; // Fuel consumption per tick

				 int max_scan_ticks = time_to_ticks(c::assist::pixelsurf_assist_ticks);
				 int scan_step = 1;

				 // Adaptive scanning based on distance
				 if (horizontal_dist > 300.f) {
					 scan_step = 3;
				 }
				 else if (horizontal_dist > 150.f) {
					 scan_step = 2;
				 }

				 int scanned_this_frame = 0;
				 bool found_trajectory = false;

				 for (int i = 0; i < max_scan_ticks && !found_trajectory; i += scan_step) {
					 if (scanned_this_frame >= MAX_SCAN_TICKS_PER_FRAME)
						 break;

					 scanned_this_frame++;

					 // Simulate physics with jetpack thrust
					 vec3_t simulated_pos = current_pos;
					 vec3_t simulated_vel = current_vel;

					 // Apply jetpack thrust in direction of target
					 vec3_t thrust_dir = direction.normalized();

					 for (int tick = 0; tick < i; tick++) {
						 // Apply gravity
						 simulated_vel.z -= (sv_gravity * fTickInterval);

						 // Apply jetpack thrust (upward and forward)
						 simulated_vel += thrust_dir * (JETPACK_THRUST * fTickInterval);

						 // Air resistance / max velocity cap
						 float speed = simulated_vel.length();
						 if (speed > 3500.0f) {
							 simulated_vel = simulated_vel.normalized() * 3500.0f;
						 }

						 // Update position
						 simulated_pos += simulated_vel * fTickInterval;
					 }

					 // Check if we reached the target
					 float dist_to_target = simulated_pos.distance_to(target_pos);
					 float height_diff = std::abs(simulated_pos.z - target_pos.z);

					 // Success criteria: within radius and acceptable height difference
					 if (dist_to_target <= route_markers.at(index).radius && height_diff <= 32.0f) {
						 // Cache the result
						 JetpackScanResult result;
						 result.valid = true;
						 result.tick_offset = i - 3; // Lead time adjustment
						 result.marker_index = index;
						 result.target_position = target_pos;
						 result.target_z = target_pos.z;
						 result.scan_time = current_time;
						 jetpack_cache[index] = result;

						 IsJetpacking = true;
						 jetpack_ticks = cmd->tick_count + result.tick_offset;
						 marker_target = index;
						 found_trajectory = true;

						 if (c::assist::assist_render) {
							 myWindowDetect.AddWindow(5, std::string("jetpacking to ").append(std::to_string((int)target_pos.z)));
						 }
						 break;
					 }
				 }
			 }

			 // Apply jetpack movement
			 if (IsJetpacking && cmd->tick_count <= jetpack_ticks && marker_target == index) {
				 vec3_t direction_to_target = target_pos - g::local->origin();
				 vec3_t target_angles = direction_to_target.to_angle();

				 // Apply viewangles toward target
				 cmd->view_angles = target_angles;
				 UtilityAssistant.handle_fix(cmd, target_angles);

				 // Activate jetpack (assuming +jetpack command or similar)
				 cmd->buttons |= in_speed; // Or whatever button activates jetpack in your plugin

				 // Apply movement inputs
				 cmd->forward_move = 450.0f; // Max forward

				 // Optional: Add strafe adjustment for better control
				 float angle_diff = target_angles.y - BackupViewAngles.y;
				 while (angle_diff > 180.f) angle_diff -= 360.f;
				 while (angle_diff < -180.f) angle_diff += 360.f;

				 if (std::abs(angle_diff) > 5.0f) {
					 cmd->side_move = (angle_diff > 0) ? 450.0f : -450.0f;
				 }

				 // Check if we've reached the target
				 float current_dist = g::local->origin().distance_to(target_pos);
				 if (current_dist <= route_markers.at(index).radius) {
					 IsJetpacking = false;
					 jetpack_ticks = 0;
					 marker_target = -1;

					 if (c::assist::assist_render) {
						 myWindowDetect.AddWindow(5, "reached marker");
					 }
				 }
			 }

			 // Reset if we've passed the tick count
			 if (cmd->tick_count > jetpack_ticks && IsJetpacking) {
				 IsJetpacking = false;
				 jetpack_ticks = 0;
				 marker_target = -1;
			 }

			 prediction::restore_ent_to_predicted_frame(BackupPredicted - 1);

			 if (IsJetpacking) {
				 break; // Only process one marker at a time
			 }
		 }

		 prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
	 }

public:

	void save_markers() {
		save();
	}

	void load_markers() {
		load();
	}

	void pre(c_usercmd* cmd) {
		static bool loaded = false;
		static std::string last_map = "";
		bool in_game = interfaces::engine->is_connected();
		const char* current_map = interfaces::engine->get_level_name();

		if (in_game)
		{
			if (!loaded || last_map != current_map)
			{
				this->load();
				last_map = current_map;
				loaded = true;
			}
		}
		else
		{
			loaded = false;
		}
		this->draw_scan_line(cmd);
		this->scan_markers_from_line(cmd);
		features::movement::pixelsurf_finder::update();
		this->sync_finder_markers();
		this->handle_marker_placement(cmd);
		this->handle_marker_clearing();
		this->assist_createmove(cmd);
		this->auto_headbang(cmd);
		this->auto_wall_bounce(cmd);
		this->auto_headbang_align(cmd);
	}

	void run(c_usercmd* cmd) {
		this->detect_multisurf_chains(route_markers, cmd, FLT_MAX);
		static bool calc_was_pressed = false;
		const bool calculate_pressed =
			menu::checkkey(c::assist::ps_calc_route_key, c::assist::ps_calc_route_key_s);
		const bool calculate_now = calculate_pressed && !calc_was_pressed;
		calc_was_pressed = calculate_pressed;

		if (c::movement::pscalc_enable)
			features::movement::pixelsurf_calculator::update(cmd, this->build_route_request(), calculate_now);
		else
			features::movement::pixelsurf_calculator::reset();
		this->smooth_chain_movement(cmd);
		this->movement_update(cmd);
	}

	void notify() {
		// Use deltaTime from ImGui IO if we have a context, otherwise skip
		if (ImGui::GetCurrentContext() == nullptr)
			return;

		float deltaTime = ImGui::GetIO().DeltaTime;
		myWindowDetect.Update(deltaTime);
		
		// If we are currently in an ImGui frame, we can draw directly
		ImDrawList* draw = ImGui::GetBackgroundDrawList();
		if (draw)
			myWindowDetect.Draw(draw, ImGui::GetIO().DisplaySize);
	}

	void draw() {
		this->draw_scan_line(nullptr);
		this->draw_route_markers();
		features::movement::pixelsurf_calculator::draw();
	}
};
inline RouteAssist RouteStudio;

class StrafeAssist {
public:
	float last_decel_adjustment;

	void on_create_move(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive()) [[unlikely]]
			return;

			const vec3_t velocity = g::local->velocity();
			const float current_speed = velocity.length_2d();

			// Skip if stationary
			if (current_speed < 1.f) [[unlikely]] {
				reset();
				return;
				}

				// Calculate velocity direction in yaw
			vel_yaw = math::rad2deg(atan2f(velocity.y, velocity.x));

			// Calculate angular delta between view and velocity
			yaw_delta = math::angle_diff(cmd->view_angles.y, vel_yaw);

			// Ideal strafe angle for maximum acceleration
			const float abs_yaw_delta = fabsf(yaw_delta);
			ideal_strafe_angle = (abs_yaw_delta > 90.f) ? 90.f : abs_yaw_delta;

			// Compute gain metrics
			const float gain_per_tick = current_speed - last_speed;
			current_gain = (gain_per_tick > 0.f) ? gain_per_tick : 0.f;

			// Ideal sync based on angle alignment
			ideal_sync = cosf(math::deg2rad(yaw_delta));

			// Store for next tick
			last_speed = current_speed;
			last_yaw = cmd->view_angles.y;
	}

	// Silent auto-strafe - smooth A-D strafing that looks natural
	void run_silent_strafe(c_usercmd* cmd, vec3_t& current_angle) {
		if (!g::local || !g::local->is_alive()) [[unlikely]]
			return;

			const auto mt = g::local->move_type();
			if (mt == movetype_ladder || mt == movetype_noclip) [[unlikely]]
				return;

				// Only strafe in air
				if (g::local->flags() & fl_onground)
					return;

				const vec3_t velocity = g::local->velocity();
				const float speed = velocity.length_2d();

				// Need minimum speed to strafe
				if (speed <= 5.f) [[unlikely]]
					return;

					// Detect movement direction based on keys
					static float direction_angle = 0.f;
					const bool back = cmd->buttons & in_back;
					const bool forward = cmd->buttons & in_forward;
					const bool right = cmd->buttons & in_moveright;
					const bool left = cmd->buttons & in_moveleft;

					// Calculate desired movement direction
					if (back) {
						direction_angle = -180.f;
						if (left)
							direction_angle -= 45.f;
						else if (right)
							direction_angle += 45.f;
					}
					else if (left) {
						direction_angle = 90.f;
						if (back)
							direction_angle += 45.f;
						else if (forward)
							direction_angle -= 45.f;
					}
					else if (right) {
						direction_angle = -90.f;
						if (back)
							direction_angle -= 45.f;
						else if (forward)
							direction_angle += 45.f;
					}
					else {
						direction_angle = 0.f;
					}

					// Apply direction to view angle
					current_angle.y += direction_angle;

					// Get velocity angle
					const float vel_angle = math::rad2deg(atan2f(velocity.y, velocity.x));

					// Determine optimal strafe angle based on speed
					float optimal_angle = calculate_optimal_strafe_angle(speed);

					// Clamp turn to optimal angle to prevent over-rotation

					// Clear original movement
					float vel_yaw = math::deg2rad(atan2f(velocity.y, velocity.x));
					float optimal = std::atan2(15.f, speed) * math::deg2rad(45.f);

					static bool flip;
					flip = !flip;

					cmd->side_move = flip ? 450.f : -450.f;
					cmd->forward_move = 0.f;

					float target = vel_yaw + (flip ? optimal : -optimal);
					float delta = math::angle_diff(target, current_angle.y);
					// Smooth angle adjustment - key to making it not look rage-like
					float turn_amount = delta;
					turn_amount = std::clamp(turn_amount, -optimal_angle, optimal_angle);

					const float smoothing_factor = std::clamp(speed / 650.f, 0.3f, 1.0f);
					delta /= smoothing_factor;

					current_angle.y += delta;

					// Apply silent A-D strafe
					// Negative delta = strafe right, positive delta = strafe left
					if (fabsf(delta) > 1.f) {
						cmd->side_move = delta > 0.f ? -850.f : 850.f;

						// Adjust view angle smoothly
						current_angle.y = math::normalize_yaw(current_angle.y - turn_amount);
					}
					interfaces::engine->set_view_angles(current_angle);
					UtilityAssistant.handle_fix(cmd, current_angle);
	}

	// Alternative: Even more subtle silent strafe
	void run_legit_strafe(c_usercmd* cmd, vec3_t& current_angle) {
		if (!g::local || !g::local->is_alive()) [[unlikely]]
			return;

			const auto mt = g::local->move_type();
			if (mt == movetype_ladder || mt == movetype_noclip) [[unlikely]]
				return;

				if (g::local->flags() & fl_onground)
					return;

				const vec3_t velocity = g::local->velocity();
				const float speed = velocity.length_2d();

				if (speed <= 5.f) [[unlikely]]
					return;

					// Only strafe if A or D is being pressed
					const bool strafe_left = cmd->buttons & in_moveleft;
					const bool strafe_right = cmd->buttons & in_moveright;

					if (!strafe_left && !strafe_right)
						return;

					// Get velocity direction
					const float vel_yaw_deg = math::rad2deg(atan2f(velocity.y, velocity.x));

					// Calculate optimal strafe angle (speed-dependent)
					float optimal_angle = calculate_optimal_strafe_angle(speed);

					// Reduce angle for more legit look
					optimal_angle *= 0.7f; // 70% of optimal for smoother appearance

					// Determine target angle based on strafe direction
					float target_angle;
					if (strafe_left) {
						target_angle = vel_yaw_deg + optimal_angle;
					}
					else { // strafe_right
						target_angle = vel_yaw_deg - optimal_angle;
					}

					// Calculate turn needed
					float yaw_diff = math::angle_diff(target_angle, current_angle.y);

					// Apply gradual smoothing - this is what makes it look human
					const float turn_speed = 0.3f; // Lower = smoother/slower, higher = snappier
					yaw_diff *= turn_speed;

					// Apply the turn
					current_angle.y = math::normalize_yaw(current_angle.y + yaw_diff);

					// Set side movement
					cmd->side_move = strafe_left ? -450.f : 450.f;

					// Reduce forward movement slightly for better control
					if (cmd->forward_move > 0.f) {
						cmd->forward_move *= 0.85f;
					}
					interfaces::engine->set_view_angles(current_angle);
					UtilityAssistant.handle_fix(cmd, current_angle);
	}

	// Perfect strafe implementation with mouse control
	void run_perfect_strafe(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive()) [[unlikely]]
			return;

			const auto mt = g::local->move_type();
			if (mt == movetype_ladder || mt == movetype_noclip) [[unlikely]]
				return;

				const vec3_t velocity = g::local->velocity();
				const float speed = velocity.length_2d();

				if (speed < 1.f) [[unlikely]]
					return;

					// Calculate velocity angle
					const float vel_angle = math::rad2deg(atan2f(velocity.y, velocity.x));
					const float view_yaw = cmd->view_angles.y;

					// Determine strafe direction based on input
					float strafe_dir = 0.f;
					if (cmd->side_move > 0.f) {
						strafe_dir = 1.f; // Strafe right
					}
					else if (cmd->side_move < 0.f) {
						strafe_dir = -1.f; // Strafe left
					}
					else {
						return; // No strafe input
					}

					// Calculate optimal strafe angle (typically ~15-30 degrees)
					const float optimal_angle = calculate_optimal_strafe_angle(speed);

					// Target angle is velocity direction +/- optimal angle
					const float target_yaw = vel_angle + (optimal_angle * strafe_dir);

					// Calculate required turn to reach target
					float yaw_delta_to_target = math::angle_diff(target_yaw, view_yaw);

					// Smooth the turn for more human-like movement
					const float turn_smoothing = 0.7f; // 1.0 = instant, lower = smoother
					yaw_delta_to_target *= turn_smoothing;

					// Convert to mouse movement
					const float mouse_sensitivity = get_mouse_sensitivity();
					const float m_yaw = get_m_yaw();

					const float required_mouse_dx = (yaw_delta_to_target / m_yaw) / mouse_sensitivity;

					// Apply mouse movement
					cmd->mouse_dx = static_cast<short>(std::clamp(required_mouse_dx, -500.f, 500.f));
					cmd->mouse_dy = 0;
	}

	// Ultra-robotic mode: Frame-perfect strafing
	void run_frame_perfect(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive()) [[unlikely]]
			return;

			const vec3_t velocity = g::local->velocity();
			const float speed = velocity.length_2d();

			if (speed < 1.f) [[unlikely]]
				return;

				// Calculate velocity direction
				const float vel_yaw_rad = atan2f(velocity.y, velocity.x);
				const float vel_yaw_deg = math::rad2deg(vel_yaw_rad);

				// Determine strafe direction
				const bool strafe_right = cmd->side_move > 0.f;
				const bool strafe_left = cmd->side_move < 0.f;

				if (!strafe_right && !strafe_left)
					return;

				// Get game parameters
				const float max_speed = 250.f; // sv_maxspeed (default)
				const float airaccel = interfaces::console->get_convar("sv_airaccelerate")->get_float();

				// Calculate optimal angle based on speed and acceleration
				float optimal_angle;
				if (speed < max_speed) {
					optimal_angle = 90.f * (1.f - (speed / max_speed));
				}
				else {
					optimal_angle = 15.f + (30.f / (speed / max_speed));
				}

				// Clamp to reasonable range
				optimal_angle = std::clamp(optimal_angle, 5.f, 90.f);

				// Apply direction
				const float strafe_multiplier = strafe_right ? 1.f : -1.f;
				const float target_angle = vel_yaw_deg + (optimal_angle * strafe_multiplier);

				// Calculate exact turn needed
				const float current_yaw = cmd->view_angles.y;
				float turn_amount = math::angle_diff(target_angle, current_yaw);

				// Convert to mouse delta with perfect precision
				const float sensitivity = get_mouse_sensitivity();
				const float m_yaw_val = get_m_yaw();

				const float precise_mouse_dx = turn_amount / (sensitivity * m_yaw_val);

				// Apply with sub-pixel precision
				cmd->mouse_dx = static_cast<short>(precise_mouse_dx);
				cmd->mouse_dy = 0;

				// Clamp to prevent extreme values
				cmd->mouse_dx = std::clamp(cmd->mouse_dx, static_cast<short>(-1000), static_cast<short>(1000));
	}

	// Robotic deceleration mode: Velocity-based view deceleration with frame-time precision
	void run_robotic_decel(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive()) [[unlikely]]
			return;

			const auto mt = g::local->move_type();
			if (mt == movetype_ladder || mt == movetype_noclip) [[unlikely]]
				return;

				const vec3_t velocity = g::local->velocity();
				const float speed = velocity.length_2d();

				if (speed < 1.f) [[unlikely]] {
					reset_decel_state();
					return;
					}

					// Get frame time for precise time-based calculations
				const float frame_time = interfaces::globals->interval_per_tick;

				// Calculate velocity direction
				const float vel_yaw_deg = math::rad2deg(atan2f(velocity.y, velocity.x));
				const float current_yaw = cmd->view_angles.y;

				// Calculate angle delta between view and velocity
				float yaw_delta = math::angle_diff(vel_yaw_deg, current_yaw);
				const float abs_yaw_delta = fabsf(yaw_delta);

				// Velocity-based deceleration factor
				// Higher speed = more aggressive deceleration to prevent overstrafe
				const float velocity_factor = std::clamp(speed / 450.f, 0.1f, 2.0f);

				// Calculate deceleration needed based on velocity alignment
				// When aligned with velocity (low delta), decelerate view rotation
				// When misaligned (high delta), accelerate to catch up
				float decel_intensity;
				if (abs_yaw_delta < 15.f) {
					// Near-perfect alignment - heavy deceleration for deadstrafe
					decel_intensity = 1.0f - (abs_yaw_delta / 15.f);
				}
				else if (abs_yaw_delta < 45.f) {
					// Moderate alignment - partial deceleration
					decel_intensity = 0.5f * (1.0f - ((abs_yaw_delta - 15.f) / 30.f));
				}
				else {
					// Poor alignment - minimal deceleration, let correction happen
					decel_intensity = 0.1f;
				}

				// Frame-time based deceleration scalar for robotic precision
				const float frame_scalar = frame_time / 0.015f; // Normalized to 66.67 tick
				const float decel_rate = decel_intensity * velocity_factor * frame_scalar;

				// Calculate target deceleration angle
				// Gradually reduce yaw delta to zero for auto-deadstrafe
				float target_yaw_adjustment = yaw_delta * decel_rate;

				// Apply robotic damping based on previous frame
				// This creates the characteristic "snappy but smooth" robotic motion
				const float damping = 0.85f; // Higher = more robotic snapping
				target_yaw_adjustment = (target_yaw_adjustment * damping) + (last_decel_adjustment * (1.f - damping));

				// Store for next frame
				last_decel_adjustment = target_yaw_adjustment;

				// Apply the deceleration to view angles
				// Negative because we're reducing the delta
				cmd->view_angles.y = math::normalize_yaw(current_yaw + target_yaw_adjustment);

				// Auto-deadstrafe detection and execution
				// When velocity delta is minimal, kill all input for perfect stop
				if (abs_yaw_delta < 5.f && speed > 50.f) {
					const float speed_decay = (speed - last_speed) / frame_time;

					// If we're losing speed quickly, engage deadstrafe
					if (speed_decay < -10.f) {
						cmd->forward_move = 0.f;
						cmd->side_move = 0.f;

						// Lock view to velocity direction for robotic precision
						cmd->view_angles.y = vel_yaw_deg;
					}
				}

				// Robotic movement adjustment based on alignment
				if (cmd->side_move != 0.f || cmd->forward_move != 0.f) {
					// Scale movement based on velocity alignment
					// Better alignment = more aggressive movement for speed maintenance
					const float movement_scalar = 1.f - (abs_yaw_delta / 90.f);

					// Apply frame-time compensation for consistent behavior
					const float movement_multiplier = movement_scalar * frame_scalar;

					cmd->side_move *= (1.f + movement_multiplier);
					cmd->forward_move *= (1.f + movement_multiplier);

					// Clamp to valid Source engine values
					cmd->side_move = std::clamp(cmd->side_move, -450.f, 450.f);
					cmd->forward_move = std::clamp(cmd->forward_move, -450.f, 450.f);
				}
				interfaces::engine->set_view_angles(cmd->view_angles);
				UtilityAssistant.handle_fix(cmd, cmd->view_angles);
	}

	void reset_decel_state() {
		last_decel_adjustment = 0.f;
	}

	// Getters
	float get_gain() const { return current_gain; }
	float get_sync() const { return ideal_sync; }
	float get_velocity_yaw() const { return vel_yaw; }
	float get_yaw_delta() const { return yaw_delta; }
	float get_ideal_angle() const { return ideal_strafe_angle; }

	void reset() {
		last_speed = 0.f;
		current_gain = 0.f;
		ideal_sync = 0.f;
		vel_yaw = 0.f;
		yaw_delta = 0.f;
		last_yaw = 0.f;
		ideal_strafe_angle = 0.f;
	}

private:
	// Calculate optimal strafe angle based on current speed
	__forceinline float calculate_optimal_strafe_angle(float speed) noexcept
	{
		// Avoid div-by-zero & jitter
		speed = std::max<float>(speed, 1.f);

		// Source engine air acceleration derived angle
		// This naturally shrinks as speed increases
		float angle = math::rad2deg(std::atan(30.f / speed));

		// Hard clamps to prevent rage-like snapping
		angle = std::clamp<float>(angle, 2.0f, 15.0f);

		return angle;
	}

	// Get mouse sensitivity from game
	__forceinline float get_mouse_sensitivity() const {
		static auto sensitivity_cvar = interfaces::console->get_convar("sensitivity");
		return sensitivity_cvar ? sensitivity_cvar->get_float() : 1.0f;
	}

	// Get m_yaw value (yaw scaling factor)
	__forceinline float get_m_yaw() const {
		static auto m_yaw_cvar = interfaces::console->get_convar("m_yaw");
		return m_yaw_cvar ? m_yaw_cvar->get_float() : 0.022f;
	}

	// State tracking
	float last_speed = 0.f;
	float current_gain = 0.f;
	float ideal_sync = 0.f;
	float vel_yaw = 0.f;
	float yaw_delta = 0.f;
	float last_yaw = 0.f;
	float ideal_strafe_angle = 0.f;

	void runner(c_usercmd* cmd, vec3_t& angle_override) {
		if (!g::local || !c::movement::enable_strafe_type)
			return;

		if (c::movement::strafe_type == 0) {
			// Perfect strafe - mouse-based
			this->run_perfect_strafe(cmd);
		}
		else if (c::movement::strafe_type == 1) {
			// Frame perfect - competitive
			this->run_frame_perfect(cmd);
		}
		else if (c::movement::strafe_type == 2) {
			// Legit strafe - even more subtle
			this->run_legit_strafe(cmd, angle_override);
		}
		else if (c::movement::strafe_type == 3) {
			// Silent strafe - natural A-D movement
			this->run_silent_strafe(cmd, angle_override);
		}
		else if (c::movement::strafe_type == 4) {
			this->run_robotic_decel(cmd);
		}
	}

public:
	void pre(c_usercmd* cmd) {
		
	}

	void run(c_usercmd* cmd) {
		this->runner(cmd, cmd->view_angles);
	}
};
inline StrafeAssist StrafeAssistant;

class CollisionAssist {
private:
	struct avoid_collision_t {
		float m_ducking_velo = 0.f;
		float m_standing_velo = 0.f;
		float m_ducking_origin = 0.f;
		float m_standing_origin = 0.f;
	};
	avoid_collision_t m_avoid_collision;
	bool avoid_founded = false;

	// ===================================================================
	// INTELLIGENT STATE TRACKING
	// ===================================================================
	struct collision_state_t {
		// Collision detection
		bool valid_standing_collision = false;
		bool valid_ducking_clearance = false;
		float standing_penetration = 0.f;
		float ducking_clearance_gain = 0.f;

		// Historical data for stability
		int consecutive_valid_detections = 0;
		int frames_since_last_duck = 0;
		int total_ducks_this_second = 0;
		int tick_counter = 0;

		// Movement context
		float player_speed = 0.f;
		float vertical_velocity = 0.f;
		bool moving_forward = false;
		bool moving_into_obstacle = false;

		// Environment analysis
		vec3_t collision_point = vec3_t(0, 0, 0);
		vec3_t collision_normal = vec3_t(0, 0, 0);
		float obstacle_height = 0.f;
		bool is_corner = false;
		bool is_overhang = false;
	};
	collision_state_t state;

	// ===================================================================
	// CONFIGURATION - Fine-tuned thresholds
	// ===================================================================
	struct config_t {
		// Detection sensitivity
		float min_obstacle_height = 50.f;        // Must be at least 50 units tall
		float max_obstacle_height = 68.f;        // Max height we can duck under
		float min_penetration_depth = 5.f;       // Minimum collision depth to react
		float min_clearance_gain = 15.f;         // Ducking must provide this much clearance

		// Speed-based adjustments
		float min_forward_speed = 50.f;          // Must be moving forward
		float max_speed_for_precision = 300.f;   // High speed = more aggressive

		// Angle requirements
		float max_normal_z = 0.5f;               // Wall steepness (lower = more vertical)
		float min_forward_alignment = 0.3f;      // Must be moving toward obstacle

		// Stability & anti-spam
		int required_consecutive_frames = 2;     // Detections needed before ducking
		int cooldown_frames = 15;                // Frames between ducks
		int max_ducks_per_second = 3;            // Rate limit

		// Distance checks
		float min_trace_distance = 60.f;         // Minimum lookahead
		float max_trace_distance = 150.f;        // Maximum lookahead
		float critical_distance = 30.f;          // Emergency duck threshold
	} cfg;

	// ===================================================================
	// ENHANCED TRACE - Returns detailed collision information
	// ===================================================================
	struct collision_info_t {
		bool hit = false;
		float distance = 0.f;
		float penetration = 0.f;
		vec3_t point = vec3_t(0, 0, 0);
		vec3_t normal = vec3_t(0, 0, 0);
		float height_diff = 0.f;
		bool is_valid_obstacle = false;
	};

	collision_info_t trace_collision(bool duck, float distance, vec3_t direction) {
		collision_info_t info;

		vec3_t start = g::local->abs_origin();
		vec3_t end = start + direction * distance;

		// Hull dimensions
		vec3_t mins = vec3_t(-16, -16, 0);
		vec3_t maxs = duck ? vec3_t(16, 16, 45) : vec3_t(16, 16, 72);

		ray_t ray;
		ray.initialize(start, end, mins, maxs);
		trace_filter filter(g::local);
		trace_t tr;
		interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &filter, &tr);

		if (tr.did_hit()) {
			info.hit = true;
			info.distance = start.distance_to(tr.end);
			info.penetration = distance - info.distance;
			info.point = tr.end;
			info.normal = tr.plane.normal;

			// Validate obstacle characteristics
			float normal_z = tr.plane.normal.z;

			// Must be a vertical wall (not floor/ceiling)
			bool is_wall = (normal_z > -0.1f && normal_z < cfg.max_normal_z);

			// Check if obstacle is appropriate height
			vec3_t trace_up_start = tr.end + vec3_t(0, 0, 10);
			vec3_t trace_up_end = trace_up_start + vec3_t(0, 0, 70);

			ray_t ray_up;
			ray_up.initialize(trace_up_start, trace_up_end);
			trace_t tr_up;
			interfaces::trace_ray->trace_ray(ray_up, MASK_PLAYERSOLID, &filter, &tr_up);

			info.height_diff = tr_up.did_hit() ? trace_up_start.distance_to(tr_up.end) : 70.f;

			// Valid obstacle: vertical wall, appropriate height, minimum penetration
			info.is_valid_obstacle = is_wall &&
				info.height_diff >= cfg.min_obstacle_height &&
				info.height_diff <= cfg.max_obstacle_height &&
				info.penetration >= cfg.min_penetration_depth;
		}

		return info;
	}

	// ===================================================================
	// MOVEMENT ANALYSIS - Understand player intent and context
	// ===================================================================
	struct movement_context_t {
		bool moving_forward = false;
		bool aligned_with_view = false;
		float forward_alignment = 0.f;
		float speed_2d = 0.f;
		bool high_speed = false;
	};

	movement_context_t analyze_movement() {
		movement_context_t ctx;

		vec3_t velocity = g::local->velocity();
		ctx.speed_2d = velocity.length_2d();
		ctx.high_speed = ctx.speed_2d > cfg.max_speed_for_precision;

		// Get view direction
		vec3_t view_forward;
		math::angle_vectors(g::cmd->view_angles, &view_forward);
		view_forward.z = 0;
		view_forward.normalized();

		// Get movement direction
		vec3_t move_dir = velocity;
		move_dir.z = 0;
		float move_length = move_dir.length();

		if (move_length > cfg.min_forward_speed) {
			move_dir = move_dir / move_length;

			// Check alignment between view and movement
			ctx.forward_alignment = view_forward.dot(move_dir);
			ctx.aligned_with_view = ctx.forward_alignment > cfg.min_forward_alignment;
			ctx.moving_forward = ctx.aligned_with_view && (g::cmd->buttons & in_forward);
		}

		return ctx;
	}

	// ===================================================================
	// ADAPTIVE TRACE DISTANCE - Based on speed and context
	// ===================================================================
	float calculate_trace_distance() {
		float speed = state.player_speed;

		// Base distance scales with speed
		float speed_factor = std::clamp(speed / 250.f, 0.4f, 1.5f);
		float base = cfg.min_trace_distance + (cfg.max_trace_distance - cfg.min_trace_distance) * speed_factor;

		// Add reaction time buffer
		float reaction_buffer = speed * 0.4f; // 200ms worth of movement

		return std::clamp(base + reaction_buffer, cfg.min_trace_distance, cfg.max_trace_distance);
	}

	// ===================================================================
	// MULTI-SAMPLE DETECTION - Verify collision across multiple points
	// ===================================================================
	bool verify_collision_consistency(bool duck, vec3_t direction, float distance) {
		int samples = 16;
		int hits = 0;
		float sample_distances[] = { 8.7f, 16.0f, 32.3f }; // Near, center, far

		for (float dist_mult : sample_distances) {
			auto info = trace_collision(duck, distance * dist_mult, direction);
			if (info.hit && info.is_valid_obstacle) {
				hits++;
			}
		}

		// Require majority agreement
		return hits >= 2;
	}

	// ===================================================================
	// DUCKING BENEFIT ANALYSIS
	// ===================================================================
	struct duck_benefit_t {
		bool standing_blocked = false;
		bool ducking_clears = false;
		float clearance_gained = 0.f;
		float confidence = 0.f;
		bool recommended = false;
	};

	duck_benefit_t evaluate_duck_benefit(vec3_t direction, float distance) {
		duck_benefit_t benefit;

		// Check standing
		auto standing_info = trace_collision(true, distance, direction);
		benefit.standing_blocked = standing_info.hit && standing_info.is_valid_obstacle;

		// Check ducking
		auto ducking_info = trace_collision(true, distance, direction);
		benefit.ducking_clears = !ducking_info.hit || !ducking_info.is_valid_obstacle;

		if (benefit.standing_blocked && benefit.ducking_clears) {
			// Calculate clearance improvement
			float standing_penetration = standing_info.penetration;
			float ducking_penetration = ducking_info.hit ? ducking_info.penetration : 0.f;
			benefit.clearance_gained = standing_penetration - ducking_penetration;

			// Build confidence score
			float conf = 1.0f;

			// Factor: Clearance gain (must be significant)
			if (benefit.clearance_gained < cfg.min_clearance_gain) {
				conf *= 0.3f;
			}
			else {
				conf *= std::min<float>(benefit.clearance_gained / 40.f, 1.0f);
			}

			// Factor: Obstacle height (must be in duckable range)
			float height = standing_info.height_diff;
			if (height < cfg.min_obstacle_height || height > cfg.max_obstacle_height) {
				conf *= 0.2f;
			}

			// Factor: Consistency check
			bool standing_consistent = verify_collision_consistency(true, direction, distance);
			bool ducking_consistent = !verify_collision_consistency(true, direction, distance);
			if (!standing_consistent || !ducking_consistent) {
				conf *= 0.5f;
			}

			// Factor: Recent history (prevent spam)
			if (state.frames_since_last_duck < cfg.cooldown_frames) {
				conf *= 0.3f;
			}

			// Factor: Rate limiting
			if (state.total_ducks_this_second >= cfg.max_ducks_per_second) {
				conf *= 0.1f;
			}

			benefit.confidence = conf;
			benefit.recommended = conf > 0.7f; // High confidence threshold
		}

		return benefit;
	}

	// ===================================================================
	// MAIN COLLISION AVOIDANCE
	// ===================================================================
	void avoid_collision(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive()) {
			avoid_founded = false;
			state = collision_state_t();
			return;
		}

		// Movement type restrictions
		const auto move_type = g::local->move_type();
		if (move_type == movetype_ladder || move_type == movetype_noclip ||
			move_type == movetype_fly || move_type == movetype_observer) {
			avoid_founded = false;
			return;
		}

		// Feature conflicts
		if (!c::movement::auto_duck_collision ||
			prediction_backup::flags & fl_onground ||
			features::movement::should_edge_bug ||
			features::movement::detect_data.ticks_left ||
			PixelSurfAssistant.ps_data.pixelsurfing ||
			features::movement::should_ps ||
			(c::movement::edge_bug && menu::checkkey(c::movement::edge_bug_key, c::movement::edge_bug_key_s)) ||
			(c::movement::delay_hop && menu::checkkey(c::movement::delay_hop_key, c::movement::delay_hop_key_s)))
		{
			avoid_founded = false;
			return;
		}

		// Update tick counter for rate limiting
		state.tick_counter++;
		if (state.tick_counter % 64 == 0) { // Reset every second
			state.total_ducks_this_second = 0;
		}

		// Analyze current movement
		auto movement = analyze_movement();
		state.player_speed = movement.speed_2d;
		state.moving_forward = movement.moving_forward;

		// Must be moving forward with intent
		if (!movement.moving_forward || movement.speed_2d < cfg.min_forward_speed) {
			avoid_founded = false;
			state.consecutive_valid_detections = 0;
			return;
		}

		// Get view direction
		vec3_t view_forward;
		math::angle_vectors(g::cmd->view_angles, &view_forward);
		view_forward.z = 0;
		view_forward.normalized();

		// Calculate adaptive trace distance
		float trace_dist = calculate_trace_distance();

		// Run prediction
		prediction::begin(cmd);
		prediction::end();

		// Evaluate duck benefit
		auto benefit = evaluate_duck_benefit(view_forward, trace_dist);

		// Update state
		state.frames_since_last_duck++;

		if (benefit.recommended) {
			state.consecutive_valid_detections++;
		}
		else {
			state.consecutive_valid_detections = std::max<int>(0, state.consecutive_valid_detections - 1);
		}

		// Decision: Duck if we have consistent valid detections
		bool should_duck = false;

		if (state.consecutive_valid_detections >= cfg.required_consecutive_frames) {
			should_duck = true;
		}
		// Emergency duck for critical distance
		else if (benefit.standing_blocked && benefit.clearance_gained > 20.f &&
			benefit.standing_blocked && trace_dist < cfg.critical_distance) {
			should_duck = true;
		}

		// Apply result
		avoid_founded = should_duck;

		if (avoid_founded) {
			cmd->buttons |= in_duck;
			state.frames_since_last_duck = 0;
			state.total_ducks_this_second++;
			state.consecutive_valid_detections = 0; // Reset after ducking
		}
	}

public:
	void pre(c_usercmd* cmd) {
		// Reserved for pre-processing
	}

	void run(c_usercmd* cmd) {
		this->avoid_collision(cmd);
	}

	// Debug accessors
	bool is_avoiding() const { return avoid_founded; }
	float get_player_speed() const { return state.player_speed; }
	int get_consecutive_detections() const { return state.consecutive_valid_detections; }
	float get_collision_penetration() const { return state.standing_penetration; }
};
inline CollisionAssist CollisionAssistant;

class AirStuckAssist {
private:

	struct air_stuck_datas
	{
		bool detected = false;
		bool touching_wall = false;
		vec3_t wall_angle;
		float FoundedForwardMove, FoundedSideMove, FoundedViewAngle, FoundedUpMove = 0.f;

		air_stuck_datas find_walldata()
		{
			if (menu::checkkey(c::movement::airstuck_key, c::movement::airstuck_key_s)) {
				air_stuck_datas airstuck_data;
				const auto mins = g::local->collideable()->mins();
				const auto maxs = g::local->collideable()->maxs();
				vec3_t best_wall_normal;
				float max_radias = m_pi * 2.f;
				float step = max_radias / 16.f;
				for (float angle = 0.f; angle < max_radias; angle += step)
				{
					vec3_t wishdir = vec3_t(cosf(angle), sinf(angle), 0.f);
					const auto startPos = g::local->abs_origin();
					const auto endPos = startPos + wishdir;
					trace_t trace;
					trace_filter filter(g::local);
					ray_t ray;

					ray.initialize(startPos, endPos, mins, maxs);

					interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &filter, &trace);

					vec3_t hit = trace.did_hit() ? trace.end : endPos;
					if (trace.did_hit()) {
						airstuck_data.touching_wall = true;
						best_wall_normal = trace.plane.normal;
						vec3_t facing_dir = { -best_wall_normal.x, -best_wall_normal.y, 0.0f };
						float wall_yaw = math::rad2deg(atan2f(facing_dir.y, facing_dir.x));
						airstuck_data.wall_angle = best_wall_normal.to_angle();
						//interfaces::debug_overlay->add_box_overlay(hit, mins, maxs, { 0.f, 0.f, 0.f }, 0, 255, 0, 200, 0.05f);	//green box if hit wall
					}
					else
					{
						//interfaces::debug_overlay->add_box_overlay(hit, mins, maxs, { 0.f, 0.f, 0.f }, 255, 0, 0, 200, 0.05f);	//red box if no
					}

					static vec3_t prev{ };
					if (angle > 0.f) {
						//interfaces::debug_overlay->add_line_overlay(prev, hit, 0, 255, 0, true, 0.05f);	//just lines
					}
					prev = hit;
				}
				return airstuck_data;
			}
		}

		void set_detected() {
			this->detected = true;
		}
		void set_undetected() {
			this->detected = false;
			this->FoundedForwardMove = 0.f;
			this->FoundedSideMove = 0.f;
			this->FoundedViewAngle = 0.f;
			this->FoundedUpMove = 0.f;
		}
		void save_movementkeys(float forward, float side, float upmove, float viewangle)
		{
			this->FoundedForwardMove = forward;
			this->FoundedSideMove = side;
			this->FoundedViewAngle = viewangle;
			this->FoundedUpMove = upmove;
		}

	}	airstuck_data;


	void handle_airstuck(c_usercmd* cmd) {
		if (!c::movement::airstuck_enabled || !g::local || !g::local->is_alive() || g::local->move_type() != movetype_walk)
			return;

		if (!menu::checkkey(c::movement::airstuck_key, c::movement::airstuck_key_s))
			return;

		if (menu::checkkey(c::movement::jump_bug_key, c::movement::jump_bug_key_s) && (!(prediction_backup::flags & 1)))
			return;

		float sv_gravity = interfaces::console->get_convar(("sv_gravity"))->get_float();
		float fTickInterval = interfaces::globals->interval_per_tick;
		float fTickRate = (fTickInterval > 0) ? (1.0f / fTickInterval) : 0.0f;
		float targetZvelo = ((sv_gravity / 2) / fTickRate) * -1.f;
		float FoundedForwardMove, FoundedSideMove;
		vec3_t FoundedViewAngle;

		if (g::local->velocity().z >= 0.f)
		{
			airstuck_data.set_undetected();
			features::movement::should_air = false;
			return;
		}

		if (!airstuck_data.detected)
		{

			const auto& wall_data = airstuck_data.find_walldata();

			if (!wall_data.touching_wall)
				return;

			wall_data.wall_angle.normalize_const();

			float rotation = deg2rad(wall_data.wall_angle.y - cmd->view_angles.y);
			float cos_rot = cos(rotation);
			float sin_rot = sin(rotation);

			float max_radias = 180.f;
			float step = max_radias / (max_radias * 2);			//0.5f

			for (float angle = -180.f; angle <= 180.f; angle += step) {		//loop for detectiong specific viewangle yaw(in lobotomy case it is -180 - 180 which is bad!). Must recode, and use specifig range yaw received from the trace.normal
				float forwardmove = cos_rot * 24.f;
				float sidemove = -sin_rot * 24.f;
				vec3_t oldvelocity = g::local->velocity();
				c_usercmd fakecmd = *cmd;			//set our fake cmd to test air stuck without affect on our real cmd

				fakecmd.forward_move = forwardmove;
				fakecmd.side_move = sidemove;
				fakecmd.view_angles.y = angle;

				prediction::begin(&fakecmd);
				prediction::end();

				if (g::local->move_type() != movetype_walk)
					continue;

				if (g::local->flags() & fl_onground)
					return;

				if (g::local->velocity().z > oldvelocity.z)		//in next tick velo > our real velo()
				{
					airstuck_data.save_movementkeys(fakecmd.forward_move, fakecmd.side_move, fakecmd.up_move, fakecmd.view_angles.y);
					airstuck_data.set_detected();
					prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
					break;
				}
				else if (g::local->velocity().z == targetZvelo)	//velocity in next tick == targetZvelo(64 tick == -6.25) so we airstucked
				{
					airstuck_data.save_movementkeys(fakecmd.forward_move, fakecmd.side_move, fakecmd.up_move, fakecmd.view_angles.y);
					airstuck_data.set_detected();
					prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
					break;
				}												//i think we maybe need find another way to detect air stuck				
				else
				{
					airstuck_data.set_undetected();
				}
				prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
			}
		}

		if (airstuck_data.detected)
		{
			prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
			cmd->forward_move = airstuck_data.FoundedForwardMove;
			cmd->side_move = airstuck_data.FoundedSideMove;
			cmd->up_move = airstuck_data.FoundedUpMove;
			cmd->view_angles.y = airstuck_data.FoundedViewAngle;
			features::movement::should_air = true;
			features::movement::should_ps = false;
		}
		else {
			features::movement::should_air = false;
		}

		if (!menu::checkkey(c::movement::airstuck_key, c::movement::airstuck_key_s))
			features::movement::should_air = false;
	}

public:

	void pre(c_usercmd* cmd) {
		this->handle_airstuck(cmd);
	}

	void run(c_usercmd* cmd) {
		
	}

};
inline AirStuckAssist AirStuckAssistant;

class JetPackAssist {
private:
	enum jp_state_t {
		JP_IDLE,
		JP_SCAN,
		JP_ASSIST_ASCEND,
		JP_ASSIST_HOVER,
		JP_ASSIST_DROP,
		JP_ASSIST_SURF
	};

	jp_state_t state = JP_IDLE;

	struct alignment_data_t {
		vec3_t wall_normal;
		vec3_t surf_position;
		vec3_t approach_velocity;
		float target_height;
		float alignment_offset;
		bool is_displacement;
		int marker_index;
		RouteAssist::jump_marker_t* target_marker;
		uint64_t scan_time;
		bool valid;
	};

	alignment_data_t alignment;

	struct trajectory_calc_t {
		vec3_t Calculate;
		float iCalcilate;
		int wait_ticks;
		bool calculated;
	};

	trajectory_calc_t trajectory;

	static constexpr uint64_t CACHE_LIFETIME_MS = 500;
	static constexpr float HOVER_TOLERANCE = 5.0f;
	static constexpr float Z_VELOCITY_ASSIST_STRENGTH = 0.25f; // How much to assist velocity.z

	// Advanced wall normal detection (like auto_headbang)
	bool scan_for_pixelsurf_wall(vec3_t player_pos, vec3_t& out_normal, vec3_t& out_position, bool& is_displacement) {
		trace_world_only fil;

		const float max_radius = m_pi * 2.f;
		const float angle_step = max_radius / 32.f;

		float closest_wall_dist = 999999.f;
		vec3_t closest_wall_normal(0, 0, 0);
		vec3_t closest_wall_pos(0, 0, 0);
		bool found_wall = false;

		for (float angle = 0.f; angle < max_radius; angle += angle_step) {
			vec3_t direction(cosf(angle), sinf(angle), 0.f);
			vec3_t end_pos = player_pos + direction * 150.f;

			ray_t ray;
			ray.initialize(player_pos, end_pos);

			trace_t tr;
			interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &fil, &tr);

			if (tr.flFraction < 1.f && fabsf(tr.plane.normal.z) < 0.1f) {
				float dist = (tr.end - player_pos).length();
				if (dist < closest_wall_dist) {
					closest_wall_dist = dist;
					closest_wall_normal = tr.plane.normal;
					closest_wall_pos = tr.end;
					found_wall = true;
					is_displacement = (strstr(tr.surface.name, "displacement") != nullptr);
				}
			}
		}

		if (found_wall) {
			out_normal = closest_wall_normal;
			out_position = closest_wall_pos;
			return true;
		}

		return false;
	}

	// Calculate alignment offset based on wall normal (like auto_headbang)
	vec3_t calculate_surf_offset(const vec3_t& base_pos, const vec3_t& wall_normal, float test_align) {
		vec3_t offset_vec(0.f, 0.f, 0.f);

		if (wall_normal.x < 0 && wall_normal.y < 0.f) {
			offset_vec = vec3_t(base_pos.x - test_align, base_pos.y - test_align, base_pos.z);
		}
		else if (wall_normal.x < 0 && wall_normal.y > 0.f) {
			offset_vec = vec3_t(base_pos.x - test_align, base_pos.y + test_align, base_pos.z);
		}
		else if (wall_normal.x > 0 && wall_normal.y < 0.f) {
			offset_vec = vec3_t(base_pos.x + test_align, base_pos.y - test_align, base_pos.z);
		}
		else if (wall_normal.x > 0 && wall_normal.y > 0.f) {
			offset_vec = vec3_t(base_pos.x + test_align, base_pos.y + test_align, base_pos.z);
		}
		else if (wall_normal.x == 0.f && wall_normal.y > 0.f) {
			offset_vec = vec3_t(base_pos.x, base_pos.y + test_align, base_pos.z);
		}
		else if (wall_normal.x == 0.f && wall_normal.y < 0.f) {
			offset_vec = vec3_t(base_pos.x, base_pos.y - test_align, base_pos.z);
		}
		else if (wall_normal.x < 0 && wall_normal.y == 0.f) {
			offset_vec = vec3_t(base_pos.x - test_align, base_pos.y, base_pos.z);
		}
		else if (wall_normal.x > 0 && wall_normal.y == 0.f) {
			offset_vec = vec3_t(base_pos.x + test_align, base_pos.y, base_pos.z);
		}

		return offset_vec;
	}

	// Validate position with traces (like jump_to_marker)
	bool validate_surf_position(const vec3_t& test_pos) {
		trace_t trace_down, trace_up;
		trace_filter flt(g::local);
		ray_t ray_down, ray_up;

		ray_down.initialize(
			vec3_t(test_pos.x, test_pos.y, test_pos.z + 54.f),
			vec3_t(test_pos.x, test_pos.y, test_pos.z - 1000.f)
		);
		interfaces::trace_ray->trace_ray(ray_down, MASK_PLAYERSOLID, &flt, &trace_down);
		vec3_t down_point = trace_down.end;

		ray_up.initialize(
			vec3_t(test_pos.x, test_pos.y, test_pos.z + 54.f),
			vec3_t(test_pos.x, test_pos.y, test_pos.z + 1000.f)
		);
		interfaces::trace_ray->trace_ray(ray_up, MASK_PLAYERSOLID, &flt, &trace_up);
		vec3_t upper_point = trace_up.end;

		if (test_pos.z + 1.f < down_point.z)
			return false;
		if (test_pos.z + 1.f > upper_point.z)
			return false;

		return true;
	}

	// Calculate trajectory (like jump_to_marker)
	bool calculate_jetpack_trajectory(c_usercmd* cmd, const vec3_t& target_pos, float& out_target_height) {
		vec3_t player_pos = g::local->origin();
		float height_diff = target_pos.z - player_pos.z;

		// Calculate hover height (above surf for alignment)
		out_target_height = target_pos.z + 20.f; // Hover 20 units above surf

		trajectory.Calculate = target_pos;
		trajectory.iCalcilate = height_diff;
		trajectory.wait_ticks = 0;
		trajectory.calculated = true;

		return true;
	}

	// ONLY ASSIST Z-AXIS: cmd->up_move and velocity.z
	void execute_z_alignment(c_usercmd* cmd) {
		if (!alignment.valid || !alignment.target_marker)
			return;

		vec3_t player_pos = g::local->origin();
		vec3_t player_vel = g::local->velocity();

		float sv_gravity = interfaces::console->get_convar("sv_gravity")->get_float();
		float tick_rate = (tick_interval > 0) ? (1.0f / tick_interval) : 0.0f;
		float target_z_velo = ((sv_gravity / 2.f) / tick_rate) * -1.f; // Pixelsurf Z velocity

		// Calculate alignment offset
		float test_align = alignment.is_displacement ? 16.001f : 15.97803f;
		vec3_t target_offset = calculate_surf_offset(alignment.surf_position, alignment.wall_normal, test_align);

		// Validate position
		if (!validate_surf_position(target_offset)) {
			state = JP_SCAN;
			return;
		}

		// Current height vs target
		float height_error = 0.f;
		float desired_z_velocity = 0.f;
		float up_move_assist = 0.f;

		// State machine - ONLY controls Z-axis
		switch (state) {
		case JP_SCAN:
			state = JP_ASSIST_ASCEND;
			break;

		case JP_ASSIST_ASCEND:
		{
			// Ascend to hover height
			height_error = alignment.target_height - player_pos.z;

			if (height_error < HOVER_TOLERANCE) {
				state = JP_ASSIST_HOVER;
				break;
			}

			// Calculate desired upward velocity
			// Simple proportional control: faster when farther
			desired_z_velocity = std::clamp(height_error * 2.0f, -300.f, 300.f);

			// Blend current Z velocity with desired (gentle assist)
			float velocity_error = desired_z_velocity - player_vel.z;
			float z_assist = velocity_error * Z_VELOCITY_ASSIST_STRENGTH;

			// Apply to velocity.z
			g::local->velocity().z += z_assist;

			// Also apply to up_move for extra control
			up_move_assist = z_assist * 1.5f;
			cmd->up_move += std::clamp(up_move_assist, -450.f, 450.f);

			break;
		}

		case JP_ASSIST_HOVER:
		{
			// Maintain hover height
			height_error = alignment.target_height - player_pos.z;

			if (fabsf(height_error) > HOVER_TOLERANCE * 2.f) {
				// Lost hover, go back to ascend
				state = JP_ASSIST_ASCEND;
				break;
			}

			// Proportional hover control
			desired_z_velocity = height_error * 3.0f; // Stronger correction for hover
			desired_z_velocity = std::clamp(desired_z_velocity, -100.f, 100.f);

			// Apply gentle Z velocity correction
			float velocity_error = desired_z_velocity - player_vel.z;
			float z_assist = velocity_error * (Z_VELOCITY_ASSIST_STRENGTH * 1.5f); // Stronger for hover

			g::local->velocity().z += z_assist;

			// Subtle up_move for fine control
			up_move_assist = z_assist * 2.0f;
			cmd->up_move += std::clamp(up_move_assist, -300.f, 300.f);

			// Check if player is moving towards surf horizontally
			vec3_t to_surf_2d = vec3_t(
				target_offset.x - player_pos.x,
				target_offset.y - player_pos.y,
				0.f
			);

			float horizontal_dist = to_surf_2d.length();
			if (horizontal_dist < 15.f) {
				// Close enough horizontally, start drop
				state = JP_ASSIST_DROP;
			}

			break;
		}

		case JP_ASSIST_DROP:
		{
			// Drop to surf position
			height_error = target_offset.z - player_pos.z;

			// Calculate desired fall velocity (controlled descent)
			// Fall faster when higher above target
			if (height_error < -5.f) {
				// Above target - let gravity work, add slight downward assist
				desired_z_velocity = -150.f; // Controlled fall
			}
			else if (height_error > 5.f) {
				// Below target - gentle upward correction
				desired_z_velocity = 50.f;
			}
			else {
				// Near target - aim for pixelsurf velocity
				desired_z_velocity = target_z_velo;
			}

			// Apply Z velocity assist
			float velocity_error = desired_z_velocity - player_vel.z;
			float z_assist = velocity_error * (Z_VELOCITY_ASSIST_STRENGTH * 0.8f);

			g::local->velocity().z += z_assist;

			// up_move for fine control
			up_move_assist = z_assist * 1.2f;
			cmd->up_move += std::clamp(up_move_assist, -450.f, 450.f);

			// Check if reached pixelsurf Z velocity
			if (fabsf(player_vel.z - target_z_velo) < 1.0f) {
				state = JP_ASSIST_SURF;
			}

			break;
		}

		case JP_ASSIST_SURF:
		{
			// Maintain pixelsurf Z velocity (like jump_to_marker)
			float z_velocity_error = target_z_velo - player_vel.z;

			// Very gentle assist to maintain surf velocity
			float z_assist = z_velocity_error * (Z_VELOCITY_ASSIST_STRENGTH * 0.5f);

			g::local->velocity().z += z_assist;

			// Minimal up_move for surf maintenance
			up_move_assist = z_assist * 0.8f;
			cmd->up_move += std::clamp(up_move_assist, -200.f, 200.f);

			// Exit surf state if velocity changes too much
			if (fabsf(player_vel.z - target_z_velo) > 3.0f) {
				if (player_vel.z > 0.f) {
					// Going up, back to ascend
					state = JP_ASSIST_ASCEND;
				}
				else if (player_vel.z < target_z_velo - 10.f) {
					// Falling too fast, back to drop
					state = JP_ASSIST_DROP;
				}
				else {
					// Lost surf completely
					state = JP_IDLE;
				}
			}

			break;
		}
		}
	}

	bool detect_and_scan_target(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive())
			return false;

		uint64_t now = GetTickCount64();

		// Check cache
		if (alignment.valid && now - alignment.scan_time < CACHE_LIFETIME_MS) {
			return true;
		}

		// Find closest marker
		float closest_dist = FLT_MAX;
		RouteAssist::jump_marker_t* closest = nullptr;
		int closest_index = -1;

		for (int i = 0; i < RouteStudio.route_markers.size(); i++) {
			auto& m = RouteStudio.route_markers[i];
			if (!m.active || interfaces::engine->get_level_name() != m.map)
				continue;

			float dist = g::local->origin().distance_to(m.position);
			if (dist <= m.radius && dist < closest_dist) {
				closest_dist = dist;
				closest = &m;
				closest_index = i;
			}
		}

		if (!closest)
			return false;

		// Scan for wall near marker
		vec3_t wall_normal, wall_pos;
		bool is_displacement;

		if (!scan_for_pixelsurf_wall(closest->position, wall_normal, wall_pos, is_displacement))
			return false;

		// Calculate trajectory
		float target_height;
		if (!calculate_jetpack_trajectory(cmd, closest->position, target_height))
			return false;

		// Store alignment data
		alignment.wall_normal = wall_normal;
		alignment.surf_position = closest->position;
		alignment.target_height = target_height;
		alignment.is_displacement = is_displacement;
		alignment.marker_index = closest_index;
		alignment.target_marker = closest;
		alignment.approach_velocity = g::local->velocity();
		alignment.approach_velocity.z = 0.f;
		alignment.scan_time = now;
		alignment.valid = true;

		return true;
	}

public:
	void run(c_usercmd* cmd) {
		if (!g::local || !g::local->is_alive()) {
			state = JP_IDLE;
			alignment.valid = false;
			return;
		}

		if (!c::movement::jetpack_to_surf)
			return;

		if (!menu::checkkey(c::movement::jetpack_to_surf_key, c::movement::jetpack_to_surf_key_s)) {
			state = JP_IDLE;
			alignment.valid = false;
			return;
		}

		// Detect target marker and scan environment
		if (state == JP_IDLE) {
			if (detect_and_scan_target(cmd)) {
				state = JP_SCAN;
			}
		}

		// Execute Z-axis alignment ONLY
		execute_z_alignment(cmd);
	}

	void draw() {
		if (!c::movement::jetpack_to_surf || !alignment.valid)
			return;

		if (!g::local || !g::local->is_alive())
			return;

		// Draw alignment target
		vec3_t screen_pos;
		if (interfaces::debug_overlay->world_to_screen(alignment.surf_position, screen_pos)) {
			color_t target_color;
			switch (state) {
			case JP_ASSIST_ASCEND: target_color = color_t(255, 255, 0, 255); break;
			case JP_ASSIST_HOVER: target_color = color_t(255, 165, 0, 255); break;
			case JP_ASSIST_DROP: target_color = color_t(255, 100, 0, 255); break;
			case JP_ASSIST_SURF: target_color = color_t(0, 255, 0, 255); break;
			default: target_color = color_t(255, 255, 255, 200); break;
			}
			im_render.drawcircle(screen_pos.x, screen_pos.y, 10.f, 32, target_color, 2.f);
		}

		// Draw hover height indicator
		vec3_t hover_pos = alignment.surf_position;
		hover_pos.z = alignment.target_height;
		if (interfaces::debug_overlay->world_to_screen(hover_pos, screen_pos)) {
			im_render.drawcircle(screen_pos.x, screen_pos.y, 8.f, 24, color_t(100, 200, 255, 200), 2.f);
		}

		// Draw vertical line between player and target
		vec3_t player_pos = g::local->origin();
		vec3_t player_screen;
		vec3_t target_same_height = alignment.surf_position;
		target_same_height.z = player_pos.z;
		vec3_t target_screen;

		if (interfaces::debug_overlay->world_to_screen(player_pos, player_screen) &&
			interfaces::debug_overlay->world_to_screen(alignment.surf_position, target_screen)) {
			im_render.drawline(player_screen.x, player_screen.y, target_screen.x, target_screen.y,
				color_t(0, 255, 255, 100), 1.f);
		}

		// Draw state text
		const char* state_text = "";
		color_t text_color = color_t(255, 255, 255, 255);
		switch (state) {
		case JP_IDLE: state_text = "JETPACK: IDLE"; text_color = color_t(128, 128, 128, 255); break;
		case JP_SCAN: state_text = "JETPACK: SCANNING"; text_color = color_t(255, 255, 0, 255); break;
		case JP_ASSIST_ASCEND: state_text = "JETPACK: Z ASSIST ASCEND"; text_color = color_t(255, 255, 0, 255); break;
		case JP_ASSIST_HOVER: state_text = "JETPACK: Z ASSIST HOVER"; text_color = color_t(255, 165, 0, 255); break;
		case JP_ASSIST_DROP: state_text = "JETPACK: Z ASSIST DROP"; text_color = color_t(255, 100, 0, 255); break;
		case JP_ASSIST_SURF: state_text = "JETPACK: Z ASSIST SURF"; text_color = color_t(0, 255, 0, 255); break;
		}

		im_render.text(10, 300, 14, fonts::esp_name_font, state_text, false, text_color);

		// Draw height info
		if (state != JP_IDLE) {
			float height_to_target = alignment.surf_position.z - player_pos.z;
			float height_to_hover = alignment.target_height - player_pos.z;
			float current_z_vel = g::local->velocity().z;

			char height_text[128];
			sprintf_s(height_text, "Height: %.1f | Hover: %.1f | Vz: %.1f",
				height_to_target, height_to_hover, current_z_vel);
			im_render.text(10, 320, 12, fonts::esp_name_font, height_text, false, color_t(200, 200, 200, 255));
		}
	}
};
inline JetPackAssist JetPackAssistant;

class AdvancedBhopAssist {
public:
	struct landing_data_t {
		vec3_t position;
		vec3_t velocity;
		vec3_t normal;
		float distance;
		float time_to_land;
		int ticks_to_land;
		bool valid;
		bool requires_strafe;
		float strafe_angle;
		float ground_slope;
		bool is_edgebug_spot;
		bool is_longjump_spot;
	};

	struct jump_opportunity_t {
		vec3_t position;
		vec3_t target_velocity;
		float optimal_speed;
		float jump_height;
		bool use_longjump;
		bool use_duck;
		float required_airtime;
		int predicted_gain;
		vec3_t landing_position;
		float confidence;
	};

private:
	// State tracking
	landing_data_t current_landing;
	landing_data_t predicted_landing;
	std::vector<landing_data_t> landing_history;

	bool bhop_active = false;
	bool longjump_queued = false;
	int longjump_ticks = 0;
	int air_ticks = 0;
	int ground_ticks = 0;

	vec3_t last_ground_pos;
	vec3_t last_air_velocity;
	float last_jump_speed = 0.f;

	// Strafe assist data
	vec3_t target_strafe_angle;
	float optimal_strafe_delta = 0.f;
	bool strafe_correction_active = false;

	// Performance optimization
	int last_prediction_tick = -1;
	landing_data_t cached_landing;

	// Constants
	static constexpr float LONGJUMP_MIN_SPEED = 200.f;
	static constexpr float LONGJUMP_OPTIMAL_SPEED = 4500.f;
	static constexpr float PREDICTION_MAX_TICKS = 64;
	static constexpr float LANDING_PREDICTION_DISTANCE = 2000.f;
	static constexpr float STRAFE_ASSIST_DISTANCE = 500.f;
	static constexpr float EDGE_DETECTION_DISTANCE = 3000.f;
	static constexpr float SLOPE_THRESHOLD = 0.7f;
	static constexpr float LONGJUMP_DUCK_TICKS = 3;
	static constexpr float STRAFE_CORRECTION_STRENGTH = 1.f;
	static constexpr float MIN_BHOP_SPEED = 50.f;
	static constexpr float MAX_PREDICTION_DISTANCE = 3000.f;

	// Inline helper functions
	__forceinline bool should_skip_processing() const {
		if (!g::local || !g::local->is_alive()) return true;
		const auto mt = g::local->move_type();
		return (mt == movetype_ladder || mt == movetype_noclip);
	}

	__forceinline float get_gravity() const {
		static auto sv_gravity = interfaces::console->get_convar("sv_gravity");
		return sv_gravity ? sv_gravity->get_float() : 800.f;
	}

	__forceinline float get_tick_interval() const {
		return interfaces::globals->interval_per_tick;
	}

public:
	// Predict landing position using trace_ray
	landing_data_t predict_landing(c_usercmd* cmd, const vec3_t& start_pos, vec3_t& velocity, int max_ticks = PREDICTION_MAX_TICKS) {
		landing_data_t result{};
		result.valid = false;

		if (velocity.length_sqr() < 1.f)
			return result;

		const float gravity = get_gravity();
		const float tick_rate = 1.0f / tick_interval;

		vec3_t current_pos = start_pos;
		vec3_t current_vel = velocity;

		// Simulate trajectory with gravity
		for (int tick = 0; tick < max_ticks; ++tick) {
			vec3_t next_pos = current_pos + current_vel * tick_interval;
			current_vel.z -= (gravity * tick_interval);

			// Don't predict too far
			if ((next_pos - start_pos).length() > MAX_PREDICTION_DISTANCE)
				break;

			// Trace ray from current to next position
			trace_t trace;
			ray_t ray;
			trace_filter filter(g::local);

			ray.initialize(current_pos, next_pos);
			interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &filter, &trace);

			// Check for ground collision
			if (trace.flFraction < 1.0f) {
				// Hit something
				if (trace.plane.normal.z >= SLOPE_THRESHOLD) {
					// It's ground
					result.valid = true;
					result.position = trace.end;
					result.velocity = current_vel;
					result.normal = trace.plane.normal;
					result.distance = (trace.end - start_pos).length();
					result.time_to_land = tick * tick_interval;
					result.ticks_to_land = tick;
					result.ground_slope = trace.plane.normal.z;

					// Check if landing requires strafe correction
					float horizontal_dist = (vec3_t(trace.end.x, trace.end.y, 0) -
						vec3_t(start_pos.x, start_pos.y, 0)).length();
					result.requires_strafe = (horizontal_dist > STRAFE_ASSIST_DISTANCE);

					// Calculate optimal strafe angle
					if (result.requires_strafe) {
						vec3_t to_landing = trace.end - start_pos;
						to_landing.z = 0;
						result.strafe_angle = to_landing.to_angle().y;
					}

					// Check for edgebug opportunity (landing on edge)
					result.is_edgebug_spot = (trace.plane.normal.z < 0.95f && trace.plane.normal.z >= SLOPE_THRESHOLD);

					// Check for longjump opportunity (high landing or gap)
					result.is_longjump_spot = (result.distance > 300.f || (trace.end.z - start_pos.z) < -64.f);

					break;
				}
			}

			current_pos = next_pos;
		}

		return result;
	}

	// Advanced landing prediction with multi-point scanning
	landing_data_t predict_landing_advanced(c_usercmd* cmd) {
		// Cache check
		if (cmd->tick_count == last_prediction_tick)
			return cached_landing;

		const vec3_t player_pos = g::local->origin();
		vec3_t player_vel = g::local->velocity();
		const int flags = g::local->flags();

		// If on ground, no prediction needed
		if (flags & fl_onground) {
			landing_data_t ground_result{};
			ground_result.valid = true;
			ground_result.position = player_pos;
			ground_result.velocity = player_vel;
			ground_result.distance = 0.f;
			ground_result.ticks_to_land = 0;
			return ground_result;
		}

		// Predict basic trajectory
		landing_data_t basic_pred = predict_landing(cmd, player_pos, player_vel, PREDICTION_MAX_TICKS);

		if (!basic_pred.valid) {
			// Try extended prediction with modified velocity (simulate strafe)
			vec3_t modified_vel = player_vel;
			float speed = player_vel.length_2d();

			if (speed > MIN_BHOP_SPEED) {
				// Try strafing left and right to find landing
				vec3_t vel_angle = player_vel.to_angle();

				for (float angle_offset : {-45.f, 45.f, -90.f, 90.f}) {
					vec3_t test_angle = vel_angle;
					test_angle.y += angle_offset;
					test_angle.normalize();

					vec3_t test_vel;
					math::angle_vectors(test_angle, &test_vel);
					test_vel = test_vel * speed;
					test_vel.z = player_vel.z;

					landing_data_t test_pred = predict_landing(cmd, player_pos, test_vel, PREDICTION_MAX_TICKS / 2);
					if (test_pred.valid) {
						basic_pred = test_pred;
						break;
					}
				}
			}
		}

		// Cache result
		last_prediction_tick = cmd->tick_count;
		cached_landing = basic_pred;

		return basic_pred;
	}

	// Calculate optimal jump timing and type
	jump_opportunity_t calculate_jump_opportunity(c_usercmd* cmd) {
		jump_opportunity_t result{};
		result.confidence = 0.f;

		const vec3_t player_pos = g::local->origin();
		vec3_t player_vel = g::local->velocity();
		const float current_speed = player_vel.length_2d();

		// Check for obstacles ahead
		vec3_t forward_dir = player_vel.normalized();
		vec3_t scan_end = player_pos + forward_dir * EDGE_DETECTION_DISTANCE;

		trace_t edge_trace;
		ray_t edge_ray;
		trace_filter filter(g::local);

		edge_ray.initialize(player_pos, scan_end);
		interfaces::trace_ray->trace_ray(edge_ray, MASK_PLAYERSOLID, &filter, &edge_trace);

		bool edge_ahead = (edge_trace.flFraction < 1.0f);

		// Determine if longjump is beneficial
		result.use_longjump = (current_speed >= LONGJUMP_MIN_SPEED);

		// Calculate optimal jump velocity
		if (result.use_longjump) {
			result.optimal_speed = LONGJUMP_OPTIMAL_SPEED;
			result.jump_height = 64.f; // Longjump height
			result.use_duck = true;
			result.required_airtime = 0.5f;
			result.predicted_gain = static_cast<int>((LONGJUMP_OPTIMAL_SPEED - current_speed) * 450.f);
			result.confidence = std::min<float>(1.0f, current_speed / LONGJUMP_OPTIMAL_SPEED);
		}
		else {
			result.optimal_speed = current_speed * 1.1f;
			result.jump_height = 56.f; // Regular jump height
			result.use_duck = false;
			result.required_airtime = 0.3f;
			result.predicted_gain = static_cast<int>((result.optimal_speed - current_speed) * 0.3f);
			result.confidence = 0.6f;
		}

		// Predict landing position
		vec3_t jump_velocity = player_vel;
		jump_velocity.z = sqrtf(2.0f * get_gravity() * result.jump_height);

		landing_data_t landing = predict_landing(cmd, player_pos, jump_velocity,
			ticks_to_time(result.required_airtime * 2.0f));

		if (landing.valid) {
			result.landing_position = landing.position;
			result.position = player_pos;
			result.target_velocity = landing.velocity;
			result.confidence *= 0.8f + (0.2f * (1.0f - std::min<float>(1.0f, landing.distance / 1000.f)));
		}

		return result;
	}

	// Strafe assist for landing hard jumps
	void assist_landing_strafe(c_usercmd* cmd, const landing_data_t& landing) {
		if (!landing.valid || !landing.requires_strafe)
			return;

		const vec3_t player_pos = g::local->origin();
		vec3_t player_vel = g::local->velocity();
		const float current_speed = player_vel.length_2d();

		if (current_speed < MIN_BHOP_SPEED)
			return;

		// Calculate direction to landing
		vec3_t to_landing = landing.position - player_pos;
		to_landing.z = 0.f;
		float dist_to_landing = to_landing.length();

		if (dist_to_landing < 1.f)
			return;

		to_landing.normalize();

		// Current velocity direction
		vec3_t vel_dir = player_vel.normalized();
		vel_dir.z = 0.f;
		vel_dir.normalize();

		// Calculate alignment
		float alignment = vel_dir.dot(to_landing);

		// If not aligned, apply strafe correction
		if (alignment < 0.95f) {
			vec3_t desired_angle = to_landing.to_angle();
			vec3_t current_angle = vel_dir.to_angle();

			float angle_diff = math::angle_diff(desired_angle.y, current_angle.y);

			// Smooth correction
			float correction = angle_diff * STRAFE_CORRECTION_STRENGTH;

			// Apply to view angles for strafe
			vec3_t target_view = cmd->view_angles;
			target_view.y += correction;
			target_view.normalize();

			// Calculate strafe inputs
			float rotation = math::deg2rad(target_view.y - cmd->view_angles.y);
			float cos_rot = cosf(rotation);
			float sin_rot = sinf(rotation);

			// Apply movement with smooth transition
			float move_scale = 450.f * (1.0f - alignment);
			cmd->forward_move += cos_rot * move_scale * 450.f;
			cmd->side_move += -sin_rot * move_scale * 450.f;

			strafe_correction_active = true;
			optimal_strafe_delta = correction;
		}
	}

	// Auto-detect and execute longjumps
	void auto_longjump(c_usercmd* cmd) {
		const int flags = g::local->flags();
		const vec3_t player_vel = g::local->velocity();
		const float current_speed = player_vel.length_2d();

		// Check if on ground and moving fast enough
		if (!(flags & fl_onground)) {
			// In air - maintain longjump duck
			if (longjump_queued && longjump_ticks > 0) {
				//cmd->buttons |= in_duck;
				longjump_ticks--;

				if (longjump_ticks <= 0) {
					longjump_queued = false;
				}
			}
			return;
		}

		// On ground - check for longjump opportunity
		if (current_speed >= LONGJUMP_MIN_SPEED) {
			jump_opportunity_t opportunity = calculate_jump_opportunity(cmd);

			if (opportunity.use_longjump && opportunity.confidence > 0.6f) {
				// Execute longjump
				//cmd->buttons |= in_jump;
				//cmd->buttons |= in_duck;

				longjump_queued = true;
				longjump_ticks = LONGJUMP_DUCK_TICKS;
				last_jump_speed = current_speed;
			}
		}
	}

	// Perfect bhop with auto-jump
	void auto_bhop(c_usercmd* cmd) {
		const int flags = g::local->flags();
		const vec3_t player_vel = g::local->velocity();
		const float current_speed = player_vel.length_2d();

		if (menu::checkkey(c::movement::jump_bug_key, c::movement::jump_bug_key_s)) return;

		// Track ground/air state
		if (flags & fl_onground) {
			ground_ticks++;
			air_ticks = 0;

			// Auto-jump on ground (perfect bhop)
			if (cmd->buttons & in_jump && ground_ticks >= 1 && current_speed > MIN_BHOP_SPEED) {
				cmd->buttons |= in_jump;
				ground_ticks = 0;
				bhop_active = true;
				last_ground_pos = g::local->origin();
			}
		}
		else {
			air_ticks++;
			ground_ticks = 0;

			// Don't hold jump in air
			cmd->buttons &= ~in_jump;
		}
	}

	// Main execution function
	void run(c_usercmd* cmd) {
		if (!g::local || !c::movement::fast_bhop)
			return;

		if (should_skip_processing())
			return;

		// Don't interfere with other systems
		if (PixelSurfAssistant.ps_data.pixelsurfing)
			return;

		// Predict landing
		predicted_landing = predict_landing_advanced(cmd);

		// Auto-bhop
		auto_bhop(cmd);

		// Auto-longjump when beneficial
		auto_longjump(cmd);

		// Assist landing for hard jumps
		if (predicted_landing.valid && predicted_landing.requires_strafe) {
			assist_landing_strafe(cmd, predicted_landing);
		}

		// Store landing in history
		if (predicted_landing.valid && landing_history.size() < 100) {
			landing_history.push_back(predicted_landing);
		}
	}

	// Visualization for debugging
	void draw_predictions() {
		if (!g::local || !g::local->is_alive())
			return;

		// Draw predicted landing
		if (predicted_landing.valid) {
			vec3_t screen_pos;
			if (interfaces::debug_overlay->world_to_screen(predicted_landing.position, screen_pos)) {
				color_t landing_color = predicted_landing.is_longjump_spot ?
					color_t(255, 0, 255, 200) : color_t(0, 255, 0, 200);

				im_render.drawcircle(screen_pos.x, screen_pos.y, 8.f, 32, landing_color, 2.f);

				// Draw distance and time
				char info[128];
				snprintf(info, sizeof(info), "%.0f units | %.2fs",
					predicted_landing.distance, predicted_landing.time_to_land);

				im_render.text(screen_pos.x, screen_pos.y - 20.f, 12,
					fonts::esp_name_font, info, true,
					color_t(255, 255, 255, 255), true);
			}

			// Draw trajectory line
			vec3_t player_screen, landing_screen;
			if (interfaces::debug_overlay->world_to_screen(g::local->origin(), player_screen) &&
				interfaces::debug_overlay->world_to_screen(predicted_landing.position, landing_screen)) {
				im_render.drawline(player_screen.x, player_screen.y,
					landing_screen.x, landing_screen.y,
					color_t(255, 255, 0, 150), 2.f);
			}
		}

		// Draw strafe correction indicator
		if (strafe_correction_active) {
			const float screen_center_x = ImGui::GetIO().DisplaySize.x * 0.5f;
			const float screen_center_y = ImGui::GetIO().DisplaySize.y * 0.5f;

			float indicator_offset = optimal_strafe_delta * 5.f;
			im_render.drawline(screen_center_x, screen_center_y,
				screen_center_x + indicator_offset, screen_center_y,
				color_t(255, 255, 0, 255), 3.f);
		}
	}

	// Getters for state
	bool is_bhop_active() const { return bhop_active; }
	bool is_longjump_queued() const { return longjump_queued; }
	bool is_strafe_assisting() const { return strafe_correction_active; }
	const landing_data_t& get_predicted_landing() const { return predicted_landing; }

	void reset() {
		bhop_active = false;
		longjump_queued = false;
		longjump_ticks = 0;
		air_ticks = 0;
		ground_ticks = 0;
		strafe_correction_active = false;
		last_prediction_tick = -1;
		landing_history.clear();
	}
};
inline AdvancedBhopAssist Advanced_BhopAssistant;

class BetaAssist {
private:


public:

	void pre(c_usercmd* cmd) {
		//this->auto_bug(cmd);
	}

	void run(c_usercmd* cmd) {
		//this->auto_bug(cmd);
	}

};
inline BetaAssist BetaAssistant;

class MovementHook {
public:

	void pre(c_usercmd* cmd) {
		BhopAssistant.run(cmd);
		Advanced_BhopAssistant.run(cmd);
		JumpTypeAssistant.pre(cmd);
		StrafeAssistant.pre(cmd);
		AlignmentAssistant.pre(cmd);
		EdgebugAssistant.pre(cmd);
		JumpbugAssistant.pre(cmd);
		AirStuckAssistant.pre(cmd);
		PixelSurfAssistant.pre(cmd);
		EdgebugAssistant.pre(cmd);
		RouteStudio.pre(cmd);
		UtilityAssistant.pre(cmd);
		CollisionAssistant.pre(cmd);
		BetaAssistant.pre(cmd);
	}

	void post(c_usercmd* cmd) {
		AlignmentAssistant.run(cmd);
		JumpTypeAssistant.run(cmd);
		JumpbugAssistant.run(cmd);
		FiremanAssistant.run(cmd);
		CollisionAssistant.run(cmd);
		AirStuckAssistant.run(cmd);
		PixelSurfAssistant.run(cmd);
		EdgebugAssistant.run(cmd);
		RouteStudio.run(cmd);
		BetaAssistant.run(cmd);
		UtilityAssistant.run(cmd);
		recorder->create_move(cmd);
	}
};
inline MovementHook movement_hook_obj;
inline MovementHook* MovementHooks = &movement_hook_obj;
