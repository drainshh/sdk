#include "draw.hpp"
#include "../../menu/fonts/icon.h"
#include "../../hooks/hooks.hpp"
#include "../../sdk/sdk.hpp"

void imgui_render::setup(IDirect3DDevice9* device) {
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(sdk::hooks::window);
	ImGui_ImplDX9_Init(device);
	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.Alpha = 1.0f;
	style.WindowPadding = ImVec2(8, 8);
	style.WindowRounding = 4.0f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(32, 32);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.ChildRounding = 4.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 4.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(4, 2);
	style.FrameRounding = 2.0f;
	style.FrameBorderSize = 1.0f;
	style.ItemSpacing = ImVec2(8, 4);
	style.ItemInnerSpacing = ImVec2(4, 4);
	style.IndentSpacing = 6.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 6.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 0.0f;
	style.GrabRounding = 4.0f;
	style.TabRounding = 4.0f;
	style.TabBorderSize = 1.0f;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.5f);
	style.MouseCursorScale = 0.75f;

	style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);                 // Black
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.30f, 0.30f, 0.30f, 0.85f);         // Dark gray
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 0.10f, 0.85f);       // Dark blue

	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);             // Darker background
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.60f);              // Dark background
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.f);              // Dark background

	style.Colors[ImGuiCol_Border] = ImVec4(0.10f, 0.10f, 0.10f, 0.7f);               // Dark gray border
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);         // Transparent

	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);              // Darker background
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);       // Dark blue

	//style.Colors[ImGuiCol_ControlBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);            // Darker background
	//style.Colors[ImGuiCol_ControlBgHovered] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);     // Dark gray
	//style.Colors[ImGuiCol_ControlBgActive] = ImVec4(0.60f, 0.60f, 0.60f, 0.10f);     // Light gray

	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);              // Darker background for title
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.10f, 1.f);        // Dark blue background for active title
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 1.f);     // Dark blue background for collapsed title

	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.70f);             // Dark background for menu bar

	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.30f);           // Darker background for scrollbar
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.30f, 0.90f); // Dark gray for scrollbar grab when hovered
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.60f, 0.60f, 0.60f, 0.10f); // Light gray for scrollbar grab when active

	style.Colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);                // Darker background for buttons
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);         // Dark gray for buttons when hovered

	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);         // Dark gray for header when hovered

	style.Colors[ImGuiCol_Separator] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);             // Dark gray separator
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);      // Dark gray separator when hovered

	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.10f, 0.10f, 0.10f, 0.70f);      // Dark gray resize grip when hovered

	style.Colors[ImGuiCol_Tab] = ImVec4(0.05f, 0.05f, 0.05f, 0.80f);                   // Darker background for tabs
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.10f, 0.10f, 0.10f, 0.80f);            // Dark gray for tabs when hovered
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.10f, 0.10f, 0.10f, 0.70f);          // Dark gray for unfocused tabs

	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.50f);       // Darker background for modal windows

	style.Colors[ImGuiCol_CheckMark] = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2]);              // checkmark color

	style.Colors[ImGuiCol_SliderGrab] = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2]);             // color for slider grab
	style.Colors[ImGuiCol_SliderGrabActive] = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2]);       // color for slider grab when active

	style.Colors[ImGuiCol_ButtonActive] = ImColor(0.10f, 0.10f, 0.10f, 1.00f);           // color for buttons when active

	style.Colors[ImGuiCol_Header] = ImColor(0.10f, 0.10f, 0.10f, 1.00f);                // color for header
	style.Colors[ImGuiCol_HeaderActive] = ImColor(0.10f, 0.10f, 0.10f, 1.00f);          // color for header when active

	style.Colors[ImGuiCol_SeparatorActive] = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2]);       // separator when active

	style.Colors[ImGuiCol_ResizeGrip] = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2]);             // resize grip color
	style.Colors[ImGuiCol_ResizeGripActive] = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2]);       // resize grip color when active

	style.Colors[ImGuiCol_TabActive] = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2]);            // for active tab
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2]);   // for unfocused active tab

	style.Colors[ImGuiCol_PlotLines] = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2]);              // for plot lines
	style.Colors[ImGuiCol_PlotLinesHovered] = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2]);       // for plot lines when hovered
	style.Colors[ImGuiCol_PlotHistogram] = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2]);          // for plot histogram
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2]);   // for plot histogram when hovered

	style.Colors[ImGuiCol_DragDropTarget] = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2]);         // for drag and drop target

	style.Colors[ImGuiCol_ScrollbarGrab] = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2]);         // Accent color for scrollbar grab

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

	ImFontConfig cfg_menu;
	cfg_menu.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_MonoHinting | ImGuiFreeTypeBuilderFlags_Monochrome;

	ImFontConfig cfg_screen;
	cfg_screen.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_MonoHinting | ImGuiFreeTypeBuilderFlags_Monochrome;
}

