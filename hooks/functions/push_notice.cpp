#include "../../features/visuals/visuals.hpp"
#include "../../features/aimbot/aimbot.hpp"
#include "../../features/misc/misc.hpp"
#include "../../menu/config/config.hpp"
#include "../../menu/menu.hpp"
#include "../../sdk/sdk.hpp"
#include "../hooks.hpp"
#include <sstream>
#include <iomanip>
#include <cstring>

std::string rgb_to_hex_safe(int r, int g, int b) {
	std::stringstream ss;
	ss << "#" << std::setfill('0') << std::setw(2) << std::hex << r
		<< std::setfill('0') << std::setw(2) << std::hex << g
		<< std::setfill('0') << std::setw(2) << std::hex << b;
	return ss.str();
}

static bool notif_match(const char* text, int str_len, const char* tok) {
	if (!text || !tok)
		return false;
	const std::size_t n = std::strlen(tok);
	if (n == 0)
		return false;
	if (std::strncmp(text, tok, n) != 0)
		return false;
	if (text[n] == '\0')
		return true;
	if (str_len > 0 && static_cast<std::size_t>(str_len) == n)
		return true;
	return false;
}

static std::string push_notice_dna_prefix() {
	return menu::menu_variant_profile(menu::active_menu_variant()).chat_prefix;
}

void __fastcall sdk::hooks::push_notice::push_notice(int ecx, int edx, const char* text, int str_len, const char* null) {

	auto print_custom = [ecx, edx](std::string prefix, std::string msg) {
		std::string accent = rgb_to_hex_safe(menu::menu_accent[0] * 255, menu::menu_accent[1] * 255, menu::menu_accent[2] * 255);

		std::string formatted = "<font color=\"" + accent + "\">" + prefix + "</font>";
		formatted += "<font color=\"#7d7d7d\"> | </font>";
		formatted += "<font color=\"#bebebe\">" + msg + "</font>";

		return ofunc(ecx, edx, formatted.c_str(), (int)formatted.length(), formatted.c_str());
	};

#define DRN(tok, body) if (notif_match(text, str_len, (tok))) return body

	DRN("#dna#_print_created",       print_custom(push_notice_dna_prefix(), "created config"));
	DRN("#dna#_print_saved",         print_custom(push_notice_dna_prefix(), "saved config"));
	DRN("#dna#_print_loaded",        print_custom(push_notice_dna_prefix(), "loaded config"));
	DRN("#dna#_print_marker_saved",  print_custom(push_notice_dna_prefix(), "markers saved"));
	DRN("#dna#_print_marker_loaded", print_custom(push_notice_dna_prefix(), "markers loaded"));
	DRN("#dna#_print_refreshed",     print_custom(push_notice_dna_prefix(), "refreshed config"));
	DRN("#dna#_print_reloaded",      print_custom(push_notice_dna_prefix(), "reloaded fonts"));
	DRN("#dna#_print_updated_hud",   print_custom(push_notice_dna_prefix(), "force updated"));
	DRN("#dna#_print_saved_c",       print_custom(push_notice_dna_prefix(), "saved checkpoint"));
	DRN("#dna#_print_save_c",        print_custom(push_notice_dna_prefix(), "failed to save position"));
	DRN("#dna#_print_tp_c",          print_custom(push_notice_dna_prefix(), "teleported to checkpoint"));
	DRN("#dna#_print_next_c",        print_custom(push_notice_dna_prefix(), "next position"));
	DRN("#dna#_print_prev_c",        print_custom(push_notice_dna_prefix(), "previous position"));
	DRN("#dna#_print_undid_c",       print_custom(push_notice_dna_prefix(), "undid position"));
	DRN("#dna#_print_jumpbugged",    print_custom(push_notice_dna_prefix(), "jumpbug"));
	DRN("#dna#_print_edgebugged",    print_custom(push_notice_dna_prefix(), "edgebug"));
	DRN("#dna#_print_pixelsurfed",   print_custom(push_notice_dna_prefix(), "pixelsurf"));
	DRN("#dna#_print_unloaded",      print_custom(push_notice_dna_prefix(), "unloaded"));
	DRN("#dna#_print_ps_db_found",   print_custom(push_notice_dna_prefix(), "new ps found!"));

	DRN("#dna#_pixelsurfed",         print_custom(push_notice_dna_prefix(), "pixelsurf"));

	DRN("#billware_print_saved",     print_custom(push_notice_dna_prefix(), "saved"));

	DRN("#dna#_spotify_paused",        print_custom("spotify", "now paused"));
	DRN("#dna#_spotify_advertisement",  print_custom("spotify", "advertisement is playing"));
	DRN("#dna#_spotify_switch",        print_custom("spotify", "now playing: " + features::visuals::current_spotify_song));

#undef DRN

	if (notif_match(text, str_len, "#dna#_print_hit_1")) {
		std::string m = "hit " + std::string(features::misc::hitinfo.player_name) + " for " + std::to_string(features::misc::hitinfo.damage) + " (" + std::to_string(features::misc::hitinfo.health) + " remaining)";
		return print_custom(push_notice_dna_prefix(), m);
	}
	if (notif_match(text, str_len, "#dna#_print_hit_2")) {
		std::string m = "hit " + std::string(features::misc::hitinfo.player_name) + " in " + std::string(features::misc::hitinfo.hitgroup) + " for " + std::to_string(features::misc::hitinfo.damage);
		return print_custom(push_notice_dna_prefix(), m);
	}

	ofunc(ecx, edx, text, str_len, null);
}
