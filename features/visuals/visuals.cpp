#include "../visuals/visuals.hpp"
#include "../../menu/config/config.hpp"
#include "../../menu/menu.hpp"
#include "../features/movement/movement.hpp"
#include "points_mascot_embed.hpp"
#include "wanted_stars_embed.hpp"
#include <imgui/imgui.h>
#include <algorithm>
#include <array>
#include <vector>
#include <filesystem>
#include <string>
#include <cctype>
#include <cfloat>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

#include <mutex>

void* getv_collideble();
bool should_override_vcollide(int model_index);

static LPDIRECT3DTEXTURE9 g_points_mascot_tex = nullptr;
static LPDIRECT3DTEXTURE9 g_points_stars_tex = nullptr;
static bool g_points_mascot_load_gave_up = false;
static bool g_points_stars_load_gave_up = false;
static bool g_points_stars_loaded_from_disk = false;
static IDirect3DDevice9* g_points_tex_device = nullptr;
static std::vector<std::filesystem::path> g_pts_meme_paths;
static std::map<std::wstring, IDirect3DTexture9*> g_pts_texture_cache;
static std::mutex g_pts_cache_mutex;

static void points_clear_texture_cache() {
	std::lock_guard<std::mutex> lock(g_pts_cache_mutex);
	for (auto& pair : g_pts_texture_cache) {
		if (pair.second) {
			pair.second->Release();
		}
	}
	g_pts_texture_cache.clear();
}
static bool g_points_mascot_pending_meme = false;
static int g_mascot_meme_last_idx = -1;
static float g_mascot_reverse_anim_start = -1e9f;
static HRESULT g_pts_mascot_last_d3dx_hr = S_OK;
static std::string g_pts_mascot_status_utf8;
static float g_pts_meme_list_scan_time = -1e9f;
static float g_pts_mascot_last_disk_swap_time = -1e9f;
static constexpr float k_pts_meme_rescan_interval = 8.f;
static constexpr float k_pts_mascot_disk_swap_cooldown = 0.22f;
static constexpr unsigned k_pts_mascot_file_max_edge = 512u;

static IDirect3DDevice9* points_resolve_d3d9_device() {
	if (gui::device)
		return gui::device;
	if (!ImGui::GetCurrentContext())
		return nullptr;
	struct ImGui_ImplDX9_Data { IDirect3DDevice9* pd3dDevice; };
	void* ud = ImGui::GetIO().BackendRendererUserData;
	if (!ud)
		return nullptr;
	return reinterpret_cast<ImGui_ImplDX9_Data*>(ud)->pd3dDevice;
}

static void points_sync_render_device(IDirect3DDevice9* dev) {
	if (dev == g_points_tex_device)
		return;

	points_clear_texture_cache();

	if (g_points_mascot_tex) {
		g_points_mascot_tex = nullptr;
	}
	if (g_points_stars_tex) {
		g_points_stars_tex->Release();
		g_points_stars_tex = nullptr;
	}
	g_points_mascot_load_gave_up = false;
	g_points_stars_load_gave_up = false;
	g_points_stars_loaded_from_disk = false;
	g_points_tex_device = dev;
}

// GTA star sheet: black on white -> ARGB white with alpha (ImGui multiply-friendly).
static void points_texture_whiten_dark_on_white(IDirect3DTexture9* tex) {
	if (!tex)
		return;
	D3DSURFACE_DESC d{};
	if (FAILED(tex->GetLevelDesc(0, &d)))
		return;
	if (d.Format != D3DFMT_A8R8G8B8 && d.Format != D3DFMT_X8R8G8B8)
		return;
	D3DLOCKED_RECT lr{};
	if (FAILED(tex->LockRect(0, &lr, nullptr, 0)))
		return;

	for (UINT y = 0; y < d.Height; ++y) {
		BYTE* row = static_cast<BYTE*>(lr.pBits) + y * lr.Pitch;
		for (UINT x = 0; x < d.Width; ++x) {
			DWORD* px = reinterpret_cast<DWORD*>(row + x * 4);
			const DWORD c = *px;
			const int b = int(c & 0xFFu);
			const int g = int((c >> 8) & 0xFFu);
			const int r = int((c >> 16) & 0xFFu);
			const int lum = (r + g + b) / 3;
			if (lum > 235)
				*px = 0u;
			else {
				const BYTE a = static_cast<BYTE>((std::min)(255, 290 - lum));
				*px = (DWORD(a) << 24) | 0x00FFFFFFu;
			}
		}
	}
	tex->UnlockRect(0);
}

static void points_push_root_unique(std::vector<std::filesystem::path>& roots, const std::filesystem::path& p) {
	for (const auto& x : roots) {
		if (x == p)
			return;
	}
	roots.push_back(p);
}

static void points_push_module_resources_by_name(std::vector<std::filesystem::path>& roots, const wchar_t* dll_name) {
	HMODULE m = GetModuleHandleW(dll_name);
	if (!m)
		return;
	wchar_t buf[MAX_PATH]{};
	if (!GetModuleFileNameW(m, buf, MAX_PATH))
		return;
	points_push_root_unique(roots, std::filesystem::path(buf).parent_path() / L"resources");
}

static void points_get_resource_roots(std::vector<std::filesystem::path>& roots) {
	roots.clear();
	// Typical inject output names (must match the built DLL filename)
	points_push_module_resources_by_name(roots, L"release.dll");
	points_push_module_resources_by_name(roots, L"debug.dll");
	points_push_module_resources_by_name(roots, L"sdk.dll");
	wchar_t buf[MAX_PATH]{};
	HMODULE self = nullptr;
	if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCWSTR>(&points_get_resource_roots), &self)
		&& GetModuleFileNameW(self, buf, MAX_PATH))
		points_push_root_unique(roots, std::filesystem::path(buf).parent_path() / L"resources");
	if (GetModuleFileNameW(nullptr, buf, MAX_PATH))
		points_push_root_unique(roots, std::filesystem::path(buf).parent_path() / L"resources");
	std::error_code ec;
	points_push_root_unique(roots, std::filesystem::current_path(ec) / L"resources");
	points_push_root_unique(roots, std::filesystem::path(L"C:\\dna") / L"resources");
}

static std::filesystem::path points_find_pts_memes_dir() {
	std::vector<std::filesystem::path> roots;
	points_get_resource_roots(roots);
	std::error_code ec;
	for (const auto& r : roots) {
		const auto p = r / L"pts_memes";
		if (std::filesystem::is_directory(p, ec))
			return p;
	}
    // If no pts_memes folder was found in the resource roots, prefer C:\dna\resources\pts_memes.
	// Create the folder if it doesn't exist so the user has a known location to drop files.
	const std::filesystem::path fallback = std::filesystem::path(L"C:\\\\dna") / L"resources" / L"pts_memes";
	// Try create parent and the folder itself; ignore errors but return the path only if it exists afterwards.
	if (!std::filesystem::exists(fallback, ec)) {
		std::error_code create_ec;
		std::filesystem::create_directories(fallback.parent_path(), create_ec);
		std::filesystem::create_directories(fallback, create_ec);
	}
	if (std::filesystem::is_directory(fallback, ec))
		return fallback;

	return {};
}

static std::string points_ext_lower(const std::filesystem::path& path) {
	std::string e = path.extension().string();
	for (char& c : e)
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	return e;
}

static void points_refresh_meme_file_list(bool force) {
	const float now = interfaces::globals ? interfaces::globals->realtime : 0.f;
	if (!force && !g_pts_meme_paths.empty() && (now - g_pts_meme_list_scan_time) < k_pts_meme_rescan_interval)
		return;
	g_pts_meme_list_scan_time = now;
	g_pts_meme_paths.clear();
	const std::filesystem::path dir = points_find_pts_memes_dir();
	if (dir.empty())
		return;
	std::error_code ec;
	for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
		if (!entry.is_regular_file())
			continue;
		const std::filesystem::path& p = entry.path();
		const std::string ext = points_ext_lower(p);
		if (ext != ".png" && ext != ".jpg" && ext != ".jpeg" && ext != ".bmp")
			continue;
		g_pts_meme_paths.push_back(std::filesystem::absolute(p, ec));
	}
	std::sort(g_pts_meme_paths.begin(), g_pts_meme_paths.end(),
		[](const std::filesystem::path& a, const std::filesystem::path& b) { return a.filename() < b.filename(); });
}

static int points_meme_pick_index(int prev, size_t n) {
	if (n == 0)
		return -1;
	uint32_t s = uint32_t(interfaces::globals->tick_count * 1664525u + 1013904223u);
	for (int k = 0; k < 16; ++k) {
		const int i = int(s % unsigned(n));
		if (i != prev || n <= 1)
			return i;
		s = s * 1103515245u + 12345u;
	}
	return 0;
}

// max_edge > 0: downscale first (faster decode for HUD memes). max_edge == 0: full quality (e.g. star sheet).
static bool points_load_texture_from_file_w(IDirect3DDevice9* dev, const std::filesystem::path& file,
	IDirect3DTexture9** out_tex, unsigned max_edge = 0) {
	*out_tex = nullptr;
	const std::wstring w = file.wstring();

	{
		std::lock_guard<std::mutex> lock(g_pts_cache_mutex);
		if (g_pts_texture_cache.count(w)) {
			*out_tex = g_pts_texture_cache[w];
			return true;
		}
	}

	g_pts_mascot_last_d3dx_hr = S_OK;
	HRESULT hr = E_FAIL;

	if (max_edge > 0) {
		hr = D3DXCreateTextureFromFileExW(dev, w.c_str(), max_edge, max_edge, 1, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
			D3DX_FILTER_LINEAR, D3DX_FILTER_NONE, 0, nullptr, nullptr, out_tex);
		g_pts_mascot_last_d3dx_hr = hr;
	}

	if (FAILED(hr) || !*out_tex) {
		if (*out_tex) {
			(*out_tex)->Release();
			*out_tex = nullptr;
		}
		hr = D3DXCreateTextureFromFileExW(dev, w.c_str(), D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_UNKNOWN,
			D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, D3DX_FILTER_NONE, 0, nullptr, nullptr, out_tex);
		g_pts_mascot_last_d3dx_hr = hr;
	}
	if (FAILED(hr) || !*out_tex) {
		if (*out_tex) {
			(*out_tex)->Release();
			*out_tex = nullptr;
		}
		hr = D3DXCreateTextureFromFileExW(dev, w.c_str(), 256, 256, 1, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
			D3DX_FILTER_LINEAR, D3DX_FILTER_NONE, 0, nullptr, nullptr, out_tex);
		g_pts_mascot_last_d3dx_hr = hr;
	}
	if (FAILED(hr) || !*out_tex) {
		if (*out_tex) {
			(*out_tex)->Release();
			*out_tex = nullptr;
		}
		hr = D3DXCreateTextureFromFileExW(dev, w.c_str(), D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_UNKNOWN,
			D3DPOOL_DEFAULT, D3DX_FILTER_LINEAR, D3DX_FILTER_NONE, 0, nullptr, nullptr, out_tex);
		g_pts_mascot_last_d3dx_hr = hr;
	}

	if (SUCCEEDED(hr) && *out_tex) {
		std::lock_guard<std::mutex> lock(g_pts_cache_mutex);
		g_pts_texture_cache[w] = *out_tex;
		return true;
	}

	return false;
}

static void points_try_roll_mascot_from_disk(IDirect3DDevice9* dev) {
	if (!g_points_mascot_pending_meme || !dev)
		return;

	const float now = interfaces::globals ? interfaces::globals->realtime : 0.f;
	if ((now - g_pts_mascot_last_disk_swap_time) < k_pts_mascot_disk_swap_cooldown)
		return;

	g_points_mascot_pending_meme = false;
	g_pts_mascot_last_disk_swap_time = now;
	points_refresh_meme_file_list(false);
	if (g_pts_meme_paths.empty())
		return;

	const int idx = points_meme_pick_index(g_mascot_meme_last_idx, g_pts_meme_paths.size());
	if (idx < 0)
		return;
	g_mascot_meme_last_idx = idx;

	IDirect3DTexture9* new_tex = nullptr;
	if (!points_load_texture_from_file_w(dev, g_pts_meme_paths[size_t(idx)], &new_tex, k_pts_mascot_file_max_edge))
		return;

	// Do NOT release here anymore, g_pts_texture_cache owns the lifetime.
	g_points_mascot_tex = new_tex;
	if (interfaces::globals)
		g_mascot_reverse_anim_start = interfaces::globals->realtime;
}

struct points_stars_disk_candidate {
	std::filesystem::path path;
	bool apply_whiten;
};

static points_stars_disk_candidate points_find_wanted_stars_file() {
	static const struct {
		const wchar_t* name;
		bool whiten;
	} k[] = {
		{ L"wanted_stars.png", false },
		{ L"pts_stars.png", false },
		{ L"wanted_stars_sheet.png", true },
		{ L"stars.png", false },
	};
	std::vector<std::filesystem::path> roots;
	points_get_resource_roots(roots);
	std::error_code ec;
	for (const auto& r : roots) {
		for (const auto& e : k) {
			const auto p = r / e.name;
			if (std::filesystem::is_regular_file(p, ec))
				return { std::filesystem::absolute(p, ec), e.whiten };
		}
	}
	return {};
}

static bool points_load_mascot_embed_jpeg(IDirect3DDevice9* dev) {
	if (!dev)
		return false;
	HRESULT hr = D3DXCreateTextureFromFileInMemoryEx(
		dev,
		g_points_mascot_jpg,
		g_points_mascot_jpg_size,
		96,
		96,
		1,
		0,
		D3DFMT_UNKNOWN,
		D3DPOOL_MANAGED,
		D3DX_FILTER_LINEAR,
		D3DX_FILTER_NONE,
		0,
		nullptr,
		nullptr,
		&g_points_mascot_tex);

	if (FAILED(hr)) {
		hr = D3DXCreateTextureFromFileInMemoryEx(
			dev,
			g_points_mascot_jpg,
			g_points_mascot_jpg_size,
			D3DX_DEFAULT,
			D3DX_DEFAULT,
			1,
			0,
			D3DFMT_UNKNOWN,
			D3DPOOL_MANAGED,
			D3DX_FILTER_LINEAR,
			D3DX_FILTER_NONE,
			0,
			nullptr,
			nullptr,
			&g_points_mascot_tex);
	}
	if (FAILED(hr)) {
		hr = D3DXCreateTextureFromFileInMemoryEx(
			dev,
			g_points_mascot_jpg,
			g_points_mascot_jpg_size,
			96,
			96,
			1,
			0,
			D3DFMT_UNKNOWN,
			D3DPOOL_DEFAULT,
			D3DX_FILTER_LINEAR,
			D3DX_FILTER_NONE,
			0,
			nullptr,
			nullptr,
			&g_points_mascot_tex);
	}
	if (FAILED(hr)) {
		g_points_mascot_load_gave_up = true;
		return false;
	}
	return true;
}

void features::visuals::points_mascot_invalidate_texture() {
	points_clear_texture_cache();

	if (g_points_mascot_tex) {
		g_points_mascot_tex = nullptr;
	}
	if (g_points_stars_tex) {
		g_points_stars_tex->Release();
		g_points_stars_tex = nullptr;
	}
	g_points_mascot_load_gave_up = false;
	g_points_stars_load_gave_up = false;
	g_points_stars_loaded_from_disk = false;
	g_points_tex_device = nullptr;
	g_points_mascot_pending_meme = false;
	g_mascot_meme_last_idx = -1;
	g_pts_meme_paths.clear();
	g_mascot_reverse_anim_start = -1e9f;
	g_pts_meme_list_scan_time = -1e9f;
}
//#include "../visuals/display/display.hpp"

mPlayer mplayer;

char* alloc_wcstcs(winrt::hstring source)
{
	char* string_alloc = (char*)malloc((source.size() + 1) * sizeof(char));
	wcstombs(string_alloc, source.c_str(), source.size() + 1);
	return string_alloc;
}

//TODO: UNDERSTAND WHY SOME SYMBOLS ARENT DECODING PROPERLY

void features::visuals::points_mascot_menu_reload() {
	IDirect3DDevice9* dev = points_resolve_d3d9_device();
	g_points_mascot_load_gave_up = false;

	// Clear cache on reload to pick up new/changed files
	points_clear_texture_cache();

	if (g_points_mascot_tex) {
		// No need to release here if it was in the cache, points_clear_texture_cache already did it.
		// But we'll null it just in case.
		g_points_mascot_tex = nullptr;
	}

	std::vector<std::filesystem::path> roots;
	points_get_resource_roots(roots);
	std::ostringstream roots_ss;
	for (size_t i = 0; i < roots.size(); ++i) {
		if (i)
			roots_ss << " | ";
		roots_ss << wstring_to_utf8(roots[i].wstring());
	}

	points_refresh_meme_file_list(true);
	const std::filesystem::path meme_dir = points_find_pts_memes_dir();
	const std::string meme_dir_u8 = meme_dir.empty() ? std::string("(not found)") : wstring_to_utf8(meme_dir.wstring());

	auto append_hr = [](std::string& s, HRESULT hr) {
		char b[24]{};
		snprintf(b, sizeof(b), " HRESULT=0x%08X", static_cast<unsigned>(hr));
		s += b;
	};

	if (!dev) {
		g_pts_mascot_status_utf8 = "pts mascot: no D3D device yet. Resources tried: " + roots_ss.str();
		return;
	}

	if (meme_dir.empty()) {
		g_pts_mascot_status_utf8 = "pts_memes folder not found. Put resources\\pts_memes next to the cheat DLL (e.g. release.dll). Roots: "
			+ roots_ss.str();
		if (points_load_mascot_embed_jpeg(dev))
			g_pts_mascot_status_utf8 += " | Embedded JPEG loaded as fallback.";
		else
			append_hr(g_pts_mascot_status_utf8, g_pts_mascot_last_d3dx_hr);
		return;
	}

	if (g_pts_meme_paths.empty()) {
		g_pts_mascot_status_utf8 = "pts_memes has no .png/.jpg/.jpeg/.bmp (or folder unreadable): " + meme_dir_u8;
		if (points_load_mascot_embed_jpeg(dev))
			g_pts_mascot_status_utf8 += " | Embedded JPEG fallback OK.";
		return;
	}

	IDirect3DTexture9* new_tex = nullptr;
	const auto& first = g_pts_meme_paths[0];
	if (points_load_texture_from_file_w(dev, first, &new_tex, k_pts_mascot_file_max_edge)) {
		g_points_mascot_tex = new_tex;
		g_mascot_meme_last_idx = 0;
		if (interfaces::globals)
			g_mascot_reverse_anim_start = interfaces::globals->realtime;
		g_pts_mascot_status_utf8 = "pts mascot OK: " + wstring_to_utf8(first.filename().wstring()) + " ("
			+ std::to_string(g_pts_meme_paths.size()) + " files) " + meme_dir_u8;
		return;
	}

	g_pts_mascot_status_utf8 = "D3DX load failed: " + wstring_to_utf8(first.wstring());
	append_hr(g_pts_mascot_status_utf8, g_pts_mascot_last_d3dx_hr);
	if (points_load_mascot_embed_jpeg(dev))
		g_pts_mascot_status_utf8 += " | Embedded JPEG fallback OK.";
}

const char* features::visuals::points_mascot_status_text() {
	return g_pts_mascot_status_utf8.c_str();
}