void imgui_render::drawline(const float x, const float y, const float x2, const float y2, const color_t& color, const float thickness) {
    std::unique_lock<std::shared_mutex> lock{ m_mutex };
	m_draw_data.emplace_back(std::make_unique<drawing::Line>(ImVec2{ x, y }, ImVec2{ x2, y2 }, color_t::U32(color), thickness));
}

void imgui_render::text(const float x, const float y, const float fontSize, ImFont* font, const std::string& text, const bool centered, const color_t& color, const bool dropShadow, const bool outline) {
    std::unique_lock<std::shared_mutex> lock{ m_mutex };
	m_draw_data.emplace_back(std::make_unique<drawing::TextSize>(fontSize, font, ImVec2{ x, y }, color_t::U32(color), text, dropShadow, centered, outline));
}

void imgui_render::text_glow(int x, int y, int size, ImFont* font, const char* text,
	bool centered, const color_t& glow_color, int flags) {
	const int layers = 6;          // more layers for smoother fade
	const float max_radius = 1.5f; // how far glow spreads
	const float alpha_mult = glow_color.a() / 255.0f;

	// Draw glow layers
	for (int i = 1; i <= layers; i++) {
		float t = static_cast<float>(i) / static_cast<float>(layers); // 0..1
		float layer_alpha = alpha_mult * (1.0f - t);                  // fade out layers
		float radius = max_radius * t;                                // spread increases with layer

		// sample around circle
		const int samples = 12; // number of points around circle
		for (int s = 0; s < samples; s++) {
			float angle = 2.0f * 3.14159265f * (s / static_cast<float>(samples));
			int dx = static_cast<int>(cosf(angle) * radius + 0.5f);
			int dy = static_cast<int>(sinf(angle) * radius + 0.5f);

			if (dx == 0 && dy == 0) continue; // skip center

			color_t layer_color(glow_color.r(), glow_color.g(), glow_color.b(), static_cast<float>(layer_alpha * 255));
			im_render.text(x + dx, y + dy, size, font, text, centered,
				layer_color.get_color_edit_alpha_int(static_cast<int>(layer_alpha * 255)), flags);
		}
	}

	// Draw main text at center (use full alpha)
	im_render.text(x, y, size, font, text, centered,
		glow_color.get_color_edit_alpha_int(glow_color.aMultiplied()), flags);
}


void imgui_render::text_shadow_glow(int x, int y, int size, ImFont* font, const char* text,
	bool centered, const color_t& text_color, const color_t& shadow_color, bool is_detected, int flags) {

	// Shadow parameters
	const int shadow_offset = 1;

	if (is_detected) {
		// White glow effect when detected
		const int layers = 3;
		const float step = 1.5f;

		for (int i = 1; i <= layers; i++) {
			float alpha_factor = powf(1.0f - (float)i / (float)layers, 2.0f);
			float glow_alpha = shadow_color.a() * alpha_factor;
			color_t layer_color(shadow_color.r(), shadow_color.g(), shadow_color.b(), glow_alpha);
			int offset = static_cast<int>(i * step);

			// 8-way glow
			for (int dx = -1; dx <= 1; dx++) {
				for (int dy = -1; dy <= 1; dy++) {
					if (dx == 0 && dy == 0) continue;
					im_render.text(x + dx * offset, y + dy * offset, size, font, text, centered,
						layer_color.get_color_edit_alpha_int(static_cast<int>(glow_alpha * 255)), flags);
				}
			}
		}
	}
	else {
		// Simple black shadow when not detected
		color_t black_shadow(0.f, 0.f, 0.f, shadow_color.a());
		im_render.text(x + shadow_offset, y + shadow_offset, size, font, text, centered,
			black_shadow.get_color_edit_alpha_int(black_shadow.aMultiplied()), flags);
	}

	// Main text
	im_render.text(x, y, size, font, text, centered,
		text_color.get_color_edit_alpha_int(text_color.aMultiplied()), flags);
}

void imgui_render::drawrectfilled(const float x, const float y, const float w, const float h, const color_t& color, const ImDrawFlags flags) {
    std::unique_lock<std::shared_mutex> lock{ m_mutex };
	m_draw_data.emplace_back(std::make_unique<drawing::RectangleFilled>(ImVec2{ x, y }, ImVec2{ x + w, y + h }, color_t::U32(color), 0.0f, flags));
}

