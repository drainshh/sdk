#pragma once
#include "../../sdk/sdk.hpp"
#include "../../hooks/hooks.hpp"
#pragma comment(lib, "windowsapp")

#include <winrt/base.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Imaging.h>

#include <chrono>
#include <functional>
#include <sstream>
#include <mutex>

#include <ppltasks.h>
#include <pplawait.h>

#include <sdkddkver.h>
#include <d3dx9.h>

#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

using namespace winrt;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media::Control;
using namespace Windows::Storage::Streams;
using namespace Windows::Foundation::Collections;
using namespace winrt::Windows::Media::Control;


struct edited_material_t {
	std::string name;        // material name (empty if group-based)
	std::string group;       // texture group name (empty if material-based)
	std::string value;       // "r,g,b"
};

static std::vector<edited_material_t> changed_materials;
static std::vector<i_material*> material_list;
static bool initialized = false;
static int selected_index = -1;
static char new_color[64] = "";
static ImGuiTextFilter mat_filter;
static char group_filter[64] = ""; // user can filter or target a texture group

// helper to parse rgb string
inline vec3_t parse_rgb(const char* str) {
	vec3_t out{ 1.f, 1.f, 1.f };
	sscanf_s(str, "%f, %f, %f", &out.x, &out.y, &out.z);
	return out;
}

class mPlayer
{
private:
	std::mutex mtx;

public:
	std::optional< GlobalSystemMediaTransportControlsSessionManager > sessionManager;
	std::optional< IRandomAccessStreamWithContentType > thumbnail;
	std::optional< GlobalSystemMediaTransportControlsSession > session;
	bool HasMedia = false;
	std::string Title;
	std::string Artist;
	std::string AlbumArtist;
	std::string AlbumTitle;
	std::string SourceAppUserModeId;
	std::string Thumbnail_type;
	int TrackNumber;
	int AlbumTrackCount;
	void* Thumbnail_buffer{ nullptr };
	int Thumbnail_size{ 0 };
	bool hasthumnnail = false;
	bool isPlaying;
	int TotalTime;
	int CurrentTime;
	LPDIRECT3DTEXTURE9 thumb{ nullptr };

	concurrency::task< void > Update(LPDIRECT3DDEVICE9 g_pd3dDevice);
	void Lock() { mtx.lock(); }
	void Unlock() { mtx.unlock(); }
};

extern mPlayer mplayer;

struct IDirect3DTexture9;
inline std::string oldtitle;
inline std::string oldartist;
inline IDirect3DTexture9* albumArtTexture = nullptr;
inline std::string strartist;
inline std::string strtitle;