void features::visuals::points_mascot_open_folder() {
	points_refresh_meme_file_list(false);
	std::filesystem::path open = points_find_pts_memes_dir();
	std::error_code ec;
	if (open.empty()) {
		std::vector<std::filesystem::path> roots;
		points_get_resource_roots(roots);
		for (const auto& r : roots) {
			const auto candidate = r / L"pts_memes";
			if (std::filesystem::is_directory(candidate, ec)) {
				open = candidate;
				break;
			}
			if (std::filesystem::is_directory(r, ec)) {
				open = r;
				break;
			}
		}
	}
	if (open.empty())
		return;
	const std::wstring w = open.wstring();
	ShellExecuteW(nullptr, L"explore", w.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void* LastThumb;
std::string PreviousTitle;
std::string PreviousArtist;
bool pending_texture_creation = false;

concurrency::task< void > mPlayer::Update(LPDIRECT3DDEVICE9 g_pd3dDevice)
{
	static auto sessions = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
	auto currentSession = sessions.GetCurrentSession();
	
	this->Lock();
	mplayer.session = currentSession;

	if (this->session != nullptr) {
		this->HasMedia = true;
		this->SourceAppUserModeId = wstring_to_utf8(this->session->SourceAppUserModelId().c_str());

		auto info = this->session->TryGetMediaPropertiesAsync().get();

		std::string currentTitle = wstring_to_utf8(info.Title().c_str());
		std::string currentArtist = wstring_to_utf8(info.Artist().c_str());
		bool trackChanged = (currentTitle != PreviousTitle) || (currentArtist != PreviousArtist);

		this->Title = currentTitle;
		this->Artist = currentArtist;
		this->AlbumArtist = wstring_to_utf8(info.AlbumArtist().c_str());
		this->AlbumTitle = wstring_to_utf8(info.AlbumTitle().c_str());
		this->TrackNumber = info.TrackNumber();
		this->AlbumTrackCount = info.AlbumTrackCount();

		// Update global strings for compatibility
		strtitle = this->Title;
		strartist = this->Artist;

		if (info.Thumbnail() && trackChanged) {
			auto thumbnail_stream = info.Thumbnail().OpenReadAsync().get();
			this->Thumbnail_type = wstring_to_utf8(thumbnail_stream.ContentType().c_str());

			Buffer buffer = Buffer(thumbnail_stream.Size());
			thumbnail_stream.ReadAsync(buffer, buffer.Capacity(), InputStreamOptions::ReadAhead).get();

			void* new_buffer = malloc(buffer.Length());
			if (new_buffer) {
				memcpy(new_buffer, buffer.data(), buffer.Length());
				
				if (this->Thumbnail_buffer) {
					free(this->Thumbnail_buffer);
				}

				this->Thumbnail_buffer = new_buffer;
				this->Thumbnail_size = buffer.Length();
				pending_texture_creation = true;
			}
			
			PreviousTitle = currentTitle;
			PreviousArtist = currentArtist;
		}
		else if (!info.Thumbnail()) {
			this->Thumbnail_size = 0;
		}

		auto timelineProperties = this->session->GetTimelineProperties();
		auto duration = timelineProperties.EndTime() - timelineProperties.StartTime();
		this->TotalTime = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

		auto currentPos = timelineProperties.Position() - timelineProperties.StartTime();
		this->CurrentTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentPos).count();

		auto playbackInfo = this->session->GetPlaybackInfo();
		if (playbackInfo) {
			auto playbackStatus = playbackInfo.PlaybackStatus();
			this->isPlaying = (playbackStatus == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing);
		}
	}
	else {
		this->HasMedia = false;
		strtitle.clear();
		strartist.clear();
	}
	this->Unlock();
	co_return;
}

static std::chrono::steady_clock::time_point progressStartTime = std::chrono::steady_clock::now();
static double calculatedPositionMs = 0.0;
static std::string lastTitle = "";
int last;

void UpdateCalculatedTrackPosition(mPlayer& mplayer)
{
	if (lastTitle != mplayer.Title) {
		progressStartTime = std::chrono::steady_clock::now();
		lastTitle = mplayer.Title;
		calculatedPositionMs = static_cast<double>(mplayer.CurrentTime);
		last = mplayer.CurrentTime;
	}

	if (mplayer.isPlaying) {
		auto now = std::chrono::steady_clock::now();
		std::chrono::duration<double, std::milli> delta = now - progressStartTime;
		if (mplayer.CurrentTime != last) {
			calculatedPositionMs = static_cast<double>(mplayer.CurrentTime);
			last = mplayer.CurrentTime;
		}
		else {
			calculatedPositionMs += delta.count();
		}
		progressStartTime = now;
	}
}

void features::visuals::RenderMediaPlayer()
{
	if (!c::misc::show_spotify_currently_playing)
		return;

	mplayer.Lock();
	UpdateCalculatedTrackPosition(mplayer);

	// Safe texture creation on render thread
	if (pending_texture_creation && mplayer.Thumbnail_buffer && mplayer.Thumbnail_size > 0) {
		if (mplayer.thumb) {
			mplayer.thumb->Release();
			mplayer.thumb = nullptr;
		}

		if (SUCCEEDED(D3DXCreateTextureFromFileInMemoryEx(gui::device, mplayer.Thumbnail_buffer, mplayer.Thumbnail_size, 30, 30, D3DX_DEFAULT, 1,
			D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &mplayer.thumb))) {
			albumArtTexture = mplayer.thumb;
		}
		pending_texture_creation = false;
	}

	float progress = 0.0f;
	if (mplayer.TotalTime > 0) {
		progress = static_cast<float>(calculatedPositionMs) / static_cast<float>(mplayer.TotalTime);
		if (progress > 1.0f)
			progress = 1.0f;
	}

	static float smoothProgress = 0.0f;
	if (progress < smoothProgress)
		smoothProgress = progress;
	smoothProgress += (progress - smoothProgress) * 0.1f;

	static ImVec2 sz{ };
	int x, y;
	interfaces::engine->get_screen_size(x, y);
	float m = c::misc::watermark ? 30 : 0;
	ImGui::SetNextWindowPos({ x - sz.x + 2, 4.5f + m });

	ImGui::Begin("Media Player", nullptr,
		ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

	float padding = 10.0f;
	float imageWidth = 30.0f;
	float imageHeight = 30.0f;


	auto sizex1 = ImGui::CalcTextSize(strartist.c_str()).x;
	auto sizex2 = ImGui::CalcTextSize(strtitle.c_str()).x;

	float sizey1 = y * 1.5f / 720.f;
	float sizey2 = y * 1.5f / 720.f;

	auto text_size1 = im_render.measure_text(strartist.c_str(), fonts::esp_name_font, 12.f);
	auto text_size2 = im_render.measure_text(strtitle.c_str(), fonts::esp_name_font, 12.f);

	float windowWidth = ImGui::GetWindowSize().x;

	ImGui::PushFont(fonts::esp_name_font);

	if (albumArtTexture) {
		ImGui::SetCursorPos(ImVec2(windowWidth - imageWidth - padding, 3));
		ImGui::Image(albumArtTexture, ImVec2(imageWidth, imageWidth));
	}
	if (albumArtTexture)
		ImGui::SetCursorPos({ windowWidth - imageWidth - padding - text_size1.x - padding + 1 + 2, 3 + imageHeight / 2 - (text_size1.y) / 2 + 6 + 1 });
	else
		ImGui::SetCursorPos({ windowWidth - padding - text_size1.x - padding + 1 + 2, 3 + imageHeight / 2 - (text_size1.y) / 2 + 6 + 1 });

	ImGui::TextColored(ImVec4(0.f, 0.f, 0.f, 0.7f), strartist.c_str());
	if (albumArtTexture)
		ImGui::SetCursorPos({ windowWidth - imageWidth - padding - text_size1.x - padding + 2, 3 + imageHeight / 2 - (text_size1.y) / 2 + 6 });
	else
		ImGui::SetCursorPos({ windowWidth - padding - text_size1.x - padding + 2, 3 + imageHeight / 2 - (text_size1.y) / 2 + 6 });
	ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 0.7f), strartist.c_str());
	if (albumArtTexture)
		ImGui::SetCursorPos({ windowWidth - imageWidth - padding - text_size2.x - padding + 1 + 2, 3 + imageHeight / 2 - (text_size2.y) / 2 - 6 + 1 });
	else
		ImGui::SetCursorPos({ windowWidth - padding - text_size2.x - padding + 1 + 2, 3 + imageHeight / 2 - (text_size2.y) / 2 - 6 + 1 });

	ImGui::TextColored(ImVec4(0.f, 0.f, 0.f, 0.7f), strtitle.c_str());
	if (albumArtTexture)
		ImGui::SetCursorPos({ windowWidth - imageWidth - padding - text_size2.x - padding + 2, 3 + imageHeight / 2 - (text_size2.y) / 2 - 6 });
	else
		ImGui::SetCursorPos({ windowWidth - padding - text_size2.x - padding + 2, 3 + imageHeight / 2 - (text_size2.y) / 2 - 6 });
	ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, 0.7f), strtitle.c_str());

	if (albumArtTexture && c::misc::progressbar_enable) {
		ImGui::PushItemWidth(108);
		ImGui::SetCursorPos({ sz.x - imageWidth - padding - 78, 40 });

		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.f, 1.f, 1.f, 0.15f));            // background
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 0.7f));       // progress fill
		ImGui::PushStyleColor(ImGuiCol_PlotHistogramHovered, ImVec4(1.f, 1.f, 1.f, 0.7f)); // hover fill
		ImGui::ProgressBar(smoothProgress, ImVec2(0.0f, 2.0f));
		ImGui::PopStyleColor(3);
		ImGui::PopItemWidth();
	}
	ImGui::Spacing();

	ImGui::SetWindowSize({ 400, 100 });
	sz = ImGui::GetWindowSize();

	ImGui::PopFont();

	ImGui::End();
	mplayer.Unlock();
}

void features::visuals::display_spotify() {
	if (!c::misc::show_spotify_currently_playing)
		return;

	int w, h;
	std::string name;
	interfaces::engine->get_screen_size(w, h);
	h = c::misc::watermark ? 30 : 5;

	//simple check for " - " in the track name
	//cuz its usually used when artist name is in the track title
	if (strtitle.find(" - ") == std::string::npos && !strartist.empty()) {
		name = strartist + " - " + strtitle;
	}
	else {
		name = strtitle;
	}

	auto text_size = im_render.get_text_size(name.c_str(), fonts::esp_name_font, 0.f, 12.f);
	auto paused_size = im_render.get_text_size("", fonts::esp_name_font, 0.f, 12.f);

	if (mplayer.isPlaying) {
		//ImGui::GetBackgroundDrawList()->AddText(fonts::esp_name_font, 12.f, ImVec2(w - 6 - text_size + 1, h + 1), ImColor(0, 0, 0, 255), name.c_str());
		//ImGui::GetBackgroundDrawList()->AddText(fonts::esp_name_font, 12.f, ImVec2(w - 6 - text_size, h), ImColor(255, 255, 255, 255), name.c_str());
	}
	else {
		ImGui::GetBackgroundDrawList()->AddText(fonts::esp_name_font, 12.f, ImVec2(w - 6 - paused_size + 1, h + 1), ImColor(0, 0, 0, 255), "");
		ImGui::GetBackgroundDrawList()->AddText(fonts::esp_name_font, 12.f, ImVec2(w - 6 - paused_size, h), ImColor(255, 255, 255, 255), "");
	}
}

void draw_screen_effect(i_material* material) {
	static auto fn = find_pattern("client.dll", "55 8B EC 83 E4 ? 83 EC ? 53 56 57 8D 44 24 ? 89 4C 24 ?");
	int w, h;
	interfaces::engine->get_screen_size(w, h);
	__asm {
		push h
		push w
		push 0
		xor edx, edx
		mov ecx, material
		call fn
		add esp, 12
	}
}

// Motion blur history structure
struct motion_blur_history_t {
	vec3_t last_position;
	float last_pitch;
	float last_yaw;
	float last_update_time;
	float disable_rotational_until;
	bool initialized;

	motion_blur_history_t() :
		last_position(0, 0, 0),
		last_pitch(0.0f),
		last_yaw(0.0f),
		last_update_time(0.0f),
		disable_rotational_until(0.0f),
		initialized(false) {}
};

static motion_blur_history_t g_mb_history;

// Normalize angle to -180 to 180 range
inline float normalize_angle(float angle) {
	angle = fmodf(angle, 360.0f);
	if (angle > 180.0f)
		angle -= 360.0f;
	else if (angle < -180.0f)
		angle += 360.0f;
	return angle;
}

// Calculate shortest angle difference
inline float angle_diff(float target, float current) {
	float diff = normalize_angle(target - current);
	return diff;
}

