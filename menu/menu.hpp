#pragma once
#include "config/config.hpp"
#include "../utils/render/draw.hpp"
#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <d3d9.h>

namespace gui
{

	constexpr int WIDTH = 700;
	constexpr int HEIGHT = 700;

	// show menu?
	inline bool open = true;

	// is menu open?
	inline bool setup;

	// winapi related
	inline HWND window = nullptr;
	inline WNDCLASSEX windowClass = { };
	inline WNDPROC originalWindowProcess = nullptr;

	// dx stuff
	inline LPDIRECT3DDEVICE9 device = nullptr;
	inline LPDIRECT3D9 d3d9 = nullptr;

	bool SetupWindowClass(const char* windowClassName) noexcept;
	void DestroyWindowClass() noexcept;

	bool SetupWindow(const char* windowName) noexcept;
	void DestroyWindow() noexcept;

	bool SetupDirectX() noexcept;
	void DestroyDirectX() noexcept;

	// setup device
	void Setup();

	void SetupMenu(LPDIRECT3DDEVICE9 device) noexcept;
	void Destroy() noexcept;

	void Render() noexcept;
	void render_checkpoints() noexcept;
}
#define IMGUI_DEFINE_MATH_OPERATORS

class ctab
{
public:
	const char* szName = nullptr;
	void (*render_function)() = nullptr;
};

namespace menu {
	inline int menu_key = 0x2D;
	inline bool open = true;
	inline int main_tab = 0;
	inline bool initialized = false;
	inline bool unhook = true;
	inline int indicator_tab = 0;
	inline int indicators_tab = 0;
	inline int chams_tab = 0;
	inline int esp_tab = 0;
	inline int weapon_selection = 0;
	inline int skin_selection = 0;
	inline bool fonts_initialized;
	inline float menu_accent[4]{ 255 / 255.f, 165 / 255.f, 255 / 255.f, 1.0f };
	inline float menu_accent2[3]{ 255 / 255.f, 255 / 255.f, 255 / 255.f };
	inline float menu_accent3[3]{ 242 / 255.f, 242 / 255.f, 242 / 255.f };
	enum class menu_variant_t {
		drainware_original = 0,
		femboy,
		fight_club,
		bladee,
		ecco2k,
		yung_lean,
		sadboys,
		icedancer,
		redlight,
		gluee,
		evil_occult,
		minimal_ghost,
		industrial_steel,
		cyber_terminal,
		angel_whiteout,
		horror_cursed,
		luxury_gold,
		whitearmor,
		yung_sherman,
		thaiboy,
		eversince,
		exeter,
		tiger,
		warlord,
		unknown_death,
		trash_island,
		spiderr,
		crest,
		cold_visions,
		count
	};

	struct menu_variant_profile_t {
		menu_variant_t id;
		const char* internal_id;
		const char* display_name;
		const char* watermark_label;
		const char* chat_prefix;
		const char* naming_style;
		const char* killsay_pool_label;
		float accent[4];
		float accent2[3];
		float accent3[3];
		bool use_custom_killsay_text;
		int default_menu_fx_mode = 1;
		int default_points_style = 0;
		int default_skybox = 0;
		int default_weather_type = 0;
		bool default_weather_enabled = false;
		float default_fog_color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
		float default_world_color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
		int default_sun_mode = 0;
	};

	enum class world_preset_t {
		lunacy = 0,
		frozen_night,
		whiteout_heaven,
		redlight_danger,
		gluee_dream,
		terminal_green,
		industrial_dusk,
		occult_bloodmoon,
		luxury_gold_night,
		sadboys_fog,
		whitearmor_clean_steel,
		thaiboy_nightlife,
		sherman_midnight,
		count
	};

	struct world_preset_profile_t {
		world_preset_t id;
		const char* internal_id;
		const char* display_name;
		const char* description;
		int skybox = 0;
		bool fog = true;
		int fog_distance = 1500;
		int fog_density = 20;
		float fog_color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
		bool world_modulate = true;
		bool world_modulate_props = true;
		float world_color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
		float world_modulate_intensity = 0.9f;
		bool weather_enabled = false;
		bool weather_use_entity_precip = true;
		int weather_type = 0;
		bool weather_overlay = true;
		float weather_overlay_density = 1.0f;
		float weather_overlay_speed = 1.0f;
		float weather_overlay_opacity = 0.55f;
		float weather_overlay_brightness = 1.0f;
		float weather_overlay_wind = 0.35f;
		float weather_overlay_tint[4]{ 0.82f, 0.88f, 1.0f, 1.0f };
		bool custom_sun = false;
		int custom_sun_mode = 0;
		int custom_sun_x = 50;
		int custom_sun_y = 5;
		int custom_sun_dist = 9000;
		float custom_sun_speed = 18.0f;
		float custom_sun_orbit_range = 25.0f;
	};

	inline constexpr std::size_t k_menu_variant_count = static_cast<std::size_t>(menu_variant_t::count);
	inline constexpr std::size_t k_world_preset_count = static_cast<std::size_t>(world_preset_t::count);

