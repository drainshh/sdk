#include "config.hpp"
#include <urlmon.h>    
#include <filesystem>
#include <fstream>
#include <cstring>
#include <type_traits>
#include "../menu.hpp"
#include "../../utils/xor.hpp"
#include "../features/movement/moveutil.h"
#pragma comment(lib, "Urlmon")

static std::string path = "C:/dna/config/";

template <typename T>
static void read_value(const nlohmann::json& src, T& dest) {
	if (src.is_null()) return;

	dest = src.get<T>();
}

template <size_t N>
static void read_value(const nlohmann::json& src, char(&dest)[N]) {
	if (src.is_null()) return;

	std::string s = src.get<std::string>();
	strncpy(dest, s.c_str(), N);
	dest[N - 1] = '\0';
}

template <typename T, size_t N>
static std::enable_if_t<!std::is_same_v<T, char>, void> read_value(const nlohmann::json& src, T(&dest)[N]) {
	if (src.is_null() || !src.is_array())
		return;

	const size_t count = (std::min)(src.size(), static_cast<size_t>(N));
	for (size_t i = 0; i < count; ++i)
		dest[i] = src[i].get<T>();
}

template <typename T>
static T read_value(const nlohmann::json& src) {
	if (src.is_null()) return T{};

	return src.get<T>();
}

template <typename T>
static void write_value(nlohmann::json& dest, const T& src) {
	dest = src;
}

template <size_t N>
static void write_value(nlohmann::json& dest, const char(&src)[N]) {
	dest = std::string(src);
}

template <typename T, size_t N>
static std::enable_if_t<!std::is_same_v<T, char>, void> write_value(nlohmann::json& dest, const T(&src)[N]) {
	dest = nlohmann::json::array();
	for (size_t i = 0; i < N; ++i)
		dest.push_back(src[i]);
}

namespace c {
	void create_directory() {
		if (std::filesystem::exists(directory) || std::filesystem::exists(directory_path) || std::filesystem::exists(directory_path_lua))
			return;

		directory = "C:/dna";
		if (!std::filesystem::exists(directory))
			std::filesystem::create_directory(directory);

		directory_path = "C:/dna/config";
		if (!std::filesystem::exists(directory_path))
			std::filesystem::create_directory(directory_path);
		else
			update_configs();

		directory_path_convar = "C:/dna/convars";
		if (!std::filesystem::exists(directory_path_convar))
			std::filesystem::create_directory(directory_path_convar);

		directory_path_lua = "C:/dna/lua";
		if (!std::filesystem::exists(directory_path_lua))
			std::filesystem::create_directory(directory_path_lua);

	}

	void update_configs() {
		configs.clear();

		for (const auto& entry : std::filesystem::directory_iterator{ directory_path }) {
			if (!entry.is_regular_file() || entry.path().extension() != (".json")) continue;

			const auto& file_name = entry.path().stem().string();
			configs.emplace_back(file_name);
		}
	}

	void create_file(const std::string_view name) {
		if (name.empty() || name.find_first_not_of(' ') == std::string_view::npos) return;
		if (std::find(configs.cbegin(), configs.cend(), name) != configs.cend()) return;

		configs.emplace_back(name);
		save(configs.size() - 1);
	}

	void delete_file(const std::size_t index) {
		if (configs.size() <= index) return;

		path.append(configs.at(index));
		path.append((".json"));
		std::filesystem::remove(path);
		path.erase(path.size() - configs.at(index).size() - 5);

		configs.erase(configs.cbegin() + index);
		configs.shrink_to_fit();
	}