void features::visuals::motion_blur(view_setup_t* setup) {
	// Early exit checks
	if (!c::visuals::mbenabled)
		return;

	if (!interfaces::engine->is_in_game() || !interfaces::engine->is_connected())
		return;

	if (!g::local || !g::local->is_alive()) {
		g_mb_history.initialized = false;
		return;
	}

	static float motion_blur_values[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	// Setup phase - calculate motion blur values
	if (setup) {
		const float current_time = interfaces::globals->realtime;
		const float time_delta = current_time - g_mb_history.last_update_time;

		// Initialize on first run or after respawn
		if (!g_mb_history.initialized || time_delta > 1.0f) {
			g_mb_history.last_position = setup->origin;
			g_mb_history.last_pitch = normalize_angle(setup->view.x);
			g_mb_history.last_yaw = normalize_angle(setup->view.y);
			g_mb_history.last_update_time = current_time;
			g_mb_history.initialized = true;

			// Reset blur values
			memset(motion_blur_values, 0, sizeof(motion_blur_values));
			return;
		}

		// Get current camera state
		const vec3_t current_position = setup->origin;
		const float current_pitch = normalize_angle(setup->view.x);
		const float current_yaw = normalize_angle(setup->view.y);

		// Calculate position change
		vec3_t position_delta = g_mb_history.last_position - current_position;
		const float position_change = position_delta.length();

		// Detect teleportation or major disruptions
		const float teleport_threshold = 30.0f;
		const float max_frame_time = 1.0f / 10.0f; // 10 FPS minimum

		if (position_change > teleport_threshold || time_delta > max_frame_time) {
			// Reset on teleport/disruption
			g_mb_history.last_position = current_position;
			g_mb_history.last_pitch = current_pitch;
			g_mb_history.last_yaw = current_yaw;
			g_mb_history.last_update_time = current_time;
			memset(motion_blur_values, 0, sizeof(motion_blur_values));
			return;
		}

		// Detect very fast movement (noclip, respawn, etc)
		const float fast_movement_threshold = 50.0f;
		if (position_change > fast_movement_threshold) {
			g_mb_history.disable_rotational_until = current_time + 1.0f;
		}

		// Calculate view vectors
		vec3_t forward_vec, right_vec, up_vec;
		math::angle_vectors(setup->view, &forward_vec, &right_vec, &up_vec);

		// Calculate FOV values
		const float horizontal_fov = setup->fov;
		const float vertical_fov = (setup->aspect_ratio > 0.0f)
			? (setup->fov / setup->aspect_ratio)
			: setup->fov;

		// Calculate angle deltas using shortest path
		const float pitch_delta = angle_diff(g_mb_history.last_pitch, current_pitch);
		const float yaw_delta = angle_diff(g_mb_history.last_yaw, current_yaw);

		// Calculate motion components
		const float forward_motion = forward_vec.dot_product(position_delta);
		const float side_motion = right_vec.dot_product(position_delta);

		// YAW BLUR (horizontal rotation)
		float yaw_blur = yaw_delta;

		// Adjust yaw based on sideways movement
		yaw_blur += (side_motion / 3.0f);

		// Clamp adjustment to prevent over-compensation
		if (yaw_delta < 0.0f)
			yaw_blur = std::clamp(yaw_blur, yaw_delta, 0.0f);
		else
			yaw_blur = std::clamp(yaw_blur, 0.0f, yaw_delta);

		// Normalize by FOV and reduce based on pitch (looking up/down reduces horizontal blur)
		const float pitch_factor = 1.0f - (fabsf(current_pitch) / 90.0f);
		motion_blur_values[0] = (yaw_blur / horizontal_fov) * pitch_factor;

		// PITCH BLUR (vertical rotation)
		float pitch_blur = pitch_delta;

		// Compensate for forward/backward motion when looking up/down
		const float pitch_compensation = 1.0f - powf(1.0f - fabsf(forward_vec.z), 2.0f);
		const float motion_adjustment = (forward_motion / 2.0f) * pitch_compensation;

		if (current_pitch > 0.0f)
			pitch_blur -= motion_adjustment;
		else
			pitch_blur += motion_adjustment;

		// Clamp adjustment
		if (pitch_delta < 0.0f)
			pitch_blur = std::clamp(pitch_blur, pitch_delta, 0.0f);
		else
			pitch_blur = std::clamp(pitch_blur, 0.0f, pitch_delta);

		motion_blur_values[1] = pitch_blur / vertical_fov;

		// FORWARD/FALLING BLUR
		if (c::visuals::mbforwardEnabled && time_delta > 0.0f) {
			float forward_blur = forward_motion / (time_delta * 30.0f);

			// Apply falling min/max range
			const float falling_range = c::visuals::mbfallingMax - c::visuals::mbfallingMin;
			if (falling_range > 0.0f) {
				const float normalized = (fabsf(forward_blur) - c::visuals::mbfallingMin) / falling_range;
				forward_blur = std::clamp(normalized, 0.0f, 1.0f) * (forward_blur >= 0.0f ? 1.0f : -1.0f);
			}

			motion_blur_values[2] = forward_blur / 60.0f;
		}
		else {
			motion_blur_values[2] = 0.0f;
		}

		// ROLL BLUR (yaw-based rolling effect at extreme pitches)
		motion_blur_values[3] = (yaw_delta / horizontal_fov) * powf(fabsf(current_pitch) / 90.0f, 3.0f);

		// Apply intensity multipliers
		const float rotation_intensity = c::visuals::mbrotationIntensity * 0.15f * c::visuals::mbstrength;
		const float falling_intensity = c::visuals::mbfallingIntensity * 0.15f * c::visuals::mbstrength;

		motion_blur_values[0] *= rotation_intensity;
		motion_blur_values[1] *= rotation_intensity;
		motion_blur_values[2] *= rotation_intensity;
		motion_blur_values[3] *= falling_intensity;

		// Disable rotational blur during fast movement
		if (current_time < g_mb_history.disable_rotational_until) {
			motion_blur_values[0] = 0.0f;
			motion_blur_values[1] = 0.0f;
			motion_blur_values[3] = 0.0f;
		}

		// Update history
		g_mb_history.last_position = current_position;
		g_mb_history.last_pitch = current_pitch;
		g_mb_history.last_yaw = current_yaw;
		g_mb_history.last_update_time = current_time;

		return;
	}

	// Render phase - apply motion blur material
	i_material* material = interfaces::material_system->find_material("dev/motion_blur", "RenderTargets", false);

	if (!material || material->is_error_material())
		return;

	// Set motion blur values
	const auto motion_blur_internal = material->find_var("$MotionBlurInternal", nullptr, false);
	if (motion_blur_internal) {
		motion_blur_internal->set_vec_component_value(motion_blur_values[0], 0);
		motion_blur_internal->set_vec_component_value(motion_blur_values[1], 1);
		motion_blur_internal->set_vec_component_value(motion_blur_values[2], 2);
		motion_blur_internal->set_vec_component_value(motion_blur_values[3], 3);
	}

	// Set viewport
	const auto motion_blur_viewport = material->find_var("$MotionBlurViewportInternal", nullptr, false);
	if (motion_blur_viewport) {
		motion_blur_viewport->set_vec_component_value(0.0f, 0);
		motion_blur_viewport->set_vec_component_value(0.0f, 1);
		motion_blur_viewport->set_vec_component_value(1.0f, 2);
		motion_blur_viewport->set_vec_component_value(1.0f, 3);
	}

	// Handle depth buffer for specific adapters
	if (c::visuals::mb_video_adapter == 0 || c::visuals::mb_video_adapter == 3) {
		static auto mat_resolve = interfaces::console->get_convar("mat_resolveFullFrameDepth");
		if (mat_resolve)
			mat_resolve->set_value(0);
	}

	// Apply the effect
	draw_screen_effect(material);
}

struct camera_stabilization_t {
	vec3_t last_position;
	float last_pitch;
	float last_yaw;
	float last_update_time;
	bool initialized;

	camera_stabilization_t() :
		last_position(0, 0, 0),
		last_pitch(0.0f),
		last_yaw(0.0f),
		last_update_time(0.0f),
		initialized(false) {}
};
static camera_stabilization_t g_camera_stab;

void features::visuals::camera_fix(view_setup_t* setup) {
	if (!c::visuals::camera_fix_enabled)
		return;

	if (!interfaces::engine->is_in_game() || !interfaces::engine->is_connected())
		return;

	if (!g::local || !g::local->is_alive()) {
		g_camera_stab.initialized = false;
		return;
	}

	if (!setup)
		return;

	const float current_time = interfaces::globals->realtime;
	const float time_delta = current_time - g_camera_stab.last_update_time;

	// Initialize on first run or after long delay
	if (!g_camera_stab.initialized || time_delta > 1.0f) {
		g_camera_stab.last_position = setup->origin;
		g_camera_stab.last_pitch = normalize_angle(setup->view.x);
		g_camera_stab.last_yaw = normalize_angle(setup->view.y);
		g_camera_stab.last_update_time = current_time;
		g_camera_stab.initialized = true;
		return;
	}

	// Get current camera state
	vec3_t current_position = setup->origin;
	float current_pitch = normalize_angle(setup->view.x);
	float current_yaw = normalize_angle(setup->view.y);

	// Calculate position change
	vec3_t position_delta = g_camera_stab.last_position - current_position;
	const float position_change = position_delta.length();

	// Detect teleportation (map change, respawn, noclip)
	const float teleport_threshold = 100.0f;
	const float max_frame_time = 0.5f;

	if (position_change > teleport_threshold || time_delta > max_frame_time) {
		// Reset on teleport
		g_camera_stab.last_position = current_position;
		g_camera_stab.last_pitch = current_pitch;
		g_camera_stab.last_yaw = current_yaw;
		g_camera_stab.last_update_time = current_time;
		return;
	}

	// CAMERA STABILIZATION
	// Smooth out jittery camera movement
	if (c::visuals::camera_smoothing > 0.0f && c::visuals::camrea_enable_smoothing) {
		const float smooth_factor = std::clamp<float>(c::visuals::camera_smoothing, 0.0f, 1.f);

		// Smooth position
		setup->origin.x = current_position.x * (1.0f - smooth_factor) + g_camera_stab.last_position.x * smooth_factor;
		setup->origin.y = current_position.y * (1.0f - smooth_factor) + g_camera_stab.last_position.y * smooth_factor;
		setup->origin.z = current_position.z * (1.0f - smooth_factor) + g_camera_stab.last_position.z * smooth_factor;

		// Smooth angles using shortest path
		float pitch_delta = angle_diff(current_pitch, g_camera_stab.last_pitch);
		float yaw_delta = angle_diff(current_yaw, g_camera_stab.last_yaw);

		setup->view.x = normalize_angle(g_camera_stab.last_pitch + pitch_delta * (1.0f - smooth_factor));
		setup->view.y = normalize_angle(g_camera_stab.last_yaw + yaw_delta * (1.0f - smooth_factor));
	}

	// ANGLE WRAPPING FIX
	// Prevent 180-degree snapping
	if (c::visuals::camera_angle_fix) {
		setup->view.x = normalize_angle(setup->view.x);
		setup->view.y = normalize_angle(setup->view.y);
		setup->view.z = normalize_angle(setup->view.z);
	}

	// TELEPORT COMPENSATION
	// Gradually adjust view after sudden position changes
	if (c::visuals::camera_teleport_fix && position_change > 10.0f) {
		const float compensation = std::clamp(position_change / 50.0f, 0.0f, 1.0f);
		// Slightly adjust angles to compensate for position jump
		setup->view.x *= (1.0f - compensation * 0.1f);
	}

	// Update history
	g_camera_stab.last_position = setup->origin;
	g_camera_stab.last_pitch = setup->view.x;
	g_camera_stab.last_yaw = setup->view.y;
	g_camera_stab.last_update_time = current_time;
}

void features::visuals::apply_zoom() {
	if (!interfaces::engine->is_in_game()) {
		return;
	}

	if (c::visuals::apply_zoom && g::local) {
		if ((g::local->fov() == 90 || g::local->fov_start() == 90)) {
			if (menu::checkkey(c::visuals::apply_zoom_key, c::visuals::apply_zoom_key_s)) {
				g::local->fov() = 40;
				g::local->fov_start() = 40;
			}
		}
	}
}

void features::visuals::fog() {
	static bool missing_fog_console_logged = false;
	static bool missing_fog_convars_logged = false;
	if (!interfaces::console) {
		if (!missing_fog_console_logged) {
			const auto& preset = menu::world_preset_profile(menu::normalize_world_preset(c::visuals::world_preset_index));
			debug::log("World visuals fog [%s]: console interface missing; fog update skipped safely.", preset.internal_id);
			missing_fog_console_logged = true;
		}
		return;
	}

	static auto fog_override = interfaces::console->get_convar("fog_override");
	static auto fog_start = interfaces::console->get_convar("fog_start");
	static auto fog_end = interfaces::console->get_convar("fog_end");
	static auto fog_maxdensity = interfaces::console->get_convar("fog_maxdensity");
	static auto fog_color = interfaces::console->get_convar("fog_color");

	if (!fog_override || !fog_start || !fog_end || !fog_maxdensity || !fog_color) {
		if (!missing_fog_convars_logged) {
			const auto& preset = menu::world_preset_profile(menu::normalize_world_preset(c::visuals::world_preset_index));
			debug::log("World visuals fog [%s]: one or more fog convars are missing; fog update skipped safely.", preset.internal_id);
			missing_fog_convars_logged = true;
		}
		return;
	}

	if (!c::visuals::fog) {
		fog_override->set_value(0);
		return;
	}
	fog_override->set_value(1);

	fog_start->set_value(0);

	fog_end->set_value(c::visuals::fog_distance);

	fog_maxdensity->set_value((float)c::visuals::fog_density * 0.01f);

	int red = c::visuals::fog_color[0] * 255;
	int green = c::visuals::fog_color[1] * 255;
	int blue = c::visuals::fog_color[2] * 255;
	char buffer_color[32];
	sprintf_s(buffer_color, sizeof(buffer_color), "%i %i %i", red, green, blue);

	fog_color->set_value(buffer_color);
}

void features::visuals::gravity_ragdoll() {
	if (!interfaces::engine->is_connected() || !interfaces::engine->is_in_game())
		return;

	static auto ragdollGravity = interfaces::console->get_convar("cl_ragdoll_gravity");
	if (!ragdollGravity)
		return;
	ragdollGravity->set_value(c::visuals::gravity_ragdoll ? -600 : 600);
}

bool update = false;
void features::visuals::skybox_changer() {
	if (!g::local) {
		update = true;
		return;
	}

	static bool missing_loader_logged = false;
	static bool missing_sv_skyname_logged = false;
	static bool invalid_skybox_index_logged = false;
	static int saved_skybox = -1;
	static int last_logged_skybox = -1;
	static auto load_skybox = reinterpret_cast<void(__fastcall*)(const char*)>(find_pattern("engine.dll", "55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45"));
	static std::string skybox_name;

	if (!load_skybox) {
		if (!missing_loader_logged) {
			const auto& preset = menu::world_preset_profile(menu::normalize_world_preset(c::visuals::world_preset_index));
			debug::log("World visuals skybox [%s]: engine LoadSkybox pointer missing; skybox change skipped safely.", preset.internal_id);
			missing_loader_logged = true;
		}
		return;
	}

	const auto sv_skyname = interfaces::console ? interfaces::console->get_convar("sv_skyname") : nullptr;
	skybox_name.clear();
	switch (c::visuals::skybox) {
	case 1: skybox_name = "cs_tibet"; break;
	case 2: skybox_name = "cs_baggage_skybox_"; break;
	case 3: skybox_name = "italy"; break;
	case 4: skybox_name = "jungle"; break;
	case 5: skybox_name = "office"; break;
	case 6: skybox_name = "sky_cs15_daylight01_hdr"; break;
	case 7: skybox_name = "sky_cs15_daylight02_hdr"; break;
	case 8: skybox_name = "vertigoblue_hdr"; break;
	case 9: skybox_name = "vertigo"; break;
	case 10: skybox_name = "sky_day02_05_hdr"; break;
	case 11: skybox_name = "nukeblank"; break;
	case 12: skybox_name = "sky_venice"; break;
	case 13: skybox_name = "sky_cs15_daylight03_hdr"; break;
	case 14: skybox_name = "sky_cs15_daylight04_hdr"; break;
	case 15: skybox_name = "sky_csgo_cloudy01"; break;
	case 16: skybox_name = "sky_csgo_night02"; break;
	case 17: skybox_name = "sky_csgo_night02b"; break;
	case 18: skybox_name = "sky_csgo_night_flat"; break;
	case 19: skybox_name = "sky_dust"; break;
	case 20: skybox_name = "vietnam"; break;
	case 21: skybox_name = "sky_lunacy"; break;
	}

	if (update || saved_skybox != c::visuals::skybox) {
		const auto& preset = menu::world_preset_profile(menu::normalize_world_preset(c::visuals::world_preset_index));
		if (c::visuals::skybox == 0) {
			if (!sv_skyname || !sv_skyname->string || !sv_skyname->string[0]) {
				if (!missing_sv_skyname_logged) {
					debug::log("World visuals skybox [%s]: sv_skyname is unavailable; default skybox restore skipped safely.", preset.internal_id);
					missing_sv_skyname_logged = true;
				}
				return;
			}
			load_skybox(sv_skyname->string);
			if (last_logged_skybox != 0) {
				debug::log("World visuals skybox [%s]: restored default skybox '%s'.", preset.internal_id, sv_skyname->string);
				last_logged_skybox = 0;
			}
		}
		else {
			if (skybox_name.empty()) {
				if (!invalid_skybox_index_logged) {
					debug::log("World visuals skybox [%s]: invalid skybox index %d; skybox change skipped safely.", preset.internal_id, c::visuals::skybox);
					invalid_skybox_index_logged = true;
				}
				return;
			}
			load_skybox(skybox_name.c_str());
			if (last_logged_skybox != c::visuals::skybox) {
				debug::log("World visuals skybox [%s]: applied skybox '%s' (index=%d).", preset.internal_id, skybox_name.c_str(), c::visuals::skybox);
				last_logged_skybox = c::visuals::skybox;
			}
		}

		saved_skybox = c::visuals::skybox;
		update = false;
	}
}

void features::visuals::notification_system() {
	if (!g::local || !c::visuals::notifcation_system)
		return;

	static float fadeAlpha = 0.0f;  // Transparency value for fade effect
	static float last_update_time = 0;
	static const float fade_in_speed = 0.007f;
	static const float fade_out_speed = 0.007f;
	static const float display_time = 2.75f; // Time the notification is visible

	static std::string current_text = "";
	static std::string notified_text = "";
	static bool is_visible = false;

	if (!notified_text.empty() && notified_text != current_text) {
		// New notification
		current_text = notified_text;
		fadeAlpha = 0.0f;
		last_update_time = interfaces::globals->realtime;
		is_visible = false;
	}

	float current_time = interfaces::globals->realtime;

	// Manage fade in and fade out
	if (is_visible) {
		if (current_time - last_update_time < display_time) {
			// Fade in
			fadeAlpha = min(fadeAlpha + fade_in_speed, 1.0f);
		}
		else {
			// Fade out
			fadeAlpha = max(fadeAlpha - fade_out_speed, 0.0f);
			if (fadeAlpha <= 0.0f) {
				is_visible = false;
				current_text = "";
			}
		}
	}

	if (c::visuals::movement_noti) {
		if (features::movement::should_edge_bug) {
			is_visible = true;
			notified_text = "Linear has assisted a Edgebug";
		}
		else if (features::movement::should_ps) {
			is_visible = true;
			notified_text = "Linear has assisted a Pixelsurf";
		}
		else if (features::movement::detected_normal_jump_bug) {
			is_visible = true;
			notified_text = "Linear has assisted a Jumpbug";
		}
		else if (features::movement::should_mj) {
			is_visible = true;
			notified_text = "Linear has assisted a Minijump";
		}
		else if (features::movement::should_lj) {
			is_visible = true;
			notified_text = "Linear has assisted a Longjump";
		}
	}

	if (!is_visible) return;

	// Screen size
	int w, h;
	interfaces::engine->get_screen_size(w, h);

	// Text and box size
	auto text_size = im_render.measure_text(current_text.c_str(), fonts::watermark_font, 15.f);
	static const ImVec2 padding = ImVec2(7, 7);
	static const ImVec2 margin = ImVec2(4, 3);
	static const int box_offset = 10;

	// Centered lower position
	float center_x = (w - text_size.x - padding.x * 2.f) / 2.0f;
	float center_y = h - 175.0f;

	// Colors with alpha fading
	ImColor bg_color(0.11f, 0.11f, 0.11f, fadeAlpha);
	ImColor outline_color(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], fadeAlpha);
	ImColor text_color(255, 255, 255, static_cast<int>(fadeAlpha * 220));

	// Draw notification box
	ImGui::GetBackgroundDrawList()->AddRectFilled(
		{ center_x - 2.f, center_y - 2.f },
		{ center_x + text_size.x + padding.x * 2.f + 2.f, center_y + text_size.y + padding.y * 2.f + 2.f },
		bg_color,
		6.f
	);
	ImGui::GetBackgroundDrawList()->AddRect(
		{ center_x - 3.f, center_y - 3.f },
		{ center_x + text_size.x + padding.x * 2.f + 3.f, center_y + text_size.y + padding.y * 2.f + 3.f },
		outline_color,
		6.f
	);

	// Draw text
	ImGui::PushFont(fonts::watermark_font);
	ImGui::GetBackgroundDrawList()->AddText(
		fonts::watermark_font,
		15.f,
		ImVec2(center_x + padding.x, center_y + padding.y),
		text_color,
		current_text.c_str()
	);
	ImGui::PopFont();
}

namespace {

static void points_try_load_mascot_once(IDirect3DDevice9* dev) {
	if (g_points_mascot_tex || g_points_mascot_load_gave_up || !dev)
		return;

	points_refresh_meme_file_list(false);
	if (!g_pts_meme_paths.empty()) {
		const int idx = points_meme_pick_index(-1, g_pts_meme_paths.size());
		if (idx >= 0
			&& points_load_texture_from_file_w(dev, g_pts_meme_paths[size_t(idx)], &g_points_mascot_tex,
				k_pts_mascot_file_max_edge)) {
			g_mascot_meme_last_idx = idx;
			if (interfaces::globals)
				g_mascot_reverse_anim_start = interfaces::globals->realtime;
			return;
		}
	}

	points_load_mascot_embed_jpeg(dev);
}

static void points_try_load_stars_once(IDirect3DDevice9* dev) {
	if (g_points_stars_tex || g_points_stars_load_gave_up || !dev)
		return;

	const points_stars_disk_candidate stars_disk = points_find_wanted_stars_file();
	if (!stars_disk.path.empty() && points_load_texture_from_file_w(dev, stars_disk.path, &g_points_stars_tex)) {
		g_points_stars_loaded_from_disk = true;
		if (stars_disk.apply_whiten)
			points_texture_whiten_dark_on_white(g_points_stars_tex);
		return;
	}

	struct stars_try_t {
		UINT w, h;
		D3DFORMAT fmt;
		D3DPOOL pool;
	};
	const stars_try_t tries[] = {
		{ 256, 256, D3DFMT_UNKNOWN, D3DPOOL_MANAGED },
		{ 256, 256, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED },
		{ D3DX_DEFAULT, D3DX_DEFAULT, D3DFMT_UNKNOWN, D3DPOOL_MANAGED },
		{ 256, 256, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT },
	};

	HRESULT hr = E_FAIL;
	for (const stars_try_t& tr : tries) {
		hr = D3DXCreateTextureFromFileInMemoryEx(
			dev,
			g_wanted_stars_png,
			g_wanted_stars_png_size,
			tr.w,
			tr.h,
			1,
			0,
			tr.fmt,
			tr.pool,
			D3DX_FILTER_LINEAR,
			D3DX_FILTER_NONE,
			0,
			nullptr,
			nullptr,
			&g_points_stars_tex);
		if (SUCCEEDED(hr))
			break;
		if (g_points_stars_tex) {
			g_points_stars_tex->Release();
			g_points_stars_tex = nullptr;
		}
	}

	if (FAILED(hr)) {
		g_points_stars_load_gave_up = true;
		return;
	}
	g_points_stars_loaded_from_disk = false;
	points_texture_whiten_dark_on_white(g_points_stars_tex);
}

// Sprite: 5 cols x 6 rows. Row 0 = filled, row 5 = outline. Sixth HUD slot reuses first column UV.
static void draw_points_wanted_star_fast(ImDrawList* dl, ImVec2 c, float r, float fill_amt, float fade_a) {
	float f = fill_amt;
	if (f < 0.f)
		f = 0.f;
	if (f > 1.f)
		f = 1.f;

	const int ring_a = int(160.f * fade_a);
	const ImU32 ring = IM_COL32(190, 190, 198, ring_a);

	if (f <= 0.001f) {
		dl->AddCircle(c, r, ring, 8, 1.15f);
		return;
	}

	const int core_a = int(255.f * fade_a * f);
	const ImU32 core = IM_COL32(255, 255, 255, core_a);
	const float inner_r = r * (0.22f + 0.78f * f);
	dl->AddCircleFilled(c, inner_r, core, 8);
	dl->AddCircle(c, r, ring, 8, 1.15f);
}

static void draw_wanted_stars_sheet(ImDrawList* dl, IDirect3DTexture9* tex, float x0, float y_mid, float star_size,
	int wanted_full, float wanted_partial, float fade_a) {
	const float cell_u = 1.f / 5.f;
	const float cell_v = 1.f / 6.f;
	const float u0 = 0.f;
	const float u1 = cell_u;
	const float v_fill0 = 0.f;
	const float v_fill1 = cell_v;
	const float v_out0 = 5.f * cell_v;
	const float v_out1 = 6.f * cell_v;

	const float spacing = star_size * 0.12f;
	const ImU32 tint = IM_COL32(255, 255, 255, int(255.f * fade_a));

	for (int i = 0; i < 6; ++i) {
		const float cx = x0 + float(i) * (star_size + spacing);
		const ImVec2 pmin(cx, y_mid - star_size * 0.5f);
		const ImVec2 pmax(cx + star_size, y_mid + star_size * 0.5f);

		dl->AddImage((ImTextureID)tex, pmin, pmax, ImVec2(u0, v_out0), ImVec2(u1, v_out1), tint);

		const float fill = (i < wanted_full) ? 1.f : (i == wanted_full ? wanted_partial : 0.f);
		if (fill <= 0.001f)
			continue;

		if (fill >= 0.999f) {
			dl->AddImage((ImTextureID)tex, pmin, pmax, ImVec2(u0, v_fill0), ImVec2(u1, v_fill1), tint);
		}
		else {
			const float clip_r = pmin.x + (pmax.x - pmin.x) * fill;
			dl->PushClipRect(pmin, ImVec2(clip_r, pmax.y), true);
			dl->AddImage((ImTextureID)tex, pmin, pmax, ImVec2(u0, v_fill0), ImVec2(u1, v_fill1), tint);
			dl->PopClipRect();
		}
	}
}

static float points_frac(float value) {
	return value - floorf(value);
}

struct themed_panel_palette_t {
	ImVec4 bg_a;
	ImVec4 bg_b;
	ImVec4 border;
	float glow_strength;
	float rounding;
};

struct watermark_style_t {
	ImVec4 background;
	ImVec4 accent;
	ImVec4 text;
	float glow_strength;
	bool outline;
	bool shadow;
	float thickness;
};

enum class movement_hud_event_t : int {
	none = 0,
	pixelsurf,
	edgebug,
	jumpbug,
	minijump,
	longjump,
	fireman,
	air
};

struct movement_hud_state_t {
	static constexpr int k_max_combo_events = 5;
	static constexpr int k_velocity_samples = 48;