	inline const std::array<menu_variant_profile_t, k_menu_variant_count> menu_variant_profiles{ {
		{ menu_variant_t::drainware_original, "drainware", "drainware / classic", "drainware", "drainware", "classic drain-style", "custom text field", { 1.0f, 0.647f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.949f, 0.949f, 0.949f }, true },
		{ menu_variant_t::femboy, "femboy", "femboy", "femboyware :3", "femboyware :3", "soft pink unserious", "femboy themed pool", { 1.0f, 0.714f, 0.756f, 1.0f }, { 1.0f, 0.86f, 0.9f }, { 1.0f, 0.72f, 0.82f }, false },
		{ menu_variant_t::fight_club, "fightclub", "fight club", "fight club", "fight club", "project mayhem / tyler", "fight club / tyler pool", { 0.6f, 0.0f, 0.0f, 1.0f }, { 0.95f, 0.1f, 0.08f }, { 0.5f, 0.02f, 0.02f }, false },
		{ menu_variant_t::bladee, "bladee", "bladee", "bladee", "bladee", "cyber angel / chrome", "bladee / drain pool", { 0.62f, 0.78f, 1.0f, 1.0f }, { 0.82f, 0.9f, 1.0f }, { 0.36f, 0.52f, 0.78f }, false, 3, 4, 21, 3, true, { 0.62f, 0.74f, 1.0f, 1.0f }, { 0.70f, 0.80f, 1.0f, 1.0f }, 3 },
		{ menu_variant_t::ecco2k, "ecco2k", "ecco2k", "ecco2k", "ecco2k", "glass fashion alien", "ecco2k / peroxide pool", { 0.82f, 0.92f, 1.0f, 1.0f }, { 0.9f, 0.96f, 1.0f }, { 0.66f, 0.82f, 0.92f }, false, 1, 4, 21, 3, false, { 0.72f, 0.88f, 1.0f, 1.0f }, { 0.80f, 0.92f, 1.0f, 1.0f }, 3 },
		{ menu_variant_t::yung_lean, "yunglean", "yung lean", "sadboys", "sadboys", "cloudy blue nostalgia", "yung lean / sadboys pool", { 0.72f, 0.48f, 1.0f, 1.0f }, { 0.82f, 0.66f, 1.0f }, { 0.42f, 0.3f, 0.72f }, false, 3, 2, 16, 3, true, { 0.40f, 0.48f, 0.70f, 1.0f }, { 0.52f, 0.62f, 0.88f, 1.0f }, 1 },
		{ menu_variant_t::sadboys, "sadboys", "sadboys", "sadboys", "sadboys", "blue dream / silver", "sadboys pool", { 0.2f, 0.52f, 1.0f, 1.0f }, { 0.38f, 0.66f, 1.0f }, { 0.16f, 0.28f, 0.55f }, false, 3, 2, 16, 3, true, { 0.32f, 0.46f, 0.78f, 1.0f }, { 0.42f, 0.58f, 0.92f, 1.0f }, 1 },
		{ menu_variant_t::icedancer, "icedancer", "icedancer", "icedancer", "icedancer", "frozen nightclub", "icedancer pool", { 0.66f, 0.95f, 1.0f, 1.0f }, { 0.86f, 0.98f, 1.0f }, { 0.42f, 0.74f, 0.86f }, false, 3, 5, 18, 3, true, { 0.64f, 0.92f, 1.0f, 1.0f }, { 0.68f, 0.92f, 1.0f, 1.0f }, 2 },
		{ menu_variant_t::redlight, "redlight", "redlight", "redlight", "redlight", "crimson neon", "redlight pool", { 1.0f, 0.08f, 0.12f, 1.0f }, { 1.0f, 0.28f, 0.28f }, { 0.55f, 0.02f, 0.04f }, false, 2, 0, 16, 2, false, { 0.70f, 0.08f, 0.08f, 1.0f }, { 1.0f, 0.34f, 0.34f, 1.0f }, 1 },
		{ menu_variant_t::gluee, "gluee", "gluee", "gluee", "gluee", "washed fantasy haze", "gluee pool", { 0.44f, 0.58f, 1.0f, 1.0f }, { 0.62f, 0.72f, 1.0f }, { 0.24f, 0.32f, 0.66f }, false, 3, 5, 21, 3, true, { 0.60f, 0.70f, 1.0f, 1.0f }, { 0.64f, 0.72f, 1.0f, 1.0f }, 3 },
		{ menu_variant_t::evil_occult, "evil_occult", "evil / occult", "blackmetal", "blackmetal", "blackmetal / occult", "evil occult pool", { 0.72f, 0.04f, 0.04f, 1.0f }, { 0.9f, 0.1f, 0.1f }, { 0.18f, 0.18f, 0.18f }, false },
		{ menu_variant_t::minimal_ghost, "minimal_ghost", "minimal / ghost", "ghost", "ghost", "clean white minimal", "minimal ghost pool", { 0.86f, 0.9f, 0.9f, 1.0f }, { 0.92f, 0.96f, 0.96f }, { 0.64f, 0.7f, 0.7f }, false },
		{ menu_variant_t::industrial_steel, "industrial_steel", "industrial / steel", "steelworks", "steelworks", "factory steel", "industrial steel pool", { 0.58f, 0.62f, 0.64f, 1.0f }, { 0.72f, 0.76f, 0.78f }, { 0.32f, 0.36f, 0.38f }, false },
		{ menu_variant_t::cyber_terminal, "cyber_terminal", "cyber / terminal", "terminal", "terminal", "green terminal", "cyber terminal pool", { 0.0f, 0.9f, 0.34f, 1.0f }, { 0.2f, 1.0f, 0.48f }, { 0.02f, 0.35f, 0.16f }, false },
		{ menu_variant_t::angel_whiteout, "angel_whiteout", "angel / whiteout", "whiteout", "whiteout", "heaven whiteout", "angel whiteout pool", { 1.0f, 0.94f, 0.78f, 1.0f }, { 1.0f, 0.96f, 0.86f }, { 0.76f, 0.72f, 0.62f }, false },
		{ menu_variant_t::horror_cursed, "horror_cursed", "horror / cursed", "cursed", "cursed", "cursed tape", "horror cursed pool", { 0.48f, 0.0f, 0.58f, 1.0f }, { 0.72f, 0.16f, 0.82f }, { 0.16f, 0.02f, 0.2f }, false },
		{ menu_variant_t::luxury_gold, "luxury_gold", "luxury / gold", "gold", "gold", "opulent gold", "luxury gold pool", { 1.0f, 0.72f, 0.18f, 1.0f }, { 1.0f, 0.84f, 0.38f }, { 0.6f, 0.42f, 0.12f }, false, 1, 3, 14, 0, false, { 1.0f, 0.82f, 0.44f, 1.0f }, { 1.0f, 0.84f, 0.38f, 1.0f }, 0 },
		{ menu_variant_t::whitearmor, "whitearmor", "whitearmor", "whitearmor", "whitearmor", "frosted silver ambient", "whitearmor pool", { 0.84f, 0.94f, 1.0f, 1.0f }, { 0.94f, 0.98f, 1.0f }, { 0.70f, 0.82f, 0.92f }, false, 3, 1, 21, 3, true, { 0.74f, 0.88f, 1.0f, 1.0f }, { 0.76f, 0.88f, 1.0f, 1.0f }, 3 },
		{ menu_variant_t::yung_sherman, "yung_sherman", "yung sherman", "sherman", "sherman", "dream blue violet haze", "yung sherman pool", { 0.54f, 0.64f, 1.0f, 1.0f }, { 0.72f, 0.58f, 1.0f }, { 0.30f, 0.26f, 0.58f }, false, 1, 2, 16, 3, true, { 0.42f, 0.36f, 0.72f, 1.0f }, { 0.52f, 0.58f, 0.95f, 1.0f }, 1 },
		{ menu_variant_t::thaiboy, "thaiboy", "thaiboy digital", "thaiboy", "thaiboy", "emerald gold luxury", "thaiboy pool", { 1.0f, 0.76f, 0.22f, 1.0f }, { 0.22f, 0.86f, 0.50f }, { 0.50f, 0.36f, 0.10f }, false, 1, 3, 14, 0, false, { 0.85f, 0.70f, 0.36f, 1.0f }, { 0.96f, 0.76f, 0.30f, 1.0f }, 0 },
		{ menu_variant_t::eversince, "eversince", "eversince", "eversince", "eversince", "cold lonely blue", "eversince pool", { 0.45f, 0.64f, 0.96f, 1.0f }, { 0.66f, 0.78f, 1.0f }, { 0.18f, 0.25f, 0.44f }, false, 3, 2, 16, 3, true, { 0.36f, 0.44f, 0.72f, 1.0f }, { 0.46f, 0.56f, 0.88f, 1.0f }, 3 },
		{ menu_variant_t::exeter, "exeter", "exeter", "exeter", "exeter", "green garden glass", "exeter pool", { 0.54f, 0.92f, 0.74f, 1.0f }, { 0.72f, 1.0f, 0.86f }, { 0.18f, 0.46f, 0.34f }, false, 1, 4, 15, 0, false, { 0.56f, 0.86f, 0.72f, 1.0f }, { 0.62f, 0.90f, 0.76f, 1.0f }, 0 },
		{ menu_variant_t::tiger, "tiger", "tiger", "tiger", "tiger", "orange blue bite", "tiger pool", { 1.0f, 0.54f, 0.16f, 1.0f }, { 0.22f, 0.48f, 1.0f }, { 0.52f, 0.24f, 0.08f }, false, 1, 0, 19, 1, false, { 0.88f, 0.50f, 0.22f, 1.0f }, { 1.0f, 0.58f, 0.24f, 1.0f }, 1 },
		{ menu_variant_t::warlord, "warlord", "warlord", "warlord", "warlord", "smoky steel war", "warlord pool", { 0.56f, 0.62f, 0.72f, 1.0f }, { 0.82f, 0.20f, 0.16f }, { 0.22f, 0.24f, 0.30f }, false, 2, 6, 18, 1, true, { 0.38f, 0.42f, 0.50f, 1.0f }, { 0.54f, 0.58f, 0.66f, 1.0f }, 1 },
		{ menu_variant_t::unknown_death, "unknown_death", "unknown death", "unknown death", "unknown death", "dirty purple early lean", "unknown death pool", { 0.42f, 0.20f, 0.62f, 1.0f }, { 0.62f, 0.32f, 0.84f }, { 0.12f, 0.08f, 0.18f }, false, 2, 6, 16, 1, true, { 0.26f, 0.16f, 0.34f, 1.0f }, { 0.42f, 0.28f, 0.58f, 1.0f }, 1 },
		{ menu_variant_t::trash_island, "trash_island", "trash island", "trash island", "trash island", "glossy digital island", "trash island pool", { 0.38f, 0.92f, 0.88f, 1.0f }, { 0.88f, 0.70f, 1.0f }, { 0.12f, 0.44f, 0.42f }, false, 1, 5, 15, 0, false, { 0.42f, 0.84f, 0.78f, 1.0f }, { 0.48f, 0.90f, 0.86f, 1.0f }, 2 },
		{ menu_variant_t::spiderr, "spiderr", "spiderr", "spiderr", "spiderr", "sharp web contrast", "spiderr pool", { 0.92f, 0.92f, 0.96f, 1.0f }, { 0.18f, 0.18f, 0.24f }, { 0.78f, 0.10f, 0.18f }, false, 1, 6, 18, 0, false, { 0.80f, 0.80f, 0.88f, 1.0f }, { 0.80f, 0.82f, 0.90f, 1.0f }, 0 },
		{ menu_variant_t::crest, "crest", "crest", "crest", "crest", "holy silver gold gothic", "crest pool", { 1.0f, 0.88f, 0.56f, 1.0f }, { 0.86f, 0.92f, 1.0f }, { 0.56f, 0.48f, 0.28f }, false, 3, 3, 21, 3, false, { 0.82f, 0.84f, 0.90f, 1.0f }, { 0.90f, 0.86f, 0.72f, 1.0f }, 3 },
		{ menu_variant_t::cold_visions, "cold_visions", "cold visions", "cold visions", "cold visions", "ultra cold cinematic", "cold visions pool", { 0.48f, 0.76f, 1.0f, 1.0f }, { 0.70f, 0.90f, 1.0f }, { 0.06f, 0.10f, 0.16f }, false, 3, 1, 18, 3, true, { 0.26f, 0.38f, 0.62f, 1.0f }, { 0.34f, 0.48f, 0.76f, 1.0f }, 3 }
	} };