	void save(const std::size_t index) {
		nlohmann::json json;

		write_value(json[xs("menu")][xs("fade_speed")], menu::fade_speed);
		write_value(json[xs("menu")][xs("menu_accent[0]")], menu::menu_accent[0]);
		write_value(json[xs("menu")][xs("menu_accent[1]")], menu::menu_accent[1]);
		write_value(json[xs("menu")][xs("menu_accent[2]")], menu::menu_accent[2]);
		write_value(json[xs("menu")][xs("menu_accent[3]")], menu::menu_accent[3]);
		write_value(json[xs("skins")][xs("skin_custom_clr")], skins::skin_custom_clr);
		write_value(json[("skins")][("skin_modulation1[0]")], skins::skin_modulation1[0]);
		write_value(json[("skins")][("skin_modulation1[1]")], skins::skin_modulation1[1]);
		write_value(json[("skins")][("skin_modulation1[2]")], skins::skin_modulation1[2]);
		write_value(json[("skins")][("skin_modulation2[0]")], skins::skin_modulation2[0]);
		write_value(json[("skins")][("skin_modulation2[1]")], skins::skin_modulation2[1]);
		write_value(json[("skins")][("skin_modulation2[2]")], skins::skin_modulation2[2]);
		write_value(json[("skins")][("skin_modulation3[0]")], skins::skin_modulation3[0]);
		write_value(json[("skins")][("skin_modulation3[1]")], skins::skin_modulation3[1]);
		write_value(json[("skins")][("skin_modulation3[2]")], skins::skin_modulation3[2]);
		write_value(json[("skins")][("skin_modulation4[0]")], skins::skin_modulation4[0]);
		write_value(json[("skins")][("skin_modulation4[1]")], skins::skin_modulation4[1]);
		write_value(json[("skins")][("skin_modulation4[2]")], skins::skin_modulation4[2]);
		write_value(json[xs("skins")][xs("agent_changer")], skins::agent_changer);
		write_value(json[xs("skins")][xs("agent_t")], skins::agent_t);
		write_value(json[xs("skins")][xs("agent_ct")], skins::agent_ct);
		write_value(json[xs("skins")][xs("knife_changer_enable")], skins::knife_changer_enable);
		write_value(json[xs("skins")][xs("knife_changer_wear")], skins::knife_changer_wear);
		write_value(json[xs("skins")][xs("knife_changer_model")], skins::knife_changer_model);
		write_value(json[xs("skins")][xs("knife_changer_vector_paint_kit")], skins::knife_changer_vector_paint_kit);
		write_value(json[xs("skins")][xs("knife_changer_paint_kit")], skins::knife_changer_paint_kit);
		write_value(json[xs("skins")][xs("gloves_endable")], skins::gloves_endable);
		write_value(json[xs("skins")][xs("gloves_model")], skins::gloves_model);
		write_value(json[xs("skins")][xs("gloves_skin")], skins::gloves_skin);
		write_value(json[xs("skins")][xs("gloves_skin_id")], skins::gloves_skin_id);
		write_value(json[xs("skins")][xs("gloves_wear")], skins::gloves_wear);
		write_value(json[xs("skins")][xs("weapon_endable")], skins::weapon_endable);
		write_value(json[xs("movement")][xs("bhop")], movement::bhop);
		write_value(json[xs("movement")][xs("auto_strafe")], movement::auto_strafe);
		write_value(json[xs("movement")][xs("auto_strafe_key")], movement::auto_strafe_key);
		write_value(json[xs("movement")][xs("auto_strafe_key_s")], movement::auto_strafe_key_s);
		write_value(json[xs("movement")][xs("strafe_optimizer")], movement::strafe_optimizer);
		write_value(json[xs("movement")][xs("strafe_optimizer_key")], movement::strafe_optimizer_key);
		write_value(json[xs("movement")][xs("strafe_optimizer_key_s")], movement::strafe_optimizer_key_s);
		write_value(json[xs("movement")][xs("strafe_optimizer_required_speed")], movement::strafe_optimizer_required_speed);
		write_value(json[xs("movement")][xs("strafe_optimizer_desired_gain")], movement::strafe_optimizer_desired_gain);
		write_value(json[xs("movement")][xs("mouse_strafe_limiter")], movement::mouse_strafe_limiter);
		write_value(json[xs("movement")][xs("mouse_strafe_limiter_affect_pitch")], movement::mouse_strafe_limiter_affect_pitch);
		write_value(json[xs("movement")][xs("mouse_strafe_limiter_key")], movement::mouse_strafe_limiter_key);
		write_value(json[xs("movement")][xs("mouse_strafe_limiter_key_s")], movement::mouse_strafe_limiter_key_s);
		write_value(json[xs("movement")][xs("mouse_strafe_limiter_value")], movement::mouse_strafe_limiter_value);
		write_value(json[xs("movement")][xs("edgejump")], movement::edgejump);
		write_value(json[xs("movement")][xs("edge_jump_on_ladder")], movement::edge_jump_on_ladder);
		write_value(json[xs("movement")][xs("ladder_bug")], movement::ladder_bug);
		write_value(json[xs("movement")][xs("ladder_bug_detection_printf")], movement::ladder_bug_detection_printf);
		write_value(json[xs("movement")][xs("ladder_bug_key")], movement::ladder_bug_key);
		write_value(json[xs("movement")][xs("ladder_bug_key_s")], movement::ladder_bug_key_s);
		write_value(json[xs("movement")][xs("edgejump_key")], movement::edgejump_key);
		write_value(json[xs("movement")][xs("edgejump_key_s")], movement::edgejump_key_s);
		write_value(json[xs("movement")][xs("long_jump_on_edge")], movement::long_jump_on_edge);
		write_value(json[xs("movement")][xs("mini_jump")], movement::mini_jump);
		write_value(json[xs("movement")][xs("mini_jump_key")], movement::mini_jump_key);
		write_value(json[xs("movement")][xs("jump_bug")], movement::jump_bug);
		write_value(json[xs("movement")][xs("jump_bug_key")], movement::jump_bug_key);
		write_value(json[xs("movement")][xs("crouch_bug")], movement::crouch_bug);
		write_value(json[xs("movement")][xs("crouch_bug_key")], movement::crouch_bug_key);
		write_value(json[xs("movement")][xs("crouch_bug_key_s")], movement::crouch_bug_key_s);
		write_value(json[xs("movement")][xs("edge_bug_priority")], movement::edge_bug_priority);
		write_value(json[xs("movement")][xs("edge_bug")], movement::edge_bug);
		write_value(json[xs("movement")][xs("visualize_edge_bug")], movement::visualize_edge_bug);
		write_value(json[xs("movement")][xs("edge_bug_advanced_search")], movement::edge_bug_advanced_search);
		write_value(json[xs("movement")][xs("auto_duck")], movement::auto_duck);
		write_value(json[xs("movement")][xs("auto_duck_key")], movement::auto_duck_key);
		write_value(json[xs("movement")][xs("edge_bug_key")], movement::edge_bug_key);
		write_value(json[xs("movement")][xs("edge_bug_ticks")], movement::edge_bug_ticks);
		write_value(json[xs("movement")][xs("edge_bug_angle_limit")], movement::edge_bug_angle_limit);
		write_value(json[xs("movement")][xs("edge_bug_rape")], movement::edge_bug_rape);
		write_value(json[xs("movement")][xs("edge_bug_strafe")], movement::edge_bug_strafe);
		write_value(json[xs("movement")][xs("delay_hop")], movement::delay_hop);
		write_value(json[xs("movement")][xs("delay_hop_key")], movement::delay_hop_key);
		write_value(json[xs("movement")][xs("stamina_hop_ticks")], movement::dh_tick);
		write_value(json[xs("movement")][xs("fastduck")], movement::fastduck);
		write_value(json[xs("movehelp")][xs("fake_duck")], movehelp::fake_duck);
		write_value(json[xs("movement")][xs("edge_bug_lock_amount")], movement::edge_bug_lock_amount);
		write_value(json[xs("movement")][xs("edge_bug_detection_printf")], movement::edge_bug_detection_printf);
		write_value(json[xs("movement")][xs("jump_bug_detection_printf")], movement::jump_bug_detection_printf);
		write_value(json[xs("movement")][xs("long_jump_detection_printf")], movement::long_jump_detection_printf);
		write_value(json[xs("movement")][xs("mini_jump_detection_printf")], movement::mini_jump_detection_printf);
		write_value(json[xs("movement")][xs("edge_bug_detection_sound")], movement::edge_bug_detection_sound);
		write_value(json[xs("movement")][xs("edge_bug_health_boost_effect")], movement::edge_bug_health_boost_effect);
		write_value(json[xs("movement")][xs("velocity_indicator")], movement::velocity_indicator);
		write_value(json[xs("movement")][xs("velocity_indicator_position")], movement::velocity_indicator_position);
		write_value(json[xs("movement")][xs("velocity_indicator_show_pre")], movement::velocity_indicator_show_pre);
		write_value(json[xs("movement")][xs("velocity_indicator_custom_color")], movement::velocity_indicator_custom_color);
		write_value(json[xs("movement")][xs("velocity_indicator_negative_clr[0]")], movement::velocity_indicator_negative_clr[0]);
		write_value(json[xs("movement")][xs("velocity_indicator_negative_clr[1]")], movement::velocity_indicator_negative_clr[1]);
		write_value(json[xs("movement")][xs("velocity_indicator_negative_clr[2]")], movement::velocity_indicator_negative_clr[2]);
		write_value(json[xs("movement")][xs("velocity_indicator_neutral_clr[0]")], movement::velocity_indicator_neutral_clr[0]);
		write_value(json[xs("movement")][xs("velocity_indicator_neutral_clr[1]")], movement::velocity_indicator_neutral_clr[1]);
		write_value(json[xs("movement")][xs("velocity_indicator_neutral_clr[2]")], movement::velocity_indicator_neutral_clr[2]);
		write_value(json[xs("movement")][xs("velocity_indicator_positive_clr[0]")], movement::velocity_indicator_positive_clr[0]);
		write_value(json[xs("movement")][xs("velocity_indicator_positive_clr[1]")], movement::velocity_indicator_positive_clr[1]);
		write_value(json[xs("movement")][xs("velocity_indicator_positive_clr[2]")], movement::velocity_indicator_positive_clr[2]);
		write_value(json[xs("movement")][xs("velocity_indicator_custom_clr[0]")], movement::velocity_indicator_custom_clr[0]);
		write_value(json[xs("movement")][xs("velocity_indicator_custom_clr[1]")], movement::velocity_indicator_custom_clr[1]);
		write_value(json[xs("movement")][xs("velocity_indicator_custom_clr[2]")], movement::velocity_indicator_custom_clr[2]);
		write_value(json[xs("movement")][xs("velocity_indicator_custom_clr[3]")], movement::velocity_indicator_custom_clr[3]);
		write_value(json[xs("movement")][xs("shadow_indicator_custom_clr[0]")], movement::shadow_indicator_custom_clr[0]);
		write_value(json[xs("movement")][xs("shadow_indicator_custom_clr[1]")], movement::shadow_indicator_custom_clr[1]);
		write_value(json[xs("movement")][xs("shadow_indicator_custom_clr[2]")], movement::shadow_indicator_custom_clr[2]);
		write_value(json[xs("movement")][xs("shadow_indicator_custom_clr[3]")], movement::shadow_indicator_custom_clr[3]);
		write_value(json[xs("movement")][xs("shadow_indicator_custom_clr2[0]")], movement::shadow_indicator_custom_clr2[0]);
		write_value(json[xs("movement")][xs("shadow_indicator_custom_clr2[1]")], movement::shadow_indicator_custom_clr2[1]);
		write_value(json[xs("movement")][xs("shadow_indicator_custom_clr2[2]")], movement::shadow_indicator_custom_clr2[2]);
		write_value(json[xs("movement")][xs("shadow_indicator_custom_clr2[3]")], movement::shadow_indicator_custom_clr2[3]);
		write_value(json[xs("movement")][xs("velocity_indicator_custom_clr2[0]")], movement::velocity_indicator_custom_clr2[0]);
		write_value(json[xs("movement")][xs("velocity_indicator_custom_clr2[1]")], movement::velocity_indicator_custom_clr2[1]);
		write_value(json[xs("movement")][xs("velocity_indicator_custom_clr2[2]")], movement::velocity_indicator_custom_clr2[2]);
		write_value(json[xs("movement")][xs("velocity_indicator_custom_clr2[3]")], movement::velocity_indicator_custom_clr2[3]);
		write_value(json[xs("movement")][xs("velocity_indicator_fade_clr3[0]")], movement::velocity_indicator_fade_clr3[0]);
		write_value(json[xs("movement")][xs("velocity_indicator_fade_clr3[1]")], movement::velocity_indicator_fade_clr3[1]);
		write_value(json[xs("movement")][xs("velocity_indicator_fade_clr3[2]")], movement::velocity_indicator_fade_clr3[2]);
		write_value(json[xs("movement")][xs("velocity_indicator_fade_clr3[3]")], movement::velocity_indicator_fade_clr3[3]);
		write_value(json[xs("movement")][xs("indicators_show[0]")], movement::indicators_show[0]);
		write_value(json[xs("movement")][xs("indicators_show[1]")], movement::indicators_show[1]);
		write_value(json[xs("movement")][xs("indicators_show[2]")], movement::indicators_show[2]);
		write_value(json[xs("movement")][xs("indicators_show[3]")], movement::indicators_show[3]);
		write_value(json[xs("movement")][xs("indicators_show[4]")], movement::indicators_show[4]);
		write_value(json[xs("movement")][xs("indicators_show[5]")], movement::indicators_show[5]);
		write_value(json[xs("movement")][xs("indicators_show[6]")], movement::indicators_show[6]);
		write_value(json[xs("movement")][xs("indicators_show[7]")], movement::indicators_show[7]);
		write_value(json[xs("movement")][xs("detection_clr_for[0]")], movement::detection_clr_for[0]);
		write_value(json[xs("movement")][xs("detection_clr_for[1]")], movement::detection_clr_for[1]);
		write_value(json[xs("movement")][xs("detection_clr_for[2]")], movement::detection_clr_for[2]);
		write_value(json[xs("movement")][xs("detection_clr_for[3]")], movement::detection_clr_for[3]);
		write_value(json[xs("movement")][xs("detection_clr_for[4]")], movement::detection_clr_for[4]);
		write_value(json[xs("movement")][xs("detection_clr_for[5]")], movement::detection_clr_for[5]);
		write_value(json[xs("movement")][xs("detection_clr_for[6]")], movement::detection_clr_for[6]);
		write_value(json[xs("movement")][xs("detection_clr_for[7]")], movement::detection_clr_for[7]);
		write_value(json[xs("movement")][xs("allow_detection_clr")], movement::allow_detection_clr);
		write_value(json[xs("movement")][xs("detection_saved_tick")], movement::detection_saved_tick);
		write_value(json[xs("movement")][xs("indicators_allow_animation")], movement::indicators_allow_animation);
		write_value(json[xs("movement")][xs("indicators_allow_move")], movement::indicators_allow_move);
		write_value(json[xs("movement")][xs("indicators_move_pos")], movement::indicators_move_pos);
		write_value(json[xs("movement")][xs("stamina_indicator")], movement::stamina_indicator);
		write_value(json[xs("movement")][xs("stamina_indicator_fade")], movement::stamina_indicator_fade);
		write_value(json[xs("movement")][xs("stamina_indicator_show_pre")], movement::stamina_indicator_show_pre);
		write_value(json[xs("movement")][xs("stamina_indicator_disable_ong_show_pre")], movement::stamina_indicator_disable_ong_show_pre);
		write_value(json[xs("movement")][xs("stamina_indicator_position")], movement::stamina_indicator_position);
		write_value(json[xs("movement")][xs("stamina_indicator_clr[0]")], movement::stamina_indicator_clr[0]);
		write_value(json[xs("movement")][xs("stamina_indicator_clr[1]")], movement::stamina_indicator_clr[1]);
		write_value(json[xs("movement")][xs("stamina_indicator_clr[2]")], movement::stamina_indicator_clr[2]);
		write_value(json[xs("movement")][xs("stamina_indicator_clr[3]")], movement::stamina_indicator_clr[3]);
		write_value(json[xs("movement")][xs("stamina_fade_clr[0]")], movement::stamina_fade_clr[0]);
		write_value(json[xs("movement")][xs("stamina_fade_clr[1]")], movement::stamina_fade_clr[1]);
		write_value(json[xs("movement")][xs("stamina_fade_clr[2]")], movement::stamina_fade_clr[2]);
		write_value(json[xs("movement")][xs("stamina_fade_clr[3]")], movement::stamina_fade_clr[3]);
		write_value(json[xs("movement")][xs("indicator_detect_clr[0]")], movement::indicator_detect_clr[0]);
		write_value(json[xs("movement")][xs("indicator_detect_clr[1]")], movement::indicator_detect_clr[1]);
		write_value(json[xs("movement")][xs("indicator_detect_clr[2]")], movement::indicator_detect_clr[2]);
		write_value(json[xs("movement")][xs("indicator_detect_clr[3]")], movement::indicator_detect_clr[3]);
		write_value(json[xs("movement")][xs("velocity_graph")], movement::velocity_graph);
		write_value(json[xs("movement")][xs("velocity_graph_color[0]")], movement::velocity_graph_color[0]);
		write_value(json[xs("movement")][xs("velocity_graph_color[1]")], movement::velocity_graph_color[1]);
		write_value(json[xs("movement")][xs("velocity_graph_color[2]")], movement::velocity_graph_color[2]);
		write_value(json[xs("movement")][xs("velocity_linewidth")], movement::velocity_linewidth);
		write_value(json[xs("movement")][xs("velocity_height")], movement::velocity_height);
		write_value(json[xs("movement")][xs("velocity_size")], movement::velocity_size);
		write_value(json[xs("movement")][xs("graph_xscreenpos")], movement::graph_xscreenpos);
		write_value(json[xs("movement")][xs("velocity_graph_show_landed_speed")], movement::velocity_graph_show_landed_speed);
		write_value(json[xs("movement")][xs("velocity_graph_show_edge_bug")], movement::velocity_graph_show_edge_bug);
		write_value(json[xs("movement")][xs("velocity_graph_show_jump_bug")], movement::velocity_graph_show_jump_bug);
		write_value(json[xs("movement")][xs("stamina_graph")], movement::stamina_graph);
		write_value(json[xs("movement")][xs("stamina_graph_color[0]")], movement::stamina_graph_color[0]);
		write_value(json[xs("movement")][xs("stamina_graph_color[1]")], movement::stamina_graph_color[1]);
		write_value(json[xs("movement")][xs("stamina_graph_color[2]")], movement::stamina_graph_color[2]);
		write_value(json[xs("movement")][xs("stamina_graph_offset")], movement::stamina_graph_offset);
		write_value(json[xs("movement")][xs("stamina_graph_height")], movement::stamina_graph_height);
		write_value(json[xs("movement")][xs("stamina_graph_offset_x")], movement::stamina_graph_offset_x);
		write_value(json[xs("movement")][xs("graph_fade")], movement::graph_fade);
		write_value(json[xs("movement")][xs("key_strokes")], movement::key_strokes);
		write_value(json[xs("movement")][xs("key_strokes_position")], movement::key_strokes_position);
		write_value(json[xs("misc")][xs("watermark")], misc::watermark);
		write_value(json[xs("misc")][xs("watermark_shadowtext_clr[0]")], misc::watermark_shadowtext_clr[0]);
		write_value(json[xs("misc")][xs("watermark_shadowtext_clr[1]")], misc::watermark_shadowtext_clr[1]);
		write_value(json[xs("misc")][xs("watermark_shadowtext_clr[2]")], misc::watermark_shadowtext_clr[2]);
		write_value(json[xs("misc")][xs("watermark_shadowtext_clr[3]")], misc::watermark_shadowtext_clr[3]);
		write_value(json[xs("misc")][xs("unlock_inventory")], misc::unlock_inventory);
		write_value(json[xs("misc")][xs("show_spotify_currently_playing")], misc::show_spotify_currently_playing);
		write_value(json[xs("misc")][xs("movement_rec")], misc::movement_rec);
		write_value(json[xs("misc")][xs("movement_rec_smoothing")], misc::movement_rec_smoothing);
		write_value(json[xs("misc")][xs("movement_rec_lockva")], misc::movement_rec_lockva);
		write_value(json[xs("misc")][xs("movement_rec_ringsize")], misc::movement_rec_ringsize);
		write_value(json[xs("misc")][xs("movement_rec_keystartrecord")], misc::movement_rec_keystartrecord);
		write_value(json[xs("misc")][xs("movement_rec_keystoprecord")], misc::movement_rec_keystoprecord);
		write_value(json[xs("misc")][xs("movement_rec_keystartplayback")], misc::movement_rec_keystartplayback);
		write_value(json[xs("misc")][xs("movement_rec_keystopplayback")], misc::movement_rec_keystopplayback);
		write_value(json[xs("misc")][xs("movement_rec_keyclearrecord")], misc::movement_rec_keyclearrecord);
		write_value(json[xs("misc")][xs("custom_console")], misc::custom_console);
		write_value(json[xs("misc")][xs("custom_console_clr[0]")], misc::custom_console_clr[0]);
		write_value(json[xs("misc")][xs("custom_console_clr[1]")], misc::custom_console_clr[1]);
		write_value(json[xs("misc")][xs("custom_console_clr[2]")], misc::custom_console_clr[2]);
		write_value(json[xs("misc")][xs("custom_console_clr[3]")], misc::custom_console_clr[3]);
		write_value(json[xs("misc")][xs("custom_console_clr2[0]")], misc::custom_console_clr2[0]);
		write_value(json[xs("misc")][xs("custom_console_clr2[1]")], misc::custom_console_clr2[1]);
		write_value(json[xs("misc")][xs("custom_console_clr2[2]")], misc::custom_console_clr2[2]);
		write_value(json[xs("misc")][xs("custom_console_clr2[3]")], misc::custom_console_clr2[3]);
		write_value(json[xs("misc")][xs("spectators_list")], misc::spectators_list);
		write_value(json[xs("misc")][xs("spectatorlist_type")], misc::spectatorlist_type);
		write_value(json[xs("misc")][xs("spectatorlist_show_target")], misc::spectatorlist_show_target);
		write_value(json[xs("misc")][xs("spectators_list_color_1[0]")], misc::spectators_list_color_1[0]);
		write_value(json[xs("misc")][xs("spectators_list_color_1[1]")], misc::spectators_list_color_1[1]);
		write_value(json[xs("misc")][xs("spectators_list_color_1[2]")], misc::spectators_list_color_1[2]);
		write_value(json[xs("misc")][xs("spectators_list_color_2[0]")], misc::spectators_list_color_2[0]);
		write_value(json[xs("misc")][xs("spectators_list_color_2[1]")], misc::spectators_list_color_2[1]);
		write_value(json[xs("misc")][xs("spectators_list_color_2[2]")], misc::spectators_list_color_2[2]);
		write_value(json[xs("misc")][xs("spectators_list_clr[0]")], misc::spectators_list_clr[0]);
		write_value(json[xs("misc")][xs("spectators_list_clr[1]")], misc::spectators_list_clr[1]);
		write_value(json[xs("misc")][xs("spectators_list_clr[2]")], misc::spectators_list_clr[2]);
		write_value(json[xs("misc")][xs("spectators_list_clr_2[0]")], misc::spectators_list_clr_2[0]);
		write_value(json[xs("misc")][xs("spectators_list_clr_2[1]")], misc::spectators_list_clr_2[1]);
		write_value(json[xs("misc")][xs("spectators_list_clr_2[2]")], misc::spectators_list_clr_2[2]);
		write_value(json[xs("misc")][xs("field_of_view")], misc::field_of_view);
		write_value(json[xs("misc")][xs("enable_fov")], misc::enable_fov);
		write_value(json[xs("misc")][xs("view_model")], misc::view_model);
		write_value(json[xs("misc")][xs("view_model_x")], misc::view_model_x);
		write_value(json[xs("misc")][xs("view_model_y")], misc::view_model_y);
		write_value(json[xs("misc")][xs("view_model_z")], misc::view_model_z);
		write_value(json[xs("misc")][xs("aspect_ratio")], misc::aspect_ratio);
		write_value(json[xs("misc")][xs("thirdperson")], misc::thirdperson);
		write_value(json[xs("misc")][xs("thirdperson_key")], misc::thirdperson_key);
		write_value(json[xs("misc")][xs("thirdperson_distance")], misc::thirdperson_distance);
		write_value(json[xs("misc")][xs("aspect_ratio_amount")], misc::aspect_ratio_amount);
		write_value(json[xs("misc")][xs("practice")], misc::practice);
		write_value(json[xs("misc")][xs("practice_window")], misc::practice_window);
		write_value(json[xs("misc")][xs("savepos")], misc::savepos);
		write_value(json[xs("misc")][xs("loadpos")], misc::loadpos);
		write_value(json[xs("misc")][xs("nextkey")], misc::nextkey);
		write_value(json[xs("misc")][xs("prevkey")], misc::prevkey);
		write_value(json[xs("misc")][xs("undokey")], misc::undokey);
		write_value(json[xs("misc")][xs("discord_rpc")], misc::discord_rpc);
		write_value(json[xs("misc")][xs("mouse_fix")], misc::mouse_fix);
		write_value(json[xs("misc")][xs("jumpstats")], misc::jumpstats);
		write_value(json[xs("misc")][xs("jumpstats_show_clr_fail")], misc::jumpstats_show_clr_fail);
		write_value(json[xs("misc")][xs("jumpstats_show_fail")], misc::jumpstats_show_fail);
		write_value(json[xs("misc")][xs("misc_reveal_ranks")], misc::misc_reveal_ranks);
		write_value(json[xs("misc")][xs("misc_hitmarker_sound_type")], misc::misc_hitmarker_sound_type);
		write_value(json[xs("misc")][xs("freecam")], misc::freecam);
		write_value(json[xs("misc")][xs("freecam_key")], misc::freecam_key);
		write_value(json[xs("visuals")][xs("skeletonesp_clr[0]")], visuals::skeletonesp_clr[0]);
		write_value(json[xs("visuals")][xs("skeletonesp_clr[1]")], visuals::skeletonesp_clr[1]);
		write_value(json[xs("visuals")][xs("skeletonesp_clr[2]")], visuals::skeletonesp_clr[2]);
		write_value(json[xs("visuals")][xs("skeletonbt_clr[0]")], visuals::skeletonbt_clr[0]);
		write_value(json[xs("visuals")][xs("skeletonbt_clr[1]")], visuals::skeletonbt_clr[1]);
		write_value(json[xs("visuals")][xs("skeletonbt_clr[2]")], visuals::skeletonbt_clr[2]);
		write_value(json[xs("visuals")][xs("selected_tick_clr[0]")], visuals::selected_tick_clr[0]);
		write_value(json[xs("visuals")][xs("selected_tick_clr[1]")], visuals::selected_tick_clr[1]);
		write_value(json[xs("visuals")][xs("selected_tick_clr[2]")], visuals::selected_tick_clr[2]);
		write_value(json[xs("visuals")][xs("glow_visible_clr[0]")], visuals::glow_visible_clr[0]);
		write_value(json[xs("visuals")][xs("glow_visible_clr[1]")], visuals::glow_visible_clr[1]);
		write_value(json[xs("visuals")][xs("glow_visible_clr[2]")], visuals::glow_visible_clr[2]);
		write_value(json[xs("visuals")][xs("glow_visible_clr[3]")], visuals::glow_visible_clr[3]);
		write_value(json[xs("visuals")][xs("glow_invisible_clr[0]")], visuals::glow_invisible_clr[0]);
		write_value(json[xs("visuals")][xs("glow_invisible_clr[1]")], visuals::glow_invisible_clr[1]);
		write_value(json[xs("visuals")][xs("glow_invisible_clr[2]")], visuals::glow_invisible_clr[2]);
		write_value(json[xs("visuals")][xs("glow_invisible_clr[3]")], visuals::glow_invisible_clr[3]);
		write_value(json[xs("visuals")][xs("glow_weapon_clr[0]")], visuals::glow_weapon_clr[0]);
		write_value(json[xs("visuals")][xs("glow_weapon_clr[1]")], visuals::glow_weapon_clr[1]);
		write_value(json[xs("visuals")][xs("glow_weapon_clr[2]")], visuals::glow_weapon_clr[2]);
		write_value(json[xs("visuals")][xs("glow_weapon_clr[3]")], visuals::glow_weapon_clr[3]);
		write_value(json[xs("visuals")][xs("enable_visuals")], visuals::enable_visuals);
		write_value(json[xs("visuals")][xs("fade_animation")], visuals::fade_animation);
		write_value(json[xs("visuals")][xs("playerbox")], visuals::playerbox);
		write_value(json[xs("visuals")][xs("playerweapon")], visuals::playerweapon);
		write_value(json[xs("visuals")][xs("player_weapon_type[0]")], visuals::player_weapon_type[0]);
		write_value(json[xs("visuals")][xs("player_weapon_type[1]")], visuals::player_weapon_type[1]);
		write_value(json[xs("visuals")][xs("playerbox_color[0]")], visuals::playerbox_color[0]);
		write_value(json[xs("visuals")][xs("playerbox_color[1]")], visuals::playerbox_color[1]);
		write_value(json[xs("visuals")][xs("playerbox_color[2]")], visuals::playerbox_color[2]);
		write_value(json[xs("visuals")][xs("playername")], visuals::playername);
		write_value(json[xs("visuals")][xs("playername_color[0]")], visuals::playername_color[0]);
		write_value(json[xs("visuals")][xs("playername_color[1]")], visuals::playername_color[1]);
		write_value(json[xs("visuals")][xs("playername_color[2]")], visuals::playername_color[2]);
		write_value(json[xs("visuals")][xs("health_color[0]")], visuals::health_color[0]);
		write_value(json[xs("visuals")][xs("health_color[1]")], visuals::health_color[1]);
		write_value(json[xs("visuals")][xs("health_color[2]")], visuals::health_color[2]);
		write_value(json[xs("visuals")][xs("skeleton")], visuals::skeleton);
		write_value(json[xs("visuals")][xs("skeleton_last")], visuals::skeleton_last);
		write_value(json[xs("visuals")][xs("skeleton_last_clr[0]")], visuals::skeleton_last_clr[0]);
		write_value(json[xs("visuals")][xs("skeleton_last_clr[1]")], visuals::skeleton_last_clr[1]);
		write_value(json[xs("visuals")][xs("skeleton_last_clr[2]")], visuals::skeleton_last_clr[2]);
		write_value(json[xs("visuals")][xs("healthesp")], visuals::healthesp);
		write_value(json[xs("visuals")][xs("override_bar")], visuals::override_bar);
		write_value(json[xs("visuals")][xs("gradient_bar")], visuals::gradient_bar);
		write_value(json[xs("visuals")][xs("health_bar_gradient[0]")], visuals::health_bar_gradient[0]);
		write_value(json[xs("visuals")][xs("health_bar_gradient[1]")], visuals::health_bar_gradient[1]);
		write_value(json[xs("visuals")][xs("health_bar_gradient[2]")], visuals::health_bar_gradient[2]);
		write_value(json[xs("visuals")][xs("healthesp_s")], visuals::healthesp_s);
		write_value(json[xs("visuals")][xs("health_bar[0]")], visuals::health_bar[0]);
		write_value(json[xs("visuals")][xs("health_bar[1]")], visuals::health_bar[1]);
		write_value(json[xs("visuals")][xs("health_bar[2]")], visuals::health_bar[2]);
		write_value(json[xs("visuals")][xs("oof_arrows")], visuals::oof_arrows);
		write_value(json[xs("visuals")][xs("oof_arrows_clr[0]")], visuals::oof_arrows_clr[0]);
		write_value(json[xs("visuals")][xs("oof_arrows_clr[1]")], visuals::oof_arrows_clr[1]);
		write_value(json[xs("visuals")][xs("oof_arrows_clr[2]")], visuals::oof_arrows_clr[2]);
		write_value(json[xs("visuals")][xs("oof_arrows_clr[3]")], visuals::oof_arrows_clr[3]);
		write_value(json[xs("visuals")][xs("oof_arrows_clr2[0]")], visuals::oof_arrows_clr2[0]);
		write_value(json[xs("visuals")][xs("oof_arrows_clr2[1]")], visuals::oof_arrows_clr2[1]);
		write_value(json[xs("visuals")][xs("oof_arrows_clr2[2]")], visuals::oof_arrows_clr2[2]);
		write_value(json[xs("visuals")][xs("oof_arrows_clr2[3]")], visuals::oof_arrows_clr2[3]);
		write_value(json[xs("visuals")][xs("oof_arrows_dist")], visuals::oof_arrows_dist);
		write_value(json[xs("visuals")][xs("oof_arrows_size")], visuals::oof_arrows_size);
		write_value(json[xs("visuals")][xs("skeletonesp")], visuals::skeletonesp);
		write_value(json[xs("visuals")][xs("radar")], visuals::radar);
		write_value(json[xs("visuals")][xs("gravity_ragdoll")], visuals::gravity_ragdoll);
		write_value(json[xs("visuals")][xs("backtrack_chams")], chams::backtrack_chams);
		write_value(json[xs("visuals")][xs("backtrack_chams_draw_all_ticks")], chams::backtrack_chams_draw_all_ticks);
		write_value(json[xs("visuals")][xs("backtrack_chams_gradient")], chams::backtrack_chams_gradient);
		write_value(json[xs("visuals")][xs("backtrack_chams_clr1[0]")], chams::backtrack_chams_clr1[0]);
		write_value(json[xs("visuals")][xs("backtrack_chams_clr1[1]")], chams::backtrack_chams_clr1[1]);
		write_value(json[xs("visuals")][xs("backtrack_chams_clr1[2]")], chams::backtrack_chams_clr1[2]);
		write_value(json[xs("visuals")][xs("backtrack_chams_clr1[3]")], chams::backtrack_chams_clr1[3]);
		write_value(json[xs("visuals")][xs("backtrack_chams_clr2[0]")], chams::backtrack_chams_clr2[0]);
		write_value(json[xs("visuals")][xs("backtrack_chams_clr2[1]")], chams::backtrack_chams_clr2[1]);
		write_value(json[xs("visuals")][xs("backtrack_chams_clr2[2]")], chams::backtrack_chams_clr2[2]);
		write_value(json[xs("visuals")][xs("backtrack_chams_clr2[3]")], chams::backtrack_chams_clr2[3]);
		write_value(json[xs("visuals")][xs("cham_type")], chams::cham_type);
		write_value(json[xs("visuals")][xs("cham_type_bt")], chams::cham_type_bt);
		write_value(json[xs("visuals")][xs("cham_type_overlay")], chams::cham_type_overlay);
		write_value(json[xs("chams")][xs("visible_chams_clr_ov[0]")], chams::visible_chams_clr_ov[0]);
		write_value(json[xs("chams")][xs("visible_chams_clr_ov[1]")], chams::visible_chams_clr_ov[1]);
		write_value(json[xs("chams")][xs("visible_chams_clr_ov[2]")], chams::visible_chams_clr_ov[2]);
		write_value(json[xs("chams")][xs("visible_chams_clr_ov[3]")], chams::visible_chams_clr_ov[3]);
		write_value(json[xs("visuals")][xs("visible_chams_clr_ov[0]")], chams::visible_chams_clr_ov[0]);
		write_value(json[xs("visuals")][xs("visible_chams_clr_ov[1]")], chams::visible_chams_clr_ov[1]);
		write_value(json[xs("visuals")][xs("visible_chams_clr_ov[2]")], chams::visible_chams_clr_ov[2]);
		write_value(json[xs("visuals")][xs("visible_chams_clr_ov[3]")], chams::visible_chams_clr_ov[3]);
		write_value(json[xs("visuals")][xs("sleeve_chams_clr[0]")], chams::sleeve_chams_clr[0]);
		write_value(json[xs("visuals")][xs("sleeve_chams_clr[1]")], chams::sleeve_chams_clr[1]);
		write_value(json[xs("visuals")][xs("sleeve_chams_clr[2]")], chams::sleeve_chams_clr[2]);
		write_value(json[xs("visuals")][xs("sleeve_chams_clr[3]")], chams::sleeve_chams_clr[3]);
		write_value(json[xs("visuals")][xs("arms_chams_clr[0]")], chams::arms_chams_clr[0]);
		write_value(json[xs("visuals")][xs("arms_chams_clr[1]")], chams::arms_chams_clr[1]);
		write_value(json[xs("visuals")][xs("arms_chams_clr[2]")], chams::arms_chams_clr[2]);
		write_value(json[xs("visuals")][xs("arms_chams_clr[3]")], chams::arms_chams_clr[3]);
		write_value(json[xs("visuals")][xs("sleeve_chams_clr_ov[0]")], chams::sleeve_chams_clr_ov[0]);
		write_value(json[xs("visuals")][xs("sleeve_chams_clr_ov[1]")], chams::sleeve_chams_clr_ov[1]);
		write_value(json[xs("visuals")][xs("sleeve_chams_clr_ov[2]")], chams::sleeve_chams_clr_ov[2]);
		write_value(json[xs("visuals")][xs("sleeve_chams_clr_ov[3]")], chams::sleeve_chams_clr_ov[3]);
		write_value(json[xs("visuals")][xs("arms_chams_clr_ov[0]")], chams::arms_chams_clr_ov[0]);
		write_value(json[xs("visuals")][xs("arms_chams_clr_ov[1]")], chams::arms_chams_clr_ov[1]);
		write_value(json[xs("visuals")][xs("arms_chams_clr_ov[2]")], chams::arms_chams_clr_ov[2]);
		write_value(json[xs("visuals")][xs("arms_chams_clr_ov[3]")], chams::arms_chams_clr_ov[3]);
		write_value(json[xs("visuals")][xs("wpn_chams_clr[0]")], chams::wpn_chams_clr[0]);
		write_value(json[xs("visuals")][xs("wpn_chams_clr[1]")], chams::wpn_chams_clr[1]);
		write_value(json[xs("visuals")][xs("wpn_chams_clr[2]")], chams::wpn_chams_clr[2]);
		write_value(json[xs("visuals")][xs("wpn_chams_clr[3]")], chams::wpn_chams_clr[3]);
		write_value(json[xs("visuals")][xs("wpn_chams_clr_ov[0]")], chams::wpn_chams_clr_ov[0]);
		write_value(json[xs("visuals")][xs("wpn_chams_clr_ov[1]")], chams::wpn_chams_clr_ov[1]);
		write_value(json[xs("visuals")][xs("wpn_chams_clr_ov[2]")], chams::wpn_chams_clr_ov[2]);
		write_value(json[xs("visuals")][xs("wpn_chams_clr_ov[3]")], chams::wpn_chams_clr_ov[3]);
		write_value(json[xs("visuals")][xs("visible_wireframe")], chams::visible_wireframe);
		write_value(json[xs("visuals")][xs("visible_chams_ov")], chams::visible_chams_ov);
		write_value(json[xs("visuals")][xs("visible_wireframe_ov")], chams::visible_wireframe_ov);
		write_value(json[xs("visuals")][xs("invisible_wireframe")], chams::invisible_wireframe);
		write_value(json[xs("visuals")][xs("sleeve_chams")], chams::sleeve_chams);
		write_value(json[xs("visuals")][xs("sleeve_wireframe")], chams::sleeve_wireframe);
		write_value(json[xs("visuals")][xs("arms_chams")], chams::arms_chams);
		write_value(json[xs("visuals")][xs("arms_wireframe")], chams::arms_wireframe);
		write_value(json[xs("visuals")][xs("sleeve_chams_ov")], chams::sleeve_chams_ov);
		write_value(json[xs("visuals")][xs("sleeve_wireframe_ov")], chams::sleeve_wireframe_ov);
		write_value(json[xs("visuals")][xs("arms_chams_ov")], chams::arms_chams_ov);
		write_value(json[xs("visuals")][xs("wpn_chams")], chams::wpn_chams);
		write_value(json[xs("visuals")][xs("wpn_wireframe")], chams::wpn_wireframe);
		write_value(json[xs("visuals")][xs("wpn_chams_ov")], chams::wpn_chams_ov);
		write_value(json[xs("visuals")][xs("wpn_wireframe_ov")], chams::wpn_wireframe_ov);
		write_value(json[xs("visuals")][xs("cham_type_overlay")], chams::cham_type_overlay);
		write_value(json[xs("visuals")][xs("cham_type_sleeve")], chams::cham_type_sleeve);
		write_value(json[xs("visuals")][xs("cham_type_sleeve_ov")], chams::cham_type_sleeve_ov);
		write_value(json[xs("visuals")][xs("cham_type_wpn")], chams::cham_type_wpn);
		write_value(json[xs("visuals")][xs("cham_type_wpn_ov")], chams::cham_type_wpn_ov);
		write_value(json[xs("visuals")][xs("cham_type_arms")], chams::cham_type_arms);
		write_value(json[xs("visuals")][xs("cham_type_arms_ov")], chams::cham_type_arms_ov);
		write_value(json[xs("visuals")][xs("glow")], visuals::glow);
		write_value(json[xs("visuals")][xs("glow_weapon")], visuals::glow_weapon);
		write_value(json[xs("visuals")][xs("custom_sun")], visuals::custom_sun);
		write_value(json[xs("visuals")][xs("custom_sun_x")], visuals::custom_sun_x);
		write_value(json[xs("visuals")][xs("custom_sun_y")], visuals::custom_sun_y);
		write_value(json[xs("visuals")][xs("custom_sun_dist")], visuals::custom_sun_dist);
		write_value(json[xs("visuals")][xs("custom_sun_mode")], visuals::custom_sun_mode);
		write_value(json[xs("visuals")][xs("custom_sun_speed")], visuals::custom_sun_speed);
		write_value(json[xs("visuals")][xs("custom_sun_orbit_range")], visuals::custom_sun_orbit_range);
		write_value(json[xs("triggerbot")][xs("enabled")], triggerbot::enabled);
		write_value(json[xs("triggerbot")][xs("triggerbot_key")], triggerbot::triggerbot_key);
		write_value(json[xs("triggerbot")][xs("triggerbot_key_s")], triggerbot::triggerbot_key_s);
		write_value(json[xs("aimbot")][xs("aimbot_hitbox")], aimbot::aimbot_hitbox);
		write_value(json[xs("aimbot")][xs("aimbot_autowall")], aimbot::aimbot_autowall);
		write_value(json[xs("aimbot")][xs("aimbot_autowall_dmg")], aimbot::aimbot_autowall_dmg);
		write_value(json[xs("aimbot")][xs("aimbot_fov")], aimbot::aimbot_fov);
		write_value(json[xs("aimbot")][xs("backtrack")], backtrack::backtrack);
		write_value(json[xs("aimbot")][xs("time")], backtrack::time);
		write_value(json[xs("aimbot")][xs("fake")], backtrack::fake);
		write_value(json[xs("aimbot")][xs("fake_latency")], backtrack::fake_latency);
		write_value(json[xs("aimbot")][xs("selected_tick")], backtrack::selected_tick);
		write_value(json[xs("movement")][xs("auto_align")], movement::auto_align);
		write_value(json[xs("aimbot")][xs("aimbot")], aimbot::aim_at_bt);
		write_value(json[xs("aimbot")][xs("aimbot")], aimbot::aimbot);
		write_value(json[xs("aimbot")][xs("aimbot_key")], aimbot::aimbot_key);
		write_value(json[xs("aimbot")][xs("pistol_hitbox")], aimbot::pistol_hitbox);
		write_value(json[xs("aimbot")][xs("pistol_aimbot_fov")], aimbot::pistol_aimbot_fov);
		write_value(json[xs("aimbot")][xs("pistol_autowall")], aimbot::pistol_autowall);
		write_value(json[xs("aimbot")][xs("pistol_autowall_dmg")], aimbot::pistol_autowall_dmg);
		write_value(json[xs("aimbot")][xs("heavy_pistol_hitbox")], aimbot::heavy_pistol_hitbox);
		write_value(json[xs("aimbot")][xs("heavy_pistol_aimbot_fov")], aimbot::heavy_pistol_aimbot_fov);
		write_value(json[xs("aimbot")][xs("heavy_pistol_autowall")], aimbot::heavy_pistol_autowall);
		write_value(json[xs("aimbot")][xs("heavy_pistol_autowall_dmg")], aimbot::heavy_pistol_autowall_dmg);
		write_value(json[xs("aimbot")][xs("shotgun_hitbox")], aimbot::shotgun_hitbox);
		write_value(json[xs("aimbot")][xs("shotgun_aimbot_fov")], aimbot::shotgun_aimbot_fov);
		write_value(json[xs("aimbot")][xs("shotgun_autowall")], aimbot::shotgun_autowall);
		write_value(json[xs("aimbot")][xs("heavy_hitbox")], aimbot::heavy_hitbox);
		write_value(json[xs("aimbot")][xs("heavy_aimbot_fov")], aimbot::heavy_aimbot_fov);
		write_value(json[xs("aimbot")][xs("heavy_autowall")], aimbot::heavy_autowall);
		write_value(json[xs("aimbot")][xs("heavy_autowall_dmg")], aimbot::heavy_autowall_dmg);
		write_value(json[xs("aimbot")][xs("smg_hitbox")], aimbot::smg_hitbox);
		write_value(json[xs("aimbot")][xs("smg_aimbot_fov")], aimbot::smg_aimbot_fov);
		write_value(json[xs("aimbot")][xs("smg_autowall")], aimbot::smg_autowall);
		write_value(json[xs("aimbot")][xs("smg_autowall_dmg")], aimbot::smg_autowall_dmg);
		write_value(json[xs("aimbot")][xs("rifle_hitbox")], aimbot::rifle_hitbox);
		write_value(json[xs("aimbot")][xs("rifle_aimbot_fov")], aimbot::rifle_aimbot_fov);
		write_value(json[xs("aimbot")][xs("rifle_autowall")], aimbot::rifle_autowall);
		write_value(json[xs("aimbot")][xs("rifle_autowall_dmg")], aimbot::rifle_autowall_dmg);
		write_value(json[xs("aimbot")][xs("sniper_hitbox")], aimbot::sniper_hitbox);
		write_value(json[xs("aimbot")][xs("sniper_aimbot_fov")], aimbot::sniper_aimbot_fov);
		write_value(json[xs("aimbot")][xs("sniper_autowall")], aimbot::sniper_autowall);
		write_value(json[xs("aimbot")][xs("sniper_autowall_dmg")], aimbot::sniper_autowall_dmg);
		write_value(json[xs("aimbot")][xs("autosniper_hitbox")], aimbot::autosniper_hitbox);
		write_value(json[xs("aimbot")][xs("autosniper_aimbot_fov")], aimbot::autosniper_aimbot_fov);
		write_value(json[xs("aimbot")][xs("autosniper_autowall")], aimbot::autosniper_autowall);
		write_value(json[xs("aimbot")][xs("autosniper_autowall_dmg")], aimbot::autosniper_autowall_dmg);
		write_value(json[xs("aimbot")][xs("aimbot_silent")], aimbot::aimbot_silent);
		write_value(json[xs("aimbot")][xs("non_sticky_aimbot")], aimbot::non_sticky_aimbot);
		write_value(json[xs("visuals")][xs("dropped_weapon_icon")], visuals::dropped_weapon_icon);
		write_value(json[xs("visuals")][xs("dropped_weapon_name")], visuals::dropped_weapon_name);
		write_value(json[xs("visuals")][xs("dropped_weapon_distance")], visuals::dropped_weapon_distance);
		write_value(json[xs("visuals")][xs("dropped_weapon_icon_clr[0]")], visuals::dropped_weapon_icon_clr[0]);
		write_value(json[xs("visuals")][xs("dropped_weapon_box_clr[1]")], visuals::dropped_weapon_icon_clr[1]);
		write_value(json[xs("visuals")][xs("dropped_weapon_icon_clr[2]")], visuals::dropped_weapon_icon_clr[2]);
		write_value(json[xs("visuals")][xs("dropped_weapon_icon_clr[3]")], visuals::dropped_weapon_icon_clr[3]);
		write_value(json[xs("visuals")][xs("dropped_weapon_name_clr[0]")], visuals::dropped_weapon_name_clr[0]);
		write_value(json[xs("visuals")][xs("dropped_weapon_name_clr[1]")], visuals::dropped_weapon_name_clr[1]);
		write_value(json[xs("visuals")][xs("dropped_weapon_name_clr[2]")], visuals::dropped_weapon_name_clr[2]);
		write_value(json[xs("visuals")][xs("dropped_weapon_name_clr[3]")], visuals::dropped_weapon_name_clr[3]);
		write_value(json[xs("visuals")][xs("fog_color[0]")], visuals::fog_color[0]);
		write_value(json[xs("visuals")][xs("fog_color[1]")], visuals::fog_color[1]);
		write_value(json[xs("visuals")][xs("fog_color[2]")], visuals::fog_color[2]);
		write_value(json[xs("visuals")][xs("fog_color[3]")], visuals::fog_color[3]);
		write_value(json[xs("visuals")][xs("fog")], visuals::fog);
		write_value(json[xs("visuals")][xs("fog_distance")], visuals::fog_distance);
		write_value(json[xs("visuals")][xs("fog_density")], visuals::fog_density);
		write_value(json[xs("misc")][xs("swayscale")], misc::swayscale);
		write_value(json[xs("misc")][xs("swayscale_value")], misc::swayscale_value);
		write_value(json[xs("visuals")][xs("skybox")], visuals::skybox);
		write_value(json[xs("visuals")][xs("dlight")], visuals::dlight);
		write_value(json[xs("visuals")][xs("dlight_clr[0]")], visuals::dlight_clr[0]);
		write_value(json[xs("visuals")][xs("dlight_clr[1]")], visuals::dlight_clr[1]);
		write_value(json[xs("visuals")][xs("dlight_clr[2]")], visuals::dlight_clr[2]);
		write_value(json[xs("visuals")][xs("dlight_clr[3]")], visuals::dlight_clr[3]);
		write_value(json[xs("visuals")][xs("visible_chams_clr[0]")], chams::visible_chams_clr[0]);
		write_value(json[xs("visuals")][xs("visible_chams_clr[1]")], chams::visible_chams_clr[1]);
		write_value(json[xs("visuals")][xs("visible_chams_clr[2]")], chams::visible_chams_clr[2]);
		write_value(json[xs("visuals")][xs("visible_chams_clr[3]")], chams::visible_chams_clr[3]);
		write_value(json[xs("visuals")][xs("invisible_chams_clr[0]")], chams::invisible_chams_clr[0]);
		write_value(json[xs("visuals")][xs("invisible_chams_clr[1]")], chams::invisible_chams_clr[1]);
		write_value(json[xs("visuals")][xs("invisible_chams_clr[2]")], chams::invisible_chams_clr[2]);
		write_value(json[xs("visuals")][xs("invisible_chams_clr[3]")], chams::invisible_chams_clr[3]);
		write_value(json[xs("visuals")][xs("invisible_chams")], chams::invisible_chams);
		write_value(json[xs("visuals")][xs("visible_chams")], chams::visible_chams);
		write_value(json[xs("visuals")][xs("cham_type_bt")], chams::cham_type_bt);
		write_value(json[xs("visuals")][xs("cham_type")], chams::cham_type);
		write_value(json[xs("misc")][xs("misc_animated_clantag")], misc::misc_animated_clantag);
		write_value(json[xs("misc")][xs("force_crosshair")], misc::force_crosshair);
		write_value(json[xs("misc")][xs("sniper_crosshair")], misc::sniper_crosshair);
		write_value(json[xs("misc")][xs("misc_clantag_spammer")], misc::misc_clantag_spammer);
		write_value(json[xs("misc")][xs("misc_clantag_speed")], misc::misc_clantag_speed);
		write_value(json[xs("misc")][xs("misc_clantag_type")], misc::misc_clantag_type);
		write_value(json[xs("misc")][xs("misc_hitmarker")], misc::misc_hitmarker);
		write_value(json[xs("misc")][xs("misc_hitmarker_sound")], misc::misc_hitmarker_sound);
		write_value(json[xs("misc")][xs("misc_hitmarker_screen_effect")], misc::misc_hitmarker_screen_effect);
		write_value(json[xs("misc")][xs("misc_kill_msg")], misc::misc_kill_msg);
		write_value(json[xs("misc")][xs("misc_kill_message")], misc::misc_kill_message);
		write_value(json[xs("misc")][xs("menu_variant")], misc::menu_variant);
		write_value(json[xs("misc")][xs("menu_fx_mode")], misc::menu_fx_mode);
		write_value(json[xs("misc")][xs("menu_fx_density")], misc::menu_fx_density);
		write_value(json[xs("visuals")][xs("fullbright")], visuals::fullbright);
		write_value(json[xs("visuals")][xs("apply_zoom")], visuals::apply_zoom);
		write_value(json[xs("visuals")][xs("apply_zoom_key")], visuals::apply_zoom_key);
		write_value(json[xs("visuals")][xs("apply_zoom_key_s")], visuals::apply_zoom_key_s);
		write_value(json[xs("visuals")][xs("player_box_outline[0]")], visuals::player_box_outline[0]);
		write_value(json[xs("visuals")][xs("player_box_outline[1]")], visuals::player_box_outline[1]);
		write_value(json[xs("visuals")][xs("boxtype")], visuals::boxtype);
		write_value(json[xs("visuals")][xs("mbenabled")], visuals::mbenabled);
		write_value(json[xs("visuals")][xs("mbforwardEnabled")], visuals::mbforwardEnabled);
		write_value(json[xs("visuals")][xs("mbfallingMin")], visuals::mbfallingMin);
		write_value(json[xs("visuals")][xs("mbfallingMax")], visuals::mbfallingMax);
		write_value(json[xs("visuals")][xs("mbfallingIntensity")], visuals::mbfallingIntensity);
		write_value(json[xs("visuals")][xs("mbrotationIntensity")], visuals::mbrotationIntensity);
		write_value(json[xs("visuals")][xs("mbstrength")], visuals::mbstrength);
		write_value(json[xs("misc")][xs("nadepred")], misc::nadepred);
		write_value(json[xs("misc")][xs("nadepred_clr[0]")], misc::nadepred_clr[0]);
		write_value(json[xs("misc")][xs("nadepred_clr[1]")], misc::nadepred_clr[1]);
		write_value(json[xs("misc")][xs("nadepred_clr[2]")], misc::nadepred_clr[2]);
		write_value(json[xs("visuals")][xs("flashalpha")], visuals::flashalpha);
		write_value(json[xs("visuals")][xs("change_flashalpha")], visuals::change_flashalpha);
		write_value(json[xs("visuals")][xs("nosmoke")], visuals::nosmoke);
		write_value(json[xs("misc")][xs("autoaccept")], misc::autoaccept);
		write_value(json[xs("visuals")][xs("trails_clr1[0]")], visuals::trails_clr1[0]);
		write_value(json[xs("visuals")][xs("trails_clr1[1]")], visuals::trails_clr1[1]);
		write_value(json[xs("visuals")][xs("trails_clr1[2]")], visuals::trails_clr1[2]);
		write_value(json[xs("visuals")][xs("trails_clr1[3]")], visuals::trails_clr1[3]);
		write_value(json[xs("visuals")][xs("trails_clr2[0]")], visuals::trails_clr2[0]);
		write_value(json[xs("visuals")][xs("trails_clr2[1]")], visuals::trails_clr2[1]);
		write_value(json[xs("visuals")][xs("trails_clr2[2]")], visuals::trails_clr2[2]);
		write_value(json[xs("visuals")][xs("trails_clr2[3]")], visuals::trails_clr2[3]);
		write_value(json[xs("visuals")][xs("world_modulate")], visuals::world_modulate);
		write_value(json[xs("visuals")][xs("world_modulate_props")], visuals::world_modulate_props);
		write_value(json[xs("visuals")][xs("world_modulate_skybox")], visuals::world_modulate_skybox);
		write_value(json[xs("visuals")][xs("world_modulate_intensity")], visuals::world_modulate_intensity);
		write_value(json[xs("visuals")][xs("world_color[0]")], visuals::world_color[0]);
		write_value(json[xs("visuals")][xs("world_color[1]")], visuals::world_color[1]);
		write_value(json[xs("visuals")][xs("world_color[2]")], visuals::world_color[2]);
		write_value(json[xs("visuals")][xs("world_color[3]")], visuals::world_color[3]);
		write_value(json[xs("visuals")][xs("trails")], visuals::trails);
		write_value(json[xs("movement")][xs("indicators_position")], movement::indicators_position);
		write_value(json[xs("movement")][xs("indicators_gap")], movement::indicators_gap);
		write_value(json[xs("misc")][xs("menu_key")], misc::menu_key);
		write_value(json[xs("misc")][xs("vote_revealer")], misc::vote_revealer);
		write_value(json[xs("misc")][xs("misc_hit_info[0]")], misc::misc_hit_info[0]);
		write_value(json[xs("misc")][xs("misc_hit_info[1]")], misc::misc_hit_info[1]);
		write_value(json[xs("visuals")][xs("visuals_flags[0]")], visuals::visuals_flags[0]);
		write_value(json[xs("visuals")][xs("visuals_flags[1]")], visuals::visuals_flags[1]);
		write_value(json[xs("visuals")][xs("visuals_flags[2]")], visuals::visuals_flags[2]);
		write_value(json[xs("visuals")][xs("visuals_flags[3]")], visuals::visuals_flags[3]);
		write_value(json[xs("visuals")][xs("visuals_flags[4]")], visuals::visuals_flags[4]);
		write_value(json[xs("visuals")][xs("visuals_flags[5]")], visuals::visuals_flags[5]);
		write_value(json[xs("visuals")][xs("removals[0]")], visuals::removals[0]);
		write_value(json[xs("visuals")][xs("removals[1]")], visuals::removals[1]);
		write_value(json[xs("visuals")][xs("removals[2]")], visuals::removals[2]);
		write_value(json[xs("movement")][xs("pixel_surf")], movement::pixel_surf);
		write_value(json[xs("movement")][xs("pixel_surf_ticks")], movement::pixel_surf_ticks);
		write_value(json[xs("movement")][xs("pixel_surf_key")], movement::pixel_surf_key);
		write_value(json[xs("movement")][xs("pixel_surf_key_s")], movement::pixel_surf_key_s);
		write_value(json[xs("movement")][xs("pixel_surf_detection_printf")], movement::pixel_surf_detection_printf);
		write_value(json[xs("movement")][xs("jump_bug_key_s")], movement::jump_bug_key_s);
		write_value(json[xs("movement")][xs("edge_bug_key_s")], movement::edge_bug_key_s);
		write_value(json[xs("movement")][xs("mini_jump_key_s")], movement::mini_jump_key_s);
		write_value(json[xs("movement")][xs("mini_jump_ej")], movement::mini_jump_ej);
		write_value(json[xs("movement")][xs("long_jump_key_s")], movement::long_jump_key_s);
		write_value(json[xs("visuals")][xs("dropped_weapon_glow_clr[0]")], visuals::dropped_weapon_glow_clr[0]);
		write_value(json[xs("visuals")][xs("dropped_weapon_glow_clr[1]")], visuals::dropped_weapon_glow_clr[1]);
		write_value(json[xs("visuals")][xs("dropped_weapon_glow_clr[2]")], visuals::dropped_weapon_glow_clr[2]);
		write_value(json[xs("visuals")][xs("dropped_weapon_glow")], visuals::dropped_weapon_glow);
		write_value(json[xs("movement")][xs("long_jump_key")], movement::long_jump_key);
		write_value(json[xs("visuals")][xs("dropped_weapon_type")], visuals::dropped_weapon_type);
		write_value(json[xs("movement")][xs("adaptive_key_for[0]")], movement::adaptive_key_for[0]);
		write_value(json[xs("movement")][xs("adaptive_key_for[1]")], movement::adaptive_key_for[1]);
		write_value(json[xs("movement")][xs("adaptive_key_for[2]")], movement::adaptive_key_for[2]);
		write_value(json[xs("movement")][xs("adaptive_key_cancelling")], movement::adaptive_key_cancelling);
		write_value(json[xs("movement")][xs("lj_null[0]")], movement::lj_null[0]);
		write_value(json[xs("movement")][xs("lj_null[1]")], movement::lj_null[1]);
		write_value(json[xs("movement")][xs("lj_null[2]")], movement::lj_null[2]);
		write_value(json[xs("movement")][xs("lj_null[3]")], movement::lj_null[3]);
		write_value(json[xs("fonts")][xs("indi_font")], fonts::indi_font);
		write_value(json[xs("fonts")][xs("indi_size")], fonts::indi_size);
		write_value(json[xs("fonts")][xs("sub_indi_font")], fonts::sub_indi_font);
		write_value(json[xs("fonts")][xs("sub_indi_size")], fonts::sub_indi_size);
		write_value(json[xs("fonts")][xs("spec_font")], fonts::spec_font);
		write_value(json[xs("fonts")][xs("sub_spec_font")], fonts::sub_spec_font);
		write_value(json[xs("fonts")][xs("esp_hp_font")], fonts::esp_hp_font);
		write_value(json[xs("fonts")][xs("esp_hp_size")], fonts::esp_hp_size);
		write_value(json[xs("fonts")][xs("esp_name_font")], fonts::esp_name_font);
		write_value(json[xs("fonts")][xs("esp_name_size")], fonts::esp_name_size);
		write_value(json[xs("fonts")][xs("esp_wpn_font")], fonts::esp_wpn_font);
		write_value(json[xs("fonts")][xs("esp_wpn_size")], fonts::esp_wpn_size);
		write_value(json[xs("fonts")][xs("esp_dropped_wpn_font")], fonts::esp_dropped_wpn_font);
		write_value(json[xs("fonts")][xs("esp_dropped_wpn_size")], fonts::esp_dropped_wpn_size);
		write_value(json[xs("fonts")][xs("sc_logs_font")], fonts::sc_logs_font);
		write_value(json[xs("fonts")][xs("sc_logs_size")], fonts::sc_logs_size);
		write_value(json[xs("fonts")][xs("spec_font_flag[0]")], fonts::spec_font_flag[0]);
		write_value(json[xs("fonts")][xs("spec_font_flag[1]")], fonts::spec_font_flag[1]);
		write_value(json[xs("fonts")][xs("spec_font_flag[2]")], fonts::spec_font_flag[2]);
		write_value(json[xs("fonts")][xs("spec_font_flag[3]")], fonts::spec_font_flag[3]);
		write_value(json[xs("fonts")][xs("spec_font_flag[4]")], fonts::spec_font_flag[4]);
		write_value(json[xs("fonts")][xs("spec_font_flag[5]")], fonts::spec_font_flag[5]);
		write_value(json[xs("fonts")][xs("spec_font_flag[6]")], fonts::spec_font_flag[6]);
		write_value(json[xs("fonts")][xs("spec_font_flag[7]")], fonts::spec_font_flag[7]);
		write_value(json[xs("fonts")][xs("spec_font_flag[8]")], fonts::spec_font_flag[8]);
		write_value(json[xs("fonts")][xs("spec_font_flag[9]")], fonts::spec_font_flag[9]);
		write_value(json[xs("fonts")][xs("sub_spec_font_flag[0]")], fonts::sub_spec_font_flag[0]);
		write_value(json[xs("fonts")][xs("sub_spec_font_flag[1]")], fonts::sub_spec_font_flag[1]);
		write_value(json[xs("fonts")][xs("sub_spec_font_flag[2]")], fonts::sub_spec_font_flag[2]);
		write_value(json[xs("fonts")][xs("sub_spec_font_flag[3]")], fonts::sub_spec_font_flag[3]);
		write_value(json[xs("fonts")][xs("sub_spec_font_flag[4]")], fonts::sub_spec_font_flag[4]);
		write_value(json[xs("fonts")][xs("sub_spec_font_flag[5]")], fonts::sub_spec_font_flag[5]);
		write_value(json[xs("fonts")][xs("sub_spec_font_flag[6]")], fonts::sub_spec_font_flag[6]);
		write_value(json[xs("fonts")][xs("sub_spec_font_flag[7]")], fonts::sub_spec_font_flag[7]);
		write_value(json[xs("fonts")][xs("sub_spec_font_flag[8]")], fonts::sub_spec_font_flag[8]);
		write_value(json[xs("fonts")][xs("sub_spec_font_flag[9]")], fonts::sub_spec_font_flag[9]);
		write_value(json[xs("fonts")][xs("sub_indi_font_flag[0]")], fonts::sub_indi_font_flag[0]);
		write_value(json[xs("fonts")][xs("sub_indi_font_flag[1]")], fonts::sub_indi_font_flag[1]);
		write_value(json[xs("fonts")][xs("sub_indi_font_flag[2]")], fonts::sub_indi_font_flag[2]);
		write_value(json[xs("fonts")][xs("sub_indi_font_flag[3]")], fonts::sub_indi_font_flag[3]);
		write_value(json[xs("fonts")][xs("sub_indi_font_flag[4]")], fonts::sub_indi_font_flag[4]);
		write_value(json[xs("fonts")][xs("sub_indi_font_flag[5]")], fonts::sub_indi_font_flag[5]);
		write_value(json[xs("fonts")][xs("sub_indi_font_flag[6]")], fonts::sub_indi_font_flag[6]);
		write_value(json[xs("fonts")][xs("sub_indi_font_flag[7]")], fonts::sub_indi_font_flag[7]);
		write_value(json[xs("fonts")][xs("sub_indi_font_flag[8]")], fonts::sub_indi_font_flag[8]);
		write_value(json[xs("fonts")][xs("sub_indi_font_flag[9]")], fonts::sub_indi_font_flag[9]);
		write_value(json[xs("fonts")][xs("sub_indi_font_flag[10]")], fonts::sub_indi_font_flag[10]);
		write_value(json[xs("fonts")][xs("sub_indi_font_flag[11]")], fonts::sub_indi_font_flag[11]);
		write_value(json[xs("fonts")][xs("indi_font_flag[0]")], fonts::indi_font_flag[0]);
		write_value(json[xs("fonts")][xs("indi_font_flag[1]")], fonts::indi_font_flag[1]);
		write_value(json[xs("fonts")][xs("indi_font_flag[2]")], fonts::indi_font_flag[2]);
		write_value(json[xs("fonts")][xs("indi_font_flag[3]")], fonts::indi_font_flag[3]);
		write_value(json[xs("fonts")][xs("indi_font_flag[4]")], fonts::indi_font_flag[4]);
		write_value(json[xs("fonts")][xs("indi_font_flag[5]")], fonts::indi_font_flag[5]);
		write_value(json[xs("fonts")][xs("indi_font_flag[6]")], fonts::indi_font_flag[6]);
		write_value(json[xs("fonts")][xs("indi_font_flag[7]")], fonts::indi_font_flag[7]);
		write_value(json[xs("fonts")][xs("indi_font_flag[8]")], fonts::indi_font_flag[8]);
		write_value(json[xs("fonts")][xs("indi_font_flag[9]")], fonts::indi_font_flag[9]);
		write_value(json[xs("fonts")][xs("indi_font_flag[10]")], fonts::indi_font_flag[10]);
		write_value(json[xs("fonts")][xs("indi_font_flag[11]")], fonts::indi_font_flag[11]);
		write_value(json[xs("fonts")][xs("esp_font_flag[0]")], fonts::esp_font_flag[0]);
		write_value(json[xs("fonts")][xs("esp_font_flag[1]")], fonts::esp_font_flag[1]);
		write_value(json[xs("fonts")][xs("esp_font_flag[2]")], fonts::esp_font_flag[2]);
		write_value(json[xs("fonts")][xs("esp_font_flag[3]")], fonts::esp_font_flag[3]);
		write_value(json[xs("fonts")][xs("esp_font_flag[4]")], fonts::esp_font_flag[4]);
		write_value(json[xs("fonts")][xs("esp_font_flag[5]")], fonts::esp_font_flag[5]);
		write_value(json[xs("fonts")][xs("esp_font_flag[6]")], fonts::esp_font_flag[6]);
		write_value(json[xs("fonts")][xs("esp_font_flag[7]")], fonts::esp_font_flag[7]);
		write_value(json[xs("fonts")][xs("esp_font_flag[8]")], fonts::esp_font_flag[8]);
		write_value(json[xs("fonts")][xs("esp_font_flag[9]")], fonts::esp_font_flag[9]);
		write_value(json[xs("fonts")][xs("esp_font_flag[10]")], fonts::esp_font_flag[10]);
		write_value(json[xs("fonts")][xs("esp_font_flag[11]")], fonts::esp_font_flag[11]);
		write_value(json[xs("fonts")][xs("esp_name_font_flag[0]")], fonts::esp_name_font_flag[0]);
		write_value(json[xs("fonts")][xs("esp_name_font_flag[1]")], fonts::esp_name_font_flag[1]);
		write_value(json[xs("fonts")][xs("esp_name_font_flag[2]")], fonts::esp_name_font_flag[2]);
		write_value(json[xs("fonts")][xs("esp_name_font_flag[3]")], fonts::esp_name_font_flag[3]);
		write_value(json[xs("fonts")][xs("esp_name_font_flag[4]")], fonts::esp_name_font_flag[4]);
		write_value(json[xs("fonts")][xs("esp_name_font_flag[5]")], fonts::esp_name_font_flag[5]);
		write_value(json[xs("fonts")][xs("esp_name_font_flag[6]")], fonts::esp_name_font_flag[6]);
		write_value(json[xs("fonts")][xs("esp_name_font_flag[7]")], fonts::esp_name_font_flag[7]);
		write_value(json[xs("fonts")][xs("esp_name_font_flag[8]")], fonts::esp_name_font_flag[8]);
		write_value(json[xs("fonts")][xs("esp_name_font_flag[9]")], fonts::esp_name_font_flag[9]);
		write_value(json[xs("fonts")][xs("esp_name_font_flag[10]")], fonts::esp_name_font_flag[10]);
		write_value(json[xs("fonts")][xs("esp_name_font_flag[11]")], fonts::esp_name_font_flag[11]);
		write_value(json[xs("fonts")][xs("esp_wpn_font_flag[0]")], fonts::esp_wpn_font_flag[0]);
		write_value(json[xs("fonts")][xs("esp_wpn_font_flag[1]")], fonts::esp_wpn_font_flag[1]);
		write_value(json[xs("fonts")][xs("esp_wpn_font_flag[2]")], fonts::esp_wpn_font_flag[2]);
		write_value(json[xs("fonts")][xs("esp_wpn_font_flag[3]")], fonts::esp_wpn_font_flag[3]);
		write_value(json[xs("fonts")][xs("esp_wpn_font_flag[4]")], fonts::esp_wpn_font_flag[4]);
		write_value(json[xs("fonts")][xs("esp_wpn_font_flag[5]")], fonts::esp_wpn_font_flag[5]);
		write_value(json[xs("fonts")][xs("esp_wpn_font_flag[6]")], fonts::esp_wpn_font_flag[6]);
		write_value(json[xs("fonts")][xs("esp_wpn_font_flag[7]")], fonts::esp_wpn_font_flag[7]);
		write_value(json[xs("fonts")][xs("esp_wpn_font_flag[8]")], fonts::esp_wpn_font_flag[8]);
		write_value(json[xs("fonts")][xs("esp_wpn_font_flag[9]")], fonts::esp_wpn_font_flag[9]);
		write_value(json[xs("fonts")][xs("esp_wpn_font_flag[10]")], fonts::esp_wpn_font_flag[10]);
		write_value(json[xs("fonts")][xs("esp_wpn_font_flag[11]")], fonts::esp_wpn_font_flag[11]);
		write_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[0]")], fonts::esp_dropped_wpn_font_flag[0]);
		write_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[1]")], fonts::esp_dropped_wpn_font_flag[1]);
		write_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[2]")], fonts::esp_dropped_wpn_font_flag[2]);
		write_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[3]")], fonts::esp_dropped_wpn_font_flag[3]);
		write_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[4]")], fonts::esp_dropped_wpn_font_flag[4]);
		write_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[5]")], fonts::esp_dropped_wpn_font_flag[5]);
		write_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[6]")], fonts::esp_dropped_wpn_font_flag[6]);
		write_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[7]")], fonts::esp_dropped_wpn_font_flag[7]);
		write_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[8]")], fonts::esp_dropped_wpn_font_flag[8]);
		write_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[9]")], fonts::esp_dropped_wpn_font_flag[9]);
		write_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[10]")], fonts::esp_dropped_wpn_font_flag[10]);
		write_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[11]")], fonts::esp_dropped_wpn_font_flag[11]);
		write_value(json[xs("fonts")][xs("sc_logs_flag[0]")], fonts::sc_logs_flag[0]);
		write_value(json[xs("fonts")][xs("sc_logs_flag[1]")], fonts::sc_logs_flag[1]);
		write_value(json[xs("fonts")][xs("sc_logs_flag[2]")], fonts::sc_logs_flag[2]);
		write_value(json[xs("fonts")][xs("sc_logs_flag[3]")], fonts::sc_logs_flag[3]);
		write_value(json[xs("fonts")][xs("sc_logs_flag[4]")], fonts::sc_logs_flag[4]);
		write_value(json[xs("fonts")][xs("sc_logs_flag[5]")], fonts::sc_logs_flag[5]);
		write_value(json[xs("fonts")][xs("sc_logs_flag[6]")], fonts::sc_logs_flag[6]);
		write_value(json[xs("fonts")][xs("sc_logs_flag[7]")], fonts::sc_logs_flag[7]);
		write_value(json[xs("fonts")][xs("sc_logs_flag[8]")], fonts::sc_logs_flag[8]);
		write_value(json[xs("fonts")][xs("sc_logs_flag[9]")], fonts::sc_logs_flag[9]);
		write_value(json[xs("fonts")][xs("sc_logs_flag[10]")], fonts::sc_logs_flag[10]);
		write_value(json[xs("fonts")][xs("sc_logs_flag[11]")], fonts::sc_logs_flag[11]);
		write_value(json[xs("sfui")][xs("sfui_on")], sfui::sfui_on);


		write_value(json[xs("movement")][xs("fireman")], movement::fireman);
		write_value(json[xs("movement")][xs("fireman_key")], movement::fireman_key);
		write_value(json[xs("movement")][xs("fireman_key_s")], movement::fireman_key_s);

		write_value(json[xs("movement")][xs("indicators_fading_speed")], movement::indicators_fading_speed);

		write_value(json[xs("visuals")][xs("mbsmoothing")], visuals::mbsmoothing);
		write_value(json[xs("visuals")][xs("fog")], visuals::fog);
		write_value(json[xs("visuals")][xs("fog_color[0]")], visuals::fog_color[0]);
		write_value(json[xs("visuals")][xs("fog_color[1]")], visuals::fog_color[1]);
		write_value(json[xs("visuals")][xs("fog_color[2]")], visuals::fog_color[2]);
		write_value(json[xs("visuals")][xs("fog_color[3]")], visuals::fog_color[3]);
		write_value(json[xs("visuals")][xs("fog_density")], visuals::fog_density);
		write_value(json[xs("visuals")][xs("fog_distance")], visuals::fog_distance);


		write_value(json[xs("visuals")][xs("playerbackground")], visuals::playerbackground);
		write_value(json[xs("visuals")][xs("playerbackground_color1")], visuals::playerbackground_color1);
		write_value(json[xs("visuals")][xs("playerbackground_color2")], visuals::playerbackground_color2);

		write_value(json[xs("movement")][xs("edgebug_plane_z")], movement::edgebug_plane_z);
		write_value(json[xs("movement")][xs("post_auto_align")], movement::post_auto_align);
		write_value(json[xs("movement")][xs("smooth_mouse")], movement::smooth_mouse);

		write_value(json[xs("movement")][xs("edge_bug_lock_type")], movement::edge_bug_lock_type);
		write_value(json[xs("movement")][xs("auto_clip")], movement::auto_clip);
		write_value(json[xs("movement")][xs("bug_frequency")], movement::bug_frequency);
		write_value(json[xs("movement")][xs("gain_assist")], movement::gain_assist);


		write_value(json[xs("movement")][xs("ai_playback")], movement::ai_playback);
		write_value(json[xs("movement")][xs("ai_learnrate")], movement::ai_learnrate);

		write_value(json[xs("movement")][xs("stage_amount")], movement::stage_amount);
		write_value(json[xs("movement")][xs("stage_push")], movement::stage_push);

		write_value(json[xs("movement")][xs("auto_clip_quick_release")], movement::auto_clip_quick_release);
		write_value(json[xs("movement")][xs("wire_bounce")], movement::wire_bounce);

		write_value(json[xs("movement")][xs("land_on_surf")], movement::land_on_surf);
		write_value(json[xs("movement")][xs("in_air_surfix")], movement::in_air_surfix);

		write_value(json[xs("movement")][xs("pixel_helper")], movement::pixel_helper);
		write_value(json[xs("movement")][xs("headbang")], movement::headbang);
		write_value(json[xs("movement")][xs("wallhop")], movement::wallhop);


		write_value(json[xs("movehelp")][xs("edgebug_scan")], movehelp::edgebug_scan);
		write_value(json[xs("movehelp")][xs("advanced_edgebug")], movehelp::advanced_edgebug);
		write_value(json[xs("movehelp")][xs("advanced_edgebug_scan")], movehelp::advanced_edgebug_scan);












		write_value(json[xs("movehelp")][xs("auto_duck")], movehelp::auto_duck);

		write_value(json[xs("movehelp")][xs("strafe_assistance")], movehelp::strafe_assistance);
		write_value(json[xs("movehelp")][xs("smart_nulls")], movehelp::smart_nulls);
		write_value(json[xs("movehelp")][xs("smart_msl")], movehelp::smart_msl);
		write_value(json[xs("movehelp")][xs("frame_msl")], movehelp::frame_msl);

		write_value(json[xs("movehelp")][xs("pixelhop")], movehelp::pixelhop);

		write_value(json[xs("movehelp")][xs("auto_onehop")], movehelp::auto_onehop);
		write_value(json[xs("movehelp")][xs("onehop_key")], movehelp::onehop_key);
		write_value(json[xs("movehelp")][xs("onehop_key_s")], movehelp::onehop_key_s);


		write_value(json[xs("movehelp")][xs("edgebug_indicator")], movehelp::edgebug_indicator);
		write_value(json[xs("movehelp")][xs("longjump_indicator")], movehelp::longjump_indicator);
		write_value(json[xs("movehelp")][xs("minijump_indicator")], movehelp::minijump_indicator);
		write_value(json[xs("movehelp")][xs("edgejump_indicator")], movehelp::edgejump_indicator);
		write_value(json[xs("movehelp")][xs("jumpbug_indicator")], movehelp::jumpbug_indicator);
		write_value(json[xs("movehelp")][xs("pixelsurf_indicator")], movehelp::pixelsurf_indicator);
		write_value(json[xs("movehelp")][xs("ladderbug_indicator")], movehelp::ladderbug_indicator);
		write_value(json[xs("movehelp")][xs("fireman_indicator")], movehelp::fireman_indicator);
		write_value(json[xs("movehelp")][xs("detection_indicator")], movehelp::detection_indicator);
		write_value(json[xs("movehelp")][xs("edgebug_lock")], movehelp::edgebug_lock);

		write_value(json[xs("movehelp")][xs("surfjump")], movehelp::surfjump);
		write_value(json[xs("movehelp")][xs("surfjump_key")], movehelp::surfjump_key);
		write_value(json[xs("movehelp")][xs("surfjump_key_s")], movehelp::surfjump_key_s);

		write_value(json[xs("movehelp")][xs("surfmove")], movehelp::surfmove);

		write_value(json[xs("visuals")][xs("remove_3dsky")], visuals::remove_3dsky);
		write_value(json[xs("visuals")][xs("remove_shadows")], visuals::remove_shadows);
		write_value(json[xs("visuals")][xs("remove_post_processing")], visuals::remove_post_processing);
		write_value(json[xs("visuals")][xs("remove_panorama_blur")], visuals::remove_panorama_blur);

		write_value(json[xs("visuals")][xs("damage_indicator")], visuals::damage_indicator);
		write_value(json[xs("visuals")][xs("damage_indi_clr[0]")], visuals::damage_indi_clr[0]);
		write_value(json[xs("visuals")][xs("damage_indi_clr[1]")], visuals::damage_indi_clr[1]);
		write_value(json[xs("visuals")][xs("damage_indi_clr[2]")], visuals::damage_indi_clr[2]);

		write_value(json[xs("visuals")][xs("damage_indi_clr_shadow[0]")], visuals::damage_indi_clr_shadow[0]);
		write_value(json[xs("visuals")][xs("damage_indi_clr_shadow[1]")], visuals::damage_indi_clr_shadow[1]);
		write_value(json[xs("visuals")][xs("damage_indi_clr_shadow[2]")], visuals::damage_indi_clr_shadow[2]);

		write_value(json[xs("movehelp")][xs("ai_alignment")], movehelp::ai_alignment);
		write_value(json[xs("movehelp")][xs("ai_headbang")], movehelp::ai_headbang);
		write_value(json[xs("movehelp")][xs("ai_pixelsurf")], movehelp::ai_pixelsurf);
		write_value(json[xs("movehelp")][xs("ai_wallhop")], movehelp::ai_wallhop);
		write_value(json[xs("movehelp")][xs("pixelsurf_ticks")], movehelp::pixelsurf_ticks);
		write_value(json[xs("movehelp")][xs("wallhop_ticks")], movehelp::wallhop_ticks);
		write_value(json[xs("movehelp")][xs("headbang_ticks")], movehelp::headbang_ticks);
		write_value(json[xs("movehelp")][xs("alignment_ticks")], movehelp::alignment_ticks);
		write_value(json[xs("movehelp")][xs("alignment_angle_smooth")], movehelp::alignment_angle_smooth);
		write_value(json[xs("movehelp")][xs("alignment_velocity_smooth")], movehelp::alignment_velocity_smooth);
		write_value(json[xs("visuals")][xs("old_font")], visuals::old_font);

		write_value(json[xs("movehelp")][xs("extended_land")], movehelp::extended_land);

		write_value(json[xs("backtrack")][xs("aim_smoothing")], backtrack::aim_smoothing);
		write_value(json[xs("backtrack")][xs("aim_behavior_slider")], backtrack::aim_behavior_slider);


		write_value(json[xs("visuals")][xs("edit_bloom")], visuals::edit_bloom);
		write_value(json[xs("visuals")][xs("bloom_value")], visuals::bloom_value);

		write_value(json[xs("movehelp")][xs("robot_strafe")], movehelp::robot_strafe);
		write_value(json[xs("movehelp")][xs("tekno_strafe")], movehelp::tekno_strafe);

		write_value(json[xs("movehelp")][xs("dynamic_surf_jump")], movehelp::dynamic_surf_jump);

		write_value(json[xs("visuals")][xs("notifcation_system")], visuals::notifcation_system);
		write_value(json[xs("visuals")][xs("movement_noti")], visuals::movement_noti);

		write_value(json[xs("movehelp")][xs("force_custom_tick")], movehelp::force_custom_tick);

		write_value(json[xs("movehelp")][xs("headbang_autoduck")], movehelp::headbang_autoduck);
		write_value(json[xs("movehelp")][xs("AI_scan_time")], movehelp::AI_scan_time);
		write_value(json[xs("movehelp")][xs("pixelsurf_lock_amount")], movehelp::pixelsurf_lock_amount);
		write_value(json[xs("movehelp")][xs("headbounce_lock")], movehelp::headbounce_lock);
		write_value(json[xs("movehelp")][xs("headbounce_lock_amount")], movehelp::headbounce_lock_amount);

		write_value(json[xs("movement")][xs("velocity_indicator_sync_shadow")], movement::velocity_indicator_sync_shadow);
		write_value(json[xs("movement")][xs("velocity_indicator_sync_shadow[0]")], movement::velocity_shadow_color[0]);
		write_value(json[xs("movement")][xs("velocity_indicator_sync_shadow[1]")], movement::velocity_shadow_color[1]);
		write_value(json[xs("movement")][xs("velocity_indicator_sync_shadow[2]")], movement::velocity_shadow_color[2]);
		write_value(json[xs("movement")][xs("velocity_indicator_sync_shadow[3]")], movement::velocity_shadow_color[3]);
		write_value(json[xs("movement")][xs("velocity_indicator_sync_shadow[4]")], movement::velocity_shadow_color[4]);

		write_value(json[xs("movehelp")][xs("push_notice")], movehelp::push_notice);
		write_value(json[xs("movehelp")][xs("predict_scale")], movehelp::predict_scale);
		write_value(json[xs("movehelp")][xs("low_power_mode")], movehelp::low_power_mode);

		write_value(json[xs("movehelp")][xs("low_power_mode")], movehelp::dynamic_surf_stamina);
		write_value(json[xs("aimbot")][xs("auto_accurate_walk")], aimbot::auto_accurate_walk);

		write_value(json[xs("movehelp")][xs("surf_ticks")], movehelp::surf_ticks);
		write_value(json[xs("movehelp")][xs("surf_move_ticks")], movehelp::surf_move_ticks);
		write_value(json[xs("movehelp")][xs("surf_align_ticks")], movehelp::surf_align_ticks);
		write_value(json[xs("movehelp")][xs("jumpbug_autojump")], movehelp::jumpbug_autojump);
		write_value(json[xs("movehelp")][xs("headbang_indicator")], movehelp::headbang_indicator);

		write_value(json[xs("visuals")][xs("dropped_weapon_icon")], visuals::dropped_weapon_icon);
		write_value(json[xs("visuals")][xs("dropped_weapon_name")], visuals::dropped_weapon_name);

		write_value(json[xs("misc")][xs("remove_view_interp")], misc::remove_view_interp);
		write_value(json[xs("misc")][xs("cs_two_model")], misc::cs_two_model);

		write_value(json[xs("movement")][xs("null_strafing")], movement::null_strafing);

		write_value(json[xs("visuals")][xs("remove_phong")], visuals::remove_phong);
		write_value(json[xs("visuals")][xs("remove_envmap")], visuals::remove_envmap);
		write_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_reflectivity);
		write_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_bumpmap);
		write_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_ssbump);
		write_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_selfillum);
		write_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_lightwarptexture);
		write_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_halflambert);
		write_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_ambientocclusion);
		write_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_rimlight);
		write_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_ignorez);
		write_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_alphatest);
		write_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_softwareskin);



		write_value(json[xs("visuals")][xs("playerweapon_name")], visuals::playerweapon_name);
		write_value(json[xs("visuals")][xs("playerweapon_icon")], visuals::playerweapon_icon);
		write_value(json[xs("movehelp")][xs("free_angle")], movehelp::free_angle);

		write_value(json[xs("route_node")][xs("enable_route_node")], route_node::enable_route_node);
		write_value(json[xs("route_node")][xs("similar_route_time")], route_node::similar_route_time);
		write_value(json[xs("route_node")][xs("route_node_scan_distance")], route_node::route_node_scan_distance);
		write_value(json[xs("route_node")][xs("route_node_best_dot")], route_node::route_node_best_dot);
		write_value(json[xs("route_node")][xs("route_node_align_distance")], route_node::route_node_align_distance);
		write_value(json[xs("route_node")][xs("route_node_align_speed")], route_node::route_node_align_speed);
		write_value(json[xs("route_node")][xs("route_node_teleport_surf_ticks")], route_node::route_node_teleport_surf_ticks);
		write_value(json[xs("route_node")][xs("route_node_end_lift")], route_node::route_node_end_lift);


		write_value(json[xs("c_strafe")][xs("strafe_ui_enable")], c_strafe::strafe_ui_enable);
		write_value(json[xs("c_strafe")][xs("strafe_enable")], c_strafe::strafe_enable);
		write_value(json[xs("c_strafe")][xs("strafe_angle")], c_strafe::strafe_angle);
		write_value(json[xs("c_strafe")][xs("strafe_smooth")], c_strafe::strafe_smooth);
		write_value(json[xs("c_strafe")][xs("strafe_deaccel")], c_strafe::strafe_deaccel);
		write_value(json[xs("c_strafe")][xs("correct_move_val")], c_strafe::correct_move_val);
		write_value(json[xs("c_strafe")][xs("calculated_speed_gain")], c_strafe::calculated_speed_gain);


		write_value(json[xs("backtrack")][xs("backtrack_choke_ticks")], backtrack::backtrack_choke_ticks);

		write_value(json[xs("movement")][xs("in_air_align_fix")], movement::in_air_align_fix);
		write_value(json[xs("movement")][xs("on_ground_align_fix")], movement::on_ground_align_fix);
		write_value(json[xs("movement")][xs("lj_jb_miss")], movement::lj_jb_miss);

		write_value(json[xs("movement")][xs("surf_strafe")], movement::surf_strafe);
		write_value(json[xs("movement")][xs("align_strafe")], movement::align_strafe);
		write_value(json[xs("movement")][xs("p_strafe")], movement::p_strafe);

		write_value(json[xs("movement")][xs("surf_jump")], movement::surf_jump);
		write_value(json[xs("movement")][xs("robot_strafing")], movement::robot_strafing);

		write_value(json[xs("movement")][xs("ladderjump_ej")], movement::ladderjump_ej);
		write_value(json[xs("movement")][xs("ladderjump_mj")], movement::ladderjump_mj);
		write_value(json[xs("movement")][xs("ladderjump_lj")], movement::ladderjump_lj);


		write_value(json[xs("movement")][xs("jumpbug_quickstop")], movement::jumpbug_quickstop);
		write_value(json[xs("movement")][xs("jumpbug_ladderbug")], movement::jumpbug_ladderbug);

		write_value(json[xs("movement")][xs("pixelsurf_ticks")], movement::pixelsurf_ticks);

		write_value(json[xs("movement")][xs("enable_strafe_type")], movement::enable_strafe_type);
		write_value(json[xs("movement")][xs("strafe_type")], movement::strafe_type);

		write_value(json[xs("movement")][xs("css_hop")], movement::css_hop);
		write_value(json[xs("movement")][xs("perfect_bhop")], movement::perfect_bhop);

		write_value(json[xs("movement")][xs("auto_headbang")], movement::auto_headbang);
		write_value(json[xs("movement")][xs("headbang_scan")], movement::headbang_scan);

		write_value(json[xs("movement")][xs("lock_aggression")], movement::lock_aggression);
		write_value(json[xs("movement")][xs("unduck_after_mj")], movement::unduck_after_mj);
		write_value(json[xs("movement")][xs("unduck_after_lj")], movement::unduck_after_lj);

		write_value(json[xs("movement")][xs("billware_wm")], movement::billware_wm);
		write_value(json[xs("movement")][xs("pixel_surf_fix")], movement::pixel_surf_fix);
		write_value(json[xs("visuals")][xs("glow_style")], visuals::glow_style);

		write_value(json[xs("movement")][xs("velocity_slam")], movement::velocity_slam);
		write_value(json[xs("movement")][xs("strafe_limiter")], movement::strafe_limiter);
		write_value(json[xs("movement")][xs("smooth_land")], movement::smooth_land);
		write_value(json[xs("movement")][xs("movement_unlocker")], movement::movement_unlocker);

		write_value(json[xs("movement")][xs("nulls_angle")], movement::nulls_angle);
		write_value(json[xs("movement")][xs("nulls_forward_back")], movement::nulls_forward_back);

		write_value(json[xs("movement")][xs("jump_to_longjump")], movement::jump_to_longjump);
		write_value(json[xs("movement")][xs("jump_to_longjump_distance")], movement::jump_to_longjump_distance);
		write_value(json[xs("movement")][xs("jump_to_longjump_speed")], movement::jump_to_longjump_speed);

		write_value(json[xs("movement")][xs("max_render_distance")], movement::max_render_distance);

		write_value(json[xs("movement")][xs("add_floor_key")], movement::add_floor_key);
		write_value(json[xs("movement")][xs("add_floor_key_s")], movement::add_floor_key_s);
		write_value(json[xs("movement")][xs("add_pixelsurf_key")], movement::add_pixelsurf_key);
		write_value(json[xs("movement")][xs("add_pixelsurf_key_s")], movement::add_pixelsurf_key_s);
		write_value(json[xs("movement")][xs("delete_point_key")], movement::delete_point_key);
		write_value(json[xs("movement")][xs("delete_point_key_s")], movement::delete_point_key_s);

		write_value(json[xs("movement")][xs("pscalc_enable")], movement::pscalc_enable);
		write_value(json[xs("movement")][xs("pscalc_jump_to_pixelsurf")], movement::pscalc_jump_to_pixelsurf);
		write_value(json[xs("movement")][xs("pscalc_drawpoint_key")], movement::pscalc_drawpoint_key);
		write_value(json[xs("movement")][xs("pscalc_drawpoint_key_s")], movement::pscalc_drawpoint_key_s);
		write_value(json[xs("movement")][xs("pscalc_clear_all_key")], movement::pscalc_clear_all_key);
		write_value(json[xs("movement")][xs("pscalc_clear_all_key_s")], movement::pscalc_clear_all_key_s);
		write_value(json[xs("movement")][xs("pscalc_jump_to_marker_key")], movement::pscalc_jump_to_marker_key);
		write_value(json[xs("movement")][xs("pscalc_jump_to_marker_key_s")], movement::pscalc_jump_to_marker_key_s);
		write_value(json[xs("misc")][xs("view_model_fov")], misc::view_model_fov);

		write_value(json[xs("visuals")][xs("cham_lights")], visuals::cham_lights);
		write_value(json[xs("visuals")][xs("backtrack_cham_lights")], visuals::backtrack_cham_lights);
		write_value(json[xs("visuals")][xs("enable_darkmode")], visuals::enable_darkmode);
		write_value(json[xs("visuals")][xs("darkmode_val")], visuals::darkmode_val);
		write_value(json[xs("movement")][xs("edge_bug_slide_amount")], movement::edge_bug_slide_amount);

		write_value(json[xs("movement")][xs("pscalc_scan_distance")], movement::pscalc_scan_distance);
		write_value(json[xs("movement")][xs("pscalc_chain_targets")], movement::pscalc_chain_targets);
		write_value(json[xs("movement")][xs("pscalc_scan_ticks")], movement::pscalc_scan_ticks);
		write_value(json[xs("movement")][xs("pscalc_model_b_stage_ticks")], movement::pscalc_model_b_stage_ticks);
		write_value(json[xs("movement")][xs("pscalc_model_b_total_ticks")], movement::pscalc_model_b_total_ticks);
		write_value(json[xs("movement")][xs("pscalc_model_b_max_nodes")], movement::pscalc_model_b_max_nodes);
		write_value(json[xs("movement")][xs("pscalc_model_b_combos_per_frame")], movement::pscalc_model_b_combos_per_frame);
		write_value(json[xs("movement")][xs("pscalc_exact_stage_mode")], movement::pscalc_exact_stage_mode);
		write_value(json[xs("movement")][xs("pscalc_no_first_stage_prune")], movement::pscalc_no_first_stage_prune);
		write_value(json[xs("movement")][xs("pscalc_min_prune_ticks")], movement::pscalc_min_prune_ticks);
		write_value(json[xs("movement")][xs("pscalc_z_tolerance")], movement::pscalc_z_tolerance);
		write_value(json[xs("movement")][xs("pscalc_lax_z_tolerance")], movement::pscalc_lax_z_tolerance);
		write_value(json[xs("movement")][xs("pscalc_log_stage_debug")], movement::pscalc_log_stage_debug);
		write_value(json[xs("movement")][xs("pscalc_allow_jump")], movement::pscalc_allow_jump);
		write_value(json[xs("movement")][xs("pscalc_allow_crouch_jump")], movement::pscalc_allow_crouch_jump);
		write_value(json[xs("movement")][xs("pscalc_allow_minijump")], movement::pscalc_allow_minijump);
		write_value(json[xs("movement")][xs("pscalc_allow_longjump")], movement::pscalc_allow_longjump);
		write_value(json[xs("movement")][xs("pscalc_allow_jumpbug")], movement::pscalc_allow_jumpbug);
		write_value(json[xs("movement")][xs("pscalc_max_displayed_combos")], movement::pscalc_max_displayed_combos);
		write_value(json[xs("movement")][xs("pscalc_debug_output")], movement::pscalc_debug_output);
		write_value(json[xs("movement")][xs("pscalc_finder_enable")], movement::pscalc_finder_enable);
		write_value(json[xs("movement")][xs("pscalc_finder_draw")], movement::pscalc_finder_draw);
		write_value(json[xs("movement")][xs("pscalc_finder_radius")], movement::pscalc_finder_radius);
		write_value(json[xs("movement")][xs("pscalc_finder_max_points")], movement::pscalc_finder_max_points);
		write_value(json[xs("movement")][xs("edgebug_predrounds")], movement::edgebug_predrounds);
		write_value(json[xs("visuals")][xs("backtrack_dot")], visuals::backtrack_dot);
		write_value(json[xs("visuals")][xs("backtrack_dot_clr[0]")], visuals::backtrack_dot_clr[0]);
		write_value(json[xs("visuals")][xs("backtrack_dot_clr[1]")], visuals::backtrack_dot_clr[1]);
		write_value(json[xs("visuals")][xs("backtrack_dot_clr[2]")], visuals::backtrack_dot_clr[2]);
		write_value(json[xs("visuals")][xs("backtrack_dot_clr[3]")], visuals::backtrack_dot_clr[3]);

		write_value(json[xs("movement")][xs("ps_ticks")], movement::ps_ticks);
		write_value(json[xs("visuals")][xs("enable_removals")], visuals::enable_removals);
		write_value(json[xs("movehelp")][xs("indicatorEnable")], movehelp::indicatorEnable);

		write_value(json[xs("visuals")][xs("custom_shaders")], visuals::custom_shaders);

		write_value(json[xs("movement")][xs("headbounce_fix")], movement::headbounce_fix);

		write_value(json[xs("movement")][xs("edgescanner")], movement::edgescanner);
		write_value(json[xs("movement")][xs("pixelladder")], movement::pixelladder);
		write_value(json[xs("movement")][xs("pixelladder_scan")], movement::pixelladder_scan);

		write_value(json[xs("movement")][xs("edgescanner_scan")], movement::edgescanner_scan);
		write_value(json[xs("movement")][xs("edgescanner_assist")], movement::edgescanner_assist);
		write_value(json[xs("movement")][xs("jump_to_surf_scan")], movement::jump_to_surf_scan);

		write_value(json[xs("movement")][xs("airstuck_enabled")], movement::airstuck_enabled);
		write_value(json[xs("movement")][xs("airstuck_indicator")], movement::airstuck_indicator);
		write_value(json[xs("movement")][xs("airstuck_check_ticks")], movement::airstuck_check_ticks);

		write_value(json[xs("movement")][xs("jumpbug_scan")], movement::jumpbug_scan);
		write_value(json[xs("movement")][xs("gain_helper")], movement::gain_helper);

		write_value(json[xs("movement")][xs("edgebug_rage")], movement::edgebug_rage);

		write_value(json[xs("movehelp")][xs("pixelladder_indicator")], movehelp::pixelladder_indicator);

		write_value(json[xs("movement")][xs("enable_angle_limit")], movement::enable_angle_limit);
		write_value(json[xs("movement")][xs("edgebug_angle_limit")], movement::edgebug_angle_limit);
		write_value(json[xs("movement")][xs("pscalc_enable_draw")], movement::pscalc_enable_draw);

		write_value(json[xs("movement")][xs("bhopfix")], movement::bhopfix);

		write_value(json[xs("movehelp")][xs("ast_indicator")], movehelp::ast_indicator);

		write_value(json[xs("assist")][xs("assist")], assist::assist);
		write_value(json[xs("assist")][xs("pixelsurf_assist")], assist::pixelsurf_assist);
		write_value(json[xs("assist")][xs("bounce_assist")], assist::bounce_assist);
		write_value(json[xs("assist")][xs("assist_broke_hop")], assist::assist_broke_hop);
		write_value(json[xs("assist")][xs("assist_stamina_value")], assist::assist_stamina_value);
		write_value(json[xs("assist")][xs("assist_render")], assist::assist_render);
		write_value(json[xs("assist")][xs("bounce_assist_render")], assist::bounce_assist_render);
		write_value(json[xs("assist")][xs("assist_render_style")], assist::assist_render_style);
		write_value(json[xs("assist")][xs("assist_bounce_broke_hop")], assist::assist_bounce_broke_hop);
		write_value(json[xs("assist")][xs("pixelsurf_assist_ticks")], assist::pixelsurf_assist_ticks);
		write_value(json[xs("assist")][xs("pixelsurf_max_points")], assist::pixelsurf_max_points);
		write_value(json[xs("assist")][xs("pixelsurf_assist_type")], assist::pixelsurf_assist_type);
		write_value(json[xs("assist")][xs("pixelsurf_assist_key")], assist::pixelsurf_assist_key);
		write_value(json[xs("assist")][xs("pixelsurf_assist_key_s")], assist::pixelsurf_assist_key_s);
		write_value(json[xs("assist")][xs("pixelsurf_line_key")], assist::pixelsurf_line_key);
		write_value(json[xs("assist")][xs("pixelsurf_line_key_s")], assist::pixelsurf_line_key_s);
		write_value(json[xs("assist")][xs("pixelsurf_point_key")], assist::pixelsurf_point_key);
		write_value(json[xs("assist")][xs("pixelsurf_point_key_s")], assist::pixelsurf_point_key_s);
		write_value(json[xs("assist")][xs("bounce_assist_key")], assist::bounce_assist_key);
		write_value(json[xs("assist")][xs("bounce_assist_key_s")], assist::bounce_assist_key_s);
		write_value(json[xs("assist")][xs("bounce_point_key")], assist::bounce_point_key);
		write_value(json[xs("assist")][xs("bounce_point_key_s")], assist::bounce_point_key_s);

		write_value(json[xs("movement")][xs("auto_duck_collision")], movement::auto_duck_collision);
		write_value(json[xs("movement")][xs("auto_duck_collision_key")], movement::auto_duck_collision_key);
		write_value(json[xs("movement")][xs("auto_duck_collision_key_s")], movement::auto_duck_collision_key_s);

		write_value(json[xs("movement")][xs("silent_edgebug")], movement::silent_edgebug);

		write_value(json[xs("movement")][xs("jetpack_to_surf")], movement::jetpack_to_surf);
		write_value(json[xs("movement")][xs("jetpack_to_surf_key")], movement::jetpack_to_surf_key);
		write_value(json[xs("movement")][xs("jetpack_to_surf_key_s")], movement::jetpack_to_surf_key_s);

		write_value(json[xs("movehelp")][xs("air_indicator")], movehelp::air_indicator);

		write_value(json[xs("movement")][xs("airstuck_key")], movement::airstuck_key);
		write_value(json[xs("movement")][xs("airstuck_key_s")], movement::airstuck_key_s);
		write_value(json[xs("movement")][xs("points_system")], movement::points_system);
		write_value(json[xs("visuals")][xs("weather_type")], visuals::weather_type);
		write_value(json[xs("visuals")][xs("enable_weather")], visuals::enable_weather);
		write_value(json[xs("visuals")][xs("weather_use_entity_precip")], visuals::weather_use_entity_precip);
		write_value(json[xs("visuals")][xs("weather_overlay")], visuals::weather_overlay);
		write_value(json[xs("visuals")][xs("weather_overlay_density")], visuals::weather_overlay_density);
		write_value(json[xs("visuals")][xs("weather_overlay_speed")], visuals::weather_overlay_speed);
		write_value(json[xs("visuals")][xs("weather_overlay_opacity")], visuals::weather_overlay_opacity);
		write_value(json[xs("visuals")][xs("weather_overlay_brightness")], visuals::weather_overlay_brightness);
		write_value(json[xs("visuals")][xs("weather_overlay_wind")], visuals::weather_overlay_wind);
		write_value(json[xs("visuals")][xs("weather_overlay_tint")], visuals::weather_overlay_tint);
		write_value(json[xs("visuals")][xs("style_points_background")], visuals::style_points_background);
		write_value(json[xs("visuals")][xs("watermark_use_custom_text_color")], visuals::watermark_use_custom_text_color);
		write_value(json[xs("visuals")][xs("watermark_text_color")], visuals::watermark_text_color);
		write_value(json[xs("visuals")][xs("watermark_use_custom_accent_color")], visuals::watermark_use_custom_accent_color);
		write_value(json[xs("visuals")][xs("watermark_accent_color")], visuals::watermark_accent_color);
		write_value(json[xs("visuals")][xs("watermark_text_outline")], visuals::watermark_text_outline);
		write_value(json[xs("visuals")][xs("watermark_text_shadow")], visuals::watermark_text_shadow);
		write_value(json[xs("visuals")][xs("watermark_text_glow_strength")], visuals::watermark_text_glow_strength);
		write_value(json[xs("visuals")][xs("watermark_text_thickness")], visuals::watermark_text_thickness);
		write_value(json[xs("visuals")][xs("points_hud_show_mascot")], visuals::points_hud_show_mascot);
		write_value(json[xs("visuals")][xs("points_hud_show_stars")], visuals::points_hud_show_stars);
		write_value(json[xs("visuals")][xs("points_hud_mascot_scale")], visuals::points_hud_mascot_scale);
		write_value(json[xs("visuals")][xs("points_hud_style")], visuals::points_hud_style);
		write_value(json[xs("visuals")][xs("points_hud_glow")], visuals::points_hud_glow);
		write_value(json[xs("visuals")][xs("points_hud_trails")], visuals::points_hud_trails);
		write_value(json[xs("visuals")][xs("points_hud_glow_strength")], visuals::points_hud_glow_strength);
		write_value(json[xs("visuals")][xs("movement_hud_combo_tracker")], visuals::movement_hud_combo_tracker);
		write_value(json[xs("visuals")][xs("movement_hud_combo_multiplier")], visuals::movement_hud_combo_multiplier);
		write_value(json[xs("visuals")][xs("movement_hud_velocity_strip")], visuals::movement_hud_velocity_strip);
		write_value(json[xs("visuals")][xs("movement_hud_trick_flash")], visuals::movement_hud_trick_flash);
		write_value(json[xs("visuals")][xs("movement_hud_timing_indicator")], visuals::movement_hud_timing_indicator);
		write_value(json[xs("visuals")][xs("movement_hud_sound_tick")], visuals::movement_hud_sound_tick);
		write_value(json[xs("visuals")][xs("world_preset_index")], visuals::world_preset_index);
		write_value(json[xs("visuals")][xs("ui_panel_style")], visuals::ui_panel_style);
		write_value(json[xs("visuals")][xs("ui_accent_pulse")], visuals::ui_accent_pulse);
		write_value(json[xs("visuals")][xs("ui_overlay_grain")], visuals::ui_overlay_grain);
		write_value(json[xs("visuals")][xs("ui_overlay_grain_strength")], visuals::ui_overlay_grain_strength);
		write_value(json[xs("visuals")][xs("ui_line_weight")], visuals::ui_line_weight);

		write_value(json[xs("movement")][xs("fast_bhop")], movement::fast_bhop);
		write_value(json[xs("movement")][xs("chain_assist")], movement::chain_assist);
		write_value(json[xs("movement")][xs("chain_strafe_correction")], movement::chain_strafe_correction);
		write_value(json[xs("movement")][xs("auto_bounce")], movement::auto_bounce);
		write_value(json[xs("movement")][xs("ps_lock_amount")], movement::ps_lock_amount);

		write_value(json[xs("visuals")][xs("auto_bounce")], visuals::mb_video_adapter);
		write_value(json[xs("visuals")][xs("speclist_x")], visuals::speclist_x);
		write_value(json[xs("visuals")][xs("speclist_y")], visuals::speclist_y);
		write_value(json[xs("movement")][xs("velocity_indicator_show_ghost")], movement::velocity_indicator_show_ghost);
		write_value(json[xs("assist")][xs("ps_calc_route_key")], assist::ps_calc_route_key);
		write_value(json[xs("assist")][xs("ps_calc_route_key_s")], assist::ps_calc_route_key_s);

		write_value(json[xs("movehelp")][xs("pixelsurf_lock")], movehelp::pixelsurf_lock);

		write_value(json[xs("movement")][xs("ej_252pre")], movement::ej_252pre);
		write_value(json[xs("movement")][xs("lj_252pre")], movement::lj_252pre);
		write_value(json[xs("movement")][xs("mj_252pre")], movement::mj_252pre);

		write_value(json[xs("movement")][xs("ej_adaptive_nulls")], movement::ej_adaptive_nulls);
		write_value(json[xs("movement")][xs("lj_adaptive_nulls")], movement::lj_adaptive_nulls);
		write_value(json[xs("movement")][xs("mj_adaptive_nulls")], movement::mj_adaptive_nulls);

		write_value(json[xs("movement")][xs("marker_align")], movement::marker_align);
		write_value(json[xs("movement")][xs("alignment_rage")], movement::alignment_rage);
		write_value(json[xs("movement")][xs("alignment_smoothing")], movement::alignment_smoothing);
		write_value(json[xs("movement")][xs("jumpbug_angle")], movement::jumpbug_angle);

		write_value(json[xs("assist")][xs("assist_enable_pixelsurf_points")], assist::assist_enable_pixelsurf_points);
		write_value(json[xs("assist")][xs("assist_enable_bounce_points")], assist::assist_enable_bounce_points);
		write_value(json[xs("assist")][xs("assist_enable_headbang_points")], assist::assist_enable_headbang_points);
		write_value(json[xs("assist")][xs("assist_enable_floor_points")], assist::assist_enable_floor_points);
		write_value(json[xs("assist")][xs("assist_enable_ceiling_points")], assist::assist_enable_ceiling_points);

		write_value(json[xs("assist")][xs("simple_display")], assist::simple_display);

		write_value(json[xs("visuals")][xs("remove_original_model")], visuals::remove_original_model);
		write_value(json[xs("visuals")][xs("style_points_combo")], visuals::style_points_combo);
		write_value(json[xs("visuals")][xs("pixelsurf_db_enable")], visuals::pixelsurf_db_enable);
		write_value(json[xs("visuals")][xs("pixelsurf_db_draw_active")], visuals::pixelsurf_db_draw_active);
		write_value(json[xs("visuals")][xs("pixelsurf_db_draw_path")], visuals::pixelsurf_db_draw_path);
		write_value(json[xs("visuals")][xs("pixelsurf_db_load_csgomate")], visuals::pixelsurf_db_load_csgomate);
		write_value(json[xs("visuals")][xs("pixelsurf_db_max_paths")], visuals::pixelsurf_db_max_paths);
		write_value(json[xs("visuals")][xs("pixelsurf_db_point_spacing")], visuals::pixelsurf_db_point_spacing);
		write_value(json[xs("visuals")][xs("pixelsurf_db_line_thickness")], visuals::pixelsurf_db_line_thickness);
		write_value(json[xs("visuals")][xs("pixelsurf_db_clr[0]")], visuals::pixelsurf_db_clr[0]);
		write_value(json[xs("visuals")][xs("pixelsurf_db_clr[1]")], visuals::pixelsurf_db_clr[1]);
		write_value(json[xs("visuals")][xs("pixelsurf_db_clr[2]")], visuals::pixelsurf_db_clr[2]);
		write_value(json[xs("visuals")][xs("pixelsurf_db_clr[3]")], visuals::pixelsurf_db_clr[3]);
		write_value(json[xs("visuals")][xs("enable_chams")], visuals::enable_chams);
		write_value(json[xs("movement")][xs("fast_stop")], movement::fast_stop);
		write_value(json[xs("movement")][xs("advanced_radius")], movement::advanced_radius);
		write_value(json[xs("movement")][xs("watermark_nickname")], movement::watermark_nickname);
		write_value(json[xs("movement")][xs("watermark_type")], movement::watermark_type);
		write_value(json[xs("movement")][xs("velocity_indicator_type")], movement::velocity_indicator_type);
		write_value(json[xs("visuals")][xs("enable_backtrack_chams")], visuals::enable_backtrack_chams);

		write_value(json[xs("visuals")][xs("camera_fix_enabled")], visuals::camera_fix_enabled);
		write_value(json[xs("visuals")][xs("camera_smoothing")], visuals::camera_smoothing);
		write_value(json[xs("visuals")][xs("camera_angle_fix")], visuals::camera_angle_fix);
		write_value(json[xs("visuals")][xs("camera_teleport_fix")], visuals::camera_teleport_fix);
		write_value(json[xs("visuals")][xs("camrea_enable_smoothing")], visuals::camrea_enable_smoothing);

		path.append(configs.at(index));

		const bool has_json_in_filename = path.rfind((".json")) != std::string::npos;
		if (!has_json_in_filename) {
			path.append((".json"));
		}

		if (std::ofstream output_file{ path }; output_file.good()) {
			output_file << std::setw(4) << json;
		}

		path.erase(path.size() - configs.at(index).size() - 5);
	}

	void load(const std::size_t index) {
		path.append(configs.at(index));
		path.append((".json"));

		if (std::ifstream input_file{ path }; input_file.good()) {
			nlohmann::json json;
			input_file >> json;

			read_value(json[xs("visuals")][xs("camrea_enable_smoothing")], visuals::camrea_enable_smoothing);
			read_value(json[xs("visuals")][xs("camera_fix_enabled")], visuals::camera_fix_enabled);
			read_value(json[xs("visuals")][xs("camera_smoothing")], visuals::camera_smoothing);
			read_value(json[xs("visuals")][xs("camera_angle_fix")], visuals::camera_angle_fix);
			read_value(json[xs("visuals")][xs("camera_teleport_fix")], visuals::camera_teleport_fix);
			read_value(json[xs("visuals")][xs("enable_backtrack_chams")], visuals::enable_backtrack_chams);
			read_value(json[xs("visuals")][xs("enable_chams")], visuals::enable_chams);
			read_value(json[xs("movement")][xs("velocity_indicator_type")], movement::velocity_indicator_type);
			read_value(json[xs("movement")][xs("velocity_indicator_show_ghost")], movement::velocity_indicator_show_ghost);
			read_value(json[xs("movement")][xs("watermark_nickname")], movement::watermark_nickname);
			read_value(json[xs("movement")][xs("watermark_type")], movement::watermark_type);
			read_value(json[xs("movement")][xs("advanced_radius")], movement::advanced_radius);
			read_value(json[xs("movement")][xs("fast_stop")], movement::fast_stop);
			read_value(json[xs("visuals")][xs("style_points_combo")], visuals::style_points_combo);
			read_value(json[xs("visuals")][xs("pixelsurf_db_enable")], visuals::pixelsurf_db_enable);
			read_value(json[xs("visuals")][xs("pixelsurf_db_draw_active")], visuals::pixelsurf_db_draw_active);
			read_value(json[xs("visuals")][xs("pixelsurf_db_draw_path")], visuals::pixelsurf_db_draw_path);
			read_value(json[xs("visuals")][xs("pixelsurf_db_load_csgomate")], visuals::pixelsurf_db_load_csgomate);
			read_value(json[xs("visuals")][xs("pixelsurf_db_max_paths")], visuals::pixelsurf_db_max_paths);
			read_value(json[xs("visuals")][xs("pixelsurf_db_point_spacing")], visuals::pixelsurf_db_point_spacing);
			read_value(json[xs("visuals")][xs("pixelsurf_db_line_thickness")], visuals::pixelsurf_db_line_thickness);
			read_value(json[xs("visuals")][xs("pixelsurf_db_clr[0]")], visuals::pixelsurf_db_clr[0]);
			read_value(json[xs("visuals")][xs("pixelsurf_db_clr[1]")], visuals::pixelsurf_db_clr[1]);
			read_value(json[xs("visuals")][xs("pixelsurf_db_clr[2]")], visuals::pixelsurf_db_clr[2]);
			read_value(json[xs("visuals")][xs("pixelsurf_db_clr[3]")], visuals::pixelsurf_db_clr[3]);
			read_value(json[xs("movement")][xs("jumpbug_angle")], movement::jumpbug_angle);
			read_value(json[xs("assist")][xs("simple_display")], assist::simple_display);
			read_value(json[xs("visuals")][xs("remove_original_model")], visuals::remove_original_model);
			read_value(json[xs("movement")][xs("alignment_smoothing")], movement::alignment_smoothing);

			read_value(json[xs("assist")][xs("assist_enable_headbang_points")], assist::assist_enable_headbang_points);
			read_value(json[xs("assist")][xs("assist_enable_floor_points")], assist::assist_enable_floor_points);
			read_value(json[xs("assist")][xs("assist_enable_ceiling_points")], assist::assist_enable_ceiling_points);
			read_value(json[xs("assist")][xs("assist_enable_pixelsurf_points")], assist::assist_enable_pixelsurf_points);
			read_value(json[xs("assist")][xs("assist_enable_bounce_points")], assist::assist_enable_bounce_points);
			read_value(json[xs("movement")][xs("alignment_rage")], movement::alignment_rage);
			read_value(json[xs("movement")][xs("marker_align")], movement::marker_align);
			read_value(json[xs("movement")][xs("ej_adaptive_nulls")], movement::ej_adaptive_nulls);
			read_value(json[xs("movement")][xs("lj_adaptive_nulls")], movement::lj_adaptive_nulls);
			read_value(json[xs("movement")][xs("mj_adaptive_nulls")], movement::mj_adaptive_nulls);

			read_value(json[xs("movement")][xs("ej_252pre")], movement::ej_252pre);
			read_value(json[xs("movement")][xs("lj_252pre")], movement::lj_252pre);
			read_value(json[xs("movement")][xs("mj_252pre")], movement::mj_252pre);
			read_value(json[xs("visuals")][xs("style_points_background")], visuals::style_points_background);
			read_value(json[xs("visuals")][xs("watermark_use_custom_text_color")], visuals::watermark_use_custom_text_color);
			read_value(json[xs("visuals")][xs("watermark_text_color")], visuals::watermark_text_color);
			read_value(json[xs("visuals")][xs("watermark_use_custom_accent_color")], visuals::watermark_use_custom_accent_color);
			read_value(json[xs("visuals")][xs("watermark_accent_color")], visuals::watermark_accent_color);
			read_value(json[xs("visuals")][xs("watermark_text_outline")], visuals::watermark_text_outline);
			read_value(json[xs("visuals")][xs("watermark_text_shadow")], visuals::watermark_text_shadow);
			read_value(json[xs("visuals")][xs("watermark_text_glow_strength")], visuals::watermark_text_glow_strength);
			read_value(json[xs("visuals")][xs("watermark_text_thickness")], visuals::watermark_text_thickness);
			read_value(json[xs("visuals")][xs("points_hud_show_mascot")], visuals::points_hud_show_mascot);
			read_value(json[xs("visuals")][xs("points_hud_show_stars")], visuals::points_hud_show_stars);
			read_value(json[xs("visuals")][xs("points_hud_mascot_scale")], visuals::points_hud_mascot_scale);
			read_value(json[xs("visuals")][xs("points_hud_style")], visuals::points_hud_style);
			read_value(json[xs("visuals")][xs("points_hud_glow")], visuals::points_hud_glow);
			read_value(json[xs("visuals")][xs("points_hud_trails")], visuals::points_hud_trails);
			read_value(json[xs("visuals")][xs("points_hud_glow_strength")], visuals::points_hud_glow_strength);
			read_value(json[xs("visuals")][xs("movement_hud_combo_tracker")], visuals::movement_hud_combo_tracker);
			read_value(json[xs("visuals")][xs("movement_hud_combo_multiplier")], visuals::movement_hud_combo_multiplier);
			read_value(json[xs("visuals")][xs("movement_hud_velocity_strip")], visuals::movement_hud_velocity_strip);
			read_value(json[xs("visuals")][xs("movement_hud_trick_flash")], visuals::movement_hud_trick_flash);
			read_value(json[xs("visuals")][xs("movement_hud_timing_indicator")], visuals::movement_hud_timing_indicator);
			read_value(json[xs("visuals")][xs("movement_hud_sound_tick")], visuals::movement_hud_sound_tick);
			read_value(json[xs("movehelp")][xs("pixelsurf_lock")], movehelp::pixelsurf_lock);
			read_value(json[xs("assist")][xs("ps_calc_route_key")], assist::ps_calc_route_key);
			read_value(json[xs("assist")][xs("ps_calc_route_key_s")], assist::ps_calc_route_key_s);
			read_value(json[xs("movement")][xs("ps_lock_amount")], movement::ps_lock_amount);
			read_value(json[xs("visuals")][xs("speclist_x")], visuals::speclist_x);
			read_value(json[xs("visuals")][xs("speclist_y")], visuals::speclist_y);
			read_value(json[xs("visuals")][xs("auto_bounce")], visuals::mb_video_adapter);
			read_value(json[xs("movement")][xs("auto_bounce")], movement::auto_bounce);
			read_value(json[xs("movement")][xs("chain_assist")], movement::chain_assist);
			read_value(json[xs("movement")][xs("chain_strafe_correction")], movement::chain_strafe_correction);
			read_value(json[xs("movement")][xs("fast_bhop")], movement::fast_bhop);
			read_value(json[xs("movement")][xs("points_system")], movement::points_system);
			read_value(json[xs("movehelp")][xs("air_indicator")], movehelp::air_indicator);
			read_value(json[xs("movement")][xs("airstuck_key")], movement::airstuck_key);
			read_value(json[xs("movement")][xs("airstuck_key_s")], movement::airstuck_key_s);
			read_value(json[xs("visuals")][xs("weather_type")], visuals::weather_type);
			read_value(json[xs("visuals")][xs("enable_weather")], visuals::enable_weather);
			read_value(json[xs("visuals")][xs("weather_use_entity_precip")], visuals::weather_use_entity_precip);
			read_value(json[xs("visuals")][xs("weather_overlay")], visuals::weather_overlay);
			read_value(json[xs("visuals")][xs("weather_overlay_density")], visuals::weather_overlay_density);
			read_value(json[xs("visuals")][xs("weather_overlay_speed")], visuals::weather_overlay_speed);
			read_value(json[xs("visuals")][xs("weather_overlay_opacity")], visuals::weather_overlay_opacity);
			read_value(json[xs("visuals")][xs("weather_overlay_brightness")], visuals::weather_overlay_brightness);
			read_value(json[xs("visuals")][xs("weather_overlay_wind")], visuals::weather_overlay_wind);
			read_value(json[xs("visuals")][xs("weather_overlay_tint")], visuals::weather_overlay_tint);
			read_value(json[xs("visuals")][xs("world_preset_index")], visuals::world_preset_index);
			read_value(json[xs("visuals")][xs("ui_panel_style")], visuals::ui_panel_style);
			read_value(json[xs("visuals")][xs("ui_accent_pulse")], visuals::ui_accent_pulse);
			read_value(json[xs("visuals")][xs("ui_overlay_grain")], visuals::ui_overlay_grain);
			read_value(json[xs("visuals")][xs("ui_overlay_grain_strength")], visuals::ui_overlay_grain_strength);
			read_value(json[xs("visuals")][xs("ui_line_weight")], visuals::ui_line_weight);
			read_value(json[xs("movement")][xs("jetpack_to_surf")], movement::jetpack_to_surf);
			read_value(json[xs("movement")][xs("jetpack_to_surf_key")], movement::jetpack_to_surf_key);
			read_value(json[xs("movement")][xs("jetpack_to_surf_key_s")], movement::jetpack_to_surf_key_s);

			read_value(json[xs("movement")][xs("silent_edgebug")], movement::silent_edgebug);
			read_value(json[xs("movement")][xs("auto_duck_collision")], movement::auto_duck_collision);
			read_value(json[xs("movement")][xs("auto_duck_collision_key")], movement::auto_duck_collision_key);
			read_value(json[xs("movement")][xs("auto_duck_collision_key_s")], movement::auto_duck_collision_key_s);

			read_value(json[xs("assist")][xs("assist")], assist::assist);
			read_value(json[xs("assist")][xs("pixelsurf_assist")], assist::pixelsurf_assist);
			read_value(json[xs("assist")][xs("bounce_assist")], assist::bounce_assist);
			read_value(json[xs("assist")][xs("assist_broke_hop")], assist::assist_broke_hop);
			read_value(json[xs("assist")][xs("assist_stamina_value")], assist::assist_stamina_value);
			read_value(json[xs("assist")][xs("assist_render")], assist::assist_render);
			read_value(json[xs("assist")][xs("bounce_assist_render")], assist::bounce_assist_render);
			read_value(json[xs("assist")][xs("assist_render_style")], assist::assist_render_style);
			read_value(json[xs("assist")][xs("assist_bounce_broke_hop")], assist::assist_bounce_broke_hop);
			read_value(json[xs("assist")][xs("pixelsurf_assist_ticks")], assist::pixelsurf_assist_ticks);
			read_value(json[xs("assist")][xs("pixelsurf_max_points")], assist::pixelsurf_max_points);
			read_value(json[xs("assist")][xs("pixelsurf_assist_type")], assist::pixelsurf_assist_type);
			read_value(json[xs("assist")][xs("pixelsurf_assist_key")], assist::pixelsurf_assist_key);
			read_value(json[xs("assist")][xs("pixelsurf_assist_key_s")], assist::pixelsurf_assist_key_s);
			read_value(json[xs("assist")][xs("pixelsurf_line_key")], assist::pixelsurf_line_key);
			read_value(json[xs("assist")][xs("pixelsurf_line_key_s")], assist::pixelsurf_line_key_s);
			read_value(json[xs("assist")][xs("pixelsurf_point_key")], assist::pixelsurf_point_key);
			read_value(json[xs("assist")][xs("pixelsurf_point_key_s")], assist::pixelsurf_point_key_s);
			read_value(json[xs("assist")][xs("bounce_assist_key")], assist::bounce_assist_key);
			read_value(json[xs("assist")][xs("bounce_assist_key_s")], assist::bounce_assist_key_s);
			read_value(json[xs("assist")][xs("bounce_point_key")], assist::bounce_point_key);
			read_value(json[xs("assist")][xs("bounce_point_key_s")], assist::bounce_point_key_s);

			read_value(json[xs("movehelp")][xs("ast_indicator")], movehelp::ast_indicator);

			read_value(json[xs("movement")][xs("bhopfix")], movement::bhopfix);
			read_value(json[xs("movement")][xs("pscalc_enable_draw")], movement::pscalc_enable_draw);
			read_value(json[xs("movement")][xs("enable_angle_limit")], movement::enable_angle_limit);
			read_value(json[xs("movement")][xs("edgebug_angle_limit")], movement::edgebug_angle_limit);

			read_value(json[xs("movement")][xs("edgebug_rage")], movement::edgebug_rage);
			read_value(json[xs("movement")][xs("gain_helper")], movement::gain_helper);
			read_value(json[xs("movement")][xs("jumpbug_scan")], movement::jumpbug_scan);
			read_value(json[xs("movehelp")][xs("pixelladder_indicator")], movehelp::pixelladder_indicator);
			read_value(json[xs("movement")][xs("airstuck_enabled")], movement::airstuck_enabled);
			read_value(json[xs("movement")][xs("airstuck_indicator")], movement::airstuck_indicator);
			read_value(json[xs("movement")][xs("airstuck_check_ticks")], movement::airstuck_check_ticks);
			read_value(json[xs("movement")][xs("jump_to_surf_scan")], movement::jump_to_surf_scan);
			read_value(json[xs("movement")][xs("edgescanner_scan")], movement::edgescanner_scan);
			read_value(json[xs("movement")][xs("edgescanner_assist")], movement::edgescanner_assist);

			read_value(json[xs("movement")][xs("pixelladder_scan")], movement::pixelladder_scan);
			read_value(json[xs("movement")][xs("pixelladder")], movement::pixelladder);
			read_value(json[xs("movement")][xs("edgescanner")], movement::edgescanner);

			read_value(json[xs("movement")][xs("max_render_distance")], movement::max_render_distance);

			read_value(json[xs("movement")][xs("add_floor_key")], movement::add_floor_key);
			read_value(json[xs("movement")][xs("add_floor_key_s")], movement::add_floor_key_s);
			read_value(json[xs("movement")][xs("add_pixelsurf_key")], movement::add_pixelsurf_key);
			read_value(json[xs("movement")][xs("add_pixelsurf_key_s")], movement::add_pixelsurf_key_s);
			read_value(json[xs("movement")][xs("delete_point_key")], movement::delete_point_key);
			read_value(json[xs("movement")][xs("delete_point_key_s")], movement::delete_point_key_s);

			read_value(json[xs("visuals")][xs("custom_shaders")], visuals::custom_shaders);

			read_value(json[xs("movement")][xs("pscalc_jump_to_marker_key")], movement::pscalc_jump_to_marker_key);
			read_value(json[xs("movement")][xs("pscalc_jump_to_marker_key_s")], movement::pscalc_jump_to_marker_key_s);

			read_value(json[xs("movehelp")][xs("indicatorEnable")], movehelp::indicatorEnable);
			read_value(json[xs("visuals")][xs("enable_removals")], visuals::enable_removals);
			read_value(json[xs("movement")][xs("ps_ticks")], movement::ps_ticks);
			read_value(json[xs("movement")][xs("backtrack_dot_clr[0]")], visuals::backtrack_dot_clr[0]);
			read_value(json[xs("movement")][xs("backtrack_dot_clr[1]")], visuals::backtrack_dot_clr[1]);
			read_value(json[xs("movement")][xs("backtrack_dot_clr[2]")], visuals::backtrack_dot_clr[2]);
			read_value(json[xs("movement")][xs("backtrack_dot_clr[3]")], visuals::backtrack_dot_clr[3]);
			read_value(json[xs("movement")][xs("backtrack_dot")], visuals::backtrack_dot);
			read_value(json[xs("movement")][xs("edgebug_predrounds")], movement::edgebug_predrounds);
			read_value(json[xs("movement")][xs("pscalc_scan_distance")], movement::pscalc_scan_distance);
			read_value(json[xs("movement")][xs("pscalc_chain_targets")], movement::pscalc_chain_targets);
			read_value(json[xs("movement")][xs("pscalc_scan_ticks")], movement::pscalc_scan_ticks);
			read_value(json[xs("movement")][xs("pscalc_model_b_stage_ticks")], movement::pscalc_model_b_stage_ticks);
			read_value(json[xs("movement")][xs("pscalc_model_b_total_ticks")], movement::pscalc_model_b_total_ticks);
			read_value(json[xs("movement")][xs("pscalc_model_b_max_nodes")], movement::pscalc_model_b_max_nodes);
			read_value(json[xs("movement")][xs("pscalc_model_b_combos_per_frame")], movement::pscalc_model_b_combos_per_frame);
			read_value(json[xs("movement")][xs("pscalc_exact_stage_mode")], movement::pscalc_exact_stage_mode);
			read_value(json[xs("movement")][xs("pscalc_no_first_stage_prune")], movement::pscalc_no_first_stage_prune);
			read_value(json[xs("movement")][xs("pscalc_min_prune_ticks")], movement::pscalc_min_prune_ticks);
			read_value(json[xs("movement")][xs("pscalc_z_tolerance")], movement::pscalc_z_tolerance);
			read_value(json[xs("movement")][xs("pscalc_lax_z_tolerance")], movement::pscalc_lax_z_tolerance);
			read_value(json[xs("movement")][xs("pscalc_log_stage_debug")], movement::pscalc_log_stage_debug);
			read_value(json[xs("movement")][xs("pscalc_allow_jump")], movement::pscalc_allow_jump);
			read_value(json[xs("movement")][xs("pscalc_allow_crouch_jump")], movement::pscalc_allow_crouch_jump);
			read_value(json[xs("movement")][xs("pscalc_allow_minijump")], movement::pscalc_allow_minijump);
			read_value(json[xs("movement")][xs("pscalc_allow_longjump")], movement::pscalc_allow_longjump);
			read_value(json[xs("movement")][xs("pscalc_allow_jumpbug")], movement::pscalc_allow_jumpbug);
			read_value(json[xs("movement")][xs("pscalc_max_displayed_combos")], movement::pscalc_max_displayed_combos);
			read_value(json[xs("movement")][xs("pscalc_debug_output")], movement::pscalc_debug_output);
			read_value(json[xs("movement")][xs("pscalc_finder_enable")], movement::pscalc_finder_enable);
			read_value(json[xs("movement")][xs("pscalc_finder_draw")], movement::pscalc_finder_draw);
			read_value(json[xs("movement")][xs("pscalc_finder_radius")], movement::pscalc_finder_radius);
			read_value(json[xs("movement")][xs("pscalc_finder_max_points")], movement::pscalc_finder_max_points);
			read_value(json[xs("visuals")][xs("enable_darkmode")], visuals::enable_darkmode);
			read_value(json[xs("visuals")][xs("darkmode_val")], visuals::darkmode_val);
			read_value(json[xs("movement")][xs("edge_bug_slide_amount")], movement::edge_bug_slide_amount);
			read_value(json[xs("visuals")][xs("cham_lights")], visuals::cham_lights);
			read_value(json[xs("visuals")][xs("backtrack_cham_lights")], visuals::backtrack_cham_lights);

			read_value(json[xs("misc")][xs("view_model_fov")], misc::view_model_fov);
			read_value(json[xs("movement")][xs("pscalc_enable")], movement::pscalc_enable);
			read_value(json[xs("movement")][xs("pscalc_jump_to_pixelsurf")], movement::pscalc_jump_to_pixelsurf);
			read_value(json[xs("movement")][xs("pscalc_drawpoint_key")], movement::pscalc_drawpoint_key);
			read_value(json[xs("movement")][xs("pscalc_drawpoint_key_s")], movement::pscalc_drawpoint_key_s);
			read_value(json[xs("movement")][xs("pscalc_clear_all_key")], movement::pscalc_clear_all_key);
			read_value(json[xs("movement")][xs("pscalc_clear_all_key_s")], movement::pscalc_clear_all_key_s);

			read_value(json[xs("movement")][xs("jump_to_longjump")], movement::jump_to_longjump);
			read_value(json[xs("movement")][xs("jump_to_longjump_distance")], movement::jump_to_longjump_distance);
			read_value(json[xs("movement")][xs("jump_to_longjump_speed")], movement::jump_to_longjump_speed);
			read_value(json[xs("movement")][xs("nulls_angle")], movement::nulls_angle);
			read_value(json[xs("movement")][xs("nulls_forward_back")], movement::nulls_forward_back);
			read_value(json[xs("movement")][xs("movement_unlocker")], movement::movement_unlocker);

			read_value(json[xs("visuals")][xs("glow_style")], visuals::glow_style);
			read_value(json[xs("movement")][xs("pixel_surf_fix")], movement::pixel_surf_fix);
			read_value(json[xs("movement")][xs("billware_wm")], movement::billware_wm);
			read_value(json[xs("movement")][xs("unduck_after_lj")], movement::unduck_after_lj);
			read_value(json[xs("movement")][xs("unduck_after_mj")], movement::unduck_after_mj);
			read_value(json[xs("movement")][xs("lock_aggression")], movement::lock_aggression);
			read_value(json[xs("movement")][xs("headbang_scan")], movement::headbang_scan);
			read_value(json[xs("movement")][xs("auto_headbang")], movement::auto_headbang);
			read_value(json[xs("movement")][xs("perfect_bhop")], movement::perfect_bhop);
			read_value(json[xs("movement")][xs("css_hop")], movement::css_hop);
			read_value(json[xs("movement")][xs("enable_strafe_type")], movement::enable_strafe_type);
			read_value(json[xs("movement")][xs("strafe_type")], movement::strafe_type);

			read_value(json[xs("movement")][xs("pixelsurf_ticks")], movement::pixelsurf_ticks);

			read_value(json[xs("movement")][xs("jumpbug_quickstop")], movement::jumpbug_quickstop);
			read_value(json[xs("movement")][xs("jumpbug_ladderbug")], movement::jumpbug_ladderbug);

			read_value(json[xs("movement")][xs("ladderjump_ej")], movement::ladderjump_ej);
			read_value(json[xs("movement")][xs("ladderjump_mj")], movement::ladderjump_mj);
			read_value(json[xs("movement")][xs("ladderjump_lj")], movement::ladderjump_lj);

			read_value(json[xs("movement")][xs("robot_strafing")], movement::robot_strafing);
			read_value(json[xs("movement")][xs("surf_jump")], movement::surf_jump);
			read_value(json[xs("movement")][xs("surf_strafe")], movement::surf_strafe);
			read_value(json[xs("movement")][xs("align_strafe")], movement::align_strafe);
			read_value(json[xs("movement")][xs("p_strafe")], movement::p_strafe);
			read_value(json[xs("movement")][xs("lj_jb_miss")], movement::lj_jb_miss);
			read_value(json[xs("movement")][xs("on_ground_align_fix")], movement::on_ground_align_fix);
			read_value(json[xs("movement")][xs("in_air_align_fix")], movement::in_air_align_fix);
			read_value(json[xs("backtrack")][xs("backtrack_choke_ticks")], backtrack::backtrack_choke_ticks);

			read_value(json[xs("c_strafe")][xs("strafe_ui_enable")], c_strafe::strafe_ui_enable);
			read_value(json[xs("c_strafe")][xs("strafe_enable")], c_strafe::strafe_enable);
			read_value(json[xs("c_strafe")][xs("strafe_angle")], c_strafe::strafe_angle);
			read_value(json[xs("c_strafe")][xs("strafe_smooth")], c_strafe::strafe_smooth);
			read_value(json[xs("c_strafe")][xs("strafe_deaccel")], c_strafe::strafe_deaccel);
			read_value(json[xs("c_strafe")][xs("correct_move_val")], c_strafe::correct_move_val);
			read_value(json[xs("c_strafe")][xs("calculated_speed_gain")], c_strafe::calculated_speed_gain);

			read_value(json[xs("route_node")][xs("enable_route_node")], route_node::enable_route_node);
			read_value(json[xs("route_node")][xs("similar_route_time")], route_node::similar_route_time);
			read_value(json[xs("route_node")][xs("route_node_scan_distance")], route_node::route_node_scan_distance);
			read_value(json[xs("route_node")][xs("route_node_best_dot")], route_node::route_node_best_dot);
			read_value(json[xs("route_node")][xs("route_node_align_distance")], route_node::route_node_align_distance);
			read_value(json[xs("route_node")][xs("route_node_align_speed")], route_node::route_node_align_speed);
			read_value(json[xs("route_node")][xs("route_node_teleport_surf_ticks")], route_node::route_node_teleport_surf_ticks);
			read_value(json[xs("route_node")][xs("route_node_end_lift")], route_node::route_node_end_lift);

			read_value(json[xs("visuals")][xs("remove_phong")], visuals::remove_phong);
			read_value(json[xs("visuals")][xs("remove_envmap")], visuals::remove_envmap);
			read_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_reflectivity);
			read_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_bumpmap);
			read_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_ssbump);
			read_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_selfillum);
			read_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_lightwarptexture);
			read_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_halflambert);
			read_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_ambientocclusion);
			read_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_rimlight);
			read_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_ignorez);
			read_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_alphatest);
			read_value(json[xs("visuals")][xs("remove_reflectivity")], visuals::remove_softwareskin);
			read_value(json[xs("visuals")][xs("playerweapon_name")], visuals::playerweapon_name);
			read_value(json[xs("visuals")][xs("playerweapon_icon")], visuals::playerweapon_icon);
			read_value(json[xs("movehelp")][xs("free_angle")], movehelp::free_angle);


			read_value(json[xs("movement")][xs("null_strafing")], movement::null_strafing);


			read_value(json[xs("movement")][xs("null_strafing")], movement::null_strafing);

			read_value(json[xs("misc")][xs("remove_view_interp")], misc::remove_view_interp);
			read_value(json[xs("misc")][xs("cs_two_model")], misc::cs_two_model);

			read_value(json[xs("visuals")][xs("dropped_weapon_icon")], visuals::dropped_weapon_icon);
			read_value(json[xs("visuals")][xs("dropped_weapon_name")], visuals::dropped_weapon_name);

			read_value(json[xs("movehelp")][xs("edgebug_scan")], movehelp::edgebug_scan);
			read_value(json[xs("movehelp")][xs("advanced_edgebug")], movehelp::advanced_edgebug);
			read_value(json[xs("movehelp")][xs("advanced_edgebug_scan")], movehelp::advanced_edgebug_scan);

			read_value(json[xs("movehelp")][xs("fake_duck")], movehelp::fake_duck);

			read_value(json[xs("movehelp")][xs("jumpbug_autojump")], movehelp::jumpbug_autojump);
			read_value(json[xs("movehelp")][xs("surf_ticks")], movehelp::surf_ticks);
			read_value(json[xs("movehelp")][xs("surf_move_ticks")], movehelp::surf_move_ticks);
			read_value(json[xs("movehelp")][xs("surf_align_ticks")], movehelp::surf_align_ticks);


			read_value(json[xs("movehelp")][xs("low_power_mode")], movehelp::dynamic_surf_stamina);
			read_value(json[xs("aimbot")][xs("auto_accurate_walk")], aimbot::auto_accurate_walk);
			read_value(json[xs("movement")][xs("velocity_indicator_sync_shadow[0]")], movement::velocity_shadow_color[0]);
			read_value(json[xs("movement")][xs("velocity_indicator_sync_shadow[1]")], movement::velocity_shadow_color[1]);
			read_value(json[xs("movement")][xs("velocity_indicator_sync_shadow[2]")], movement::velocity_shadow_color[2]);
			read_value(json[xs("movement")][xs("velocity_indicator_sync_shadow[3]")], movement::velocity_shadow_color[3]);
			read_value(json[xs("movement")][xs("velocity_indicator_sync_shadow[4]")], movement::velocity_shadow_color[4]);

			read_value(json[xs("movehelp")][xs("push_notice")], movehelp::push_notice);
			read_value(json[xs("movehelp")][xs("predict_scale")], movehelp::predict_scale);
			read_value(json[xs("movehelp")][xs("low_power_mode")], movehelp::low_power_mode);

			read_value(json[xs("movehelp")][xs("pixelsurf_lock_amount")], movehelp::pixelsurf_lock_amount);
			read_value(json[xs("movehelp")][xs("headbounce_lock")], movehelp::headbounce_lock);
			read_value(json[xs("movehelp")][xs("headbounce_lock_amount")], movehelp::headbounce_lock_amount);
			read_value(json[xs("movehelp")][xs("AI_scan_time")], movehelp::AI_scan_time);
			read_value(json[xs("movehelp")][xs("headbang_autoduck")], movehelp::headbang_autoduck);
			read_value(json[xs("movehelp")][xs("headbang_indicator")], movehelp::headbang_indicator);

			read_value(json[xs("movehelp")][xs("force_custom_tick")], movehelp::force_custom_tick);

			read_value(json[xs("movehelp")][xs("dynamic_surf_jump")], movehelp::dynamic_surf_jump);

			read_value(json[xs("visuals")][xs("notifcation_system")], visuals::notifcation_system);
			read_value(json[xs("visuals")][xs("movement_noti")], visuals::movement_noti);
			read_value(json[xs("movehelp")][xs("tekno_strafe")], movehelp::tekno_strafe);
			read_value(json[xs("movehelp")][xs("robot_strafe")], movehelp::robot_strafe);

			read_value(json[xs("movehelp")][xs("extended_land")], movehelp::extended_land);
			read_value(json[xs("visuals")][xs("edit_bloom")], visuals::edit_bloom);
			read_value(json[xs("visuals")][xs("bloom_value")], visuals::bloom_value);

			read_value(json[xs("backtrack")][xs("aim_smoothing")], backtrack::aim_smoothing);
			read_value(json[xs("backtrack")][xs("aim_behavior_slider")], backtrack::aim_behavior_slider);

			read_value(json[xs("movehelp")][xs("ai_alignment")], movehelp::ai_alignment);
			read_value(json[xs("movehelp")][xs("ai_headbang")], movehelp::ai_headbang);
			read_value(json[xs("movehelp")][xs("ai_pixelsurf")], movehelp::ai_pixelsurf);
			read_value(json[xs("movehelp")][xs("ai_wallhop")], movehelp::ai_wallhop);
			read_value(json[xs("movehelp")][xs("pixelsurf_ticks")], movehelp::pixelsurf_ticks);
			read_value(json[xs("movehelp")][xs("wallhop_ticks")], movehelp::wallhop_ticks);
			read_value(json[xs("movehelp")][xs("headbang_ticks")], movehelp::headbang_ticks);
			read_value(json[xs("movehelp")][xs("alignment_ticks")], movehelp::alignment_ticks);
			read_value(json[xs("movehelp")][xs("alignment_angle_smooth")], movehelp::alignment_angle_smooth);
			read_value(json[xs("movehelp")][xs("alignment_velocity_smooth")], movehelp::alignment_velocity_smooth);
			read_value(json[xs("visuals")][xs("old_font")], visuals::old_font);

			read_value(json[xs("visuals")][xs("damage_indicator")], visuals::damage_indicator);
			read_value(json[xs("visuals")][xs("damage_indi_clr[0]")], visuals::damage_indi_clr[0]);
			read_value(json[xs("visuals")][xs("damage_indi_clr[1]")], visuals::damage_indi_clr[1]);
			read_value(json[xs("visuals")][xs("damage_indi_clr[2]")], visuals::damage_indi_clr[2]);

			read_value(json[xs("visuals")][xs("damage_indi_clr_shadow[0]")], visuals::damage_indi_clr_shadow[0]);
			read_value(json[xs("visuals")][xs("damage_indi_clr_shadow[1]")], visuals::damage_indi_clr_shadow[1]);
			read_value(json[xs("visuals")][xs("damage_indi_clr_shadow[2]")], visuals::damage_indi_clr_shadow[2]);

			read_value(json[xs("movehelp")][xs("edgebug_lock")], movehelp::edgebug_lock);
			read_value(json[xs("visuals")][xs("remove_3dsky")], visuals::remove_3dsky);
			read_value(json[xs("visuals")][xs("remove_shadows")], visuals::remove_shadows);
			read_value(json[xs("visuals")][xs("remove_post_processing")], visuals::remove_post_processing);
			read_value(json[xs("visuals")][xs("remove_panorama_blur")], visuals::remove_panorama_blur);

			read_value(json[xs("movehelp")][xs("auto_duck")], movehelp::auto_duck);

			read_value(json[xs("movehelp")][xs("strafe_assistance")], movehelp::strafe_assistance);
			read_value(json[xs("movehelp")][xs("smart_nulls")], movehelp::smart_nulls);
			read_value(json[xs("movehelp")][xs("smart_msl")], movehelp::smart_msl);
			read_value(json[xs("movehelp")][xs("frame_msl")], movehelp::frame_msl);

			read_value(json[xs("movehelp")][xs("pixelhop")], movehelp::pixelhop);

			read_value(json[xs("movehelp")][xs("auto_onehop")], movehelp::auto_onehop);
			read_value(json[xs("movehelp")][xs("onehop_key")], movehelp::onehop_key);
			read_value(json[xs("movehelp")][xs("onehop_key_s")], movehelp::onehop_key_s);


			read_value(json[xs("movehelp")][xs("edgebug_indicator")], movehelp::edgebug_indicator);
			read_value(json[xs("movehelp")][xs("longjump_indicator")], movehelp::longjump_indicator);
			read_value(json[xs("movehelp")][xs("minijump_indicator")], movehelp::minijump_indicator);
			read_value(json[xs("movehelp")][xs("edgejump_indicator")], movehelp::edgejump_indicator);
			read_value(json[xs("movehelp")][xs("jumpbug_indicator")], movehelp::jumpbug_indicator);
			read_value(json[xs("movehelp")][xs("pixelsurf_indicator")], movehelp::pixelsurf_indicator);
			read_value(json[xs("movehelp")][xs("ladderbug_indicator")], movehelp::ladderbug_indicator);
			read_value(json[xs("movehelp")][xs("fireman_indicator")], movehelp::fireman_indicator);
			read_value(json[xs("movehelp")][xs("detection_indicator")], movehelp::detection_indicator);

			read_value(json[xs("movehelp")][xs("surfjump")], movehelp::surfjump);
			read_value(json[xs("movehelp")][xs("surfjump_key")], movehelp::surfjump_key);
			read_value(json[xs("movehelp")][xs("surfjump_key_s")], movehelp::surfjump_key_s);

			read_value(json[xs("movehelp")][xs("surfmove")], movehelp::surfmove);




			read_value(json[xs("visuals")][xs("playerbackground")], visuals::playerbackground);




			read_value(json[xs("movement")][xs("wallhop")], movement::wallhop);
			read_value(json[xs("movement")][xs("headbang")], movement::headbang);
			read_value(json[xs("movement")][xs("pixel_helper")], movement::pixel_helper);

			read_value(json[xs("movement")][xs("land_on_surf")], movement::land_on_surf);
			read_value(json[xs("movement")][xs("in_air_surfix")], movement::in_air_surfix);

			read_value(json[xs("movement")][xs("wire_bounce")], movement::wire_bounce);
			read_value(json[xs("movement")][xs("auto_clip_quick_release")], movement::auto_clip_quick_release);
			read_value(json[xs("movement")][xs("stage_amount")], movement::stage_amount);
			read_value(json[xs("movement")][xs("stage_push")], movement::stage_push);

			read_value(json[xs("movement")][xs("ai_playback")], movement::ai_playback);
			read_value(json[xs("movement")][xs("ai_learnrate")], movement::ai_learnrate);

			read_value(json[xs("movement")][xs("gain_assist")], movement::gain_assist);
			read_value(json[xs("movement")][xs("bug_frequency")], movement::bug_frequency);
			read_value(json[xs("movement")][xs("auto_clip")], movement::auto_clip);
			read_value(json[xs("movement")][xs("edge_bug_lock_type")], movement::edge_bug_lock_type);
			read_value(json[xs("movement")][xs("smooth_mouse")], movement::smooth_mouse);
			read_value(json[xs("movement")][xs("post_auto_align")], movement::post_auto_align);
			read_value(json[xs("movement")][xs("edgebug_plane_z")], movement::edgebug_plane_z);

			read_value(json[xs("visuals")][xs("mbsmoothing")], visuals::mbsmoothing);
			read_value(json[xs("visuals")][xs("fog")], visuals::fog);
			read_value(json[xs("visuals")][xs("fog_color[0]")], visuals::fog_color[0]);
			read_value(json[xs("visuals")][xs("fog_color[1]")], visuals::fog_color[1]);
			read_value(json[xs("visuals")][xs("fog_color[2]")], visuals::fog_color[2]);
			read_value(json[xs("visuals")][xs("fog_color[3]")], visuals::fog_color[3]);
			read_value(json[xs("visuals")][xs("fog_density")], visuals::fog_density);
			read_value(json[xs("visuals")][xs("fog_distance")], visuals::fog_distance);

			read_value(json[xs("movement")][xs("shadow_indicator_custom_clr[0]")], movement::shadow_indicator_custom_clr[0]);
			read_value(json[xs("movement")][xs("shadow_indicator_custom_clr[1]")], movement::shadow_indicator_custom_clr[1]);
			read_value(json[xs("movement")][xs("shadow_indicator_custom_clr[2]")], movement::shadow_indicator_custom_clr[2]);
			read_value(json[xs("movement")][xs("shadow_indicator_custom_clr[3]")], movement::shadow_indicator_custom_clr[3]);

			read_value(json[xs("movement")][xs("indicators_fading_speed")], movement::indicators_fading_speed);

			read_value(json[xs("movement")][xs("fireman")], movement::fireman);
			read_value(json[xs("movement")][xs("fireman_key")], movement::fireman_key);
			read_value(json[xs("movement")][xs("fireman_key_s")], movement::fireman_key_s);

			read_value(json[xs("menu")][xs("fade_speed")], menu::fade_speed);
			read_value(json[xs("menu")][xs("menu_accent[0]")], menu::menu_accent[0]);
			read_value(json[xs("menu")][xs("menu_accent[1]")], menu::menu_accent[1]);
			read_value(json[xs("menu")][xs("menu_accent[2]")], menu::menu_accent[2]);
			read_value(json[xs("menu")][xs("menu_accent[3]")], menu::menu_accent[3]);
			read_value(json[xs("skins")][xs("skin_custom_clr")], skins::skin_custom_clr);
			read_value(json[("skins")][("skin_modulation1[0]")], skins::skin_modulation1[0]);
			read_value(json[("skins")][("skin_modulation1[1]")], skins::skin_modulation1[1]);
			read_value(json[("skins")][("skin_modulation1[2]")], skins::skin_modulation1[2]);
			read_value(json[("skins")][("skin_modulation2[0]")], skins::skin_modulation2[0]);
			read_value(json[("skins")][("skin_modulation2[1]")], skins::skin_modulation2[1]);
			read_value(json[("skins")][("skin_modulation2[2]")], skins::skin_modulation2[2]);
			read_value(json[("skins")][("skin_modulation3[0]")], skins::skin_modulation3[0]);
			read_value(json[("skins")][("skin_modulation3[1]")], skins::skin_modulation3[1]);
			read_value(json[("skins")][("skin_modulation3[2]")], skins::skin_modulation3[2]);
			read_value(json[("skins")][("skin_modulation4[0]")], skins::skin_modulation4[0]);
			read_value(json[("skins")][("skin_modulation4[1]")], skins::skin_modulation4[1]);
			read_value(json[("skins")][("skin_modulation4[2]")], skins::skin_modulation4[2]);
			read_value(json[xs("skins")][xs("agent_changer")], skins::agent_changer);
			read_value(json[xs("skins")][xs("agent_t")], skins::agent_t);
			read_value(json[xs("skins")][xs("agent_ct")], skins::agent_ct);
			read_value(json[xs("skins")][xs("knife_changer_enable")], skins::knife_changer_enable);
			read_value(json[xs("skins")][xs("knife_changer_wear")], skins::knife_changer_wear);
			read_value(json[xs("skins")][xs("knife_changer_model")], skins::knife_changer_model);
			read_value(json[xs("skins")][xs("knife_changer_vector_paint_kit")], skins::knife_changer_vector_paint_kit);
			read_value(json[xs("skins")][xs("knife_changer_paint_kit")], skins::knife_changer_paint_kit);
			read_value(json[xs("skins")][xs("gloves_endable")], skins::gloves_endable);
			read_value(json[xs("skins")][xs("gloves_model")], skins::gloves_model);
			read_value(json[xs("skins")][xs("gloves_skin")], skins::gloves_skin);
			read_value(json[xs("skins")][xs("gloves_skin_id")], skins::gloves_skin_id);
			read_value(json[xs("skins")][xs("gloves_wear")], skins::gloves_wear);
			read_value(json[xs("skins")][xs("weapon_endable")], skins::weapon_endable);
			read_value(json[xs("movement")][xs("bhop")], movement::bhop);
			read_value(json[xs("movement")][xs("auto_strafe")], movement::auto_strafe);
			read_value(json[xs("movement")][xs("auto_strafe_key")], movement::auto_strafe_key);
			read_value(json[xs("movement")][xs("auto_strafe_key_s")], movement::auto_strafe_key_s);
			read_value(json[xs("movement")][xs("strafe_optimizer")], movement::strafe_optimizer);
			read_value(json[xs("movement")][xs("strafe_optimizer_key")], movement::strafe_optimizer_key);
			read_value(json[xs("movement")][xs("strafe_optimizer_key_s")], movement::strafe_optimizer_key_s);
			read_value(json[xs("movement")][xs("strafe_optimizer_required_speed")], movement::strafe_optimizer_required_speed);
			read_value(json[xs("movement")][xs("strafe_optimizer_desired_gain")], movement::strafe_optimizer_desired_gain);
			read_value(json[xs("movement")][xs("mouse_strafe_limiter")], movement::mouse_strafe_limiter);
			read_value(json[xs("movement")][xs("mouse_strafe_limiter_affect_pitch")], movement::mouse_strafe_limiter_affect_pitch);
			read_value(json[xs("movement")][xs("mouse_strafe_limiter_key")], movement::mouse_strafe_limiter_key);
			read_value(json[xs("movement")][xs("mouse_strafe_limiter_key_s")], movement::mouse_strafe_limiter_key_s);
			read_value(json[xs("movement")][xs("mouse_strafe_limiter_value")], movement::mouse_strafe_limiter_value);
			read_value(json[xs("movement")][xs("edgejump")], movement::edgejump);
			read_value(json[xs("movement")][xs("edge_jump_on_ladder")], movement::edge_jump_on_ladder);
			read_value(json[xs("movement")][xs("ladder_bug")], movement::ladder_bug);
			read_value(json[xs("movement")][xs("ladder_bug_detection_printf")], movement::ladder_bug_detection_printf);
			read_value(json[xs("movement")][xs("ladder_bug_key")], movement::ladder_bug_key);
			read_value(json[xs("movement")][xs("ladder_bug_key_s")], movement::ladder_bug_key_s);
			read_value(json[xs("movement")][xs("edgejump_key")], movement::edgejump_key);
			read_value(json[xs("movement")][xs("edgejump_key_s")], movement::edgejump_key_s);
			read_value(json[xs("movement")][xs("long_jump_on_edge")], movement::long_jump_on_edge);
			read_value(json[xs("movement")][xs("mini_jump")], movement::mini_jump);
			read_value(json[xs("movement")][xs("mini_jump_key")], movement::mini_jump_key);
			read_value(json[xs("movement")][xs("jump_bug")], movement::jump_bug);
			read_value(json[xs("movement")][xs("jump_bug_key")], movement::jump_bug_key);
			read_value(json[xs("movement")][xs("crouch_bug")], movement::crouch_bug);
			read_value(json[xs("movement")][xs("crouch_bug_key")], movement::crouch_bug_key);
			read_value(json[xs("movement")][xs("crouch_bug_key_s")], movement::crouch_bug_key_s);
			read_value(json[xs("movement")][xs("edge_bug_priority")], movement::edge_bug_priority);
			read_value(json[xs("movement")][xs("edge_bug")], movement::edge_bug);
			read_value(json[xs("movement")][xs("visualize_edge_bug")], movement::visualize_edge_bug);
			read_value(json[xs("movement")][xs("edge_bug_advanced_search")], movement::edge_bug_advanced_search);
			read_value(json[xs("movement")][xs("edge_bug_key")], movement::edge_bug_key);
			read_value(json[xs("movement")][xs("edge_bug_ticks")], movement::edge_bug_ticks);
			read_value(json[xs("movement")][xs("edge_bug_angle_limit")], movement::edge_bug_angle_limit);
			read_value(json[xs("movement")][xs("edge_bug_rape")], movement::edge_bug_rape);
			read_value(json[xs("movement")][xs("edge_bug_strafe")], movement::edge_bug_strafe);
			read_value(json[xs("movement")][xs("delay_hop")], movement::delay_hop);
			read_value(json[xs("movement")][xs("delay_hop_key")], movement::delay_hop_key);
			read_value(json[xs("movement")][xs("stamina_hop_ticks")], movement::dh_tick);
			read_value(json[xs("movement")][xs("auto_duck")], movement::auto_duck);
			read_value(json[xs("movement")][xs("auto_duck_key")], movement::auto_duck_key);
			read_value(json[xs("movement")][xs("fastduck")], movement::fastduck);
			read_value(json[xs("movement")][xs("edge_bug_lock_amount")], movement::edge_bug_lock_amount);
			read_value(json[xs("movement")][xs("edge_bug_detection_printf")], movement::edge_bug_detection_printf);
			read_value(json[xs("movement")][xs("jump_bug_detection_printf")], movement::jump_bug_detection_printf);
			read_value(json[xs("movement")][xs("long_jump_detection_printf")], movement::long_jump_detection_printf);
			read_value(json[xs("movement")][xs("mini_jump_detection_printf")], movement::mini_jump_detection_printf);
			read_value(json[xs("movement")][xs("edge_bug_detection_sound")], movement::edge_bug_detection_sound);
			read_value(json[xs("movement")][xs("edge_bug_health_boost_effect")], movement::edge_bug_health_boost_effect);
			read_value(json[xs("movement")][xs("velocity_indicator")], movement::velocity_indicator);
			read_value(json[xs("movement")][xs("velocity_indicator_position")], movement::velocity_indicator_position);
			read_value(json[xs("movement")][xs("velocity_indicator_show_pre")], movement::velocity_indicator_show_pre);
			read_value(json[xs("movement")][xs("velocity_indicator_custom_color")], movement::velocity_indicator_custom_color);
			read_value(json[xs("movement")][xs("shadow_indicator_custom_clr2[0]")], movement::shadow_indicator_custom_clr2[0]);
			read_value(json[xs("movement")][xs("shadow_indicator_custom_clr2[1]")], movement::shadow_indicator_custom_clr2[1]);
			read_value(json[xs("movement")][xs("shadow_indicator_custom_clr2[2]")], movement::shadow_indicator_custom_clr2[2]);
			read_value(json[xs("movement")][xs("shadow_indicator_custom_clr2[3]")], movement::shadow_indicator_custom_clr2[3]);
			read_value(json[xs("movement")][xs("velocity_indicator_sync_shadow")], movement::velocity_indicator_sync_shadow);
			read_value(json[xs("movement")][xs("velocity_indicator_negative_clr[0]")], movement::velocity_indicator_negative_clr[0]);
			read_value(json[xs("movement")][xs("velocity_indicator_negative_clr[1]")], movement::velocity_indicator_negative_clr[1]);
			read_value(json[xs("movement")][xs("velocity_indicator_negative_clr[2]")], movement::velocity_indicator_negative_clr[2]);
			read_value(json[xs("movement")][xs("velocity_indicator_neutral_clr[0]")], movement::velocity_indicator_neutral_clr[0]);
			read_value(json[xs("movement")][xs("velocity_indicator_neutral_clr[1]")], movement::velocity_indicator_neutral_clr[1]);
			read_value(json[xs("movement")][xs("velocity_indicator_neutral_clr[2]")], movement::velocity_indicator_neutral_clr[2]);
			read_value(json[xs("movement")][xs("velocity_indicator_positive_clr[0]")], movement::velocity_indicator_positive_clr[0]);
			read_value(json[xs("movement")][xs("velocity_indicator_positive_clr[1]")], movement::velocity_indicator_positive_clr[1]);
			read_value(json[xs("movement")][xs("velocity_indicator_positive_clr[2]")], movement::velocity_indicator_positive_clr[2]);
			read_value(json[xs("movement")][xs("velocity_indicator_custom_clr[0]")], movement::velocity_indicator_custom_clr[0]);
			read_value(json[xs("movement")][xs("velocity_indicator_custom_clr[1]")], movement::velocity_indicator_custom_clr[1]);
			read_value(json[xs("movement")][xs("velocity_indicator_custom_clr[2]")], movement::velocity_indicator_custom_clr[2]);
			read_value(json[xs("movement")][xs("velocity_indicator_custom_clr[3]")], movement::velocity_indicator_custom_clr[3]);
			read_value(json[xs("movement")][xs("velocity_indicator_custom_clr2[0]")], movement::velocity_indicator_custom_clr2[0]);
			read_value(json[xs("movement")][xs("velocity_indicator_custom_clr2[1]")], movement::velocity_indicator_custom_clr2[1]);
			read_value(json[xs("movement")][xs("velocity_indicator_custom_clr2[2]")], movement::velocity_indicator_custom_clr2[2]);
			read_value(json[xs("movement")][xs("velocity_indicator_fade_clr3[0]")], movement::velocity_indicator_fade_clr3[0]);
			read_value(json[xs("movement")][xs("velocity_indicator_fade_clr3[1]")], movement::velocity_indicator_fade_clr3[1]);
			read_value(json[xs("movement")][xs("velocity_indicator_fade_clr3[2]")], movement::velocity_indicator_fade_clr3[2]);
			read_value(json[xs("movement")][xs("velocity_indicator_fade_clr3[3]")], movement::velocity_indicator_fade_clr3[3]);
			read_value(json[xs("movement")][xs("indicator_detect_clr[0]")], movement::indicator_detect_clr[0]);
			read_value(json[xs("movement")][xs("indicator_detect_clr[1]")], movement::indicator_detect_clr[1]);
			read_value(json[xs("movement")][xs("indicator_detect_clr[2]")], movement::indicator_detect_clr[2]);
			read_value(json[xs("movement")][xs("indicators_show[0]")], movement::indicators_show[0]);
			read_value(json[xs("movement")][xs("indicators_show[1]")], movement::indicators_show[1]);
			read_value(json[xs("movement")][xs("indicators_show[2]")], movement::indicators_show[2]);
			read_value(json[xs("movement")][xs("indicators_show[3]")], movement::indicators_show[3]);
			read_value(json[xs("movement")][xs("indicators_show[4]")], movement::indicators_show[4]);
			read_value(json[xs("movement")][xs("indicators_show[5]")], movement::indicators_show[5]);
			read_value(json[xs("movement")][xs("indicators_show[6]")], movement::indicators_show[6]);
			read_value(json[xs("movement")][xs("indicators_show[7]")], movement::indicators_show[7]);
			read_value(json[xs("movement")][xs("detection_clr_for[0]")], movement::detection_clr_for[0]);
			read_value(json[xs("movement")][xs("detection_clr_for[1]")], movement::detection_clr_for[1]);
			read_value(json[xs("movement")][xs("detection_clr_for[2]")], movement::detection_clr_for[2]);
			read_value(json[xs("movement")][xs("detection_clr_for[3]")], movement::detection_clr_for[3]);
			read_value(json[xs("movement")][xs("detection_clr_for[4]")], movement::detection_clr_for[4]);
			read_value(json[xs("movement")][xs("detection_clr_for[5]")], movement::detection_clr_for[5]);
			read_value(json[xs("movement")][xs("detection_clr_for[6]")], movement::detection_clr_for[6]);
			read_value(json[xs("movement")][xs("detection_clr_for[7]")], movement::detection_clr_for[7]);
			read_value(json[xs("movement")][xs("allow_detection_clr")], movement::allow_detection_clr);
			read_value(json[xs("movement")][xs("detection_saved_tick")], movement::detection_saved_tick);
			read_value(json[xs("movement")][xs("indicators_allow_animation")], movement::indicators_allow_animation);
			read_value(json[xs("movement")][xs("indicators_allow_move")], movement::indicators_allow_move);
			read_value(json[xs("movement")][xs("indicators_move_pos")], movement::indicators_move_pos);
			read_value(json[xs("movement")][xs("stamina_indicator")], movement::stamina_indicator);
			read_value(json[xs("movement")][xs("stamina_indicator")], movement::stamina_indicator);
			read_value(json[xs("movement")][xs("stamina_indicator_fade")], movement::stamina_indicator_fade);
			read_value(json[xs("movement")][xs("stamina_indicator_show_pre")], movement::stamina_indicator_show_pre);
			read_value(json[xs("movement")][xs("stamina_indicator_disable_ong_show_pre")], movement::stamina_indicator_disable_ong_show_pre);
			read_value(json[xs("movement")][xs("stamina_indicator_position")], movement::stamina_indicator_position);
			read_value(json[xs("movement")][xs("stamina_indicator_clr[0]")], movement::stamina_indicator_clr[0]);
			read_value(json[xs("movement")][xs("stamina_indicator_clr[1]")], movement::stamina_indicator_clr[1]);
			read_value(json[xs("movement")][xs("stamina_indicator_clr[2]")], movement::stamina_indicator_clr[2]);
			read_value(json[xs("movement")][xs("stamina_indicator_clr[3]")], movement::stamina_indicator_clr[3]);
			read_value(json[xs("movement")][xs("stamina_fade_clr[0]")], movement::stamina_fade_clr[0]);
			read_value(json[xs("movement")][xs("stamina_fade_clr[1]")], movement::stamina_fade_clr[1]);
			read_value(json[xs("movement")][xs("stamina_fade_clr[2]")], movement::stamina_fade_clr[2]);
			read_value(json[xs("movement")][xs("stamina_fade_clr[3]")], movement::stamina_fade_clr[3]);
			read_value(json[xs("movement")][xs("velocity_graph")], movement::velocity_graph);
			read_value(json[xs("movement")][xs("velocity_graph_color[0]")], movement::velocity_graph_color[0]);
			read_value(json[xs("movement")][xs("velocity_graph_color[1]")], movement::velocity_graph_color[1]);
			read_value(json[xs("movement")][xs("velocity_graph_color[2]")], movement::velocity_graph_color[2]);
			read_value(json[xs("movement")][xs("velocity_linewidth")], movement::velocity_linewidth);
			read_value(json[xs("movement")][xs("velocity_height")], movement::velocity_height);
			read_value(json[xs("movement")][xs("velocity_size")], movement::velocity_size);
			read_value(json[xs("movement")][xs("graph_xscreenpos")], movement::graph_xscreenpos);
			read_value(json[xs("movement")][xs("velocity_graph_show_landed_speed")], movement::velocity_graph_show_landed_speed);
			read_value(json[xs("movement")][xs("velocity_graph_show_edge_bug")], movement::velocity_graph_show_edge_bug);
			read_value(json[xs("movement")][xs("velocity_graph_show_jump_bug")], movement::velocity_graph_show_jump_bug);
			read_value(json[xs("movement")][xs("stamina_graph")], movement::stamina_graph);
			read_value(json[xs("movement")][xs("stamina_graph_color[0]")], movement::stamina_graph_color[0]);
			read_value(json[xs("movement")][xs("stamina_graph_color[1]")], movement::stamina_graph_color[1]);
			read_value(json[xs("movement")][xs("stamina_graph_color[2]")], movement::stamina_graph_color[2]);
			read_value(json[xs("movement")][xs("stamina_graph_offset")], movement::stamina_graph_offset);
			read_value(json[xs("movement")][xs("stamina_graph_height")], movement::stamina_graph_height);
			read_value(json[xs("movement")][xs("stamina_graph_offset_x")], movement::stamina_graph_offset_x);
			read_value(json[xs("movement")][xs("graph_fade")], movement::graph_fade);
			read_value(json[xs("misc")][xs("watermark")], misc::watermark);
			read_value(json[xs("misc")][xs("watermark_shadowtext_clr[0]")], misc::watermark_shadowtext_clr[0]);
			read_value(json[xs("misc")][xs("watermark_shadowtext_clr[1]")], misc::watermark_shadowtext_clr[1]);
			read_value(json[xs("misc")][xs("watermark_shadowtext_clr[2]")], misc::watermark_shadowtext_clr[2]);
			read_value(json[xs("misc")][xs("watermark_shadowtext_clr[3]")], misc::watermark_shadowtext_clr[3]);
			read_value(json[xs("misc")][xs("unlock_inventory")], misc::unlock_inventory);
			read_value(json[xs("misc")][xs("show_spotify_currently_playing")], misc::show_spotify_currently_playing);
			read_value(json[xs("misc")][xs("movement_rec")], misc::movement_rec);
			read_value(json[xs("misc")][xs("movement_rec_smoothing")], misc::movement_rec_smoothing);
			read_value(json[xs("misc")][xs("movement_rec_lockva")], misc::movement_rec_lockva);
			read_value(json[xs("misc")][xs("movement_rec_ringsize")], misc::movement_rec_ringsize);
			read_value(json[xs("misc")][xs("movement_rec_keystartrecord")], misc::movement_rec_keystartrecord);
			read_value(json[xs("misc")][xs("movement_rec_keystoprecord")], misc::movement_rec_keystoprecord);
			read_value(json[xs("misc")][xs("movement_rec_keystartplayback")], misc::movement_rec_keystartplayback);
			read_value(json[xs("misc")][xs("movement_rec_keystopplayback")], misc::movement_rec_keystopplayback);
			read_value(json[xs("misc")][xs("movement_rec_keyclearrecord")], misc::movement_rec_keyclearrecord);
			read_value(json[xs("misc")][xs("spectators_list")], misc::spectators_list);
			read_value(json[xs("misc")][xs("spectatorlist_type")], misc::spectatorlist_type);
			read_value(json[xs("misc")][xs("spectatorlist_show_target")], misc::spectatorlist_show_target);
			read_value(json[xs("misc")][xs("custom_console")], misc::custom_console);
			read_value(json[xs("misc")][xs("custom_console_clr[0]")], misc::custom_console_clr[0]);
			read_value(json[xs("misc")][xs("custom_console_clr[1]")], misc::custom_console_clr[1]);
			read_value(json[xs("misc")][xs("custom_console_clr[2]")], misc::custom_console_clr[2]);
			read_value(json[xs("misc")][xs("custom_console_clr[3]")], misc::custom_console_clr[3]);
			read_value(json[xs("misc")][xs("custom_console_clr2[0]")], misc::custom_console_clr2[0]);
			read_value(json[xs("misc")][xs("custom_console_clr2[1]")], misc::custom_console_clr2[1]);
			read_value(json[xs("misc")][xs("custom_console_clr2[2]")], misc::custom_console_clr2[2]);
			read_value(json[xs("misc")][xs("custom_console_clr2[3]")], misc::custom_console_clr2[3]);
			read_value(json[xs("misc")][xs("spectators_list_color_1[0]")], misc::spectators_list_color_1[0]);
			read_value(json[xs("misc")][xs("spectators_list_color_1[1]")], misc::spectators_list_color_1[1]);
			read_value(json[xs("misc")][xs("spectators_list_color_1[2]")], misc::spectators_list_color_1[2]);
			read_value(json[xs("misc")][xs("spectators_list_color_2[0]")], misc::spectators_list_color_2[0]);
			read_value(json[xs("misc")][xs("spectators_list_color_2[1]")], misc::spectators_list_color_2[1]);
			read_value(json[xs("misc")][xs("spectators_list_color_2[2]")], misc::spectators_list_color_2[2]);
			read_value(json[xs("misc")][xs("spectators_list_clr[0]")], misc::spectators_list_clr[0]);
			read_value(json[xs("misc")][xs("spectators_list_clr[1]")], misc::spectators_list_clr[1]);
			read_value(json[xs("misc")][xs("spectators_list_clr[2]")], misc::spectators_list_clr[2]);
			read_value(json[xs("misc")][xs("spectators_list_clr_2[0]")], misc::spectators_list_clr_2[0]);
			read_value(json[xs("misc")][xs("spectators_list_clr_2[1]")], misc::spectators_list_clr_2[1]);
			read_value(json[xs("misc")][xs("spectators_list_clr_2[2]")], misc::spectators_list_clr_2[2]);
			read_value(json[xs("misc")][xs("field_of_view")], misc::field_of_view);
			read_value(json[xs("misc")][xs("enable_fov")], misc::enable_fov);
			read_value(json[xs("misc")][xs("view_model")], misc::view_model);
			read_value(json[xs("misc")][xs("view_model_x")], misc::view_model_x);
			read_value(json[xs("misc")][xs("view_model_y")], misc::view_model_y);
			read_value(json[xs("misc")][xs("view_model_z")], misc::view_model_z);
			read_value(json[xs("misc")][xs("aspect_ratio")], misc::aspect_ratio);
			read_value(json[xs("misc")][xs("aspect_ratio_amount")], misc::aspect_ratio_amount);
			read_value(json[xs("misc")][xs("thirdperson")], misc::thirdperson);
			read_value(json[xs("misc")][xs("thirdperson_key")], misc::thirdperson_key);
			read_value(json[xs("misc")][xs("thirdperson_distance")], misc::thirdperson_distance);
			read_value(json[xs("misc")][xs("practice")], misc::practice);
			read_value(json[xs("misc")][xs("practice_window")], misc::practice_window);
			read_value(json[xs("misc")][xs("savepos")], misc::savepos);
			read_value(json[xs("misc")][xs("loadpos")], misc::loadpos);
			read_value(json[xs("misc")][xs("nextkey")], misc::nextkey);
			read_value(json[xs("misc")][xs("prevkey")], misc::prevkey);
			read_value(json[xs("misc")][xs("undokey")], misc::undokey);
			read_value(json[xs("misc")][xs("discord_rpc")], misc::discord_rpc);
			read_value(json[xs("misc")][xs("mouse_fix")], misc::mouse_fix);
			read_value(json[xs("misc")][xs("jumpstats")], misc::jumpstats);
			read_value(json[xs("misc")][xs("jumpstats_show_clr_fail")], misc::jumpstats_show_clr_fail);
			read_value(json[xs("misc")][xs("jumpstats_show_fail")], misc::jumpstats_show_fail);
			read_value(json[xs("misc")][xs("misc_reveal_ranks")], misc::misc_reveal_ranks);
			read_value(json[xs("misc")][xs("misc_hitmarker_sound_type")], misc::misc_hitmarker_sound_type);
			read_value(json[xs("misc")][xs("freecam")], misc::freecam);
			read_value(json[xs("misc")][xs("freecam_key")], misc::freecam_key);
			read_value(json[xs("visuals")][xs("skeletonesp_clr[0]")], visuals::skeletonesp_clr[0]);
			read_value(json[xs("visuals")][xs("skeletonesp_clr[1]")], visuals::skeletonesp_clr[1]);
			read_value(json[xs("visuals")][xs("skeletonesp_clr[2]")], visuals::skeletonesp_clr[2]);
			read_value(json[xs("visuals")][xs("skeletonbt_clr[0]")], visuals::skeletonbt_clr[0]);
			read_value(json[xs("visuals")][xs("skeletonbt_clr[1]")], visuals::skeletonbt_clr[1]);
			read_value(json[xs("visuals")][xs("skeletonbt_clr[2]")], visuals::skeletonbt_clr[2]);
			read_value(json[xs("visuals")][xs("selected_tick_clr[0]")], visuals::selected_tick_clr[0]);
			read_value(json[xs("visuals")][xs("selected_tick_clr[1]")], visuals::selected_tick_clr[1]);
			read_value(json[xs("visuals")][xs("selected_tick_clr[2]")], visuals::selected_tick_clr[2]);
			read_value(json[xs("visuals")][xs("glow_visible_clr[0]")], visuals::glow_visible_clr[0]);
			read_value(json[xs("visuals")][xs("glow_visible_clr[1]")], visuals::glow_visible_clr[1]);
			read_value(json[xs("visuals")][xs("glow_visible_clr[2]")], visuals::glow_visible_clr[2]);
			read_value(json[xs("visuals")][xs("glow_visible_clr[3]")], visuals::glow_visible_clr[3]);
			read_value(json[xs("visuals")][xs("glow_invisible_clr[0]")], visuals::glow_invisible_clr[0]);
			read_value(json[xs("visuals")][xs("glow_invisible_clr[1]")], visuals::glow_invisible_clr[1]);
			read_value(json[xs("visuals")][xs("glow_invisible_clr[2]")], visuals::glow_invisible_clr[2]);
			read_value(json[xs("visuals")][xs("glow_invisible_clr[3]")], visuals::glow_invisible_clr[3]);
			read_value(json[xs("visuals")][xs("glow_weapon_clr[0]")], visuals::glow_weapon_clr[0]);
			read_value(json[xs("visuals")][xs("glow_weapon_clr[1]")], visuals::glow_weapon_clr[1]);
			read_value(json[xs("visuals")][xs("glow_weapon_clr[2]")], visuals::glow_weapon_clr[2]);
			read_value(json[xs("visuals")][xs("glow_weapon_clr[3]")], visuals::glow_weapon_clr[3]);
			read_value(json[xs("visuals")][xs("enable_visuals")], visuals::enable_visuals);
			read_value(json[xs("visuals")][xs("fade_animation")], visuals::fade_animation);
			read_value(json[xs("visuals")][xs("playerbox")], visuals::playerbox);
			read_value(json[xs("visuals")][xs("playerbox_color[0]")], visuals::playerbox_color[0]);
			read_value(json[xs("visuals")][xs("playerbox_color[1]")], visuals::playerbox_color[1]);
			read_value(json[xs("visuals")][xs("playerbox_color[2]")], visuals::playerbox_color[2]);
			read_value(json[xs("visuals")][xs("playername")], visuals::playername);
			read_value(json[xs("visuals")][xs("playername_color[0]")], visuals::playername_color[0]);
			read_value(json[xs("visuals")][xs("playername_color[1]")], visuals::playername_color[1]);
			read_value(json[xs("visuals")][xs("playername_color[2]")], visuals::playername_color[2]);
			read_value(json[xs("visuals")][xs("health_color[0]")], visuals::health_color[0]);
			read_value(json[xs("visuals")][xs("health_color[1]")], visuals::health_color[1]);
			read_value(json[xs("visuals")][xs("health_color[2]")], visuals::health_color[2]);
			read_value(json[xs("visuals")][xs("playerweapon")], visuals::playerweapon);
			read_value(json[xs("visuals")][xs("player_weapon_type[0]")], visuals::player_weapon_type[0]);
			read_value(json[xs("visuals")][xs("player_weapon_type[1]")], visuals::player_weapon_type[1]);
			read_value(json[xs("visuals")][xs("skeleton")], visuals::skeleton);
			read_value(json[xs("visuals")][xs("skeleton_last_clr[0]")], visuals::skeleton_last_clr[0]);
			read_value(json[xs("visuals")][xs("skeleton_last_clr[1]")], visuals::skeleton_last_clr[1]);
			read_value(json[xs("visuals")][xs("skeleton_last_clr[2]")], visuals::skeleton_last_clr[2]);
			read_value(json[xs("visuals")][xs("skeleton_last")], visuals::skeleton_last);
			read_value(json[xs("visuals")][xs("healthesp")], visuals::healthesp);
			read_value(json[xs("visuals")][xs("healthesp_s")], visuals::healthesp_s);
			read_value(json[xs("visuals")][xs("override_bar")], visuals::override_bar);
			read_value(json[xs("visuals")][xs("gradient_bar")], visuals::gradient_bar);
			read_value(json[xs("visuals")][xs("health_bar_gradient[0]")], visuals::health_bar_gradient[0]);
			read_value(json[xs("visuals")][xs("health_bar_gradient[1]")], visuals::health_bar_gradient[1]);
			read_value(json[xs("visuals")][xs("health_bar_gradient[2]")], visuals::health_bar_gradient[2]);
			read_value(json[xs("visuals")][xs("healthesp_s")], visuals::healthesp_s);
			read_value(json[xs("visuals")][xs("health_bar[0]")], visuals::health_bar[0]);
			read_value(json[xs("visuals")][xs("health_bar[1]")], visuals::health_bar[1]);
			read_value(json[xs("visuals")][xs("health_bar[2]")], visuals::health_bar[2]);
			read_value(json[xs("visuals")][xs("oof_arrows")], visuals::oof_arrows);
			read_value(json[xs("visuals")][xs("oof_arrows_clr[0]")], visuals::oof_arrows_clr[0]);
			read_value(json[xs("visuals")][xs("oof_arrows_clr[1]")], visuals::oof_arrows_clr[1]);
			read_value(json[xs("visuals")][xs("oof_arrows_clr[2]")], visuals::oof_arrows_clr[2]);
			read_value(json[xs("visuals")][xs("oof_arrows_clr[3]")], visuals::oof_arrows_clr[3]);
			read_value(json[xs("visuals")][xs("oof_arrows_clr2[0]")], visuals::oof_arrows_clr2[0]);
			read_value(json[xs("visuals")][xs("oof_arrows_clr2[1]")], visuals::oof_arrows_clr2[1]);
			read_value(json[xs("visuals")][xs("oof_arrows_clr2[2]")], visuals::oof_arrows_clr2[2]);
			read_value(json[xs("visuals")][xs("oof_arrows_clr2[3]")], visuals::oof_arrows_clr2[3]);
			read_value(json[xs("visuals")][xs("oof_arrows_dist")], visuals::oof_arrows_dist);
			read_value(json[xs("visuals")][xs("oof_arrows_size")], visuals::oof_arrows_size);
			read_value(json[xs("visuals")][xs("skeletonesp")], visuals::skeletonesp);
			read_value(json[xs("visuals")][xs("radar")], visuals::radar);
			read_value(json[xs("visuals")][xs("gravity_ragdoll")], visuals::gravity_ragdoll);
			read_value(json[xs("visuals")][xs("backtrack_chams")], chams::backtrack_chams);
			read_value(json[xs("visuals")][xs("backtrack_chams_draw_all_ticks")], chams::backtrack_chams_draw_all_ticks);
			read_value(json[xs("visuals")][xs("backtrack_chams_gradient")], chams::backtrack_chams_gradient);
			read_value(json[xs("visuals")][xs("backtrack_chams_clr1[0]")], chams::backtrack_chams_clr1[0]);
			read_value(json[xs("visuals")][xs("backtrack_chams_clr1[1]")], chams::backtrack_chams_clr1[1]);
			read_value(json[xs("visuals")][xs("backtrack_chams_clr1[2]")], chams::backtrack_chams_clr1[2]);
			read_value(json[xs("visuals")][xs("backtrack_chams_clr1[3]")], chams::backtrack_chams_clr1[3]);
			read_value(json[xs("visuals")][xs("backtrack_chams_clr2[0]")], chams::backtrack_chams_clr2[0]);
			read_value(json[xs("visuals")][xs("backtrack_chams_clr2[1]")], chams::backtrack_chams_clr2[1]);
			read_value(json[xs("visuals")][xs("backtrack_chams_clr2[2]")], chams::backtrack_chams_clr2[2]);
			read_value(json[xs("visuals")][xs("backtrack_chams_clr2[3]")], chams::backtrack_chams_clr2[3]);
			read_value(json[xs("visuals")][xs("cham_type")], chams::cham_type);
			read_value(json[xs("visuals")][xs("cham_type_bt")], chams::cham_type_bt);
			read_value(json[xs("visuals")][xs("cham_type_overlay")], chams::cham_type_overlay);
			read_value(json[xs("visuals")][xs("sleeve_chams_clr[0]")], chams::sleeve_chams_clr[0]);
			read_value(json[xs("visuals")][xs("sleeve_chams_clr[1]")], chams::sleeve_chams_clr[1]);
			read_value(json[xs("visuals")][xs("sleeve_chams_clr[2]")], chams::sleeve_chams_clr[2]);
			read_value(json[xs("visuals")][xs("sleeve_chams_clr[3]")], chams::sleeve_chams_clr[3]);
			read_value(json[xs("visuals")][xs("arms_chams_clr[0]")], chams::arms_chams_clr[0]);
			read_value(json[xs("visuals")][xs("arms_chams_clr[1]")], chams::arms_chams_clr[1]);
			read_value(json[xs("visuals")][xs("arms_chams_clr[2]")], chams::arms_chams_clr[2]);
			read_value(json[xs("visuals")][xs("arms_chams_clr[3]")], chams::arms_chams_clr[3]);
			read_value(json[xs("visuals")][xs("sleeve_chams_clr_ov[0]")], chams::sleeve_chams_clr_ov[0]);
			read_value(json[xs("visuals")][xs("sleeve_chams_clr_ov[1]")], chams::sleeve_chams_clr_ov[1]);
			read_value(json[xs("visuals")][xs("sleeve_chams_clr_ov[2]")], chams::sleeve_chams_clr_ov[2]);
			read_value(json[xs("visuals")][xs("sleeve_chams_clr_ov[3]")], chams::sleeve_chams_clr_ov[3]);
			read_value(json[xs("visuals")][xs("arms_chams_clr_ov[0]")], chams::arms_chams_clr_ov[0]);
			read_value(json[xs("visuals")][xs("arms_chams_clr_ov[1]")], chams::arms_chams_clr_ov[1]);
			read_value(json[xs("visuals")][xs("arms_chams_clr_ov[2]")], chams::arms_chams_clr_ov[2]);
			read_value(json[xs("visuals")][xs("arms_chams_clr_ov[3]")], chams::arms_chams_clr_ov[3]);
			read_value(json[xs("visuals")][xs("wpn_chams_clr[0]")], chams::wpn_chams_clr[0]);
			read_value(json[xs("visuals")][xs("wpn_chams_clr[1]")], chams::wpn_chams_clr[1]);
			read_value(json[xs("visuals")][xs("wpn_chams_clr[2]")], chams::wpn_chams_clr[2]);
			read_value(json[xs("visuals")][xs("wpn_chams_clr[3]")], chams::wpn_chams_clr[3]);
			read_value(json[xs("visuals")][xs("wpn_chams_clr_ov[0]")], chams::wpn_chams_clr_ov[0]);
			read_value(json[xs("visuals")][xs("wpn_chams_clr_ov[1]")], chams::wpn_chams_clr_ov[1]);
			read_value(json[xs("visuals")][xs("wpn_chams_clr_ov[2]")], chams::wpn_chams_clr_ov[2]);
			read_value(json[xs("visuals")][xs("wpn_chams_clr_ov[3]")], chams::wpn_chams_clr_ov[3]);
			read_value(json[xs("visuals")][xs("visible_wireframe")], chams::visible_wireframe);
			read_value(json[xs("visuals")][xs("visible_chams_ov")], chams::visible_chams_ov);
			read_value(json[xs("visuals")][xs("visible_wireframe_ov")], chams::visible_wireframe_ov);
			read_value(json[xs("visuals")][xs("invisible_wireframe")], chams::invisible_wireframe);
			read_value(json[xs("visuals")][xs("sleeve_chams")], chams::sleeve_chams);
			read_value(json[xs("visuals")][xs("sleeve_wireframe")], chams::sleeve_wireframe);
			read_value(json[xs("visuals")][xs("arms_chams")], chams::arms_chams);
			read_value(json[xs("visuals")][xs("arms_wireframe")], chams::arms_wireframe);
			read_value(json[xs("visuals")][xs("sleeve_chams_ov")], chams::sleeve_chams_ov);
			read_value(json[xs("visuals")][xs("sleeve_wireframe_ov")], chams::sleeve_wireframe_ov);
			read_value(json[xs("visuals")][xs("arms_chams_ov")], chams::arms_chams_ov);
			read_value(json[xs("visuals")][xs("wpn_chams")], chams::wpn_chams);
			read_value(json[xs("visuals")][xs("wpn_wireframe")], chams::wpn_wireframe);
			read_value(json[xs("visuals")][xs("wpn_chams_ov")], chams::wpn_chams_ov);
			read_value(json[xs("visuals")][xs("wpn_wireframe_ov")], chams::wpn_wireframe_ov);
			read_value(json[xs("visuals")][xs("cham_type_overlay")], chams::cham_type_overlay);
			read_value(json[xs("visuals")][xs("visible_chams_clr_ov[0]")], chams::visible_chams_clr_ov[0]);
			read_value(json[xs("visuals")][xs("visible_chams_clr_ov[1]")], chams::visible_chams_clr_ov[1]);
			read_value(json[xs("visuals")][xs("visible_chams_clr_ov[2]")], chams::visible_chams_clr_ov[2]);
			read_value(json[xs("visuals")][xs("visible_chams_clr_ov[3]")], chams::visible_chams_clr_ov[3]);
			read_value(json[xs("chams")][xs("visible_chams_clr_ov[0]")], chams::visible_chams_clr_ov[0]);
			read_value(json[xs("chams")][xs("visible_chams_clr_ov[1]")], chams::visible_chams_clr_ov[1]);
			read_value(json[xs("chams")][xs("visible_chams_clr_ov[2]")], chams::visible_chams_clr_ov[2]);
			read_value(json[xs("chams")][xs("visible_chams_clr_ov[3]")], chams::visible_chams_clr_ov[3]);
			read_value(json[xs("visuals")][xs("cham_type_sleeve")], chams::cham_type_sleeve);
			read_value(json[xs("visuals")][xs("cham_type_sleeve_ov")], chams::cham_type_sleeve_ov);
			read_value(json[xs("visuals")][xs("cham_type_wpn")], chams::cham_type_wpn);
			read_value(json[xs("visuals")][xs("cham_type_wpn_ov")], chams::cham_type_wpn_ov);
			read_value(json[xs("visuals")][xs("cham_type_arms")], chams::cham_type_arms);
			read_value(json[xs("visuals")][xs("cham_type_arms_ov")], chams::cham_type_arms_ov);
			read_value(json[xs("visuals")][xs("glow")], visuals::glow);
			read_value(json[xs("visuals")][xs("glow_weapon")], visuals::glow_weapon);
			read_value(json[xs("visuals")][xs("custom_sun")], visuals::custom_sun);
			read_value(json[xs("visuals")][xs("custom_sun_x")], visuals::custom_sun_x);
			read_value(json[xs("visuals")][xs("custom_sun_y")], visuals::custom_sun_y);
			read_value(json[xs("visuals")][xs("custom_sun_dist")], visuals::custom_sun_dist);
			read_value(json[xs("visuals")][xs("custom_sun_mode")], visuals::custom_sun_mode);
			read_value(json[xs("visuals")][xs("custom_sun_speed")], visuals::custom_sun_speed);
			read_value(json[xs("visuals")][xs("custom_sun_orbit_range")], visuals::custom_sun_orbit_range);
			read_value(json[xs("triggerbot")][xs("enabled")], triggerbot::enabled);
			read_value(json[xs("triggerbot")][xs("triggerbot_key")], triggerbot::triggerbot_key);
			read_value(json[xs("triggerbot")][xs("triggerbot_key_s")], triggerbot::triggerbot_key_s);
			read_value(json[xs("aimbot")][xs("aimbot_hitbox")], aimbot::aimbot_hitbox);
			read_value(json[xs("aimbot")][xs("aimbot_autowall")], aimbot::aimbot_autowall);
			read_value(json[xs("aimbot")][xs("aimbot_autowall_dmg")], aimbot::aimbot_autowall_dmg);
			read_value(json[xs("aimbot")][xs("aimbot_fov")], aimbot::aimbot_fov);
			read_value(json[xs("aimbot")][xs("backtrack")], backtrack::backtrack);
			read_value(json[xs("aimbot")][xs("time")], backtrack::time);
			read_value(json[xs("aimbot")][xs("fake")], backtrack::fake);
			read_value(json[xs("aimbot")][xs("fake_latency")], backtrack::fake_latency);
			read_value(json[xs("aimbot")][xs("selected_tick")], backtrack::selected_tick);
			read_value(json[xs("movement")][xs("auto_align")], movement::auto_align);
			read_value(json[xs("aimbot")][xs("aimbot")], aimbot::aim_at_bt);
			read_value(json[xs("aimbot")][xs("aimbot")], aimbot::aimbot);
			read_value(json[xs("aimbot")][xs("aimbot_key")], aimbot::aimbot_key);
			read_value(json[xs("aimbot")][xs("pistol_hitbox")], aimbot::pistol_hitbox);
			read_value(json[xs("aimbot")][xs("pistol_aimbot_fov")], aimbot::pistol_aimbot_fov);
			read_value(json[xs("aimbot")][xs("pistol_autowall")], aimbot::pistol_autowall);
			read_value(json[xs("aimbot")][xs("pistol_autowall_dmg")], aimbot::pistol_autowall_dmg);
			read_value(json[xs("aimbot")][xs("heavy_pistol_hitbox")], aimbot::heavy_pistol_hitbox);
			read_value(json[xs("aimbot")][xs("heavy_pistol_aimbot_fov")], aimbot::heavy_pistol_aimbot_fov);
			read_value(json[xs("aimbot")][xs("heavy_pistol_autowall")], aimbot::heavy_pistol_autowall);
			read_value(json[xs("aimbot")][xs("heavy_pistol_autowall_dmg")], aimbot::heavy_pistol_autowall_dmg);
			read_value(json[xs("aimbot")][xs("shotgun_hitbox")], aimbot::shotgun_hitbox);
			read_value(json[xs("aimbot")][xs("shotgun_aimbot_fov")], aimbot::shotgun_aimbot_fov);
			read_value(json[xs("aimbot")][xs("shotgun_autowall")], aimbot::shotgun_autowall);
			read_value(json[xs("aimbot")][xs("heavy_hitbox")], aimbot::heavy_hitbox);
			read_value(json[xs("aimbot")][xs("heavy_aimbot_fov")], aimbot::heavy_aimbot_fov);
			read_value(json[xs("aimbot")][xs("heavy_autowall")], aimbot::heavy_autowall);
			read_value(json[xs("aimbot")][xs("heavy_autowall_dmg")], aimbot::heavy_autowall_dmg);
			read_value(json[xs("aimbot")][xs("smg_hitbox")], aimbot::smg_hitbox);
			read_value(json[xs("aimbot")][xs("smg_aimbot_fov")], aimbot::smg_aimbot_fov);
			read_value(json[xs("aimbot")][xs("smg_autowall")], aimbot::smg_autowall);
			read_value(json[xs("aimbot")][xs("smg_autowall_dmg")], aimbot::smg_autowall_dmg);
			read_value(json[xs("aimbot")][xs("rifle_hitbox")], aimbot::rifle_hitbox);
			read_value(json[xs("aimbot")][xs("rifle_aimbot_fov")], aimbot::rifle_aimbot_fov);
			read_value(json[xs("aimbot")][xs("rifle_autowall")], aimbot::rifle_autowall);
			read_value(json[xs("aimbot")][xs("rifle_autowall_dmg")], aimbot::rifle_autowall_dmg);
			read_value(json[xs("aimbot")][xs("sniper_hitbox")], aimbot::sniper_hitbox);
			read_value(json[xs("aimbot")][xs("sniper_aimbot_fov")], aimbot::sniper_aimbot_fov);
			read_value(json[xs("aimbot")][xs("sniper_autowall")], aimbot::sniper_autowall);
			read_value(json[xs("aimbot")][xs("sniper_autowall_dmg")], aimbot::sniper_autowall_dmg);
			read_value(json[xs("aimbot")][xs("autosniper_hitbox")], aimbot::autosniper_hitbox);
			read_value(json[xs("aimbot")][xs("autosniper_aimbot_fov")], aimbot::autosniper_aimbot_fov);
			read_value(json[xs("aimbot")][xs("autosniper_autowall")], aimbot::autosniper_autowall);
			read_value(json[xs("aimbot")][xs("autosniper_autowall_dmg")], aimbot::autosniper_autowall_dmg);
			read_value(json[xs("aimbot")][xs("aimbot_silent")], aimbot::aimbot_silent);
			read_value(json[xs("aimbot")][xs("non_sticky_aimbot")], aimbot::non_sticky_aimbot);
			read_value(json[xs("visuals")][xs("dropped_weapon_icon")], visuals::dropped_weapon_icon);
			read_value(json[xs("visuals")][xs("dropped_weapon_name")], visuals::dropped_weapon_name);
			read_value(json[xs("visuals")][xs("dropped_weapon_distance")], visuals::dropped_weapon_distance);
			read_value(json[xs("visuals")][xs("dropped_weapon_icon_clr[0]")], visuals::dropped_weapon_icon_clr[0]);
			read_value(json[xs("visuals")][xs("dropped_weapon_box_clr[1]")], visuals::dropped_weapon_icon_clr[1]);
			read_value(json[xs("visuals")][xs("dropped_weapon_icon_clr[2]")], visuals::dropped_weapon_icon_clr[2]);
			read_value(json[xs("visuals")][xs("dropped_weapon_icon_clr[3]")], visuals::dropped_weapon_icon_clr[3]);
			read_value(json[xs("visuals")][xs("dropped_weapon_name_clr[0]")], visuals::dropped_weapon_name_clr[0]);
			read_value(json[xs("visuals")][xs("dropped_weapon_name_clr[1]")], visuals::dropped_weapon_name_clr[1]);
			read_value(json[xs("visuals")][xs("dropped_weapon_name_clr[2]")], visuals::dropped_weapon_name_clr[2]);
			read_value(json[xs("visuals")][xs("dropped_weapon_name_clr[3]")], visuals::dropped_weapon_name_clr[3]);
			read_value(json[xs("misc")][xs("swayscale")], misc::swayscale);
			read_value(json[xs("misc")][xs("swayscale_value")], misc::swayscale_value);
			read_value(json[xs("visuals")][xs("skybox")], visuals::skybox);
			read_value(json[xs("visuals")][xs("dlight")], visuals::dlight);
			read_value(json[xs("visuals")][xs("dlight_clr[0]")], visuals::dlight_clr[0]);
			read_value(json[xs("visuals")][xs("dlight_clr[1]")], visuals::dlight_clr[1]);
			read_value(json[xs("visuals")][xs("dlight_clr[2]")], visuals::dlight_clr[2]);
			read_value(json[xs("visuals")][xs("dlight_clr[3]")], visuals::dlight_clr[3]);
			read_value(json[xs("visuals")][xs("visible_chams_clr[0]")], chams::visible_chams_clr[0]);
			read_value(json[xs("visuals")][xs("visible_chams_clr[1]")], chams::visible_chams_clr[1]);
			read_value(json[xs("visuals")][xs("visible_chams_clr[2]")], chams::visible_chams_clr[2]);
			read_value(json[xs("visuals")][xs("visible_chams_clr[3]")], chams::visible_chams_clr[3]);
			read_value(json[xs("visuals")][xs("invisible_chams_clr[0]")], chams::invisible_chams_clr[0]);
			read_value(json[xs("visuals")][xs("invisible_chams_clr[1]")], chams::invisible_chams_clr[1]);
			read_value(json[xs("visuals")][xs("invisible_chams_clr[2]")], chams::invisible_chams_clr[2]);
			read_value(json[xs("visuals")][xs("invisible_chams_clr[3]")], chams::invisible_chams_clr[3]);
			read_value(json[xs("visuals")][xs("invisible_chams")], chams::invisible_chams);
			read_value(json[xs("visuals")][xs("visible_chams")], chams::visible_chams);
			read_value(json[xs("visuals")][xs("cham_type_bt")], chams::cham_type_bt);
			read_value(json[xs("visuals")][xs("cham_type")], chams::cham_type);
			read_value(json[xs("misc")][xs("force_crosshair")], misc::force_crosshair);
			read_value(json[xs("misc")][xs("sniper_crosshair")], misc::sniper_crosshair);
			read_value(json[xs("misc")][xs("misc_animated_clantag")], misc::misc_animated_clantag);
			read_value(json[xs("misc")][xs("misc_clantag_spammer")], misc::misc_clantag_spammer);
			read_value(json[xs("misc")][xs("misc_clantag_speed")], misc::misc_clantag_speed);
			read_value(json[xs("misc")][xs("misc_clantag_type")], misc::misc_clantag_type);
			read_value(json[xs("misc")][xs("misc_hitmarker")], misc::misc_hitmarker);
			read_value(json[xs("misc")][xs("misc_hitmarker_sound")], misc::misc_hitmarker_sound);
			read_value(json[xs("misc")][xs("misc_hitmarker_screen_effect")], misc::misc_hitmarker_screen_effect);
			read_value(json[xs("misc")][xs("misc_kill_msg")], misc::misc_kill_msg);
			read_value(json[xs("misc")][xs("misc_kill_message")], misc::misc_kill_message);
			read_value(json[xs("misc")][xs("menu_variant")], misc::menu_variant);
			read_value(json[xs("misc")][xs("menu_fx_mode")], misc::menu_fx_mode);
			read_value(json[xs("misc")][xs("menu_fx_density")], misc::menu_fx_density);
			menu::apply_menu_variant(menu::normalize_menu_variant(misc::menu_variant));
			read_value(json[xs("visuals")][xs("fullbright")], visuals::fullbright);
			read_value(json[xs("visuals")][xs("apply_zoom")], visuals::apply_zoom);
			read_value(json[xs("visuals")][xs("apply_zoom_key")], visuals::apply_zoom_key);
			read_value(json[xs("visuals")][xs("apply_zoom_key_s")], visuals::apply_zoom_key_s);
			read_value(json[xs("visuals")][xs("boxtype")], visuals::boxtype);
			read_value(json[xs("visuals")][xs("player_box_outline[0]")], visuals::player_box_outline[0]);
			read_value(json[xs("visuals")][xs("player_box_outline[1]")], visuals::player_box_outline[1]);
			read_value(json[xs("visuals")][xs("mbenabled")], visuals::mbenabled);
			read_value(json[xs("visuals")][xs("mbforwardEnabled")], visuals::mbforwardEnabled);
			read_value(json[xs("visuals")][xs("mbfallingMin")], visuals::mbfallingMin);
			read_value(json[xs("visuals")][xs("mbfallingMax")], visuals::mbfallingMax);
			read_value(json[xs("visuals")][xs("mbfallingIntensity")], visuals::mbfallingIntensity);
			read_value(json[xs("visuals")][xs("mbrotationIntensity")], visuals::mbrotationIntensity);
			read_value(json[xs("visuals")][xs("mbstrength")], visuals::mbstrength);
			read_value(json[xs("misc")][xs("nadepred")], misc::nadepred);
			read_value(json[xs("misc")][xs("nadepred_clr[0]")], misc::nadepred_clr[0]);
			read_value(json[xs("misc")][xs("nadepred_clr[1]")], misc::nadepred_clr[1]);
			read_value(json[xs("misc")][xs("nadepred_clr[2]")], misc::nadepred_clr[2]);
			read_value(json[xs("visuals")][xs("flashalpha")], visuals::flashalpha);
			read_value(json[xs("visuals")][xs("change_flashalpha")], visuals::change_flashalpha);
			read_value(json[xs("visuals")][xs("nosmoke")], visuals::nosmoke);
			read_value(json[xs("misc")][xs("autoaccept")], misc::autoaccept);
			read_value(json[xs("visuals")][xs("trails_clr1[0]")], visuals::trails_clr1[0]);
			read_value(json[xs("visuals")][xs("trails_clr1[1]")], visuals::trails_clr1[1]);
			read_value(json[xs("visuals")][xs("trails_clr1[2]")], visuals::trails_clr1[2]);
			read_value(json[xs("visuals")][xs("trails_clr1[3]")], visuals::trails_clr1[3]);
			read_value(json[xs("visuals")][xs("trails_clr2[0]")], visuals::trails_clr2[0]);
			read_value(json[xs("visuals")][xs("trails_clr2[1]")], visuals::trails_clr2[1]);
			read_value(json[xs("visuals")][xs("trails_clr2[2]")], visuals::trails_clr2[2]);
			read_value(json[xs("visuals")][xs("trails_clr2[3]")], visuals::trails_clr2[3]);
			read_value(json[xs("visuals")][xs("world_modulate")], visuals::world_modulate);
			read_value(json[xs("visuals")][xs("world_modulate_props")], visuals::world_modulate_props);
			read_value(json[xs("visuals")][xs("world_modulate_skybox")], visuals::world_modulate_skybox);
			read_value(json[xs("visuals")][xs("world_modulate_intensity")], visuals::world_modulate_intensity);
			read_value(json[xs("visuals")][xs("world_color[0]")], visuals::world_color[0]);
			read_value(json[xs("visuals")][xs("world_color[1]")], visuals::world_color[1]);
			read_value(json[xs("visuals")][xs("world_color[2]")], visuals::world_color[2]);
			read_value(json[xs("visuals")][xs("world_color[3]")], visuals::world_color[3]);
			read_value(json[xs("visuals")][xs("trails")], visuals::trails);
			read_value(json[xs("movement")][xs("indicators_position")], movement::indicators_position);
			read_value(json[xs("movement")][xs("indicators_gap")], movement::indicators_gap);
			read_value(json[xs("misc")][xs("menu_key")], misc::menu_key);
			read_value(json[xs("misc")][xs("vote_revealer")], misc::vote_revealer);
			read_value(json[xs("misc")][xs("misc_hit_info[0]")], misc::misc_hit_info[0]);
			read_value(json[xs("misc")][xs("misc_hit_info[1]")], misc::misc_hit_info[1]);
			read_value(json[xs("visuals")][xs("visuals_flags[0]")], visuals::visuals_flags[0]);
			read_value(json[xs("visuals")][xs("visuals_flags[1]")], visuals::visuals_flags[1]);
			read_value(json[xs("visuals")][xs("visuals_flags[2]")], visuals::visuals_flags[2]);
			read_value(json[xs("visuals")][xs("visuals_flags[3]")], visuals::visuals_flags[3]);
			read_value(json[xs("visuals")][xs("visuals_flags[4]")], visuals::visuals_flags[4]);
			read_value(json[xs("visuals")][xs("visuals_flags[5]")], visuals::visuals_flags[5]);
			read_value(json[xs("visuals")][xs("removals[0]")], visuals::removals[0]);
			read_value(json[xs("visuals")][xs("removals[1]")], visuals::removals[1]);
			read_value(json[xs("visuals")][xs("removals[2]")], visuals::removals[2]);
			read_value(json[xs("movement")][xs("pixel_surf")], movement::pixel_surf);
			read_value(json[xs("movement")][xs("pixel_surf_ticks")], movement::pixel_surf_ticks);
			read_value(json[xs("movement")][xs("pixel_surf_key")], movement::pixel_surf_key);
			read_value(json[xs("movement")][xs("pixel_surf_key_s")], movement::pixel_surf_key_s);
			read_value(json[xs("movement")][xs("pixel_surf_detection_printf")], movement::pixel_surf_detection_printf);
			read_value(json[xs("movement")][xs("jump_bug_key_s")], movement::jump_bug_key_s);
			read_value(json[xs("movement")][xs("edge_bug_key_s")], movement::edge_bug_key_s);
			read_value(json[xs("movement")][xs("mini_jump_key_s")], movement::mini_jump_key_s);
			read_value(json[xs("movement")][xs("mini_jump_ej")], movement::mini_jump_ej);
			read_value(json[xs("movement")][xs("long_jump_key_s")], movement::long_jump_key_s);
			read_value(json[xs("visuals")][xs("dropped_weapon_glow_clr[0]")], visuals::dropped_weapon_glow_clr[0]);
			read_value(json[xs("visuals")][xs("dropped_weapon_glow_clr[1]")], visuals::dropped_weapon_glow_clr[1]);
			read_value(json[xs("visuals")][xs("dropped_weapon_glow_clr[2]")], visuals::dropped_weapon_glow_clr[2]);
			read_value(json[xs("visuals")][xs("dropped_weapon_glow")], visuals::dropped_weapon_glow);
			read_value(json[xs("movement")][xs("long_jump_key")], movement::long_jump_key);
			read_value(json[xs("visuals")][xs("dropped_weapon_type")], visuals::dropped_weapon_type);
			read_value(json[xs("movement")][xs("adaptive_key_cancelling")], movement::adaptive_key_cancelling);
			read_value(json[xs("movement")][xs("adaptive_key_for[0]")], movement::adaptive_key_for[0]);
			read_value(json[xs("movement")][xs("adaptive_key_for[1]")], movement::adaptive_key_for[1]);
			read_value(json[xs("movement")][xs("adaptive_key_for[2]")], movement::adaptive_key_for[2]);
			read_value(json[xs("movement")][xs("lj_null[0]")], movement::lj_null[0]);
			read_value(json[xs("movement")][xs("lj_null[1]")], movement::lj_null[1]);
			read_value(json[xs("movement")][xs("lj_null[2]")], movement::lj_null[2]);
			read_value(json[xs("movement")][xs("lj_null[3]")], movement::lj_null[3]);
			read_value(json[xs("movement")][xs("key_strokes")], movement::key_strokes);
			read_value(json[xs("movement")][xs("key_strokes_position")], movement::key_strokes_position);
			read_value(json[xs("fonts")][xs("indi_font")], fonts::indi_font);
			read_value(json[xs("fonts")][xs("indi_size")], fonts::indi_size);
			read_value(json[xs("fonts")][xs("sub_indi_font")], fonts::sub_indi_font);
			read_value(json[xs("fonts")][xs("sub_indi_size")], fonts::sub_indi_size);
			read_value(json[xs("fonts")][xs("spec_font")], fonts::spec_font);
			read_value(json[xs("fonts")][xs("sub_spec_font")], fonts::sub_spec_font);
			read_value(json[xs("fonts")][xs("esp_hp_font")], fonts::esp_hp_font);
			read_value(json[xs("fonts")][xs("esp_hp_size")], fonts::esp_hp_size);
			read_value(json[xs("fonts")][xs("esp_name_font")], fonts::esp_name_font);
			read_value(json[xs("fonts")][xs("esp_name_size")], fonts::esp_name_size);
			read_value(json[xs("fonts")][xs("esp_wpn_font")], fonts::esp_wpn_font);
			read_value(json[xs("fonts")][xs("esp_wpn_size")], fonts::esp_wpn_size);
			read_value(json[xs("fonts")][xs("esp_dropped_wpn_font")], fonts::esp_dropped_wpn_font);
			read_value(json[xs("fonts")][xs("esp_dropped_wpn_size")], fonts::esp_dropped_wpn_size);
			read_value(json[xs("fonts")][xs("sc_logs_font")], fonts::sc_logs_font);
			read_value(json[xs("fonts")][xs("sc_logs_size")], fonts::sc_logs_size);
			read_value(json[xs("fonts")][xs("spec_font_flag[0]")], fonts::spec_font_flag[0]);
			read_value(json[xs("fonts")][xs("spec_font_flag[1]")], fonts::spec_font_flag[1]);
			read_value(json[xs("fonts")][xs("spec_font_flag[2]")], fonts::spec_font_flag[2]);
			read_value(json[xs("fonts")][xs("spec_font_flag[3]")], fonts::spec_font_flag[3]);
			read_value(json[xs("fonts")][xs("spec_font_flag[4]")], fonts::spec_font_flag[4]);
			read_value(json[xs("fonts")][xs("spec_font_flag[5]")], fonts::spec_font_flag[5]);
			read_value(json[xs("fonts")][xs("spec_font_flag[6]")], fonts::spec_font_flag[6]);
			read_value(json[xs("fonts")][xs("spec_font_flag[7]")], fonts::spec_font_flag[7]);
			read_value(json[xs("fonts")][xs("spec_font_flag[8]")], fonts::spec_font_flag[8]);
			read_value(json[xs("fonts")][xs("spec_font_flag[9]")], fonts::spec_font_flag[9]);
			read_value(json[xs("fonts")][xs("sub_spec_font_flag[0]")], fonts::sub_spec_font_flag[0]);
			read_value(json[xs("fonts")][xs("sub_spec_font_flag[1]")], fonts::sub_spec_font_flag[1]);
			read_value(json[xs("fonts")][xs("sub_spec_font_flag[2]")], fonts::sub_spec_font_flag[2]);
			read_value(json[xs("fonts")][xs("sub_spec_font_flag[3]")], fonts::sub_spec_font_flag[3]);
			read_value(json[xs("fonts")][xs("sub_spec_font_flag[4]")], fonts::sub_spec_font_flag[4]);
			read_value(json[xs("fonts")][xs("sub_spec_font_flag[5]")], fonts::sub_spec_font_flag[5]);
			read_value(json[xs("fonts")][xs("sub_spec_font_flag[6]")], fonts::sub_spec_font_flag[6]);
			read_value(json[xs("fonts")][xs("sub_spec_font_flag[7]")], fonts::sub_spec_font_flag[7]);
			read_value(json[xs("fonts")][xs("sub_spec_font_flag[8]")], fonts::sub_spec_font_flag[8]);
			read_value(json[xs("fonts")][xs("sub_spec_font_flag[9]")], fonts::sub_spec_font_flag[9]);
			read_value(json[xs("fonts")][xs("sub_indi_font_flag[0]")], fonts::sub_indi_font_flag[0]);
			read_value(json[xs("fonts")][xs("sub_indi_font_flag[1]")], fonts::sub_indi_font_flag[1]);
			read_value(json[xs("fonts")][xs("sub_indi_font_flag[2]")], fonts::sub_indi_font_flag[2]);
			read_value(json[xs("fonts")][xs("sub_indi_font_flag[3]")], fonts::sub_indi_font_flag[3]);
			read_value(json[xs("fonts")][xs("sub_indi_font_flag[4]")], fonts::sub_indi_font_flag[4]);
			read_value(json[xs("fonts")][xs("sub_indi_font_flag[5]")], fonts::sub_indi_font_flag[5]);
			read_value(json[xs("fonts")][xs("sub_indi_font_flag[6]")], fonts::sub_indi_font_flag[6]);
			read_value(json[xs("fonts")][xs("sub_indi_font_flag[7]")], fonts::sub_indi_font_flag[7]);
			read_value(json[xs("fonts")][xs("sub_indi_font_flag[8]")], fonts::sub_indi_font_flag[8]);
			read_value(json[xs("fonts")][xs("sub_indi_font_flag[9]")], fonts::sub_indi_font_flag[9]);
			read_value(json[xs("fonts")][xs("sub_indi_font_flag[10]")], fonts::sub_indi_font_flag[10]);
			read_value(json[xs("fonts")][xs("sub_indi_font_flag[11]")], fonts::sub_indi_font_flag[11]);
			read_value(json[xs("fonts")][xs("indi_font_flag[0]")], fonts::indi_font_flag[0]);
			read_value(json[xs("fonts")][xs("indi_font_flag[1]")], fonts::indi_font_flag[1]);
			read_value(json[xs("fonts")][xs("indi_font_flag[2]")], fonts::indi_font_flag[2]);
			read_value(json[xs("fonts")][xs("indi_font_flag[3]")], fonts::indi_font_flag[3]);
			read_value(json[xs("fonts")][xs("indi_font_flag[4]")], fonts::indi_font_flag[4]);
			read_value(json[xs("fonts")][xs("indi_font_flag[5]")], fonts::indi_font_flag[5]);
			read_value(json[xs("fonts")][xs("indi_font_flag[6]")], fonts::indi_font_flag[6]);
			read_value(json[xs("fonts")][xs("indi_font_flag[7]")], fonts::indi_font_flag[7]);
			read_value(json[xs("fonts")][xs("indi_font_flag[8]")], fonts::indi_font_flag[8]);
			read_value(json[xs("fonts")][xs("indi_font_flag[9]")], fonts::indi_font_flag[9]);
			read_value(json[xs("fonts")][xs("indi_font_flag[10]")], fonts::indi_font_flag[10]);
			read_value(json[xs("fonts")][xs("indi_font_flag[11]")], fonts::indi_font_flag[11]);
			read_value(json[xs("fonts")][xs("esp_font_flag[0]")], fonts::esp_font_flag[0]);
			read_value(json[xs("fonts")][xs("esp_font_flag[1]")], fonts::esp_font_flag[1]);
			read_value(json[xs("fonts")][xs("esp_font_flag[2]")], fonts::esp_font_flag[2]);
			read_value(json[xs("fonts")][xs("esp_font_flag[3]")], fonts::esp_font_flag[3]);
			read_value(json[xs("fonts")][xs("esp_font_flag[4]")], fonts::esp_font_flag[4]);
			read_value(json[xs("fonts")][xs("esp_font_flag[5]")], fonts::esp_font_flag[5]);
			read_value(json[xs("fonts")][xs("esp_font_flag[6]")], fonts::esp_font_flag[6]);
			read_value(json[xs("fonts")][xs("esp_font_flag[7]")], fonts::esp_font_flag[7]);
			read_value(json[xs("fonts")][xs("esp_font_flag[8]")], fonts::esp_font_flag[8]);
			read_value(json[xs("fonts")][xs("esp_font_flag[9]")], fonts::esp_font_flag[9]);
			read_value(json[xs("fonts")][xs("esp_font_flag[10]")], fonts::esp_font_flag[10]);
			read_value(json[xs("fonts")][xs("esp_font_flag[11]")], fonts::esp_font_flag[11]);
			read_value(json[xs("fonts")][xs("esp_name_font_flag[0]")], fonts::esp_name_font_flag[0]);
			read_value(json[xs("fonts")][xs("esp_name_font_flag[1]")], fonts::esp_name_font_flag[1]);
			read_value(json[xs("fonts")][xs("esp_name_font_flag[2]")], fonts::esp_name_font_flag[2]);
			read_value(json[xs("fonts")][xs("esp_name_font_flag[3]")], fonts::esp_name_font_flag[3]);
			read_value(json[xs("fonts")][xs("esp_name_font_flag[4]")], fonts::esp_name_font_flag[4]);
			read_value(json[xs("fonts")][xs("esp_name_font_flag[5]")], fonts::esp_name_font_flag[5]);
			read_value(json[xs("fonts")][xs("esp_name_font_flag[6]")], fonts::esp_name_font_flag[6]);
			read_value(json[xs("fonts")][xs("esp_name_font_flag[7]")], fonts::esp_name_font_flag[7]);
			read_value(json[xs("fonts")][xs("esp_name_font_flag[8]")], fonts::esp_name_font_flag[8]);
			read_value(json[xs("fonts")][xs("esp_name_font_flag[9]")], fonts::esp_name_font_flag[9]);
			read_value(json[xs("fonts")][xs("esp_name_font_flag[10]")], fonts::esp_name_font_flag[10]);
			read_value(json[xs("fonts")][xs("esp_name_font_flag[11]")], fonts::esp_name_font_flag[11]);
			read_value(json[xs("fonts")][xs("esp_wpn_font_flag[0]")], fonts::esp_wpn_font_flag[0]);
			read_value(json[xs("fonts")][xs("esp_wpn_font_flag[1]")], fonts::esp_wpn_font_flag[1]);
			read_value(json[xs("fonts")][xs("esp_wpn_font_flag[2]")], fonts::esp_wpn_font_flag[2]);
			read_value(json[xs("fonts")][xs("esp_wpn_font_flag[3]")], fonts::esp_wpn_font_flag[3]);
			read_value(json[xs("fonts")][xs("esp_wpn_font_flag[4]")], fonts::esp_wpn_font_flag[4]);
			read_value(json[xs("fonts")][xs("esp_wpn_font_flag[5]")], fonts::esp_wpn_font_flag[5]);
			read_value(json[xs("fonts")][xs("esp_wpn_font_flag[6]")], fonts::esp_wpn_font_flag[6]);
			read_value(json[xs("fonts")][xs("esp_wpn_font_flag[7]")], fonts::esp_wpn_font_flag[7]);
			read_value(json[xs("fonts")][xs("esp_wpn_font_flag[8]")], fonts::esp_wpn_font_flag[8]);
			read_value(json[xs("fonts")][xs("esp_wpn_font_flag[9]")], fonts::esp_wpn_font_flag[9]);
			read_value(json[xs("fonts")][xs("esp_wpn_font_flag[10]")], fonts::esp_wpn_font_flag[10]);
			read_value(json[xs("fonts")][xs("esp_wpn_font_flag[11]")], fonts::esp_wpn_font_flag[11]);
			read_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[0]")], fonts::esp_dropped_wpn_font_flag[0]);
			read_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[1]")], fonts::esp_dropped_wpn_font_flag[1]);
			read_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[2]")], fonts::esp_dropped_wpn_font_flag[2]);
			read_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[3]")], fonts::esp_dropped_wpn_font_flag[3]);
			read_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[4]")], fonts::esp_dropped_wpn_font_flag[4]);
			read_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[5]")], fonts::esp_dropped_wpn_font_flag[5]);
			read_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[6]")], fonts::esp_dropped_wpn_font_flag[6]);
			read_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[7]")], fonts::esp_dropped_wpn_font_flag[7]);
			read_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[8]")], fonts::esp_dropped_wpn_font_flag[8]);
			read_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[9]")], fonts::esp_dropped_wpn_font_flag[9]);
			read_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[10]")], fonts::esp_dropped_wpn_font_flag[10]);
			read_value(json[xs("fonts")][xs("esp_dropped_wpn_font_flag[11]")], fonts::esp_dropped_wpn_font_flag[11]);
			read_value(json[xs("fonts")][xs("sc_logs_flag[0]")], fonts::sc_logs_flag[0]);
			read_value(json[xs("fonts")][xs("sc_logs_flag[1]")], fonts::sc_logs_flag[1]);
			read_value(json[xs("fonts")][xs("sc_logs_flag[2]")], fonts::sc_logs_flag[2]);
			read_value(json[xs("fonts")][xs("sc_logs_flag[3]")], fonts::sc_logs_flag[3]);
			read_value(json[xs("fonts")][xs("sc_logs_flag[4]")], fonts::sc_logs_flag[4]);
			read_value(json[xs("fonts")][xs("sc_logs_flag[5]")], fonts::sc_logs_flag[5]);
			read_value(json[xs("fonts")][xs("sc_logs_flag[6]")], fonts::sc_logs_flag[6]);
			read_value(json[xs("fonts")][xs("sc_logs_flag[7]")], fonts::sc_logs_flag[7]);
			read_value(json[xs("fonts")][xs("sc_logs_flag[8]")], fonts::sc_logs_flag[8]);
			read_value(json[xs("fonts")][xs("sc_logs_flag[9]")], fonts::sc_logs_flag[9]);
			read_value(json[xs("fonts")][xs("sc_logs_flag[10]")], fonts::sc_logs_flag[10]);
			read_value(json[xs("fonts")][xs("sc_logs_flag[11]")], fonts::sc_logs_flag[11]);
			read_value(json[xs("sfui")][xs("sfui_on")], sfui::sfui_on);

		}

		path.erase(path.size() - configs.at(index).size() - 5);
	}
}