	std::array<movement_hud_event_t, k_max_combo_events> combo_events{};
	std::array<float, k_velocity_samples> speed_samples{};
	int combo_size = 0;
	int speed_head = 0;
	int speed_count = 0;
	float heat = 0.0f;
	float flash_alpha = 0.0f;
	float last_event_time = -1000.0f;
	float last_takeoff_time = -1000.0f;
	float last_landing_time = -1000.0f;
	float timing_trend = 0.0f;
	float last_sound_time = -1000.0f;
	movement_hud_event_t last_event = movement_hud_event_t::none;
	bool last_grounded = true;
};

static ImVec4 make_color(const float r, const float g, const float b, const float a) {
	return ImVec4(r, g, b, a);
}

static ImVec4 color_from_config(const float (&value)[4]) {
	return ImVec4(value[0], value[1], value[2], value[3]);
}

static ImVec4 color_lerp(const ImVec4& a, const ImVec4& b, const float t) {
	const float clamped = std::clamp(t, 0.0f, 1.0f);
	return ImVec4(
		std::lerp(a.x, b.x, clamped),
		std::lerp(a.y, b.y, clamped),
		std::lerp(a.z, b.z, clamped),
		std::lerp(a.w, b.w, clamped));
}

static ImVec4 scale_alpha(const ImVec4& color, const float alpha_scale) {
	return ImVec4(color.x, color.y, color.z, color.w * alpha_scale);
}

static float color_luminance(const ImVec4& color) {
	return (color.x * 0.2126f) + (color.y * 0.7152f) + (color.z * 0.0722f);
}

static themed_panel_palette_t resolve_theme_panel_palette(const int style, const float alpha_scale, const float default_rounding) {
	themed_panel_palette_t palette{
		ImVec4(0.08f, 0.09f, 0.12f, 0.80f * alpha_scale),
		ImVec4(0.03f, 0.04f, 0.06f, 0.96f * alpha_scale),
		ImVec4(0.14f, 0.16f, 0.20f, 0.85f * alpha_scale),
		0.18f,
		(std::max)(default_rounding, 7.0f)
	};

	switch (style) {
	case 2: // matte
		palette.bg_a = ImVec4(0.05f, 0.05f, 0.06f, 0.92f * alpha_scale);
		palette.bg_b = ImVec4(0.02f, 0.02f, 0.03f, 0.98f * alpha_scale);
		palette.border = ImVec4(0.18f, 0.18f, 0.20f, 0.95f * alpha_scale);
		palette.glow_strength = 0.10f;
		palette.rounding = (std::min)(default_rounding, 5.0f);
		break;
	case 3: // metallic
		palette.bg_a = ImVec4(0.12f, 0.13f, 0.15f, 0.88f * alpha_scale);
		palette.bg_b = ImVec4(0.05f, 0.06f, 0.08f, 0.98f * alpha_scale);
		palette.border = ImVec4(0.28f, 0.30f, 0.34f, 0.95f * alpha_scale);
		palette.glow_strength = 0.14f;
		palette.rounding = (std::max)(default_rounding, 6.0f);
		break;
	case 4: // terminal
		palette.bg_a = ImVec4(0.01f, 0.06f, 0.03f, 0.92f * alpha_scale);
		palette.bg_b = ImVec4(0.00f, 0.02f, 0.01f, 0.98f * alpha_scale);
		palette.border = ImVec4(0.08f, 0.28f, 0.16f, 0.95f * alpha_scale);
		palette.glow_strength = 0.12f;
		palette.rounding = 3.0f;
		break;
	case 5: // halo
		palette.bg_a = ImVec4(0.86f, 0.90f, 0.98f, 0.18f * alpha_scale);
		palette.bg_b = ImVec4(0.18f, 0.20f, 0.26f, 0.84f * alpha_scale);
		palette.border = ImVec4(0.88f, 0.92f, 0.98f, 0.52f * alpha_scale);
		palette.glow_strength = 0.22f;
		palette.rounding = (std::max)(default_rounding, 9.0f);
		break;
	case 1: // glass
	default:
		break;
	}

	return palette;
}

static int resolve_ui_panel_style() {
	const int requested = std::clamp(c::visuals::ui_panel_style, 0, 5);
	if (requested != 0)
		return requested;

	switch (menu::active_menu_variant()) {
	case menu::menu_variant_t::cyber_terminal:
		return 4; // terminal
	case menu::menu_variant_t::angel_whiteout:
	case menu::menu_variant_t::minimal_ghost:
	case menu::menu_variant_t::whitearmor:
	case menu::menu_variant_t::ecco2k:
		return 5; // halo
	case menu::menu_variant_t::luxury_gold:
	case menu::menu_variant_t::thaiboy:
	case menu::menu_variant_t::crest:
	case menu::menu_variant_t::industrial_steel:
	case menu::menu_variant_t::warlord:
		return 3; // metallic
	case menu::menu_variant_t::redlight:
	case menu::menu_variant_t::fight_club:
	case menu::menu_variant_t::evil_occult:
	case menu::menu_variant_t::horror_cursed:
	case menu::menu_variant_t::spiderr:
		return 2; // matte
	default:
		return 1; // glass
	}
}

static float resolve_ui_accent_pulse(const float time) {
	if (!c::visuals::ui_accent_pulse)
		return 1.0f;
	return 0.72f + (((sinf(time * 2.35f) * 0.5f) + 0.5f) * 0.28f);
}

static float resolve_ui_line_weight() {
	return c::visuals::ui_line_weight == 1 ? 2.0f : 1.0f;
}

static void draw_panel_grain(ImDrawList* dl, const ImVec2 min, const ImVec2 max, const ImVec4& accent, const float alpha_scale, const float time) {
	if (!c::visuals::ui_overlay_grain)
		return;

	const float strength = std::clamp(c::visuals::ui_overlay_grain_strength, 0.05f, 0.45f);
	const int samples = 12 + static_cast<int>(strength * 26.0f);
	const float width = max.x - min.x;
	const float height = max.y - min.y;

	for (int i = 0; i < samples; ++i) {
		const float seed = static_cast<float>(i + 1);
		const float fx = points_frac(sinf(seed * 12.9898f + time * 0.15f) * 43758.5453f);
		const float fy = points_frac(sinf(seed * 78.233f + time * 0.19f) * 12345.6789f);
		const float len = 2.0f + points_frac(sinf(seed * 91.77f + time * 0.11f) * 34567.1234f) * 6.0f;
		const float alpha = alpha_scale * strength * (0.05f + points_frac(sinf(seed * 42.42f + time * 0.13f) * 9876.543f) * 0.14f);
		const ImVec2 p0(min.x + fx * width, min.y + fy * height);
		const ImVec2 p1(p0.x + len, p0.y + (i % 2 == 0 ? 0.0f : 1.0f));
		dl->AddLine(p0, p1, ImColor(accent.x, accent.y, accent.z, alpha), 1.0f);
	}
}

static void draw_theme_panel(ImDrawList* dl, const ImVec2 min, const ImVec2 max, const ImVec4& accent, const float alpha_scale, float rounding) {
	const int style = resolve_ui_panel_style();
	const float pulse = resolve_ui_accent_pulse(interfaces::globals ? interfaces::globals->realtime : 0.0f);
	const float line_weight = resolve_ui_line_weight();
	const auto palette = resolve_theme_panel_palette(style, alpha_scale, rounding);
	const ImVec4 bgA = palette.bg_a;
	const ImVec4 bgB = palette.bg_b;
	const ImVec4 border = palette.border;
	const float glow_strength = palette.glow_strength;
	rounding = palette.rounding;

	if (alpha_scale > 0.05f) {
		const ImColor glow(accent.x, accent.y, accent.z, glow_strength * pulse * alpha_scale);
		const int layers = 4;
		for (int i = 0; i < layers; ++i) {
			const float t = static_cast<float>(i) / static_cast<float>(layers - 1);
			const float expand = 2.0f + t * 7.0f;
			const float a = glow.Value.w * (1.0f - t);
			dl->AddRectFilled(ImVec2(min.x - expand, min.y - expand), ImVec2(max.x + expand, max.y + expand),
				ImColor(glow.Value.x, glow.Value.y, glow.Value.z, a), rounding + expand, ImDrawFlags_RoundCornersAll);
		}
	}

	dl->AddRectFilledMultiColorRounded(min, max,
		ImColor(bgA.x, bgA.y, bgA.z, bgA.w),
		ImColor(bgB.x, bgB.y, bgB.z, bgB.w),
		ImColor(bgB.x, bgB.y, bgB.z, bgB.w),
		ImColor(bgA.x, bgA.y, bgA.z, bgA.w),
		ImColor(bgA.x, bgA.y, bgA.z, bgA.w),
		rounding, ImDrawFlags_RoundCornersAll);

	dl->AddRect(min, max, ImColor(border.x, border.y, border.z, border.w), rounding, ImDrawFlags_RoundCornersAll, line_weight);

	const float grad_height = c::visuals::ui_line_weight == 1 ? 4.0f : 3.0f;
	const float grad_width = (max.x - min.x) * 0.76f;
	const float center_x = (min.x + max.x) * 0.5f;
	const int slices = 56;

	for (int i = 0; i < slices; ++i) {
		const float slice_x0 = center_x - grad_width * 0.5f + (grad_width / slices) * i;
		const float slice_x1 = slice_x0 + (grad_width / slices);
		const float dist = fabsf((slice_x0 + slice_x1) * 0.5f - center_x) / (grad_width * 0.5f);
		const float alpha = (1.0f - dist) * 0.72f * pulse * alpha_scale;
		dl->AddRectFilled(ImVec2(slice_x0, min.y + 1.0f), ImVec2(slice_x1, min.y + 1.0f + grad_height),
			ImColor(accent.x, accent.y, accent.z, alpha), rounding, 0);
	}

	draw_panel_grain(dl, min, max, accent, alpha_scale, interfaces::globals ? interfaces::globals->realtime : 0.0f);
}

static void draw_pts_style_ornaments(ImDrawList* dl, const ImVec2 min, const ImVec2 max, const int pts_style, const ImVec4& accent, const float alpha_scale, const float time) {
	if (!c::visuals::points_hud_glow || alpha_scale <= 0.05f)
		return;

	switch (pts_style) {
	case 1: // frost
	case 5: // crystal
		for (int i = 0; i < 6; ++i) {
			const float t = static_cast<float>(i) / 5.0f;
			const ImVec2 a(min.x + 12.0f + t * 18.0f, min.y + 6.0f + sinf(time * 2.4f + i) * 2.0f);
			const ImVec2 b(a.x + 5.0f, a.y + 8.0f);
			dl->AddLine(a, b, ImColor(accent.x, accent.y, accent.z, 0.32f * alpha_scale), 1.0f);
			dl->AddLine(ImVec2(b.x - 2.0f, b.y - 3.0f), ImVec2(b.x + 3.0f, b.y + 2.0f), ImColor(1.0f, 1.0f, 1.0f, 0.20f * alpha_scale), 1.0f);
		}
		break;
	case 2: // dream
		for (int i = 0; i < 5; ++i) {
			const float x = min.x + 20.0f + i * 22.0f;
			const float y = max.y - 8.0f - fabsf(sinf(time * 1.8f + i) * 5.0f);
			dl->AddCircleFilled(ImVec2(x, y), 2.0f + (i % 2), ImColor(accent.x, accent.y, accent.z, 0.18f * alpha_scale));
		}
		break;
	case 3: // luxury
		dl->AddLine(ImVec2(min.x + 10.0f, max.y - 6.0f), ImVec2(max.x - 10.0f, max.y - 6.0f), ImColor(accent.x, accent.y, accent.z, 0.34f * alpha_scale), 1.5f);
		dl->AddLine(ImVec2(min.x + 18.0f, min.y + 10.0f), ImVec2(max.x - 18.0f, min.y + 10.0f), ImColor(1.0f, 0.94f, 0.74f, 0.20f * alpha_scale), 1.0f);
		break;
	case 4: // chrome
		dl->AddLine(ImVec2(max.x - 12.0f, min.y + 8.0f), ImVec2(max.x - 30.0f, max.y - 8.0f), ImColor(1.0f, 1.0f, 1.0f, 0.16f * alpha_scale), 1.0f);
		dl->AddLine(ImVec2(min.x + 12.0f, min.y + 8.0f), ImVec2(min.x + 26.0f, max.y - 10.0f), ImColor(accent.x, accent.y, accent.z, 0.14f * alpha_scale), 1.0f);
		break;
	case 6: // cursed
		dl->AddCircle(ImVec2(min.x + 18.0f, max.y - 14.0f), 6.0f, ImColor(accent.x, accent.y, accent.z, 0.24f * alpha_scale), 16, 1.0f);
		dl->AddLine(ImVec2(min.x + 18.0f, max.y - 22.0f), ImVec2(min.x + 18.0f, max.y - 6.0f), ImColor(accent.x, accent.y, accent.z, 0.20f * alpha_scale), 1.0f);
		dl->AddLine(ImVec2(min.x + 10.0f, max.y - 14.0f), ImVec2(min.x + 26.0f, max.y - 14.0f), ImColor(accent.x, accent.y, accent.z, 0.20f * alpha_scale), 1.0f);
		break;
	default:
		break;
	}
}

static ImVec4 resolve_watermark_accent() {
	if (c::visuals::watermark_use_custom_accent_color)
		return color_from_config(c::visuals::watermark_accent_color);
	return ImVec4(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], menu::menu_accent[3]);
}

static watermark_style_t resolve_watermark_style() {
	const int panel_style = resolve_ui_panel_style();
	const auto palette = resolve_theme_panel_palette(panel_style, 1.0f, 5.5f);

	watermark_style_t style{};
	style.background = color_lerp(palette.bg_a, palette.bg_b, 0.55f);
	style.accent = resolve_watermark_accent();
	style.glow_strength = std::clamp(c::visuals::watermark_text_glow_strength, 0.0f, 1.75f);
	style.outline = c::visuals::watermark_text_outline;
	style.shadow = c::visuals::watermark_text_shadow;
	style.thickness = std::clamp(c::visuals::watermark_text_thickness, 0.5f, 3.0f);

	if (c::visuals::watermark_use_custom_text_color) {
		style.text = color_from_config(c::visuals::watermark_text_color);
	}
	else {
		const float luminance = color_luminance(style.background);
		if (panel_style == 5) {
			style.text = make_color(0.97f, 0.98f, 1.0f, 1.0f);
		}
		else if (luminance >= 0.62f) {
			style.text = make_color(0.10f, 0.11f, 0.14f, 1.0f);
		}
		else {
			style.text = make_color(0.94f, 0.96f, 0.99f, 1.0f);
		}
	}

	return style;
}

static ImVec4 resolve_watermark_secondary_text(const watermark_style_t& style) {
	return color_lerp(style.text, style.accent, 0.18f);
}

static void draw_watermark_text_segment(ImDrawList* draw_list, const ImVec2& position, const std::string& text, const ImVec4& text_color, const watermark_style_t& style, const ImVec4* glow_tint = nullptr) {
	if (text.empty())
		return;

	const float offset = std::clamp(style.thickness, 0.5f, 3.0f);
	const float luminance = color_luminance(text_color);
	const ImVec4 shadow_color = luminance > 0.5f
		? make_color(0.02f, 0.03f, 0.05f, text_color.w * 0.78f)
		: make_color(0.94f, 0.96f, 1.0f, text_color.w * 0.22f);
	const ImVec4 outline_color = luminance > 0.5f
		? make_color(0.01f, 0.02f, 0.03f, text_color.w * 0.88f)
		: make_color(0.92f, 0.95f, 1.0f, text_color.w * 0.18f);
	const ImVec4 glow_color = glow_tint ? *glow_tint : color_lerp(style.accent, text_color, 0.35f);

	if (style.shadow) {
		draw_list->AddText(ImVec2(position.x + offset, position.y + offset), ImColor(shadow_color.x, shadow_color.y, shadow_color.z, shadow_color.w), text.c_str());
	}

	if (style.outline) {
		draw_list->AddText(ImVec2(position.x - offset, position.y), ImColor(outline_color.x, outline_color.y, outline_color.z, outline_color.w), text.c_str());
		draw_list->AddText(ImVec2(position.x + offset, position.y), ImColor(outline_color.x, outline_color.y, outline_color.z, outline_color.w), text.c_str());
		draw_list->AddText(ImVec2(position.x, position.y - offset), ImColor(outline_color.x, outline_color.y, outline_color.z, outline_color.w), text.c_str());
		draw_list->AddText(ImVec2(position.x, position.y + offset), ImColor(outline_color.x, outline_color.y, outline_color.z, outline_color.w), text.c_str());
	}

	if (style.glow_strength > 0.01f) {
		for (int layer = 0; layer < 2; ++layer) {
			const float layer_offset = offset + static_cast<float>(layer) + 0.35f;
			const float alpha = (0.22f - layer * 0.08f) * style.glow_strength * text_color.w;
			draw_list->AddText(ImVec2(position.x - layer_offset, position.y), ImColor(glow_color.x, glow_color.y, glow_color.z, alpha), text.c_str());
			draw_list->AddText(ImVec2(position.x + layer_offset, position.y), ImColor(glow_color.x, glow_color.y, glow_color.z, alpha), text.c_str());
			draw_list->AddText(ImVec2(position.x, position.y - layer_offset), ImColor(glow_color.x, glow_color.y, glow_color.z, alpha), text.c_str());
			draw_list->AddText(ImVec2(position.x, position.y + layer_offset), ImColor(glow_color.x, glow_color.y, glow_color.z, alpha), text.c_str());
		}
	}

	draw_list->AddText(position, ImColor(text_color.x, text_color.y, text_color.z, text_color.w), text.c_str());
}

static const char* movement_hud_event_label(const movement_hud_event_t event) {
	switch (event) {
	case movement_hud_event_t::pixelsurf: return "pixelsurf";
	case movement_hud_event_t::edgebug: return "edgebug";
	case movement_hud_event_t::jumpbug: return "jumpbug";
	case movement_hud_event_t::minijump: return "minijump";
	case movement_hud_event_t::longjump: return "longjump";
	case movement_hud_event_t::fireman: return "fireman";
	case movement_hud_event_t::air: return "air";
	default: return "";
	}
}

static int movement_hud_points_gain(const movement_hud_event_t event) {
	switch (event) {
	case movement_hud_event_t::pixelsurf:
	case movement_hud_event_t::edgebug:
	case movement_hud_event_t::fireman:
		return 125;
	case movement_hud_event_t::air:
		return 150;
	case movement_hud_event_t::jumpbug:
		return 50;
	case movement_hud_event_t::minijump:
		return 90;
	case movement_hud_event_t::longjump:
		return 110;
	default:
		return 0;
	}
}

static float movement_hud_heat_gain(const movement_hud_event_t event) {
	switch (event) {
	case movement_hud_event_t::pixelsurf: return 150.0f;
	case movement_hud_event_t::edgebug: return 135.0f;
	case movement_hud_event_t::jumpbug: return 95.0f;
	case movement_hud_event_t::minijump: return 85.0f;
	case movement_hud_event_t::longjump: return 110.0f;
	case movement_hud_event_t::fireman: return 120.0f;
	case movement_hud_event_t::air: return 140.0f;
	default: return 0.0f;
	}
}

static void movement_hud_push_speed_sample(movement_hud_state_t& state, const float speed) {
	state.speed_samples[state.speed_head] = speed;
	state.speed_head = (state.speed_head + 1) % movement_hud_state_t::k_velocity_samples;
	state.speed_count = (std::min)(state.speed_count + 1, movement_hud_state_t::k_velocity_samples);
}

static float movement_hud_recent_speed(const movement_hud_state_t& state, const int recent_index) {
	if (state.speed_count <= 0)
		return 0.0f;

	const int clamped = std::clamp(recent_index, 0, state.speed_count - 1);
	const int index = (state.speed_head - 1 - clamped + movement_hud_state_t::k_velocity_samples) % movement_hud_state_t::k_velocity_samples;
	return state.speed_samples[index];
}

static float movement_hud_trend(const movement_hud_state_t& state) {
	if (state.speed_count < 8)
		return 0.0f;
	return movement_hud_recent_speed(state, 0) - movement_hud_recent_speed(state, 6);
}

static void movement_hud_clear_combo(movement_hud_state_t& state) {
	state.combo_size = 0;
	state.combo_events.fill(movement_hud_event_t::none);
}

static void movement_hud_append_combo(movement_hud_state_t& state, const movement_hud_event_t event) {
	if (state.combo_size < movement_hud_state_t::k_max_combo_events) {
		state.combo_events[state.combo_size++] = event;
		return;
	}

	for (int i = 1; i < movement_hud_state_t::k_max_combo_events; ++i)
		state.combo_events[i - 1] = state.combo_events[i];
	state.combo_events.back() = event;
}