inline constexpr const static char collide_data[] = {
	char(0xB8), char(0x01), char(0x00), char(0x00), char(0x56), char(0x50), char(0x48), char(0x59), char(0x00), char(0x01),
	char(0x00), char(0x00), char(0x9C), char(0x01), char(0x00), char(0x00), char(0x00), char(0x00), char(0x80), char(0x3F),
	char(0x00), char(0x00), char(0x80), char(0x3F), char(0x00), char(0x00), char(0x80), char(0x3F), char(0x00), char(0x00),
	char(0x00), char(0x00), char(0x20), char(0x16), char(0x6A), char(0xC1), char(0xC0), char(0x0E), char(0x1C), char(0xC1),
	char(0x80), char(0x13), char(0xD0), char(0x3F), char(0xE2), char(0x26), char(0x11), char(0x48), char(0xE2), char(0x26),
	char(0x11), char(0x48), char(0xE2), char(0x26), char(0x11), char(0x48), char(0x72), char(0x4E), char(0x08), char(0xF44),
	char(0xD1), char(0x9C), char(0x01), char(0x00), char(0x80), char(0x01), char(0x00), char(0x00), char(0x00), char(0x00),
	char(0x00), char(0x00), char(0x00), char(0x00), char(0x00), char(0x00), char(0x49), char(0x56), char(0x50), char(0x53),
	char(0xD0), char(0x00), char(0x00), char(0x00), char(0x06), char(0x00), char(0x00), char(0x00), char(0x04), char(0x15),
	char(0x00), char(0x00), char(0x0C), char(0x00), char(0x00), char(0x00), char(0x00), char(0x90), char(0x00), char(0x00),
	char(0x00), char(0x00), char(0x0A), char(0x00), char(0x01), char(0x00), char(0x03), char(0x00), char(0x02), char(0x00),
	char(0x12), char(0x00), char(0x01), char(0x80), char(0x00), char(0x00), char(0x02), char(0x00), char(0xFD), char(0x7F),
	char(0x01), char(0x00), char(0x15), char(0x00), char(0x03), char(0x00), char(0x28), char(0x00), char(0x02), char(0xB0),
	char(0x00), char(0x00), char(0x00), char(0x00), char(0x06), char(0x00), char(0x04), char(0x00), char(0x0F), char(0x00),
	char(0x01), char(0x00), char(0xF6), char(0x7F), char(0x03), char(0xA0), char(0x00), char(0x00), char(0x00), char(0x00),
	char(0x06), char(0x00), char(0x05), char(0x00), char(0x15), char(0x00), char(0x04), char(0x00), char(0xFA), char(0x7F),
	char(0x04), char(0x70), char(0x00), char(0x00), char(0x00), char(0x00), char(0x06), char(0x00), char(0x06), char(0x00),
	char(0x13), char(0x00), char(0x05), char(0x00), char(0xFA), char(0x7F), char(0x05), char(0x60), char(0x00), char(0x00),
	char(0x00), char(0x00), char(0xEE), char(0x7F), char(0x02), char(0x00), char(0x18), char(0x00), char(0x06), char(0x00),
	char(0xFA), char(0x7F), char(0x06), char(0x50), char(0x00), char(0x00), char(0x01), char(0x00), char(0xF1), char(0x7F),
	char(0x04), char(0x00), char(0x03), char(0x00), char(0x03), char(0x00), char(0xEB), char(0x7F), char(0x07), char(0x40),
	char(0x00), char(0x00), char(0x03), char(0x00), char(0xFD), char(0x7F), char(0x04), char(0x00), char(0x04), char(0x00),
	char(0x07), char(0x00), char(0x0C), char(0x00), char(0x08), char(0x10), char(0x00), char(0x00), char(0x05), char(0x00),
	char(0x06), char(0x00), char(0x07), char(0x00), char(0xFC), char(0x7F), char(0x04), char(0x00), char(0xEB), char(0x7F),
	char(0x09), char(0x00), char(0x00), char(0x00), char(0x05), char(0x00), char(0xED), char(0x7F), char(0x06), char(0x00),
	char(0x03), char(0x00), char(0x07), char(0x00), char(0xFA), char(0x7F), char(0x0A), char(0x30), char(0x00), char(0x00),
	char(0x07), char(0x00), char(0xFD), char(0x7F), char(0x06), char(0x00), char(0x03), char(0x00), char(0x03), char(0x00),
	char(0xF4), char(0x7F), char(0x0B), char(0x20), char(0x00), char(0x00), char(0x03), char(0x00), char(0xFD), char(0x7F),
	char(0x06), char(0x00), char(0xE8), char(0x7F), char(0x02), char(0x00), char(0xD8), char(0x7F), char(0x46), char(0xAD),
	char(0x9D), char(0xC3), char(0x1F), char(0x0D), char(0x9C), char(0xC3), char(0x80), char(0xAE), char(0xAA), char(0x43),
	char(0x00), char(0x00), char(0x00), char(0x00), char(0x46), char(0xAD), char(0x9D), char(0xC3), char(0x33), char(0x4C),
	char(0x92), char(0x43), char(0x80), char(0xAE), char(0xAA), char(0x43), char(0x00), char(0x00), char(0x00), char(0x00),
	char(0x46), char(0xAD), char(0x9D), char(0xC3), char(0x1F), char(0x0D), char(0x9C), char(0xC3), char(0x59), char(0x0E),
	char(0xA9), char(0xC3), char(0x00), char(0x00), char(0x00), char(0x00), char(0x46), char(0xAD), char(0x9D), char(0xC3),
	char(0x33), char(0x4C), char(0x92), char(0x43), char(0x59), char(0x0E), char(0xA9), char(0xC3), char(0x00), char(0x00),
	char(0x00), char(0x00), char(0xE4), char(0x0B), char(0x8F), char(0x43), char(0x33), char(0x4C), char(0x92), char(0x43),
	char(0x80), char(0xAE), char(0xAA), char(0x43), char(0x00), char(0x00), char(0x00), char(0x00), char(0xE4), char(0x0B),
	char(0x8F), char(0x43), char(0x1F), char(0x0D), char(0x9C), char(0xC3), char(0x80), char(0xAE), char(0xAA), char(0x43),
	char(0x00), char(0x00), char(0x00), char(0x00), char(0xE4), char(0x0B), char(0x8F), char(0x43), char(0x1F), char(0x0D),
	char(0x9C), char(0xC3), char(0x59), char(0x0E), char(0xA9), char(0xC3), char(0x00), char(0x00), char(0x00), char(0x00),
	char(0xE4), char(0x0B), char(0x8F), char(0x43), char(0x33), char(0x4C), char(0x92), char(0x43), char(0x59), char(0x0E),
	char(0xA9), char(0xC3), char(0x00), char(0x00), char(0x00), char(0x00), char(0x00), char(0x00), char(0x00), char(0x00),
	char(0xB0), char(0xFE), char(0xFF), char(0xFF), char(0x20), char(0x16), char(0x6A), char(0xC1), char(0xC0), char(0x0E),
	char(0x1C), char(0xC1), char(0x80), char(0x13), char(0xD0), char(0x3F), char(0x72), char(0x4E), char(0x08), char(0x44),
	char(0x8A), char(0x8B), char(0x9C), char(0x00), char(0x73), char(0x6F), char(0x6C), char(0x69), char(0x64), char(0x20),
	char(0x7B), char(0x0A), char(0x22), char(0x69), char(0x6E), char(0x64), char(0x65), char(0x78), char(0x22), char(0x20),
	char(0x22), char(0x30), char(0x22), char(0x0A), char(0x22), char(0x6D), char(0x61), char(0x73), char(0x73), char(0x22),
	char(0x20), char(0x22), char(0x35), char(0x30), char(0x30), char(0x30), char(0x30), char(0x2E), char(0x30), char(0x30),
	char(0x30), char(0x30), char(0x30), char(0x30), char(0x22), char(0x0A), char(0x22), char(0x73), char(0x75), char(0x72),
	char(0x66), char(0x61), char(0x63), char(0x65), char(0x70), char(0x72), char(0x6F), char(0x70), char(0x22), char(0x20),
	char(0x22), char(0x64), char(0x65), char(0x66), char(0x61), char(0x75), char(0x6C), char(0x74), char(0x22), char(0x0A),
	char(0x22), char(0x76), char(0x6F), char(0x6C), char(0x75), char(0x6D), char(0x65), char(0x22), char(0x20), char(0x22),
	char(0x31), char(0x35), char(0x30), char(0x38), char(0x30), char(0x32), char(0x33), char(0x32), char(0x30), char(0x35),
	char(0x38), char(0x38), char(0x38), char(0x30), char(0x2E), char(0x30), char(0x30), char(0x30), char(0x30), char(0x30),
	char(0x30), char(0x22), char(0x0A), char(0x7D), char(0x0A), char(0x00)
};