	inline const std::array<world_preset_profile_t, k_world_preset_count> world_preset_profiles{ {
		{ world_preset_t::lunacy, "lunacy", "lunacy / moon", "cold white-blue moonlight with soft snow and a deliberate moon lock.", 21, true, 1500, 20, { 0.45f, 0.55f, 0.75f, 1.0f }, true, true, { 0.70f, 0.78f, 1.00f, 1.0f }, 0.90f, true, true, 3, true, 0.95f, 0.70f, 0.42f, 0.95f, 0.18f, { 0.84f, 0.90f, 1.0f, 1.0f }, true, 3, 42, 228, 12000, 10.0f, 24.0f },
		{ world_preset_t::frozen_night, "frozen_night", "frozen night", "Sharp blue frost, deeper shadows, brighter snow, longer moon draw distance.", 18, true, 1650, 28, { 0.34f, 0.46f, 0.72f, 1.0f }, true, true, { 0.62f, 0.74f, 0.92f, 1.0f }, 0.96f, true, true, 3, true, 1.10f, 0.85f, 0.56f, 1.08f, 0.24f, { 0.80f, 0.92f, 1.0f, 1.0f }, true, 3, 38, 212, 18000, 14.0f, 30.0f },
		{ world_preset_t::whiteout_heaven, "whiteout_heaven", "whiteout heaven", "Angel-white haze with airy fog, gentle bloom feel, and a high clean sun.", 11, true, 1900, 12, { 0.92f, 0.94f, 1.0f, 1.0f }, true, true, { 0.94f, 0.96f, 1.0f, 1.0f }, 1.05f, false, false, 0, false, 0.45f, 0.45f, 0.18f, 1.08f, 0.08f, { 1.0f, 0.98f, 0.92f, 1.0f }, true, 0, 18, 92, 22000, 0.0f, 0.0f },
		{ world_preset_t::redlight_danger, "redlight_danger", "redlight danger", "Dark crimson contrast with a warning glow and hostile late-night mood.", 16, true, 1350, 24, { 0.42f, 0.08f, 0.10f, 1.0f }, true, true, { 0.82f, 0.26f, 0.28f, 1.0f }, 0.82f, false, false, 0, false, 0.55f, 0.65f, 0.18f, 0.92f, 0.22f, { 1.0f, 0.28f, 0.22f, 1.0f }, true, 1, 28, 186, 14000, 26.0f, 8.0f },
		{ world_preset_t::gluee_dream, "gluee_dream", "gluee dream", "Dreamy aqua haze, surreal floaty weather, and a softer chrome-tinted moon.", 21, true, 1750, 18, { 0.52f, 0.66f, 0.96f, 1.0f }, true, true, { 0.70f, 0.78f, 1.0f, 1.0f }, 0.92f, true, true, 1, true, 0.70f, 0.58f, 0.36f, 0.96f, 0.16f, { 0.82f, 0.90f, 1.0f, 1.0f }, true, 2, 34, 204, 16000, 18.0f, 34.0f },
		{ world_preset_t::terminal_green, "terminal_green", "terminal green", "Matrix-like green fog, scanline-ready contrast, and square industrial light.", 10, true, 1250, 18, { 0.08f, 0.22f, 0.14f, 1.0f }, true, true, { 0.36f, 0.98f, 0.52f, 1.0f }, 0.72f, true, false, 4, true, 0.85f, 0.72f, 0.24f, 0.76f, 0.04f, { 0.46f, 1.0f, 0.60f, 1.0f }, false, 0, 50, 5, 9000, 0.0f, 0.0f },
		{ world_preset_t::industrial_dusk, "industrial_dusk", "industrial dusk", "Steel-grey dusk with restrained rain and a colder factory palette.", 20, true, 1500, 22, { 0.32f, 0.34f, 0.40f, 1.0f }, true, true, { 0.62f, 0.64f, 0.70f, 1.0f }, 0.86f, true, true, 2, true, 0.90f, 1.10f, 0.30f, 0.84f, 0.28f, { 0.78f, 0.82f, 0.90f, 1.0f }, true, 1, 22, 172, 15000, 20.0f, 12.0f },
		{ world_preset_t::occult_bloodmoon, "occult_bloodmoon", "occult blood moon", "Black-red sky, ash, and a ritual blood-moon atmosphere for cursed themes.", 16, true, 1180, 26, { 0.18f, 0.02f, 0.04f, 1.0f }, true, true, { 0.62f, 0.18f, 0.20f, 1.0f }, 0.78f, true, true, 1, true, 0.82f, 0.54f, 0.38f, 0.74f, 0.18f, { 0.92f, 0.30f, 0.24f, 1.0f }, true, 3, 48, 192, 17000, 9.0f, 18.0f },
		{ world_preset_t::luxury_gold_night, "luxury_gold_night", "luxury gold night", "Opulent gold-tinted nightlife with polished highlights and restrained fog.", 16, true, 1600, 14, { 0.52f, 0.40f, 0.18f, 1.0f }, true, true, { 0.98f, 0.82f, 0.42f, 1.0f }, 0.96f, false, false, 0, false, 0.50f, 0.55f, 0.16f, 1.00f, 0.10f, { 1.0f, 0.86f, 0.52f, 1.0f }, true, 1, 20, 148, 18000, 16.0f, 10.0f },
		{ world_preset_t::sadboys_fog, "sadboys_fog", "sadboys fog", "Melancholy silver-blue fog with muted snowfall and cold distant moonlight.", 16, true, 1400, 25, { 0.32f, 0.44f, 0.62f, 1.0f }, true, true, { 0.56f, 0.68f, 0.86f, 1.0f }, 0.82f, true, true, 3, true, 0.70f, 0.62f, 0.32f, 0.86f, 0.14f, { 0.76f, 0.84f, 1.0f, 1.0f }, true, 3, 44, 220, 15000, 7.5f, 16.0f },
		{ world_preset_t::whitearmor_clean_steel, "whitearmor_clean_steel", "whitearmor clean steel", "Ultra-clean silver-blue steel with refined snow and crisp shadow range.", 21, true, 1750, 16, { 0.72f, 0.84f, 0.96f, 1.0f }, true, true, { 0.82f, 0.90f, 1.0f, 1.0f }, 1.02f, true, true, 3, true, 0.80f, 0.70f, 0.28f, 1.02f, 0.12f, { 0.88f, 0.94f, 1.0f, 1.0f }, true, 3, 34, 210, 22000, 12.0f, 20.0f },
		{ world_preset_t::thaiboy_nightlife, "thaiboy_nightlife", "thaiboy nightlife", "Aqua-and-gold nightlife glow with faster rain and a brighter premium sun arc.", 14, true, 1550, 16, { 0.14f, 0.28f, 0.34f, 1.0f }, true, true, { 0.88f, 0.74f, 0.34f, 1.0f }, 0.94f, true, true, 2, true, 0.96f, 1.18f, 0.34f, 1.06f, 0.30f, { 0.52f, 0.94f, 0.88f, 1.0f }, true, 2, 18, 138, 19000, 24.0f, 26.0f },
		{ world_preset_t::sherman_midnight, "sherman_midnight", "sherman midnight", "Nocturnal purple-blue melancholy with slower snow and distant moon shimmer.", 16, true, 1450, 22, { 0.24f, 0.22f, 0.42f, 1.0f }, true, true, { 0.54f, 0.60f, 0.92f, 1.0f }, 0.84f, true, true, 3, true, 0.76f, 0.54f, 0.30f, 0.92f, 0.12f, { 0.72f, 0.76f, 1.0f, 1.0f }, true, 3, 46, 218, 18000, 11.0f, 22.0f }
	} };