static std::string movement_hud_combo_text(const movement_hud_state_t& state) {
	std::string text;
	for (int i = 0; i < state.combo_size; ++i) {
		const char* label = movement_hud_event_label(state.combo_events[i]);
		if (!label || !label[0])
			continue;
		if (!text.empty())
			text += " -> ";
		text += label;
	}
	return text;
}

static void draw_hud_pill(ImDrawList* draw_list, const ImVec2& min, const ImVec2& max, const ImVec4& accent, const float alpha, const char* text) {
	if (!text || !text[0])
		return;

	draw_list->AddRectFilled(min, max, ImColor(accent.x, accent.y, accent.z, 0.16f * alpha), 8.0f, ImDrawFlags_RoundCornersAll);
	draw_list->AddRect(min, max, ImColor(accent.x, accent.y, accent.z, 0.48f * alpha), 8.0f, ImDrawFlags_RoundCornersAll, 1.0f);
	draw_list->AddText(ImVec2(min.x + 7.0f, min.y + 3.0f), ImColor(1.0f, 1.0f, 1.0f, alpha), text);
}

static void draw_compact_velocity_strip(ImDrawList* draw_list, const ImVec2& min, const ImVec2& max, const movement_hud_state_t& state, const ImVec4& accent, const float alpha) {
	const int sample_count = (std::min)(state.speed_count, 34);
	if (sample_count < 3)
		return;

	float min_speed = FLT_MAX;
	float max_speed = 0.0f;
	for (int i = 0; i < sample_count; ++i) {
		const float speed = movement_hud_recent_speed(state, i);
		min_speed = (std::min)(min_speed, speed);
		max_speed = (std::max)(max_speed, speed);
	}

	if (min_speed == FLT_MAX)
		return;

	const float span = (std::max)(max_speed - min_speed, 24.0f);
	const float width = max.x - min.x;
	const float height = max.y - min.y;
	draw_list->AddRectFilled(min, max, ImColor(0.02f, 0.03f, 0.05f, 0.28f * alpha), 6.0f, ImDrawFlags_RoundCornersAll);
	draw_list->AddRect(min, max, ImColor(accent.x, accent.y, accent.z, 0.22f * alpha), 6.0f, ImDrawFlags_RoundCornersAll, 1.0f);

	for (int step = sample_count - 1; step > 0; --step) {
		const float older = movement_hud_recent_speed(state, step);
		const float newer = movement_hud_recent_speed(state, step - 1);
		const float x0 = min.x + width * (1.0f - (static_cast<float>(step) / static_cast<float>(sample_count - 1)));
		const float x1 = min.x + width * (1.0f - (static_cast<float>(step - 1) / static_cast<float>(sample_count - 1)));
		const float y0 = max.y - (((older - min_speed) / span) * height);
		const float y1 = max.y - (((newer - min_speed) / span) * height);

		ImVec4 line_color = accent;
		if (newer > older + 0.45f)
			line_color = color_lerp(make_color(0.18f, 0.94f, 0.50f, 1.0f), accent, 0.25f);
		else if (newer < older - 0.45f)
			line_color = color_lerp(make_color(1.0f, 0.24f, 0.24f, 1.0f), accent, 0.15f);
		else
			line_color = color_lerp(accent, make_color(0.92f, 0.94f, 0.98f, 1.0f), 0.30f);

		draw_list->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), ImColor(line_color.x, line_color.y, line_color.z, 0.95f * alpha), 2.0f);
	}
}

} // namespace

void features::visuals::points_system() {
	if (!g::local || !c::movement::points_system)
		return;

	// This runs from the D3D9 Present hook. If ImGui/fonts/device aren't ready, pushing fonts / issuing draws can crash.
	if (!ImGui::GetCurrentContext())
		return;

	if (!fonts::points_big_font)
		return;

	static int points = 0;
	static int combo_count = 0;
	static movement_hud_state_t hud_state{};
	static bool last_ps = false;
	static bool last_air = false;
	static bool last_edge_bug = false;
	static bool last_jump_bug = false;
	static bool last_fireman = false;
	static bool last_minijump = false;
	static bool last_longjump = false;
	static float fade_alpha = 0.f;
	static float combo_fade_alpha = 0.f;
	static float last_point_time = 0.f;
	static float last_combo_time = 0.f;

	float current_time = interfaces::globals->realtime;
	const float frame_time = interfaces::globals->frame_time;
	const bool grounded = (g::local->flags() & fl_onground) != 0;
	const float speed = g::local->velocity().length_2d();
	movement_hud_push_speed_sample(hud_state, speed);
	hud_state.timing_trend = std::lerp(hud_state.timing_trend, movement_hud_trend(hud_state), std::clamp(frame_time * 8.0f, 0.0f, 1.0f));

	if (grounded != hud_state.last_grounded) {
		if (grounded)
			hud_state.last_landing_time = current_time;
		else
			hud_state.last_takeoff_time = current_time;
		hud_state.last_grounded = grounded;
	}

	bool gained_points = false;
	const auto register_event = [&](const movement_hud_event_t event) {
		const float combo_gap = 1.15f;
		if ((current_time - hud_state.last_event_time) > combo_gap)
			movement_hud_clear_combo(hud_state);

		movement_hud_append_combo(hud_state, event);
		hud_state.last_event = event;
		hud_state.last_event_time = current_time;
		hud_state.flash_alpha = 1.0f;

		const float combo_multiplier = c::visuals::movement_hud_combo_multiplier
			? (1.0f + std::clamp(static_cast<float>(hud_state.combo_size - 1) * 0.18f, 0.0f, 1.25f))
			: 1.0f;

		points += static_cast<int>(std::round(movement_hud_points_gain(event) * combo_multiplier));
		hud_state.heat = (std::min)(600.0f, hud_state.heat + movement_hud_heat_gain(event) * combo_multiplier);
		combo_count = hud_state.combo_size;
		gained_points = true;

		if (c::visuals::movement_hud_sound_tick && current_time - hud_state.last_sound_time > 0.10f) {
			MessageBeep(MB_ICONASTERISK);
			hud_state.last_sound_time = current_time;
		}
	};

	const bool ps_event = features::movement::detected_normal_pixel_surf || features::movement::should_ps;
	const bool air_event = features::movement::should_air;
	const bool edge_event = features::movement::detected_normal_edge_bug || features::movement::should_edge_bug;
	const bool jump_bug_event = features::movement::detected_normal_jump_bug;
	const bool fireman_event = features::movement::should_fireman;
	const bool minijump_event = features::movement::should_mj;
	const bool longjump_event = features::movement::should_lj;

	if (ps_event && !last_ps)
		register_event(movement_hud_event_t::pixelsurf);
	else if (air_event && !last_air)
		register_event(movement_hud_event_t::air);
	else if (edge_event && !last_edge_bug)
		register_event(movement_hud_event_t::edgebug);
	else if (jump_bug_event && !last_jump_bug)
		register_event(movement_hud_event_t::jumpbug);
	else if (fireman_event && !last_fireman)
		register_event(movement_hud_event_t::fireman);
	else if (minijump_event && !last_minijump)
		register_event(movement_hud_event_t::minijump);
	else if (longjump_event && !last_longjump)
		register_event(movement_hud_event_t::longjump);

	// Update last states
	last_ps = ps_event;
	last_air = air_event;
	last_edge_bug = edge_event;
	last_jump_bug = jump_bug_event;
	last_fireman = fireman_event;
	last_minijump = minijump_event;
	last_longjump = longjump_event;

	if (gained_points) {
		fade_alpha = 1.f;
		combo_fade_alpha = 1.f;
		last_point_time = current_time;
		last_combo_time = current_time;
		if (c::visuals::points_hud_show_mascot)
			g_points_mascot_pending_meme = true;
	}

	// Fade out after 3 seconds
	const float display_duration = 3.f;
	const float fade_speed = 3.f;
	const float heat_decay = grounded ? 22.0f : 12.0f;
	hud_state.heat = (std::max)(0.0f, hud_state.heat - heat_decay * frame_time);
	hud_state.flash_alpha = (std::max)(0.0f, hud_state.flash_alpha - frame_time * 3.6f);

	if (current_time - last_point_time > display_duration) {
		fade_alpha = std::max<float>(0.f, fade_alpha - fade_speed * frame_time);
		// Reset points when fully faded
		if (fade_alpha <= 0.01f) {
			points = 0;
			combo_count = 0;
			movement_hud_clear_combo(hud_state);
		}
	}

	// Combo fade (slightly faster)
	const float combo_fade_speed = 2.5f;
	if (current_time - last_combo_time > display_duration) {
		combo_fade_alpha = std::max<float>(0.f, combo_fade_alpha - combo_fade_speed * frame_time);
	}

	if (hud_state.combo_size > 0) {
		const bool combo_timed_out =
			(grounded && (current_time - hud_state.last_event_time) > 0.90f) ||
			((current_time - hud_state.last_event_time) > 1.75f);

		if (combo_timed_out) {
			movement_hud_clear_combo(hud_state);
			combo_count = 0;
		}
	}

	// Don't render if fully faded
	if (fade_alpha <= 0.01f)
		return;

	// Get screen dimensions
	int w, h;
	interfaces::engine->get_screen_size(w, h);

	IDirect3DDevice9* pdev = points_resolve_d3d9_device();
	points_sync_render_device(pdev);

	// Format points text
	std::string points_text = std::to_string(points) + " pts";

	ImGui::PushFont(fonts::points_big_font);
	auto text_size = ImGui::CalcTextSize(points_text.c_str());
	ImGui::PopFont();

	static const ImVec2 padding = ImVec2(7, 7);
	static const ImVec2 margin = ImVec2(3, 3);
	static const float spacing = -30.f; // Space between points system and watermark

	// Calculate watermark width (approximate from your watermark function)
	std::string wm_text = "dna | beta | 000 fps"; // Approximate max size
	ImGui::PushFont(fonts::points_big_font);
	auto wm_size = ImGui::CalcTextSize(wm_text.c_str());
	ImGui::PopFont();
	float wm_width = wm_size.x + padding.x + margin.x * 2.f;

	// Calculate sine wave for pulsing effect (same rhythm as letter wave below)
	float sine_value = sinf(current_time * 3.f) * 0.5f + 0.5f; // 0 to 1 range
	float pulse_intensity = 0.3f + sine_value * 0.7f; // 0.3 to 1.0 range
	pulse_intensity *= fade_alpha;

	const int pts_style = std::clamp(c::visuals::points_hud_style, 0, 6);
	ImVec4 pts_accent(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], menu::menu_accent[3]);
	float panel_rounding = 5.5f;
	float wave_amplitude_scale = 1.0f;
	float sparkle_density = 0.0f;

	switch (pts_style) {
	case 1: // frost / whitearmor
		pts_accent = ImVec4(0.72f, 0.90f, 1.0f, 1.0f);
		panel_rounding = 8.0f;
		sparkle_density = 1.0f;
		wave_amplitude_scale = 0.55f;
		break;
	case 2: // dream / sherman / lean
		pts_accent = ImVec4(0.58f, 0.50f, 1.0f, 1.0f);
		panel_rounding = 9.5f;
		sparkle_density = 0.65f;
		wave_amplitude_scale = 0.75f;
		break;
	case 3: // luxury / thaiboy
		pts_accent = ImVec4(1.0f, 0.74f, 0.22f, 1.0f);
		panel_rounding = 6.0f;
		sparkle_density = 0.55f;
		wave_amplitude_scale = 0.45f;
		break;
	case 4: // chrome / bladee / ecco
		pts_accent = ImVec4(0.82f, 0.94f, 1.0f, 1.0f);
		panel_rounding = 4.0f;
		sparkle_density = 0.85f;
		wave_amplitude_scale = 0.60f;
		break;
	case 5: // crystal / gluee / icedancer
		pts_accent = ImVec4(0.62f, 0.86f, 1.0f, 1.0f);
		panel_rounding = 7.0f;
		sparkle_density = 1.15f;
		wave_amplitude_scale = 1.10f;
		break;
	case 6: // cursed / warlord / spiderr
		pts_accent = ImVec4(0.82f, 0.12f, 0.18f, 1.0f);
		panel_rounding = 3.0f;
		sparkle_density = 0.35f;
		wave_amplitude_scale = 0.90f;
		break;
	default:
		break;
	}

	// Position to the left of watermark — optional mascot + text + optional wanted row above
	pts_accent.x = std::clamp((pts_accent.x * 0.78f) + (menu::menu_accent[0] * 0.22f), 0.0f, 1.0f);
	pts_accent.y = std::clamp((pts_accent.y * 0.78f) + (menu::menu_accent[1] * 0.22f), 0.0f, 1.0f);
	pts_accent.z = std::clamp((pts_accent.z * 0.78f) + (menu::menu_accent[2] * 0.22f), 0.0f, 1.0f);

	float points_right_x = w - wm_width - spacing;

	const bool hud_mascot = c::visuals::points_hud_show_mascot;
	const bool hud_stars = c::visuals::points_hud_show_stars;

	if (hud_mascot) {
		points_try_load_mascot_once(pdev);
		points_try_roll_mascot_from_disk(pdev);
	}
	if (hud_stars)
		points_try_load_stars_once(pdev);

	float mascot_aspect = 0.78f;
	if (hud_mascot && g_points_mascot_tex) {
		D3DSURFACE_DESC td{};
		if (SUCCEEDED(g_points_mascot_tex->GetLevelDesc(0, &td)) && td.Height > 0)
			mascot_aspect = float(td.Width) / float(td.Height);
	}

	const float stars_row_h = hud_stars ? 22.f : 0.f;
	const float stars_gap = hud_stars ? 5.f : 0.f;
	const float user_mascot_scale = std::clamp(c::visuals::points_hud_mascot_scale, 0.35f, 5.f);
	const float mascot_target_h = (text_size.y * 1.35f + padding.y) * user_mascot_scale;
	const float mascot_scale = (0.92f + 0.08f * pulse_intensity) * (fade_alpha > 0.01f ? 1.f : 0.f);
	const float mascot_display_h = mascot_target_h * mascot_scale;
	const bool mascot_visible = hud_mascot && g_points_mascot_tex;
	const float mascot_display_w = mascot_visible ? (mascot_display_h * mascot_aspect) : 0.f;
	const float mascot_left_pad = mascot_visible ? 6.f : 0.f;
	const float mascot_gap = mascot_visible ? 8.f : 0.f;

	const float text_panel_inner_w = text_size.x + padding.x * 2.f + margin.x * 2.f + 4.f;
	const float text_panel_h = text_size.y + padding.y * 2.f + margin.y * 2.f + 4.f;
	const float panel_h = (std::max)(text_panel_h, mascot_target_h + 8.f);
	const float total_panel_w = mascot_left_pad + mascot_display_w + mascot_gap + text_panel_inner_w;

	const float box_top = padding.y - 2.f + stars_row_h + stars_gap;
	const float panel_right = points_right_x - padding.x + 2.f;
	ImVec2 top_left(panel_right - total_panel_w, box_top);
	ImVec2 bottom_right(panel_right, box_top + panel_h);

	ImVec2 text_pos(
		top_left.x + mascot_left_pad + mascot_display_w + mascot_gap + padding.x + margin.x,
		top_left.y + padding.y + margin.y);

	const float dance_x = sinf(current_time * 6.f + 1.1f) * 2.5f * fade_alpha;
	const float dance_y = sinf(current_time * 6.f) * 4.f * fade_alpha;

	ImVec2 mascot_min(
		top_left.x + mascot_left_pad + dance_x,
		top_left.y + (panel_h - mascot_display_h) * 0.5f + dance_y);
	ImVec2 mascot_max(mascot_min.x + mascot_display_w, mascot_min.y + mascot_display_h);

	const float wanted_meter = std::clamp(hud_state.heat, 0.0f, 600.0f) / 100.0f;
	const int wanted_full = (std::min)(6, static_cast<int>(floorf(wanted_meter)));
	const float wanted_partial = wanted_meter - floorf(wanted_meter);
	const float star_draw_size = 18.f;
	const float star_spacing = star_draw_size * 0.12f;
	const float stars_band_w = 6.f * star_draw_size + 5.f * star_spacing;
	const float stars_x0 = top_left.x + (total_panel_w - stars_band_w) * 0.5f;
	const float stars_cy = padding.y - 2.f + stars_row_h * 0.5f;

	// Colors
	ImColor accent(pts_accent.x, pts_accent.y, pts_accent.z, fade_alpha);
	ImColor white(1.f, 1.f, 1.f, pulse_intensity);
	ImColor shadow(pts_accent.x, pts_accent.y, pts_accent.z, pulse_intensity);
	ImColor fg_color(10, 10, 12, 0);
	ImColor fg2_color(10, 10, 12, 0);
	ImColor bg_color(0.14f, 0.14f, 0.14f, 0.f);
	ImColor bgfade_color(10, 10, 12, 0);
	ImColor bg2_color(10, 10, 12, 0);
	ImColor bg3_color(4, 4, 5, 0);

	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

	if (hud_stars) {
		if (g_points_stars_tex)
			draw_wanted_stars_sheet(draw_list, g_points_stars_tex, stars_x0, stars_cy, star_draw_size, wanted_full, wanted_partial, fade_alpha);
		else {
			const float star_r = 6.f;
			const float star_step = star_r * 2.f + 4.f;
			for (int si = 0; si < 6; ++si) {
				const float star_fill = (si < wanted_full) ? 1.f : (si == wanted_full ? wanted_partial : 0.f);
				const ImVec2 sc(stars_x0 + star_r + float(si) * star_step, stars_cy);
				draw_points_wanted_star_fast(draw_list, sc, star_r, star_fill, fade_alpha);
			}
		}
	}

	// Glow fade multiplier (slower than main fade for lingering effect)
	float glow_fade_multiplier = 0.5f; // 0.5 = glow fades at half the speed
	float glow_alpha_final = fade_alpha + (1.f - fade_alpha) * glow_fade_multiplier;
	glow_alpha_final = (std::min)(glow_alpha_final, 1.f); // clamp to 1.0

	if (c::visuals::style_points_background) {
		const float panel_alpha = fade_alpha * (c::visuals::points_hud_glow ? glow_alpha_final : 0.82f);
		draw_theme_panel(draw_list, top_left, bottom_right, pts_accent, panel_alpha, panel_rounding);
		draw_pts_style_ornaments(draw_list, top_left, bottom_right, pts_style, pts_accent, fade_alpha * std::clamp(sparkle_density, 0.25f, 1.25f), current_time);
		// Pulsing glow effect (fewer layers than before — full-screen rects are expensive)
		if (false && pulse_intensity > 0.15f) {
			ImColor glow(
				menu::menu_accent[0],
				menu::menu_accent[1],
				menu::menu_accent[2],
				glow_alpha_final
			);

			const float glow_expand = 7.f * glow_alpha_final;
			const int layers = 4;

			for (int i = 0; i < layers; i++) {
				float t = float(i) / float(layers - 1);
				float expand = glow_expand * t;
				float alpha = glow_alpha_final * 0.5f * (1.f - t);

				ImColor col(glow.Value.x, glow.Value.y, glow.Value.z, alpha);

				draw_list->AddRectFilled(
					ImVec2(top_left.x - expand, top_left.y - expand),
					ImVec2(bottom_right.x + expand, bottom_right.y + expand),
					col,
					5.5f,
					ImDrawFlags_RoundCornersAll
				);
			}
		}
		// Background
		draw_list->AddRectFilledMultiColorRounded(
			top_left, bottom_right,
			bgfade_color, fg2_color, fg2_color, bg_color, bg_color,
			5.5f, ImDrawFlags_RoundCornersAll
		);
		draw_list->AddRectFilledMultiColorRounded(
			top_left, bottom_right,
			bgfade_color, bg_color, bg_color, fg2_color, fg2_color,
			5.5f, ImDrawFlags_RoundCornersAll
		);
		draw_list->AddRect(top_left, bottom_right, bg2_color, 5.5f);
		draw_list->AddRect(
			{ top_left.x - 1.f, top_left.y - 1.f },
			{ bottom_right.x + 1.f, bottom_right.y + 1.f },
			bg3_color, 5.5f
		);
		draw_list->AddRect(
			{ top_left.x - 2.f, top_left.y - 2.f },
			{ bottom_right.x + 2.f, bottom_right.y + 2.f },
			bg2_color, 5.5f
		);

		// Gradient bar at top
		const float grad_width = text_size.x * 1.1f;
		const float grad_height = 3.f;
		const float center_x = text_pos.x + text_size.x * 0.5f;
		const float grad_top_y = text_pos.y - grad_height - 3.f;
		const float grad_bottom_y = grad_top_y + grad_height;
		const int slices = 60;

		for (int i = 0; i < slices; i++) {
			float slice_x0 = center_x - grad_width * 0.5f + (grad_width / slices) * i;
			float slice_x1 = slice_x0 + (grad_width / slices);

			float dist = fabsf((slice_x0 + slice_x1) * 0.5f - center_x) / (grad_width * 0.5f);
			float alpha = 1.f - dist;
			if (alpha < 0.f) alpha = 0.f;

			ImColor col = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 0.0f * alpha * pulse_intensity);

			draw_list->AddRectFilled(
				ImVec2(slice_x0, grad_top_y),
				ImVec2(slice_x1, grad_bottom_y),
				col,
				2.0f, 0
			);
		}
	}

	if (mascot_visible) {
		const ImU32 tint = IM_COL32(255, 255, 255, int(255.f * fade_alpha));
		const float rev_dur = 0.58f;
		float scale_mul = 1.f;
		float u0 = 0.f, u1 = 1.f;
		if (interfaces::globals && g_mascot_reverse_anim_start > -1e8f) {
			const float tlin = std::clamp((current_time - g_mascot_reverse_anim_start) / rev_dur, 0.f, 1.f);
			const float sm = tlin * tlin * (3.f - 2.f * tlin);
			scale_mul = 1.f + 0.58f * (1.f - sm);
			u0 = 1.f - sm;
			u1 = sm;
			if (tlin >= 0.999f) {
				u0 = 0.f;
				u1 = 1.f;
				scale_mul = 1.f;
			}
		}
		const ImVec2 ctr((mascot_min.x + mascot_max.x) * 0.5f, (mascot_min.y + mascot_max.y) * 0.5f);
		const float hw = (mascot_max.x - mascot_min.x) * 0.5f * scale_mul;
		const float hh = (mascot_max.y - mascot_min.y) * 0.5f * scale_mul;
		const ImVec2 dmin(ctr.x - hw, ctr.y - hh);
		const ImVec2 dmax(ctr.x + hw, ctr.y + hh);
		draw_list->AddImage((ImTextureID)g_points_mascot_tex, dmin, dmax, ImVec2(u0, 0.f), ImVec2(u1, 1.f), tint);
	}

	// Draw text with wave per letter
	ImGui::PushFont(fonts::points_big_font);

	float letter_spacing = 0.f;

	// Wave settings
	float wave_amplitude = 4.f * wave_amplitude_scale;   // height of wave
	float wave_speed = 6.f;   // speed of movement
	float wave_offset = current_time * wave_speed;

	// Ghost shadow settings
	int shadow_layers = 4;           // number of shadow "ghosts"
	float shadow_step = 1.15f;       // spacing between ghost layers
	ImColor ghost_color = ImColor(pts_accent.x, pts_accent.y, pts_accent.z, pts_accent.w);  // your accent color

	for (size_t i = 0; i < points_text.size(); i++)
	{
		const char c = points_text[i];
		std::string s(1, c);

		// Wave offset for this letter
		float offset_y = sinf(wave_offset + i * 0.15f) * wave_amplitude;

		ImVec2 letter_pos(
			text_pos.x + letter_spacing,
			text_pos.y + offset_y
		);

		// ===== GHOST TRAILS ABOVE & BELOW =====
		if (c::visuals::points_hud_trails)
		for (int g = 1; g <= shadow_layers; g++)
		{
			float t = float(g) / float(shadow_layers + 1);
			float ghost_offset = shadow_step * g;
			float ghost_offset_down = 0.7f * g;
			float alpha = (1.f - t) * fade_alpha;

			// GHOST BELOW   (positive Y)
			ImVec2 ghost_pos_down(letter_pos.x, letter_pos.y + ghost_offset_down);

			// GHOST ABOVE   (negative Y)
			ImVec2 ghost_pos_up(letter_pos.x, letter_pos.y - ghost_offset);

			ImU32 faded_ghost = ImGui::GetColorU32(ImVec4(
				((ghost_color >> 0) & 0xFF) / 255.f,
				((ghost_color >> 8) & 0xFF) / 255.f,
				((ghost_color >> 16) & 0xFF) / 255.f,
				alpha * 0.7f  // smaller alpha → closer to original text
			));

			draw_list->AddText(ghost_pos_down, faded_ghost, s.c_str());
			draw_list->AddText(ghost_pos_up, faded_ghost, s.c_str());
		}

		// ===== MAIN TEXT =====
		draw_list->AddText(
			ImVec2(letter_pos.x, letter_pos.y),
			IM_COL32(255, 255, 255, 255 * fade_alpha),
			s.c_str()
		);

		// Advance spacing
		letter_spacing += ImGui::CalcTextSize(s.c_str()).x;
	}

	ImGui::PopFont();

	// Draw combo text if combo > 1
	if (c::visuals::style_points_combo && combo_count > 1 && combo_fade_alpha > 0.01f) {
		std::string combo_text = "x" + std::to_string(combo_count) + " combo";

		ImGui::PushFont(fonts::points_big_font);
		auto combo_size = ImGui::CalcTextSize(combo_text.c_str());

		// Position below main text
		float combo_y_offset = text_size.y + 8.f;
		ImVec2 combo_base_pos(
			text_pos.x + (text_size.x - combo_size.x) * 0.5f, // Center it
			text_pos.y + combo_y_offset
		);

		float combo_letter_spacing = 0.f;

		// Combo wave settings (same as main text but phase offset)
		float combo_wave_amplitude = 4.f * wave_amplitude_scale;
		float combo_wave_speed = 6.f;
		float combo_wave_offset = current_time * combo_wave_speed + 1.5f; // Phase offset

		// Combo ghost settings (same as main text)
		int combo_shadow_layers = 4;
		float combo_shadow_step = 1.15f;
		ImColor combo_ghost_color = ImColor(pts_accent.x, pts_accent.y, pts_accent.z, pts_accent.w);

		for (size_t i = 0; i < combo_text.size(); i++)
		{
			const char c = combo_text[i];
			std::string s(1, c);

			// Wave offset for this letter
			float offset_y = sinf(combo_wave_offset + i * 0.15f) * combo_wave_amplitude;

			ImVec2 letter_pos(
				combo_base_pos.x + combo_letter_spacing,
				combo_base_pos.y + offset_y
			);

			// ===== GHOST TRAILS ABOVE & BELOW =====
			if (c::visuals::points_hud_trails)
			for (int g = 1; g <= combo_shadow_layers; g++)
			{
				float t = float(g) / float(combo_shadow_layers + 1);
				float ghost_offset = combo_shadow_step * g;
				float ghost_offset_down = 0.7f * g;
				float alpha = (1.f - t) * combo_fade_alpha;

				// GHOST BELOW
				ImVec2 ghost_pos_down(letter_pos.x, letter_pos.y + ghost_offset_down);

				// GHOST ABOVE
				ImVec2 ghost_pos_up(letter_pos.x, letter_pos.y - ghost_offset);

				ImU32 faded_ghost = ImGui::GetColorU32(ImVec4(
					((combo_ghost_color >> 0) & 0xFF) / 255.f,
					((combo_ghost_color >> 8) & 0xFF) / 255.f,
					((combo_ghost_color >> 16) & 0xFF) / 255.f,
					alpha * 0.7f
				));

				draw_list->AddText(ghost_pos_down, faded_ghost, s.c_str());
				draw_list->AddText(ghost_pos_up, faded_ghost, s.c_str());
			}

			// ===== MAIN COMBO TEXT =====
			draw_list->AddText(
				letter_pos,
				IM_COL32(255, 255, 255, 255 * combo_fade_alpha),
				s.c_str()
			);

			combo_letter_spacing += ImGui::CalcTextSize(s.c_str()).x;
		}

		ImGui::PopFont();
	}

	const bool show_secondary_panel =
		c::visuals::movement_hud_combo_tracker ||
		c::visuals::movement_hud_velocity_strip ||
		c::visuals::movement_hud_trick_flash ||
		c::visuals::movement_hud_timing_indicator;

	if (show_secondary_panel) {
		const float secondary_gap = 10.0f;
		const float secondary_h = 64.0f;
		const ImVec2 secondary_min(top_left.x, bottom_right.y + secondary_gap);
		const ImVec2 secondary_max(bottom_right.x, secondary_min.y + secondary_h);
		const float secondary_alpha = fade_alpha * 0.92f;
		const float content_left = secondary_min.x + 10.0f;
		const float content_right = secondary_max.x - 10.0f;

		draw_theme_panel(draw_list, secondary_min, secondary_max, pts_accent, secondary_alpha, panel_rounding);
		draw_pts_style_ornaments(draw_list, secondary_min, secondary_max, pts_style, pts_accent, secondary_alpha * 0.75f, current_time);

		ImGui::PushFont(fonts::watermark_font ? fonts::watermark_font : fonts::points_big_font);

		float cursor_y = secondary_min.y + 8.0f;
		if (c::visuals::movement_hud_combo_tracker) {
			std::string chain_text = movement_hud_combo_text(hud_state);
			if (chain_text.empty())
				chain_text = "waiting for movement chain";
			else if (c::visuals::movement_hud_combo_multiplier && combo_count > 1) {
				char multiplier_text[32]{};
				_snprintf_s(multiplier_text, sizeof(multiplier_text), _TRUNCATE, "  x%.2f", 1.0f + std::clamp(static_cast<float>(combo_count - 1) * 0.18f, 0.0f, 1.25f));
				chain_text += multiplier_text;
			}

			draw_list->AddText(ImVec2(content_left, cursor_y), ImColor(1.0f, 1.0f, 1.0f, secondary_alpha), chain_text.c_str());
			cursor_y += 18.0f;
		}

		if (c::visuals::movement_hud_velocity_strip) {
			draw_compact_velocity_strip(draw_list, ImVec2(content_left, cursor_y), ImVec2(content_right, cursor_y + 22.0f), hud_state, pts_accent, secondary_alpha);
			cursor_y += 30.0f;
		}

		if (c::visuals::movement_hud_trick_flash && hud_state.flash_alpha > 0.01f && hud_state.last_event != movement_hud_event_t::none) {
			const float pill_width = 96.0f;
			const ImVec4 flash_color = color_lerp(pts_accent, make_color(1.0f, 1.0f, 1.0f, 1.0f), 0.18f);
			draw_hud_pill(draw_list, ImVec2(content_left, secondary_max.y - 24.0f), ImVec2(content_left + pill_width, secondary_max.y - 4.0f), flash_color, secondary_alpha * hud_state.flash_alpha, movement_hud_event_label(hud_state.last_event));
		}

		if (c::visuals::movement_hud_timing_indicator) {
			const float trend = hud_state.timing_trend;
			const char* timing_text = trend > 8.0f ? "perfect" : (trend > 0.75f ? "early" : "late");
			ImVec4 timing_color = trend > 8.0f
				? make_color(0.16f, 0.92f, 0.48f, 1.0f)
				: (trend > 0.75f ? make_color(1.0f, 0.78f, 0.26f, 1.0f) : make_color(1.0f, 0.28f, 0.28f, 1.0f));
			timing_color = color_lerp(timing_color, pts_accent, pts_style == 6 ? 0.10f : 0.18f);

			const float pill_width = 86.0f;
			draw_hud_pill(draw_list, ImVec2(secondary_max.x - pill_width - 10.0f, secondary_max.y - 24.0f), ImVec2(secondary_max.x - 10.0f, secondary_max.y - 4.0f), timing_color, secondary_alpha, timing_text);
		}

		ImGui::PopFont();
	}
}