namespace features::chams {
	void material_changer();
	void run(i_mat_render_context* ctx, const draw_model_state_t& state, const model_render_info_t& info, matrix_t* matrix);
	void run_bt(i_mat_render_context* ctx, const draw_model_state_t& state, const model_render_info_t& info, matrix_t* matrix);
	void run_other(i_mat_render_context* ctx, const draw_model_state_t& state, const model_render_info_t& info, matrix_t* matrix);
	void old_shaders();
	void initialize();
	void texture_override();
}

struct bbox_t {
	int x, y, w, h;
	bbox_t() = default;
	bbox_t(const int x, const int y, const int w, const int h) {
		this->x = x;
		this->y = y;
		this->w = w;
		this->h = h;
	}
};

namespace features::visuals {
	inline bool is_spotted = false;
	inline bool has_initalized = false;
	inline bool console_vis = false;
	inline int tick;
	inline int igonre_ticks = 0;
	inline int velocity_old = 0;
	inline vec3_t origin_old(0, 0, 0);
	inline std::string current_spotify_song;

	bool get_playerbox(player_t* entity, bbox_t& in);

	namespace player {
		void run();
		void animate(player_t* entity, const int index);
		void flags(player_t* entity, bbox_t bbox);
		void backtrack_dot(player_t* entity);
		void backtrack_skeleton(player_t* entity);
		void skeleton(player_t* entity);
		void draw_name(player_t* entity, bbox_t bbox);
		void draw_health(player_t* entity, bbox_t bbox);
		void draw_armor(player_t* entity, bbox_t bbox);
		void draw_weapon(player_t* entity, bbox_t bbox);
		void draw_box(player_t* entity, bbox_t bbox);
		void draw_box_background(player_t* entity, bbox_t bbox);
	}

