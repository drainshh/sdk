#include "../hooks.hpp"
#include "../../menu/menu.hpp"
#include "../../features/misc/misc.hpp"
#include "../../features/visuals/visuals.hpp"
#include "../../includes/discord/discord_rpc.h"
#include "../../sdk/math/math.hpp"
#include <TlHelp32.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx9.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "../../features/movement/movement.hpp"
#include "../../features/movement/moveutil.h"

LRESULT __stdcall sdk::hooks::wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	if (message == WM_KEYDOWN && LOWORD(wparam) == menu::menu_key)
		menu::open = !menu::open;

	LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ImGui_ImplWin32_WndProcHandler(window, message, wparam, lparam);

	return CallWindowProcW(wndproc_original, hwnd, message, wparam, lparam);
}

long __stdcall sdk::hooks::present::present(IDirect3DDevice9* device, RECT* source_rect, RECT* dest_rect, HWND dest_window_override, RGNDATA* dirty_region) {
	gui::device = device;

	static auto once = [=]() {
		if (!gui::setup) {
			gui::SetupMenu(device);
			gui::setup = true;
			return ofunc(device, source_rect, dest_rect, dest_window_override, dirty_region);
			}
		}();

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		{
			const auto im_draw = ImGui::GetBackgroundDrawList();
			im_render.render_present(im_draw);
			features::visuals::init();
			features::visuals::watermark();
			features::visuals::clarity_watermark();
			features::visuals::RenderMediaPlayer();
			features::visuals::points_system();
			features::weather::render_overlay_precipitation();
			features::movement::assist_endscene();
			features::misc::spectators_list();
			features::visuals::removals();
			features::visuals::fullbright();
			gui::render_checkpoints();
			RouteStudio.notify();
			if (gui::open) {
				gui::Render();
			}
		}
		ImGui::EndFrame();
		ImGui::Render();

		if (device->BeginScene() == D3D_OK) {
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
			device->EndScene();
		}
		return ofunc(device, source_rect, dest_rect, dest_window_override, dirty_region);
}

LRESULT __stdcall sdk::hooks::reset::reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pp) {
	features::visuals::points_mascot_invalidate_texture();
	menu::note_theme_device_reset();
	ImGui_ImplDX9_InvalidateDeviceObjects();

	auto result = ofunc(device, pp);

	ImGui_ImplDX9_CreateDeviceObjects();

	return result;
}