void features::visuals::jump_distance_display()
{
	if (!g::local || !g::local->is_alive())
		return;

	static vec3_t takeoff_pos = { };
	static float jump_distance = 0.f;
	static bool was_onground = false;

	static float fade = 0.f;
	static float land_time = 0.f;

	float current_time = interfaces::globals->realtime;

	bool onground = (g::local->flags() & fl_onground);

	// ================================
	//   REAL GOKZ TAKEOFF CAPTURE
	// ================================
	if (was_onground && !onground)
	{
		vec3_t vel = g::local->velocity();

		if (vel.z > 1.f && (g::cmd->buttons & in_jump))
			takeoff_pos = g::local->origin();
	}

	// ================================
	//   REAL GOKZ LANDING CAPTURE
	// ================================
	if (!was_onground && onground)
	{
		vec3_t land_pos = g::local->origin();

		float dx = land_pos.x - takeoff_pos.x;
		float dy = land_pos.y - takeoff_pos.y;

		float planar = sqrtf(dx * dx + dy * dy);

		// Player horizontal hull radius in Source
		constexpr float PLAYER_RADIUS = 16.f;

		// Add radius at takeoff + landing
		jump_distance = planar + (PLAYER_RADIUS * 2.f);

		fade = 1.f;
		land_time = current_time;
	}

	was_onground = onground;

	// Fade behavior
	if (current_time - land_time > 3.f)
		fade = std::max<float>(0.f, fade - 3.f * interfaces::globals->frame_time);

	if (fade <= 0.01f)
		return;

	// ======================
	//  RENDER (unchanged)
	// ======================
	std::string text = std::to_string((int)jump_distance) + "u";

	int w, h;
	interfaces::engine->get_screen_size(w, h);

	ImVec2 base_pos(w * 0.75f, 90.f);

	float wave_amp = 4.f;
	float wave_speed = 6.f;
	float wave_off = current_time * wave_speed;

	ImDrawList* draw = ImGui::GetBackgroundDrawList();
	ImGui::PushFont(fonts::points_big_font);

	float spacing = 0.f;

	for (size_t i = 0; i < text.size(); i++)
	{
		std::string s(1, text[i]);

		float offset_y = sinf(wave_off + i * 0.15f) * wave_amp;

		ImVec2 pos(base_pos.x + spacing, base_pos.y + offset_y);

		// Ghost trails identical to your points
		const int ghost_layers = 4;
		for (int g = 1; g <= ghost_layers; g++)
		{
			float t = float(g) / (ghost_layers + 1);
			float alpha = (1.f - t) * fade;

			ImU32 ghost_col = ImGui::GetColorU32(ImVec4(
				menu::menu_accent[0],
				menu::menu_accent[1],
				menu::menu_accent[2],
				alpha
			));

			float up = 1.15f * g;
			float down = 0.7f * g;

			draw->AddText(ImVec2(pos.x, pos.y - up), ghost_col, s.c_str());
			draw->AddText(ImVec2(pos.x, pos.y + down), ghost_col, s.c_str());
		}

		draw->AddText(pos, IM_COL32(255, 255, 255, 255 * fade), s.c_str());

		spacing += ImGui::CalcTextSize(s.c_str()).x;
	}

	ImGui::PopFont();
}