	struct theme_cache_t {
		menu_variant_t variant = menu_variant_t::drainware_original;
		ImVec4 accent{ 1.0f, 0.647f, 1.0f, 1.0f };
		ImVec4 accent_dim{ 1.0f, 0.647f, 1.0f, 0.32f };
		ImU32 accent_u32 = 0;
		ImU32 accent_dim_u32 = 0;
		ImFont* ui_font = nullptr;
		ImFont* watermark_font = nullptr;
		int rebuilds = 0;
		int device_resets = 0;
		bool dirty = true;
	};

	inline theme_cache_t active_theme_cache{};

	inline void mark_theme_cache_dirty() {
		active_theme_cache.dirty = true;
	}

	inline void note_theme_device_reset() {
		++active_theme_cache.device_resets;
		mark_theme_cache_dirty();
	}

	inline constexpr std::size_t menu_variant_index(const menu_variant_t variant) {
		return static_cast<std::size_t>(variant);
	}

	inline constexpr std::size_t world_preset_index(const world_preset_t preset) {
		return static_cast<std::size_t>(preset);
	}

	inline menu_variant_t normalize_menu_variant(const int variant) {
		const int clamped = std::clamp(variant, 0, static_cast<int>(k_menu_variant_count) - 1);
		return static_cast<menu_variant_t>(clamped);
	}