void imgui_render::drawrectfilledgradient(const float x, const float y, const float w, const float h, const color_t& colUprLeft, const color_t& colUprRight, const color_t& colBotRight, const color_t& colBotLeft) {
    std::unique_lock<std::shared_mutex> lock{ m_mutex };
	m_draw_data.emplace_back(std::make_unique<drawing::RectangleMultiColor>(ImVec2{ x, y }, ImVec2{ x + w, y + h },
		color_t::U32(colUprLeft), color_t::U32(colUprRight), color_t::U32(colBotRight), color_t::U32(colBotLeft)));
}

void imgui_render::drawrect(const float x, const float y, const float w, const float h, const color_t& color, const ImDrawFlags flags, const float thickness) {
    std::unique_lock<std::shared_mutex> lock{ m_mutex };
	m_draw_data.emplace_back(std::make_unique<drawing::Rectangle>(ImVec2{ x, y }, ImVec2{ x + w, y + h }, color_t::U32(color), 0.0f, flags, thickness));
}

void imgui_render::drawcornerbox(const float x, const float y, const float w, const float h, const color_t& colour) {
	im_render.drawline(x, y, x, y + (h / c::visuals::corner_lenght), color_t(colour));
	im_render.drawline(x, y, x + (w / c::visuals::corner_lenght), y, color_t(colour));
	im_render.drawline(x + w, y, x + w - (w / c::visuals::corner_lenght), y, color_t(colour));
	im_render.drawline(x + w, y, x + w, y + (h / c::visuals::corner_lenght), color_t(colour));
	im_render.drawline(x, y + h, x + (w / c::visuals::corner_lenght), y + h, color_t(colour));
	im_render.drawline(x, y + h, x, y + h - (h / c::visuals::corner_lenght), color_t(colour));
	im_render.drawline(x + w, y + h, x + w - (w / c::visuals::corner_lenght), y + h, color_t(colour));
	im_render.drawline(x + w, y + h, x + w, y + h - (h / c::visuals::corner_lenght), color_t(colour));
}

void imgui_render::drawcircle(const float x, const float y, const float radius, const int points, const color_t& color, const float thickness) {
    std::unique_lock<std::shared_mutex> lock{ m_mutex };
	m_draw_data.emplace_back(std::make_unique<drawing::Circle>(ImVec2{ x, y }, radius, points, color_t::U32(color), thickness));
}

void imgui_render::drawtrianglefilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const color_t& color) {
    std::unique_lock<std::shared_mutex> lock{ m_mutex };
	m_draw_data.emplace_back(std::make_unique<drawing::TriangleFilled>(p1, p2, p3, color_t::U32(color)));
}

void imgui_render::drawpolygon(const std::vector<ImVec2>& verts, const color_t& color) {
    std::unique_lock<std::shared_mutex> lock{ m_mutex };
	m_draw_data.emplace_back(std::make_unique<drawing::Polygon>(verts, color_t::U32(color)));
}

void imgui_render::drawpolyline(const std::vector<ImVec2>& verts, const color_t& color, const ImDrawFlags flags, const float thickness) {
    std::unique_lock<std::shared_mutex> lock{ m_mutex };
	m_draw_data.emplace_back(std::make_unique<drawing::Polyline>(verts, color_t::U32(color), flags, thickness));
}

int imgui_render::get_text_size(std::string_view text, ImFont* font, float wrap_width, float font_size) {
	const auto font_ptr = font;

	if (font_size < 0.0f)
		font_size = font_ptr->FontSize;

	const auto text_size = font_ptr->CalcTextSizeA(font_size, 10.0e+10f, wrap_width, text.data());

	return { static_cast<int>(text_size.x) };
}

ImVec2 imgui_render::measure_text(std::string_view text, ImFont* font, float font_size) {
	const auto font_ptr = font;

	if (font_size < 0.0f)
		font_size = font_ptr->FontSize;

	const auto text_size = font_ptr->CalcTextSizeA(font_size, 10.0e+10f, 0.0f, text.data());

	return { (float)text_size.x, (float)text_size.y };
	return { (float)text_size.x, (float)text_size.y };
}

void imgui_render::circle_filled_3d(const vec3_t& origin, float radius, const color_t& color) {
	std::vector< vec3_t > Points3D;
	float step = static_cast<float>(std::numbers::pi) * 2.0f / 256;

	for (float a = 0; a < (std::numbers::pi * 2.0f); a += step) {
		vec3_t start(radius * cosf(a) + origin.x, radius * sinf(a) + origin.y, origin.z);
		vec3_t end(radius * cosf(a + step) + origin.x, radius * sinf(a + step) + origin.y, origin.z);

		ImVec2 out, out1, pos3d;

		if (world_to_screen(end, &out1) && world_to_screen(start, &out)) {
			if (world_to_screen(origin, &pos3d)) {
				im_render.drawtrianglefilled(out, out1, pos3d, color);
			}
		}
	}
}