void features::visuals::watermark() {
	// ... indicators ...
	const auto active_variant = menu::active_menu_variant();
	const auto& active_profile = menu::menu_variant_profile(active_variant);
	if (active_variant != menu::menu_variant_t::drainware_original && fonts::watermark_font) {
		int sw, sh;
		interfaces::engine->get_screen_size(sw, sh);
		im_render.text(sw / 2, sh - 100, 12.f, fonts::watermark_font, active_profile.watermark_label, true,
			color_t(static_cast<int>(active_profile.accent[0] * 255.f), static_cast<int>(active_profile.accent[1] * 255.f), static_cast<int>(active_profile.accent[2] * 255.f), 150), false);
	}

	if (!c::movement::billware_wm)
		return;

	if (!(c::movement::watermark_type == 0))
		return;

	static float frame_rate = 0.f;
	static float last_update_time = 0.f;
	static const float update_interval = 0.5f;

	float current_time = interfaces::globals->realtime;

	// prevent division by zero
	float ft = interfaces::globals->frame_time;
	if (ft <= 0.f)
		ft = 0.0001f;

	if (current_time - last_update_time >= update_interval) {
		const float alpha = 0.7f;
		frame_rate = alpha * frame_rate + (1.f - alpha) * (1.f / ft);

		last_update_time = current_time;
	}

	int fps = static_cast<int>(frame_rate);
	std::string fps_number = std::to_string(fps);
	std::string fps_label = " fps";

	std::string part1 = active_profile.watermark_label;

	std::string sep1 = " | ";
	std::string part2 = c::movement::watermark_nickname;
	std::string sep2 = " | ";

	int w, h;
	interfaces::engine->get_screen_size(w, h);

	std::string full_text = part1 + sep1 + part2 + sep2 + fps_number + fps_label;
	auto text_size = im_render.measure_text(full_text.c_str(), fonts::watermark_font, 15.f);
	static const ImVec2 padding = ImVec2(7, 7);
	static const ImVec2 margin = ImVec2(3, 3);

	ImVec2 text_pos = ImVec2(w - text_size.x - padding.x - margin.x, padding.y + margin.y);

	ImVec2 top_left(w - text_size.x - padding.x - margin.x * 2.f - 2.f, padding.y - 2.f);
	ImVec2 bottom_right(w - padding.x + 2.f, text_size.y + padding.y + margin.y * 2.f + 2.f);

	// === BACKGROUND DRAWING ===
	const auto watermark_style = resolve_watermark_style();
	const ImVec4 watermark_secondary = resolve_watermark_secondary_text(watermark_style);
	ImColor accent(watermark_style.accent.x, watermark_style.accent.y, watermark_style.accent.z, watermark_style.accent.w);
	ImColor darkgrey(watermark_secondary.x, watermark_secondary.y, watermark_secondary.z, watermark_secondary.w);
	ImColor white(watermark_style.text.x, watermark_style.text.y, watermark_style.text.z, watermark_style.text.w);
	ImColor fg_color(10, 10, 12, 0);
	ImColor fg2_color(10, 10, 12, 0);
	ImColor bg_color(0.14f, 0.14f, 0.14f, 0.f);
	ImColor bgfade_color(10, 10, 12, 0);
	ImColor bg2_color(10, 10, 12, 0);
	ImColor bg3_color(4, 4, 5, 0);

	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	draw_theme_panel(draw_list, top_left, bottom_right, watermark_style.accent, 1.0f, 5.5f);

	// Track when single-tick events occurred
	static int last_jump_bug_tick = 0;
	static int last_fireman_tick = 0;
	static int last_air_tick = 0;
	static int last_edgebug_tick = 0;

	// Update saved ticks when events occur
	if (features::movement::detected_normal_jump_bug)
		last_jump_bug_tick = interfaces::globals->tick_count;
	if (features::movement::should_fireman)
		last_fireman_tick = interfaces::globals->tick_count;
	if (features::movement::should_air)
		last_air_tick = interfaces::globals->tick_count;
	if (features::movement::should_edge_bug)
		last_edgebug_tick = interfaces::globals->tick_count;

	// Check if we're within the glow duration for single-tick events
	bool jump_bug_active = (interfaces::globals->tick_count - last_jump_bug_tick) < c::movement::detection_saved_tick;
	bool fireman_active = (interfaces::globals->tick_count - last_fireman_tick) < c::movement::detection_saved_tick;
	bool air_active = (interfaces::globals->tick_count - last_air_tick) < c::movement::detection_saved_tick;
	bool edgebug_active = (interfaces::globals->tick_count - last_edgebug_tick) < c::movement::detection_saved_tick;

	// fade strength
	static float glow_strength = 0.f;
	const float fade_speed = 3.f; // faster fade (increase for faster)
	bool spotted = features::visuals::is_spotted ||
		features::movement::should_ps ||
		edgebug_active ||
		jump_bug_active ||
		fireman_active ||
		air_active;

	// fade in/out
	if (spotted)
		glow_strength = std::min<float>(1.f, glow_strength + fade_speed * interfaces::globals->frame_time);
	else
		glow_strength = std::max<float>(0.f, glow_strength - fade_speed * interfaces::globals->frame_time);

	// ===== ROUNDED-RECT NEON GLOW (matches background shape) =====
	if (glow_strength > 0.01f)
	{
		ImColor neon(
			watermark_style.accent.x,
			watermark_style.accent.y,
			watermark_style.accent.z,
			1.f
		);

		const float base_alpha = 0.75f * glow_strength;
		const float base_expand = 7.f * glow_strength; // how far glow expands

		const int layers = 10;  // more = smoother
		for (int i = 0; i < layers; i++)
		{
			float t = float(i) / float(layers - 1);

			float expand = base_expand * t;           // outer expansion size
			float alpha = base_alpha * (1.f - t);    // fade outward

			ImColor col(
				neon.Value.x,
				neon.Value.y,
				neon.Value.z,
				alpha
			);

			draw_list->AddRectFilled(
				ImVec2(top_left.x - expand, top_left.y - expand),
				ImVec2(bottom_right.x + expand, bottom_right.y + expand),
				col,
				5.5f, // <= matches background perfectly
				ImDrawFlags_RoundCornersAll
			);
		}

		// strong inner neon punch
		draw_list->AddRectFilled(
			ImVec2(top_left.x - 2, top_left.y - 2),
			ImVec2(bottom_right.x + 2, bottom_right.y + 2),
			ImColor(neon.Value.x, neon.Value.y, neon.Value.z, glow_strength * 0.9f),
			5.5f,
			ImDrawFlags_RoundCornersAll
		);
	}

	draw_list->AddRectFilledMultiColorRounded(
		top_left, bottom_right,
		bgfade_color, fg2_color, fg2_color, bg_color, bg_color,
		5.5f, ImDrawFlags_RoundCornersAll
	);
	draw_list->AddRectFilledMultiColorRounded(
		top_left, bottom_right,
		bgfade_color, bg_color, bg_color, fg2_color, fg2_color,
		5.5f, ImDrawFlags_RoundCornersAll
	);
	draw_list->AddRect(top_left, bottom_right, bg2_color, 5.5f);
	draw_list->AddRect(
		{ top_left.x - 1.f, top_left.y - 1.f },
		{ bottom_right.x + 1.f, bottom_right.y + 1.f },
		bg3_color, 5.5f
	);
	draw_list->AddRect(
		{ top_left.x - 2.5f, top_left.y - 2.5f },
		{ bottom_right.x + 2.5f, bottom_right.y + 2.5f },
		bg2_color, 5.5f
	);

	// === SNOW EFFECT ===
	static std::vector<ImVec2> snow_positions;
	static const int max_snowflakes = 10;
	static const float snow_speed = 0.07f;
	static const float snow_opacity = 0.4f;

	if (snow_positions.empty()) {
		for (int i = 0; i < max_snowflakes; ++i) {
			float x = top_left.x + (rand() % static_cast<int>(bottom_right.x - top_left.x));
			float y = top_left.y + (rand() % static_cast<int>(bottom_right.y - top_left.y));
			snow_positions.push_back(ImVec2(x, y));
		}
	}

	for (auto& pos : snow_positions) {
		ImGui::PushFont(fonts::dna_icon_rain);
		//draw_list->AddText(pos, ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 50.f), "A");
		ImGui::PopFont();
		//draw_list->AddCircleFilled(pos, 0.5f, ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], snow_opacity));

		pos.y += snow_speed;
		if (pos.y > bottom_right.y - 1) {
			pos.y = top_left.y + (rand() % static_cast<int>(bottom_right.y - top_left.y));
			pos.x = top_left.x + (rand() % static_cast<int>(bottom_right.x - top_left.x));
		}
	}

	// === GRADIENT CUT (TOP BAR) ===
	const ImColor bloodRedStrong = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 0.75f);
	const float grad_width = text_size.x * 1.1f;
	const float grad_height = 3.f;
	const float center_x = text_pos.x + text_size.x * 0.5f;
	const float grad_top_y = text_pos.y - grad_height - 3.f;
	const float grad_bottom_y = grad_top_y + grad_height;
	const int slices = 60;

	for (int i = 0; i < slices; i++) {
		float slice_x0 = center_x - grad_width * 0.5f + (grad_width / slices) * i;
		float slice_x1 = slice_x0 + (grad_width / slices);

		float dist = fabsf((slice_x0 + slice_x1) * 0.5f - center_x) / (grad_width * 0.5f);
		float alpha = 1.f - dist;
		if (alpha < 0.f) alpha = 0.f;

		ImColor col = ImColor(watermark_style.accent.x, watermark_style.accent.y, watermark_style.accent.z, 0.75f * alpha);

		draw_list->AddRectFilled(
			ImVec2(slice_x0, grad_top_y),
			ImVec2(slice_x1, grad_bottom_y),
			col,
			2.0f, 0
		);
	}

	//// === ICON (PARTIALLY CLIPPED, SAME AS CLARITY) ===
	//{
	//	ImGui::PushFont(fonts::logo_watermark); // same icon font

	//	const char* icon = "A";
	//	ImVec2 icon_size = ImGui::CalcTextSize(icon);

	//	// same visible ratio as clarity
	//	const float visible_ratio = 0.7f;

	//	// position identical logic
	//	ImVec2 icon_pos(
	//		top_left.x + 6.f - (icon_size.x * (1.f - visible_ratio)),
	//		text_pos.y - 25.f
	//	);

	//	// clip strictly to watermark bounds
	//	draw_list->PushClipRect(top_left, bottom_right, true);
	//	draw_list->AddText(icon_pos,
	//		ImColor(
	//			menu::menu_accent[0],
	//			menu::menu_accent[1],
	//			menu::menu_accent[2],
	//			0.15f // same subtle fade as clarity
	//		),
	//		icon
	//	);
	//	draw_list->PopClipRect();

	//	ImGui::PopFont();
	//}

	// === TEXT DRAWING ===
	ImGui::PushFont(fonts::watermark_font);

	ImVec2 cursor = text_pos;

	// Draw text parts with alternating colors
	const ImVec4 accent_glow = scale_alpha(watermark_style.accent, 0.85f);
	const ImVec4 text_glow = color_lerp(watermark_style.accent, watermark_style.text, 0.45f);
	draw_watermark_text_segment(draw_list, cursor, part1, watermark_style.accent, watermark_style, &accent_glow);
	cursor.x += ImGui::CalcTextSize(part1.c_str()).x;

	draw_watermark_text_segment(draw_list, cursor, sep1, watermark_secondary, watermark_style, &text_glow);
	cursor.x += ImGui::CalcTextSize(sep1.c_str()).x;

	draw_watermark_text_segment(draw_list, cursor, part2, watermark_style.text, watermark_style, &text_glow);
	cursor.x += ImGui::CalcTextSize(part2.c_str()).x;

	draw_watermark_text_segment(draw_list, cursor, sep2, watermark_secondary, watermark_style, &text_glow);
	cursor.x += ImGui::CalcTextSize(sep2.c_str()).x;

	draw_watermark_text_segment(draw_list, cursor, fps_number, watermark_style.text, watermark_style, &text_glow);
	cursor.x += ImGui::CalcTextSize(fps_number.c_str()).x;

	draw_watermark_text_segment(draw_list, cursor, fps_label, watermark_secondary, watermark_style, &text_glow);

	ImGui::PopFont();
}

void features::visuals::clarity_watermark() {
	if (!c::movement::billware_wm)
		return;

	if (!(c::movement::watermark_type == 1))
		return;

	static float frame_rate = 0.f;
	static float last_update_time = 0.f;
	static const float update_interval = 0.5f;

	float current_time = interfaces::globals->realtime;

	// prevent division by zero
	float ft = interfaces::globals->frame_time;
	if (ft <= 0.f)
		ft = 0.0001f;

	if (current_time - last_update_time >= update_interval) {
		const float alpha = 0.7f;
		frame_rate = alpha * frame_rate + (1.f - alpha) * (1.f / ft);

		last_update_time = current_time;
	}

	int fps = static_cast<int>(frame_rate);
	std::string fps_number = std::to_string(fps);
	std::string fps_label = " fps";

	const auto clarity_variant = menu::active_menu_variant();
	const auto& clarity_profile = menu::menu_variant_profile(clarity_variant);
	std::string part1 = clarity_variant == menu::menu_variant_t::drainware_original ? "clarity" : clarity_profile.watermark_label;

	std::string sep1 = " | ";
	std::string part2 = c::movement::watermark_nickname;
	std::string sep2 = " | ";

	int w, h;
	interfaces::engine->get_screen_size(w, h);

	std::string full_text = part1 + sep1 + part2 + sep2 + fps_number + fps_label;
	auto text_size = im_render.measure_text(full_text.c_str(), fonts::clarity_watermark, 15.f);
	static const ImVec2 padding = ImVec2(7, 7);
	static const ImVec2 margin = ImVec2(3, 3);

	ImVec2 text_pos = ImVec2(w - text_size.x - padding.x - margin.x, padding.y + margin.y);

	ImVec2 top_left(w - text_size.x - padding.x - margin.x * 2.f - 2.f, padding.y - 2.f);
	ImVec2 bottom_right(w - padding.x + 2.f, text_size.y + padding.y + margin.y * 2.f + 2.f);

	// === BACKGROUND DRAWING ===
	const auto watermark_style = resolve_watermark_style();
	const ImVec4 watermark_secondary = resolve_watermark_secondary_text(watermark_style);
	ImColor accent(watermark_style.accent.x, watermark_style.accent.y, watermark_style.accent.z, watermark_style.accent.w);
	ImColor accent2(watermark_style.accent.x, watermark_style.accent.y, watermark_style.accent.z, 0.15f);
	ImColor darkgrey(watermark_secondary.x, watermark_secondary.y, watermark_secondary.z, watermark_secondary.w);
	ImColor darkgrey2(
		std::clamp(watermark_secondary.x * 0.62f, 0.0f, 1.0f),
		std::clamp(watermark_secondary.y * 0.62f, 0.0f, 1.0f),
		std::clamp(watermark_secondary.z * 0.62f, 0.0f, 1.0f),
		watermark_secondary.w);
	ImColor white(watermark_style.text.x, watermark_style.text.y, watermark_style.text.z, watermark_style.text.w);
	ImColor fg_color(10, 10, 12, 0);
	ImColor fg2_color(10, 10, 12, 0);
	ImColor bg_color(0.14f, 0.14f, 0.14f, 0.f);
	ImColor bgfade_color(10, 10, 12, 0);
	ImColor bg2_color(20, 20, 22, 0);
	ImColor bg3_color(4, 4, 5, 0);

	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	draw_theme_panel(draw_list, top_left, bottom_right, watermark_style.accent, 1.0f, 5.5f);

	draw_list->AddRectFilledMultiColorRounded(
		top_left, bottom_right,
		bgfade_color, fg2_color, fg2_color, bg_color, bg_color,
		5.5f, ImDrawFlags_RoundCornersAll
	);
	draw_list->AddRectFilledMultiColorRounded(
		top_left, bottom_right,
		bgfade_color, bg_color, bg_color, fg2_color, fg2_color,
		5.5f, ImDrawFlags_RoundCornersAll
	);
	draw_list->AddRect(top_left, bottom_right, bg2_color, 5.5f);
	draw_list->AddRect(
		{ top_left.x - 1.f, top_left.y - 1.f },
		{ bottom_right.x + 1.f, bottom_right.y + 1.f },
		bg3_color, 5.5f
	);
	draw_list->AddRect(
		{ top_left.x - 2.5f, top_left.y - 2.5f },
		{ bottom_right.x + 2.5f, bottom_right.y + 2.5f },
		bg2_color, 5.5f
	);

	// === TEXT DRAWING ===
	ImGui::PushFont(fonts::clarity_watermark);

	ImVec2 cursor = text_pos;

	// === ICON (PARTIALLY CLIPPED, NO BOX) ===
	ImGui::PushFont(fonts::watermark_icons);

	const char* icon = "A";
	ImVec2 icon_size = ImGui::CalcTextSize(icon);

	// show only ~70% of icon width
	const float visible_ratio = 0.7f;
	const float advance_x = icon_size.x * visible_ratio;

	// draw icon shifted left so part is clipped
	ImVec2 icon_pos(
		top_left.x + 6.f - (icon_size.x * (1.f - visible_ratio)),
		cursor.y - 17.f
	);

	// clip strictly to watermark bounds
	draw_list->PushClipRect(top_left, bottom_right, true);
	draw_list->AddText(icon_pos, accent2, icon);
	draw_list->PopClipRect();

	// advance cursor by visible portion only
	//cursor.x = top_left.x + 6.f + advance_x + 4.f;

	ImGui::PopFont();

	// Draw text parts with alternating colors
	const ImVec4 accent_glow = scale_alpha(watermark_style.accent, 0.85f);
	const ImVec4 text_glow = color_lerp(watermark_style.accent, watermark_style.text, 0.45f);
	draw_watermark_text_segment(draw_list, cursor, part1, watermark_style.accent, watermark_style, &accent_glow);
	cursor.x += ImGui::CalcTextSize(part1.c_str()).x;

	draw_watermark_text_segment(draw_list, cursor, sep1, make_color(darkgrey2.Value.x, darkgrey2.Value.y, darkgrey2.Value.z, darkgrey2.Value.w), watermark_style, &text_glow);
	cursor.x += ImGui::CalcTextSize(sep1.c_str()).x;

	draw_watermark_text_segment(draw_list, cursor, part2, watermark_style.text, watermark_style, &text_glow);
	cursor.x += ImGui::CalcTextSize(part2.c_str()).x;

	draw_watermark_text_segment(draw_list, cursor, sep2, make_color(darkgrey2.Value.x, darkgrey2.Value.y, darkgrey2.Value.z, darkgrey2.Value.w), watermark_style, &text_glow);
	cursor.x += ImGui::CalcTextSize(sep2.c_str()).x;

	draw_watermark_text_segment(draw_list, cursor, fps_number, watermark_style.text, watermark_style, &text_glow);
	cursor.x += ImGui::CalcTextSize(fps_number.c_str()).x;

	draw_watermark_text_segment(draw_list, cursor, fps_label, watermark_secondary, watermark_style, &text_glow);

	ImGui::PopFont();
}

void features::visuals::init() {
	static bool show_init_screen = true;
	static float init_start_time = interfaces::globals->realtime;
	const float init_duration = 5.0f; // extended duration for longer load
	static constexpr int max_particles = 75;
	static struct particle_t {
		ImVec2 pos;
		ImVec2 velocity;
		float alpha;
		float size;
	} particles[max_particles];
	static bool initialized_particles = false;

	float elapsed = interfaces::globals->realtime - init_start_time;
	float alpha = 1.0f;

	// Fade out
	if (elapsed >= init_duration) {
		alpha = 1.0f - (elapsed - init_duration);
		if (alpha <= 0.0f) {
			show_init_screen = false;
			has_initalized = true;
			initialized_particles = false;
			return;
		}
	}

	// Screen setup
	int screen_w, screen_h;
	interfaces::engine->get_screen_size(screen_w, screen_h);
	ImVec2 center(screen_w / 2.f, screen_h / 2.f);
	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

	// Background overlay
	draw_list->AddRectFilled(ImVec2(0, 0), ImVec2(screen_w, screen_h), ImColor(0.02f, 0.02f, 0.02f, 0.8f * alpha));

	const float spinner_radius = 35.f;
	const float thickness = 4.f;
	const float rotation_speed = 0.4f; // slower rotation
	const float arc_length = m_pi * 1.2f; // length of the arc in radians
	float current_time = interfaces::globals->realtime;

	// Calculate start angle for rotation
	float start_angle = current_time * rotation_speed;
	float end_angle = start_angle + arc_length;

	// Center Text
	std::string loading_text = "initializing...";
	std::string software_text = "dna";
	auto text_size = ImGui::CalcTextSize(loading_text.c_str());
	ImVec2 text_pos(center.x - text_size.x / 2.f, center.y + spinner_radius + 30.f);
	draw_list->AddText(ImVec2(text_pos.x - 1, text_pos.y - 1), ImColor(1.f, 1.f, 1.f, alpha * 0.4f), loading_text.c_str());	
	draw_list->AddText(text_pos, ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], alpha), loading_text.c_str());

	ImGui::PushFont(fonts::logo);
	draw_list->AddText(ImVec2(text_pos.x - 7, text_pos.y - 100), ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], alpha), "A");
	ImGui::PopFont();

	// "DNA" below
	auto software_size = ImGui::CalcTextSize(software_text.c_str());
	draw_list->AddText(ImVec2(center.x - software_size.x / 2.f - 6, text_pos.y + text_size.y + 10.f),
		ImColor(1.f, 1.f, 1.f, alpha * 0.7f), software_text.c_str());

	draw_list->AddText(ImVec2(center.x - software_size.x / 2.f - 7, text_pos.y + text_size.y + 11.f),
		ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], alpha * 0.7f), software_text.c_str());

	// Initialize particles once
	if (!initialized_particles) {
		for (auto& p : particles) {
			p.pos = ImVec2(rand() % screen_w, rand() % screen_h);
			p.velocity = ImVec2((rand() % 10 - 5) * 0.15f, -(0.8f + (rand() % 5) * 0.15f));
			p.alpha = 0.4f + (rand() % 60) / 100.f;
			p.size = 1.8f + (rand() % 50) / 30.f;
		}
		initialized_particles = true;
	}

	// Animate particles
	for (auto& p : particles) {
		p.pos.x += p.velocity.x;
		p.pos.y += p.velocity.y;
		if (p.pos.y < 0 || p.pos.x < 0 || p.pos.x > screen_w) {
			p.pos = ImVec2(rand() % screen_w, screen_h + (rand() % 50));
			p.velocity = ImVec2((rand() % 10 - 5) * 0.15f, -(0.8f + (rand() % 5) * 0.15f));
			p.alpha = 0.4f + (rand() % 60) / 100.f;
			p.size = 1.8f + (rand() % 50) / 30.f;
		}
		draw_list->AddCircleFilled(p.pos, p.size, ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], p.alpha * alpha));
	}

	// Progress calculation
	float progress = (elapsed > init_duration) ? 1.f : (elapsed / init_duration);
	const float bar_width = screen_w * 0.6f;
	const float bar_height = 8.f;
	ImVec2 bar_pos(center.x - bar_width / 2.f, text_pos.y + 50.f);

	// Draw background of progress bar
	draw_list->AddRectFilled(bar_pos, ImVec2(bar_pos.x + bar_width, bar_pos.y + bar_height), ImColor(0.1f, 0.1f, 0.1f, alpha), 4.f);

	// Glowing layered effect for the progress fill
	const int glow_layers = 4;
	for (int i = glow_layers; i > 0; --i) {
		float layer_opacity = alpha * 0.2f / i;
		float layer_size = (bar_width * progress) * (1.f + 0.2f * i);
		draw_list->AddRectFilled(
			ImVec2(bar_pos.x, bar_pos.y),
			ImVec2(bar_pos.x + bar_width * progress, bar_pos.y + bar_height),
			ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], layer_opacity),
			4.f
		);
	}

	// Main fill
	ImVec2 fill_end = ImVec2(bar_pos.x + bar_width * progress, bar_pos.y + bar_height);
	draw_list->AddRectFilled(bar_pos, fill_end, ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], alpha), 4.f);
}

void features::visuals::low_fps_warning() {
	if (!g::local || !g::local->is_alive())
		return;

	float frame_time = interfaces::globals->frame_time;
	if (frame_time <= 0.0f)
		return;

	float fps = 1.0f / frame_time;
	if (fps >= 60.0f)
		return; // No warning needed

	// Get screen size
	int screen_w, screen_h;
	interfaces::engine->get_screen_size(screen_w, screen_h);
	ImDrawList* draw = ImGui::GetBackgroundDrawList();

	// Text segments
	const char* left = "! ";
	const char* mid = "LOW FPS";
	const char* right = " !";

	std::string full = std::string(left) + mid + right;
	ImVec2 full_size = ImGui::CalcTextSize(full.c_str());

	// Centered position
	ImVec2 center_pos = ImVec2(screen_w / 2.f - full_size.x / 2.f, screen_h / 2.f - full_size.y / 2.f);
	ImVec2 segment_pos = center_pos;

	// Pulsing alpha for red LOW FPS
	float pulse = 0.5f + 0.5f * std::sin(ImGui::GetTime() * 4.f); // 0..1
	float red_alpha = 0.5f + 0.5f * pulse;                        // 0.5..1

	ImFont* font = fonts::watermark_font;
	if (font)
		ImGui::PushFont(font);

	// Left white "!"
	draw->AddText(segment_pos, ImColor(1.f, 1.f, 1.f, 1.f), left);
	segment_pos.x += ImGui::CalcTextSize(left).x;

	// Red pulsing "LOW FPS"
	draw->AddText(segment_pos, ImColor(0.95f, 0.1f, 0.1f, red_alpha), mid);
	segment_pos.x += ImGui::CalcTextSize(mid).x;

	// Right white "!"
	draw->AddText(segment_pos, ImColor(1.f, 1.f, 1.f, 1.f), right);

	if (font)
		ImGui::PopFont();
}