	inline const menu_variant_profile_t& menu_variant_profile(const menu_variant_t variant) {
		return menu_variant_profiles[menu_variant_index(normalize_menu_variant(static_cast<int>(variant)))];
	}

	inline world_preset_t normalize_world_preset(const int preset) {
		const int clamped = std::clamp(preset, 0, static_cast<int>(k_world_preset_count) - 1);
		return static_cast<world_preset_t>(clamped);
	}

	inline const world_preset_profile_t& world_preset_profile(const world_preset_t preset) {
		return world_preset_profiles[world_preset_index(normalize_world_preset(static_cast<int>(preset)))];
	}

	inline int recommended_world_preset_for_variant(const menu_variant_t variant) {
		switch (variant) {
		case menu_variant_t::bladee:
		case menu_variant_t::ecco2k:
		case menu_variant_t::gluee:
		case menu_variant_t::trash_island:
		case menu_variant_t::exeter:
			return static_cast<int>(world_preset_t::gluee_dream);
		case menu_variant_t::icedancer:
		case menu_variant_t::cold_visions:
			return static_cast<int>(world_preset_t::frozen_night);
		case menu_variant_t::whitearmor:
			return static_cast<int>(world_preset_t::whitearmor_clean_steel);
		case menu_variant_t::yung_lean:
		case menu_variant_t::sadboys:
		case menu_variant_t::eversince:
			return static_cast<int>(world_preset_t::sadboys_fog);
		case menu_variant_t::yung_sherman:
		case menu_variant_t::unknown_death:
			return static_cast<int>(world_preset_t::sherman_midnight);
		case menu_variant_t::thaiboy:
			return static_cast<int>(world_preset_t::thaiboy_nightlife);
		case menu_variant_t::luxury_gold:
		case menu_variant_t::crest:
			return static_cast<int>(world_preset_t::luxury_gold_night);
		case menu_variant_t::fight_club:
		case menu_variant_t::redlight:
			return static_cast<int>(world_preset_t::redlight_danger);
		case menu_variant_t::industrial_steel:
		case menu_variant_t::warlord:
		case menu_variant_t::tiger:
			return static_cast<int>(world_preset_t::industrial_dusk);
		case menu_variant_t::cyber_terminal:
			return static_cast<int>(world_preset_t::terminal_green);
		case menu_variant_t::evil_occult:
		case menu_variant_t::horror_cursed:
		case menu_variant_t::spiderr:
			return static_cast<int>(world_preset_t::occult_bloodmoon);
		case menu_variant_t::minimal_ghost:
		case menu_variant_t::angel_whiteout:
			return static_cast<int>(world_preset_t::whiteout_heaven);
		case menu_variant_t::drainware_original:
		case menu_variant_t::femboy:
		default:
			return static_cast<int>(world_preset_t::lunacy);
		}
	}