bool imgui_render::world_to_screen(const vec3_t& origin, ImVec2* screen) {
	const view_matrix_t& world_to_screen_matrix = interfaces::engine->world_to_screen_matrix();
	const float flWidth = world_to_screen_matrix[3][0] * origin.x + world_to_screen_matrix[3][1] * origin.y + world_to_screen_matrix[3][2] * origin.z + world_to_screen_matrix[3][3];

	// check is point can't fit on screen, because it's behind us
	if (flWidth < 0.001f)
		return false;

	// compute the scene coordinates of a point in 3D
	const float flInverse = 1.0f / flWidth;
	screen->x = (world_to_screen_matrix[0][0] * origin.x + world_to_screen_matrix[0][1] * origin.y + world_to_screen_matrix[0][2] * origin.z + world_to_screen_matrix[0][3]) * flInverse;
	screen->y = (world_to_screen_matrix[1][0] * origin.x + world_to_screen_matrix[1][1] * origin.y + world_to_screen_matrix[1][2] * origin.z + world_to_screen_matrix[1][3]) * flInverse;

	// screen transform
	// get the screen position in pixels of given point
	const ImVec2 vecDisplaySize = ImGui::GetIO().DisplaySize;
	screen->x = (vecDisplaySize.x * 0.5f) + (screen->x * vecDisplaySize.x) * 0.5f;
	screen->y = (vecDisplaySize.y * 0.5f) - (screen->y * vecDisplaySize.y) * 0.5f;
	return true;
}


void imgui_render::render_present(ImDrawList* draw) {
    std::shared_lock<std::shared_mutex> lock{ m_mutex };

	if (m_draw_data_safe.empty())
		return;

	for (const auto& data : m_draw_data_safe)
		data->draw(draw);
}

void imgui_render::clear_data() {
    std::unique_lock<std::shared_mutex> lock{ m_mutex };
	if (!m_draw_data.empty()) {
		m_draw_data.clear();
	}
}

void imgui_render::swawp_data() {
	std::unique_lock<std::shared_mutex> lock{ m_mutex };
	m_draw_data.swap(m_draw_data_safe);
}

void imgui_render::add_to_render(const std::function<void()>& fun) {
	// Only clear existing draw data if there is any — this avoids unnecessary work.
	if (!m_draw_data.empty())
		clear_data();

	// Populate m_draw_data via provided fun (calls draw helpers that push into m_draw_data)
	fun();

	// Swap into safe deque for rendering under lock
	swawp_data();
}

void imgui_render::unload() {
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void fonts::reset_fonts( ) {
	// reset font variables to default values
	font_directory_watermark = "C:/windows/fonts/tahoma.ttf";
	font_directory_menu_main = "C:/windows/fonts/tahomabd.ttf";
	font_directory_indicator = "C:/windows/fonts/verdanab.ttf";
	font_directory_sub_indicator = "C:/windows/fonts/verdanab.ttf";
	font_directory_misc = "C:/windows/fonts/tahomabd.ttf";
	font_directory_logs = "C:/windows/fonts/tahoma.ttf";
	font_directory_spec = "C:/windows/fonts/tahomabd.ttf";
	font_directory_sub_spec = "C:/windows/fonts/tahoma.ttf";
	font_directory_health = "C:/windows/fonts/tahoma.ttf";
	font_directory_name = "C:/windows/fonts/tahoma.ttf";
    font_directory_wpn = "C:/windows/fonts/tahoma.ttf";
	font_directory_dropped_wpn = "C:/windows/fonts/tahoma.ttf";

	// reset other font-related variables to default values
	c::fonts::indi_font = 0;
	c::fonts::sub_indi_font = 0;
	c::fonts::spec_font = 0;
	c::fonts::sub_spec_font = 0;
	c::fonts::sc_logs_font = 0;
	c::fonts::esp_name_font = 0;
	c::fonts::esp_hp_font = 0;
	c::fonts::esp_wpn_font = 0;
	c::fonts::esp_dropped_wpn_font = 0;
	c::fonts::watermark_font = 0;

	//reset size
	c::fonts::indi_size = 28;
	c::fonts::sub_indi_size = 28;
	c::fonts::spec_size = 12;
	c::fonts::esp_name_size = 12;
	c::fonts::esp_hp_size = 12;
	c::fonts::esp_wpn_size = 12;
	c::fonts::esp_dropped_wpn_size = 12;
}

void imgui_render::clear_textures() {

}

void imgui_render::clear_unused_avatars() {

}

//

void ctexture::init(int width, int height, const std::uint8_t* data)  {
	//texture = ImGui_CreateTextureRGBA(width, height, data);
}

void ctexture::clear() {
	//if (texture)
		//ImGui_DestroyTexture(texture);
	//texture = nullptr;
}