bool once = false;
void features::visuals::removals() {
	if (!c::visuals::enable_removals || !interfaces::engine->is_in_game() || !g::local)
		return;

	static convar* shadows = interfaces::console->get_convar("cl_csm_enabled");
	static convar* sway = interfaces::console->get_convar("cl_wpn_sway_scale");
	static convar* postproccesing = interfaces::console->get_convar("mat_postprocess_enable");
	static convar* sky = interfaces::console->get_convar("r_3dsky");
	static convar* mat_postprocess_enable = interfaces::console->get_convar("mat_disable_bloom");
	static convar* blur = interfaces::console->get_convar("@panorama_disable_blur");
	static bool* disable_postprocessing = *reinterpret_cast<bool**>(find_pattern("client.dll", "83 EC 4C 80 3D") + 0x5);
	*disable_postprocessing = c::visuals::removals[1];

	if (c::misc::swayscale) {
		sway->set_value(0);
	}
	else {
		sway->set_value(1);
	}

	if (c::visuals::skybox || c::visuals::remove_3dsky) {
		sky->set_value(0);
	}
	else {
		sky->set_value(1);
	}
	if (c::visuals::remove_shadows) {
		shadows->set_value(0);
	}
	else {
		shadows->set_value(1);
	}
	if (c::visuals::remove_post_processing) {
		mat_postprocess_enable->set_value(0);
	}
	else {
		mat_postprocess_enable->set_value(1);
	}
	if (c::visuals::remove_panorama_blur && !once) {
		blur->set_value(0);
		once = true;
	}
	else if (once && !c::visuals::remove_panorama_blur) {
		blur->set_value(1);
		once = false;
	}
}

void features::visuals::fullbright() {
	static auto bright = interfaces::console->get_convar("mat_fullbright");
	bright->set_value(c::visuals::fullbright ? 1 : 0);
}

void features::visuals::flashalpha() {
	if (!interfaces::engine->is_connected() || !interfaces::engine->is_in_game() || !g::local)
		return;

	if (c::visuals::change_flashalpha) {

		g::local->flash_alpha() = 0;
	}
}

void features::visuals::nosmoke() {
	static auto linegoesthrusmoke = find_pattern("client.dll", "A3 ? ? ? ? 57 8B CB");
	static bool set = true;
	std::vector<const char*> vistasmoke_wireframe = {
		"particle/vistasmokev1/vistasmokev1_smokegrenade",
	};

	std::vector<const char*> vistasmoke_nodraw = {
		"particle/vistasmokev1/vistasmokev1_fire",
		"particle/vistasmokev1/vistasmokev1_emods",
		"particle/vistasmokev1/vistasmokev1_emods_impactdust",
	};

	if (!c::visuals::nosmoke) {
		if (set) {
			for (auto material_name : vistasmoke_wireframe) {
				i_material* smoke = interfaces::material_system->find_material(material_name, TEXTURE_GROUP_OTHER);
				smoke->set_material_var_flag(material_var_flags_t::material_var_wireframe, false);
			}
			for (auto material_name : vistasmoke_nodraw) {
				i_material* smoke = interfaces::material_system->find_material(material_name, TEXTURE_GROUP_OTHER);
				smoke->set_material_var_flag(material_var_flags_t::material_var_no_draw, false);
			}
			set = false;
		}
		return;
	}

	set = true;

	for (auto mat_s : vistasmoke_wireframe) {
		i_material* smoke = interfaces::material_system->find_material(mat_s, TEXTURE_GROUP_OTHER);
		smoke->set_material_var_flag(material_var_flags_t::material_var_wireframe, true);
	}

	for (auto mat_n : vistasmoke_nodraw) {
		i_material* smoke = interfaces::material_system->find_material(mat_n, TEXTURE_GROUP_OTHER);
		smoke->set_material_var_flag(material_var_flags_t::material_var_wireframe, true);
	}

	if (linegoesthrusmoke) {
		static auto smokecout = *(DWORD*)(linegoesthrusmoke + 0x1);
		if (smokecout)
			*(int*)(smokecout) = 0;
	}
}

std::tuple<float, float, float> draw_rainbow(float speed) {
	constexpr float pi = std::numbers::pi_v<float>;
	float r = std::sin(speed * interfaces::globals->realtime) * 0.5f + 0.5f;
	float g = std::sin(speed * interfaces::globals->realtime + 2 * pi / 3) * 0.5f + 0.5f;
	float b = std::sin(speed * interfaces::globals->realtime + 4 * pi / 3) * 0.5f + 0.5f;

	r = std::pow(r, 2.2f);
	g = std::pow(g, 2.2f);
	b = std::pow(b, 2.2f);

	return std::make_tuple(r, g, b);
}

void features::visuals::jump_trail() {
	if (!g::local || !interfaces::engine->is_in_game() || !interfaces::engine->is_connected() || !g::local->is_alive())
		return;

	if (c::visuals::trails) {
		const int move_type = g::local->move_type();

		vec3_t origin = g::local->origin();
		int vel = g::local->velocity().length_2d();

		const auto [r, g, b] { draw_rainbow(4.f) };
		if (!(g::local->flags() & fl_onground) && tick == 0) {

			color_t rainbow_col = color_t(r * 255, g * 255, b * 255);

			BeamInfo_t beam_info;
			beam_info.m_nType = 0;
			beam_info.m_pszModelName = "sprites/physbeam.vmt";
			beam_info.m_nModelIndex = -1;
			beam_info.m_flHaloScale = 0.0;
			beam_info.m_flLife = 2.5f;
			beam_info.m_flWidth = 5;
			beam_info.m_flEndWidth = 5;
			beam_info.m_flFadeLength = 0.0;
			beam_info.m_flAmplitude = 2.0;
			beam_info.m_flBrightness = 255.f;
			beam_info.m_flSpeed = 0.5;
			beam_info.m_nStartFrame = 0.;
			beam_info.m_flFrameRate = 0.;
			beam_info.m_flRed = (float)rainbow_col.r();
			beam_info.m_flGreen = (float)rainbow_col.g();
			beam_info.m_flBlue = (float)rainbow_col.b();
			beam_info.m_nSegments = 2;
			beam_info.m_bRenderable = true;
			beam_info.m_nFlags = 0;
			beam_info.m_vecStart = origin_old;
			beam_info.m_vecEnd = origin;

			Beam_t* myBeam = interfaces::render_beams->create_beam_point(beam_info);
			if (myBeam && !(move_type == movetype_ladder || move_type == movetype_noclip || move_type == movetype_observer)) {
				interfaces::render_beams->draw_beam(myBeam);
			}

			velocity_old = vel;
		}
		if (tick == 0) {
			origin_old = origin;
			tick = igonre_ticks + 1;
		}
		tick = tick - 1;
	}
}

void features::visuals::dlights(player_t* entity) {
	if (!c::visuals::dlight || !interfaces::engine->is_in_game() || !interfaces::engine->is_connected())
		return;

	if (interfaces::engine->is_in_game() && interfaces::engine->is_connected()) {
		vec3_t getorig = entity->origin();
		vec3_t  getheadorig = entity->get_eye_pos();

		if (entity->is_enemy() && !entity->dormant()) {

			dlight_t* elight = interfaces::effects->cl_alloc_elight(entity->index());
			elight->color.r = float(c::visuals::dlight_clr[0] * 255.f);
			elight->color.g = float(c::visuals::dlight_clr[1] * 255.f);;
			elight->color.b = float(c::visuals::dlight_clr[2] * 255.f);;
			elight->color.exponent = 8.f;
			elight->direction = getheadorig;
			elight->origin = getheadorig;
			elight->radius = 200.0f;
			elight->die_time = interfaces::globals->cur_time + 0.1f;
			elight->decay = 50.0f;
			elight->key = entity->index();

			dlight_t* dlight = interfaces::effects->cl_alloc_dlight(entity->index());
			dlight->color.r = float(c::visuals::dlight_clr[0] * 255.f);
			dlight->color.g = float(c::visuals::dlight_clr[1] * 255.f);;
			dlight->color.b = float(c::visuals::dlight_clr[2] * 255.f);;
			dlight->color.exponent = 8.f;
			dlight->direction = getorig;
			dlight->origin = getorig;
			dlight->radius = 100.f;
			dlight->die_time = interfaces::globals->cur_time + 0.1f;
			dlight->decay = dlight->radius / 2.f;
			dlight->key = entity->index();
		}
	}
}

static int buttons = 0;
void features::visuals::run_freecam(c_usercmd* cmd, vec3_t angles) {
	static vec3_t currentviewangles = vec3_t{};
	static vec3_t realviewangles = vec3_t{};
	static bool wascrouching = false;
	static bool washoldingattack = false;
	static bool washoldinguse = false;
	static bool hassetangles = false;
	buttons = cmd->buttons;

	if (!c::misc::freecam || !menu::checkkey(c::misc::freecam_key, c::misc::freecam_key_s)) {
		if (hassetangles) {
			interfaces::engine->set_view_angles(realviewangles);
			cmd->view_angles = currentviewangles;
			if (wascrouching)
				cmd->buttons |= in_duck;
			if (washoldingattack)
				cmd->buttons |= in_attack;
			if (washoldinguse)
				cmd->buttons |= in_use;
			wascrouching = false;
			washoldingattack = false;
			washoldinguse = false;
			hassetangles = false;
		}
		currentviewangles = vec3_t{};
		return;
	}

	if (!g::local || !g::local->is_alive())
		return;

	if (currentviewangles.null()) {
		currentviewangles = cmd->view_angles;
		realviewangles = angles;
		wascrouching = cmd->buttons & in_duck;
	}

	cmd->forward_move = 0;
	cmd->side_move = 0;
	cmd->buttons = 0;

	if (wascrouching)
		cmd->buttons |= in_duck;

	if (washoldingattack)
		cmd->buttons |= in_attack;

	if (washoldinguse)
		cmd->buttons |= in_use;

	cmd->view_angles = currentviewangles;
	hassetangles = true;
}

void features::visuals::freecam(view_setup_t* setup) {
	static vec3_t origin = vec3_t{ };

	if (!c::misc::freecam || !GetAsyncKeyState(c::misc::freecam_key)) {
		origin = vec3_t{ };
		return;
	}

	if (!g::local || !g::local->is_alive())
		return;

	float cam_speed = fabsf(static_cast<float>(2)); // cfg later 

	if (origin.null())
		origin = setup->origin;

	vec3_t forward{ }, right{ }, up{ };

	math::angle_vectors_alternative(setup->view, &forward, &right, &up);

	const bool inback = buttons & in_back;
	const bool inforward = buttons & in_forward;
	const bool rightBtn = buttons & in_moveright;
	const bool inleft = buttons & in_moveleft;
	const bool inshift = buttons & in_speed;
	const bool induck = buttons & in_duck;
	const bool injump = buttons & in_jump;

	if (induck)
		cam_speed *= 0.45f;

	if (inshift)
		cam_speed *= 1.65f;

	if (inforward)
		origin += forward * cam_speed;

	if (rightBtn)
		origin += right * cam_speed;

	if (inleft)
		origin -= right * cam_speed;

	if (inback)
		origin -= forward * cam_speed;

	if (injump)
		origin += up * cam_speed;

	setup->origin = origin;
}

void features::visuals::key_strokes() {
	if (!c::movement::key_strokes)
		return;

	if (!g::local || !g::local->is_alive())
		return;

	if (!interfaces::engine->is_in_game() || !interfaces::engine->is_connected())
		return;

	int w, h;
	interfaces::engine->get_screen_size(w, h);
	c_usercmd* cmd = g::cmd;

	color_t clr;

	if (cmd->buttons & in_moveleft && cmd->buttons & in_moveright)
		clr = color_t(0.6f, 0.2f, 0.2f);
	else
		clr = color_t(1.f, 1.f, 1.f);

	im_render.text(w / 2 + 13, c::movement::key_strokes_position, 12, fonts::key_strokes_font, "_", true, color_t(1.f, 1.f, 1.f), true);
	im_render.text(w / 2 - 14, c::movement::key_strokes_position, 12, fonts::key_strokes_font, "_", true, color_t(1.f, 1.f, 1.f), true);
	im_render.text(w / 2, c::movement::key_strokes_position, 12, fonts::key_strokes_font, "_", true, color_t(1.f, 1.f, 1.f), true);
	im_render.text(w / 2 - 14, c::movement::key_strokes_position + 14, 12, fonts::key_strokes_font, "_", true, color_t(1.f, 1.f, 1.f), true);
	im_render.text(w / 2, c::movement::key_strokes_position + 13, 12, fonts::key_strokes_font, "_", true, color_t(1.f, 1.f, 1.f), true);
	im_render.text(w / 2 + 13, c::movement::key_strokes_position + 13, 12, fonts::key_strokes_font, "_", true, color_t(1.f, 1.f, 1.f), true);

	if (cmd->buttons & in_jump)
		im_render.text(w / 2 + 13, c::movement::key_strokes_position, 12, fonts::key_strokes_font, "J", true, color_t(1.f, 1.f, 1.f), true);

	if (cmd->buttons & in_duck)
		im_render.text(w / 2 - 14, c::movement::key_strokes_position, 12, fonts::key_strokes_font, "C", true, color_t(1.f, 1.f, 1.f), true);

	if (cmd->buttons & in_forward)
		im_render.text(w / 2, c::movement::key_strokes_position, 12, fonts::key_strokes_font, "W", true, color_t(1.f, 1.f, 1.f), true);

	if (cmd->buttons & in_moveleft)
		im_render.text(w / 2 - 14, c::movement::key_strokes_position + 14, 12, fonts::key_strokes_font, "A", true, color_t(1.f, 1.f, 1.f), true);

	if (cmd->buttons & in_back)
		im_render.text(w / 2, c::movement::key_strokes_position + 13, 12, fonts::key_strokes_font, "S", true, color_t(1.f, 1.f, 1.f), true);

	if (cmd->buttons & in_moveright)
		im_render.text(w / 2 + 13, c::movement::key_strokes_position + 13, 12, fonts::key_strokes_font, "D", true, color_t(1.f, 1.f, 1.f), true);

	if (cmd->mouse_dx < 0.f)
		im_render.text(w / 2 - 14, c::movement::key_strokes_position + 28, 12, fonts::key_strokes_font, "<", true, color_t(1.f, 1.f, 1.f), true);

	if (cmd->mouse_dx > 0.f)
		im_render.text(w / 2 + 13, c::movement::key_strokes_position + 28, 12, fonts::key_strokes_font, ">", true, color_t(1.f, 1.f, 1.f), true);
}

void features::visuals::console() {
	static i_material* material[5];
	static float time = 0.f;

	// Define two colors for interpolation
	const float color1[4] = { c::misc::custom_console_clr[0], c::misc::custom_console_clr[1], c::misc::custom_console_clr[2], c::misc::custom_console_clr[3] }; // Example: Red
	const float color2[4] = { c::misc::custom_console_clr2[0], c::misc::custom_console_clr2[1], c::misc::custom_console_clr2[2], c::misc::custom_console_clr2[3] }; // Example: Blue

	// Increment time for interpolation
	time += interfaces::globals->frame_time * 0.5f; // Adjust speed
	if (time > 1.0f)
		time = 0.0f; // Reset time to loop effect

	// Compute interpolated color
	float interp_color[4];
	for (int i = 0; i < 4; i++)
		interp_color[i] = (1.0f - time) * color1[i] + time * color2[i];

	// Initialize materials if not already set
	if (!material[0] || !material[1] || !material[2] || !material[3] || !material[4]) {
		for (material_handle_t h = interfaces::material_system->first_material();
			h != interfaces::material_system->invalid_material_handle();
			h = interfaces::material_system->next_material(h)) {

			const auto mat = interfaces::material_system->get_material(h);
			if (!mat)
				continue;

			if (strstr(mat->get_name(), "vgui_white"))
				material[0] = mat;
			else if (strstr(mat->get_name(), "800corner1"))
				material[1] = mat;
			else if (strstr(mat->get_name(), "800corner2"))
				material[2] = mat;
			else if (strstr(mat->get_name(), "800corner3"))
				material[3] = mat;
			else if (strstr(mat->get_name(), "800corner4"))
				material[4] = mat;
		}
	}
	else {
		for (unsigned int num = 0; num < 5; num++) {
			if (!c::misc::custom_console || !interfaces::engine->is_console_visible()) {
				material[num]->color_modulate(1.f, 1.f, 1.f);
				material[num]->alpha_modulate(1.f);
				continue;
			}

			// Apply interpolated gradient color
			material[num]->color_modulate(interp_color[0], interp_color[1], interp_color[2]);
			material[num]->alpha_modulate(interp_color[3]);
		}
	}
}

float calculate_sun_position(float base_angle, const bool vertical_axis) {
	const float time = interfaces::globals ? interfaces::globals->realtime : 0.f;
	const float speed = std::clamp(c::visuals::custom_sun_speed, 0.0f, 360.0f);
	const float orbit_range = std::clamp(c::visuals::custom_sun_orbit_range, 0.0f, 180.0f);
	float animated_angle = base_angle;

	switch (std::clamp(c::visuals::custom_sun_mode, 0, 3)) {
	case 1: // slow drift
		animated_angle += time * speed;
		break;
	case 2: // orbit
		animated_angle += vertical_axis
			? sinf(time * speed * 0.017453292f) * orbit_range
			: time * speed;
		break;
	case 3: // cold moon lock: tiny, deliberate shimmer rather than runaway motion
		animated_angle += sinf(time * speed * 0.017453292f) * 4.0f;
		break;
	default:
		break;
	}

	animated_angle = std::fmod(animated_angle, 360.f);
	if (animated_angle < 0.f)
		animated_angle += 360.f;
	return animated_angle;
}

void features::visuals::custom_sun() {
	static bool missing_custom_sun_console_logged = false;
	if (!interfaces::console) {
		if (!missing_custom_sun_console_logged) {
			const auto& preset = menu::world_preset_profile(menu::normalize_world_preset(c::visuals::world_preset_index));
			debug::log("World visuals sun [%s]: console interface missing; custom sun skipped safely.", preset.internal_id);
			missing_custom_sun_console_logged = true;
		}
		return;
	}

	auto cl_csm_rot_override = interfaces::console->get_convar(xs("cl_csm_rot_override"));
	auto cl_csm_max_shadow_dist = interfaces::console->get_convar(xs("cl_csm_max_shadow_dist"));
	auto cl_csm_rot_x = interfaces::console->get_convar(xs("cl_csm_rot_x"));
	auto cl_csm_rot_y = interfaces::console->get_convar(xs("cl_csm_rot_y"));
	static bool missing_custom_sun_convars_logged = false;

	if (!cl_csm_rot_override || !cl_csm_max_shadow_dist || !cl_csm_rot_x || !cl_csm_rot_y) {
		if (!missing_custom_sun_convars_logged) {
			const auto& preset = menu::world_preset_profile(menu::normalize_world_preset(c::visuals::world_preset_index));
			debug::log("World visuals sun [%s]: one or more shadow/sun convars are missing; custom sun skipped safely.", preset.internal_id);
			missing_custom_sun_convars_logged = true;
		}
		return;
	}

	if (!c::visuals::custom_sun) {
		cl_csm_rot_override->set_value(0);
		return;
	}

	const int shadow_dist = std::clamp(c::visuals::custom_sun_dist, 1000, 50000);
	cl_csm_max_shadow_dist->set_value(shadow_dist);
	cl_csm_rot_override->set_value(1);
	cl_csm_rot_x->set_value(calculate_sun_position(static_cast<float>(c::visuals::custom_sun_x), true));
	cl_csm_rot_y->set_value(calculate_sun_position(static_cast<float>(c::visuals::custom_sun_y), false));
}