	inline void apply_world_preset(const world_preset_profile_t& preset) {
		c::visuals::world_preset_index = static_cast<int>(preset.id);
		c::visuals::skybox = preset.skybox;
		c::visuals::fog = preset.fog;
		c::visuals::fog_distance = preset.fog_distance;
		c::visuals::fog_density = preset.fog_density;
		c::visuals::world_modulate = preset.world_modulate;
		c::visuals::world_modulate_props = preset.world_modulate_props;
		c::visuals::world_modulate_skybox = false;
		c::visuals::world_modulate_intensity = preset.world_modulate_intensity;
		c::visuals::enable_weather = preset.weather_enabled;
		c::visuals::weather_use_entity_precip = preset.weather_use_entity_precip;
		c::visuals::weather_type = preset.weather_type;
		c::visuals::weather_overlay = preset.weather_overlay;
		c::visuals::weather_overlay_density = preset.weather_overlay_density;
		c::visuals::weather_overlay_speed = preset.weather_overlay_speed;
		c::visuals::weather_overlay_opacity = preset.weather_overlay_opacity;
		c::visuals::weather_overlay_brightness = preset.weather_overlay_brightness;
		c::visuals::weather_overlay_wind = preset.weather_overlay_wind;
		c::visuals::custom_sun = preset.custom_sun;
		c::visuals::custom_sun_mode = preset.custom_sun_mode;
		c::visuals::custom_sun_x = preset.custom_sun_x;
		c::visuals::custom_sun_y = preset.custom_sun_y;
		c::visuals::custom_sun_dist = preset.custom_sun_dist;
		c::visuals::custom_sun_speed = preset.custom_sun_speed;
		c::visuals::custom_sun_orbit_range = preset.custom_sun_orbit_range;

		for (int i = 0; i < 4; ++i) {
			c::visuals::fog_color[i] = preset.fog_color[i];
			c::visuals::world_color[i] = preset.world_color[i];
			c::visuals::weather_overlay_tint[i] = preset.weather_overlay_tint[i];
		}
	}