	namespace glow {
		void run();
	}


	void rendericon(LPDIRECT3DDEVICE9 device);
	void clarity_watermark();
	void watermark();
	void init();
	void low_fps_warning();
	void display_spotify();
	void jump_trail();
	void removals();
	void oof_arrows(player_t* player);
	void RenderMediaPlayer();
	void motion_blur(view_setup_t* setup);
	void camera_fix(view_setup_t* setup);
	void cinematic_camera(view_setup_t* setup);
	void robotic_angles(view_setup_t* setup);
	void dropped_weapons(player_t* entity);
	void bomb_esp(entity_t* entity);
	void apply_zoom();
	void fog();
	void gravity_ragdoll();
	void skybox_changer();
	void notification_system();
	void fullbright();
	void flashalpha();
	void nosmoke();
	void dlights(player_t* entity);
	void run_freecam(c_usercmd* cmd, vec3_t angles);
	void freecam(view_setup_t* setup);
	void key_strokes();
	void console();
	void custom_sun();
	void points_system();
	void points_mascot_invalidate_texture();
	void points_mascot_menu_reload();
	const char* points_mascot_status_text();
	void points_mascot_open_folder();
	void jump_distance_display();

	void custom_bloom();

	void adjust_graphics_quality();
	void linear_spectate();

	struct blur_history_t {
		vec3_t previous_pos{};
		vec3_t previous_velocity{};
		float previous_pitch = 0.f;
		float previous_yaw = 0.f;
		float last_time_update = 0.f;
		float no_rotational_mb_until = 0.f;

		// Smoothed values for cinematic feel
		float smoothed_yaw_velocity = 0.f;
		float smoothed_pitch_velocity = 0.f;
		float smoothed_forward_velocity = 0.f;

		// Peak detection for dynamic intensity
		float peak_rotation_speed = 0.f;
		float peak_movement_speed = 0.f;
	};

	static blur_history_t history;

	struct FrameCapHistory {
		vec3_t frame_previous_pos = { 0.0f, 0.0f, 0.0f };
		float frame_last_time_update = 0.0f;
	};
	inline FrameCapHistory frame_history;

	void draw_hit_markers();

}

namespace features::weather {
	void reset_weather(const bool cleanup = true);
	void update_weather();
	void render_overlay_precipitation();
	void* getv_collideble();
	bool should_override_vcollide(int model_index);
	bool has_active_precipitation_backend();

	namespace features::weather {
		void reset_weather(const bool cleanup = true);
		void update_weather();
		void render_overlay_precipitation();

		void* getv_collideble();
		bool should_override_vcollide(int model_index);
		bool has_active_precipitation_backend();
	}
}

namespace features::grenade_prediction {

	class c_nadepoint {
	public:
		c_nadepoint() {
			m_valid = false;
		}

		c_nadepoint(vec3_t start, vec3_t end, bool plane, bool valid, vec3_t normal, bool detonate) {
			m_start = start;
			m_end = end;
			m_plane = plane;
			m_valid = valid;
			m_normal = normal;
			m_detonate = detonate;
		}

		vec3_t m_start, m_end, m_normal;
		bool m_valid, m_plane, m_detonate;
	};

	inline std::array< c_nadepoint, 500 > _points{ };
	inline bool _predicted = false;
	void predict(c_usercmd* ucmd);
	bool detonated(weapon_t* weapon, float time, trace_t& trace);
	void run(c_usercmd* ucmd);
	void draw();
}