	inline void rebuild_theme_cache_if_needed() {
		const menu_variant_t variant = normalize_menu_variant(c::misc::menu_variant);
		const bool accent_changed =
			active_theme_cache.accent.x != menu_accent[0] ||
			active_theme_cache.accent.y != menu_accent[1] ||
			active_theme_cache.accent.z != menu_accent[2] ||
			active_theme_cache.accent.w != menu_accent[3];

		if (!active_theme_cache.dirty && active_theme_cache.variant == variant && !accent_changed)
			return;

		active_theme_cache.variant = variant;
		active_theme_cache.accent = ImVec4(menu_accent[0], menu_accent[1], menu_accent[2], menu_accent[3]);
		active_theme_cache.accent_dim = ImVec4(menu_accent[0], menu_accent[1], menu_accent[2], 0.32f);
		active_theme_cache.accent_u32 = ImGui::ColorConvertFloat4ToU32(active_theme_cache.accent);
		active_theme_cache.accent_dim_u32 = ImGui::ColorConvertFloat4ToU32(active_theme_cache.accent_dim);
		active_theme_cache.ui_font = nullptr;
		active_theme_cache.watermark_font = nullptr;
		active_theme_cache.dirty = false;
		++active_theme_cache.rebuilds;
	}

	inline const theme_cache_t& theme_cache() {
		rebuild_theme_cache_if_needed();
		return active_theme_cache;
	}

	// experimental (Scripts → beta): femboy theme toggles accent + chat prefix; backup for restore
	inline bool experimental_femboy_theme_active = false;
	inline bool experimental_femboy_accent_backed_up = false;
	inline float experimental_femboy_accent_backup[4]{};
	inline bool experimental_fight_club_mode = false;
	inline bool experimental_fight_club_accent_backed_up = false;
	inline float experimental_fight_club_accent_backup[4]{};
	inline bool themed_variant_accent_backed_up = false;
	inline float themed_variant_accent_backup[4]{};
	inline float fade_speed = 1.f / 0.25f;
	inline float alpha = 0.f;
	static const auto red = '\x07';
	inline auto color_p = red;
	inline static int m_tabs = 0;
	inline static int font_tab = 0;

	inline ImVec2 menu_pos = ImVec2();
	inline ImVec2 menu_size = ImVec2();

	inline menu_variant_t active_menu_variant() {
		return normalize_menu_variant(c::misc::menu_variant);
	}

	inline const char* menu_variant_label(const menu_variant_t variant) {
		return menu_variant_profile(variant).display_name;
	}

	inline void restore_variant_accent_backup(float (&backup)[4], bool& backed_up) {
		if (!backed_up)
			return;
		for (int i = 0; i < 4; ++i)
			menu_accent[i] = backup[i];
		backed_up = false;
	}

	inline void backup_current_accent(float (&backup)[4], bool& backed_up) {
		if (backed_up)
			return;
		for (int i = 0; i < 4; ++i)
			backup[i] = menu_accent[i];
		backed_up = true;
	}

	inline void restore_drainware_accent() {
		restore_variant_accent_backup(themed_variant_accent_backup, themed_variant_accent_backed_up);
		experimental_femboy_theme_active = false;
		experimental_fight_club_mode = false;
		c::misc::menu_variant = static_cast<int>(menu_variant_t::drainware_original);
		mark_theme_cache_dirty();
	}

	inline void sync_legacy_variant_flags(const menu_variant_t variant) {
		experimental_femboy_theme_active = variant == menu_variant_t::femboy;
		experimental_fight_club_mode = variant == menu_variant_t::fight_club;
	}

	inline void apply_variant_palette(const menu_variant_profile_t& profile) {
		for (int i = 0; i < 4; ++i)
			menu_accent[i] = profile.accent[i];
		for (int i = 0; i < 3; ++i) {
			menu_accent2[i] = profile.accent2[i];
			menu_accent3[i] = profile.accent3[i];
		}
		mark_theme_cache_dirty();
	}

	inline void apply_variant_hud_fx(const menu_variant_profile_t& profile) {
		c::misc::menu_fx_mode = std::clamp(profile.default_menu_fx_mode, 0, 4);
		c::visuals::points_hud_style = std::clamp(profile.default_points_style, 0, 6);
	}

	inline void apply_variant_visual_preset(const menu_variant_profile_t& profile) {
		apply_world_preset(world_preset_profile(static_cast<world_preset_t>(recommended_world_preset_for_variant(profile.id))));
		c::visuals::skybox = profile.default_skybox;
		c::visuals::enable_weather = profile.default_weather_enabled;
		c::visuals::weather_type = std::clamp(profile.default_weather_type, 0, 4);
		c::visuals::weather_use_entity_precip = true;
		c::visuals::weather_overlay = profile.default_weather_enabled;
		c::visuals::fog = true;

		for (int i = 0; i < 4; ++i) {
			c::visuals::fog_color[i] = profile.default_fog_color[i];
			c::visuals::world_color[i] = profile.default_world_color[i];
		}

		c::visuals::fog_distance = 1800;
		c::visuals::fog_density = profile.default_weather_enabled ? 28 : 18;
		c::visuals::world_modulate = true;
		c::visuals::world_modulate_props = true;
		c::visuals::world_modulate_skybox = false;
		c::visuals::world_modulate_intensity = 0.85f;
		c::visuals::custom_sun = profile.default_sun_mode != 0;
		c::visuals::custom_sun_mode = profile.default_sun_mode;
		c::visuals::custom_sun_dist = profile.default_sun_mode == 3 ? 14000 : 18000;
		c::visuals::custom_sun_speed = profile.default_sun_mode == 3 ? 8.0f : 24.0f;
	}

	inline void apply_menu_variant(const menu_variant_t variant) {
		const menu_variant_t normalized = normalize_menu_variant(static_cast<int>(variant));

		if (normalized == menu_variant_t::drainware_original) {
			restore_drainware_accent();
			sync_legacy_variant_flags(normalized);
			apply_variant_hud_fx(menu_variant_profile(normalized));
			return;
		}

		backup_current_accent(themed_variant_accent_backup, themed_variant_accent_backed_up);
		const auto& profile = menu_variant_profile(normalized);
		apply_variant_palette(profile);
		apply_variant_hud_fx(profile);
		c::misc::menu_variant = static_cast<int>(normalized);
		sync_legacy_variant_flags(normalized);
		mark_theme_cache_dirty();
	}

	void load_font_index();
	void render_tab(const char* szTabBar, const ctab* arrTabs, const std::size_t nTabsCount, int* nCurrentTab, const ImVec4& colActive, const ImVec4& colInactive, bool tabEnabled, ImGuiTabBarFlags flags = ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_NoTooltip);
	bool iskeydown(int key);
	bool iskeyup(int key);
	bool checkkey(int key, int keystyle);
	inline std::vector<std::string> get_installed_fonts();
	inline static std::vector<std::string> fonts = get_installed_fonts();

	inline static const char* agents[] = {
		"Cmdr. Davida 'Goggles' Fernandez | SEAL Frogman",
		"Cmdr. Frank 'Wet Sox' Baroud | SEAL Frogman",
		"Lieutenant Rex Krikey | SEAL Frogman",
		"Michael Syfers | FBI Sniper",
		"Operator | FBI SWAT",
		"Special Agent Ava | FBI",
		"Markus Delrow | FBI HRT",
		"Sous-Lieutenant Medic | Gendarmerie Nationale",
		"Chem-Haz Capitaine | Gendarmerie Nationale",
		"Chef d'Escadron Rouchard | Gendarmerie Nationale",
		"Aspirant | Gendarmerie Nationale",
		"Officer Jacques Beltram | Gendarmerie Nationale",
		"D Squadron Officer | NZSAS",
		"B Squadron Officer | SAS",
		"Seal Team 6 Soldier | NSWC SEAL",
		"Buckshot | NSWC SEAL",
		"Lt. Commander Ricksaw | NSWC SEAL",
		"'Blueberries' Buckshot | NSWC SEAL",
		"3rd Commando Company | KSK",
		"'Two Times' McCoy | TACP Cavalry",
		"''Two Times' McCoy | USAF TACP",
		"'Primeiro Tenente | Brazilian 1st Battalion",
		"'Cmdr. Mae 'Dead Cold' Jamison | SWAT",
		"'1st Lieutenant Farlow | SWAT",
		"'John 'Van Healen' Kask | SWAT",
		"'Bio-Haz Specialist | SWAT",
		"'Sergeant Bombson | SWAT",
		"'Chem-Haz Specialist | SWAT", //
		"'Lieutenant 'Tree Hugger' Farlow | SWAT",
		"Getaway Sally | The Professionals",
		"Number K | The Professionals",
		"Little Kev | The Professionals",
		"Safecracker Voltzmann | The Professionals",
		"dna Darryl The Strapped | The Professionals",
		"Sir dna Loudmouth Darryl | The Professionals",
		"Sir dna Darryl Royale | The Professionals",
		"Sir dna Skullhead Darryl | The Professionals",
		"Sir dna Silent Darryl | The Professionals",
		"Sir dna Miami Darryl | The Professionals",
		"Street Soldier | Phoenix",
		"Soldier | Phoenix",
		"Slingshot | Phoenix",
		"Enforcer | Phoenix",
		"Mr. Muhlik | Elite Crew",
		"Prof. Shahmat | Elite Crew",
		"Osiris | Elite Crew",
		"Ground Rebel | Elite Crew",
		"The Elite Mr. Muhlik | Elite Crew",
		"Trapper | Guerrilla Warfare",
		"Trapper Aggressor | Guerrilla Warfare",
		"Vypa Sista of the Revolution | Guerrilla Warfare",
		"Col. Mangos Dabisi | Guerrilla Warfare",
		"Arno The Overgrown | Guerrilla Warfare",
		"'Medium Rare' Crasswater | Guerrilla Warfare",
		"Crasswater The Forgotten | Guerrilla Warfare",
		"Elite Trapper Solman | Guerrilla Warfare",
		"'The Doctor' Romanov | Sabre",
		"Blackwolf | Sabre",
		"Maximus | Sabre",
		"Dragomir | Sabre",
		"Rezan The Ready | Sabre",
		"Rezan the Redshirt | Sabre",
		"Dragomir | Sabre Footsoldier",
	};
}
