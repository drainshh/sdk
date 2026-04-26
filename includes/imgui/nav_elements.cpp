#include "nav_elements.h"
#include "../../utils/render/draw.hpp"

struct tab_element {
    float element_opacity, rect_opacity, text_opacity;
};

struct subtab_element {
    float element_opacity, rect_opacity, text_opacity;
    int call_order;
};

bool elements::tab(const char* icon, const char* label, bool active, int tab_index)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    // Sizes
    const float tab_height = 31.0f;
    const float icon_size = 18.0f;
    const float spacing = 10.0f;

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 label_size = ImGui::CalcTextSize(label);
    ImVec2 rect_size = ImVec2(160, tab_height);

    ImRect rect(pos, pos + rect_size);

    ImGui::ItemSize(rect, style.FramePadding.y);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);

    // Handle expansion/collapse
    if (pressed) {
        if (expanded_tab_id == tab_index) {
            expanded_tab_id = -1; // Collapse if clicking same tab
        }
        else {
            expanded_tab_id = tab_index; // Expand this tab
        }
    }

    bool is_expanded = (expanded_tab_id == tab_index);

    // Animation data
    static std::map<ImGuiID, tab_element> anim;
    auto& a = anim.try_emplace(id, tab_element{ 0.f, 0.f, 0.f }).first->second;

    a.element_opacity = ImLerp(a.element_opacity, (is_expanded ? 0.08f : hovered ? 0.04f : 0.0f), 0.15f * (1.0f - g.IO.DeltaTime));
    a.rect_opacity = ImLerp(a.rect_opacity, (is_expanded ? 1.0f : 0.0f), 0.20f * (1.0f - g.IO.DeltaTime));
    a.text_opacity = ImLerp(a.text_opacity, (is_expanded ? 1.0f : hovered ? 0.7f : 0.5f), 0.10f * (1.0f - g.IO.DeltaTime));

    if (a.element_opacity > 0.0f)
        window->DrawList->AddRectFilled(
            rect.Min, rect.Max,
            ImColor(242, 242, 242, int(50 * a.element_opacity)),
            4.0f
        );

    // Icon
    ImVec2 icon_pos = ImVec2(rect.Min.x + 10, rect.Min.y + (tab_height - icon_size) * 0.5f);
    window->DrawList->AddText(fonts::dna_font, icon_size, icon_pos, ImColor(0.4f, 0.4f, 0.4f, a.text_opacity), icon);

    // Label
    ImVec2 text_pos = ImVec2(icon_pos.x + 20 + spacing, rect.Min.y + (tab_height - label_size.y) * 0.5f);
    window->DrawList->AddText(text_pos, ImColor(255.f, 255.f, 255.f, a.text_opacity), label);

    return pressed;
}

bool elements::subtab(const char* name, bool boolean)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(name);
    const ImVec2 label_size = ImGui::CalcTextSize(name);
    ImVec2 pos = window->DC.CursorPos;

    static std::map <ImGuiID, subtab_element> anim;
    auto it_anim = anim.find(id);
    if (it_anim == anim.end()) {
        anim.insert({ id, { 0.0f, 0.0f, 0.0f, 0 } });
        it_anim = anim.find(id);
    }

    // Track call order per frame to determine indent level
    static int frame_counter = 0;
    static int current_frame = 0;

    if (g.FrameCount != current_frame) {
        current_frame = g.FrameCount;
        frame_counter = 0;
    }

    it_anim->second.call_order = frame_counter;
    frame_counter++;

    // Calculate progressive indent based on call order
    float base_indent = 10.0f;
    float vine_indent = base_indent + (it_anim->second.call_order * 8.0f);

    pos.x += vine_indent;
    window->DC.CursorPos = pos;

    const ImRect rect(pos, ImVec2(pos.x + 145, pos.y + 32));
    ImGui::ItemSize(ImVec4(rect.Min.x, rect.Min.y, rect.Max.x, rect.Max.y + 3), style.FramePadding.y);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held, NULL);

    it_anim->second.element_opacity = ImLerp(it_anim->second.element_opacity, (boolean ? 0.04f : 0.0f), 0.09f * (1.0f - ImGui::GetIO().DeltaTime));
    it_anim->second.rect_opacity = ImLerp(it_anim->second.rect_opacity, (boolean ? 1.0f : 0.0f), 0.20f * (1.0f - ImGui::GetIO().DeltaTime));
    it_anim->second.text_opacity = ImLerp(it_anim->second.text_opacity, (boolean ? 1.0f : 0.3f), 0.07f * (1.0f - ImGui::GetIO().DeltaTime));

    // Draw vine connector lines in |- style
    float line_thickness = 1.5f;
    ImColor vine_color = ImColor(0.4f,0.4f,0.4f, 0.25f);

    // Vertical line - always starts from parent tab position
    float vertical_line_x = pos.x - vine_indent + base_indent - 5.0f;
    float line_start_y = rect.Min.y - 23.0f;
    float line_end_y = rect.Min.y + rect.GetHeight() / 2;

    // Draw continuous vertical line
    window->DrawList->AddLine(ImVec2(vertical_line_x, line_start_y), ImVec2(vertical_line_x, line_end_y), vine_color, line_thickness);

    // Horizontal line extending from vertical line to subtab (grows with indent)
    float line_y = rect.Min.y + rect.GetHeight() / 2;
    window->DrawList->AddLine(ImVec2(vertical_line_x, line_y), ImVec2(rect.Min.x, line_y), vine_color, line_thickness);

    window->DrawList->AddRectFilled(rect.Min, rect.Max, ImColor(1.0f, 1.0f, 1.0f, it_anim->second.element_opacity), 3.0f);
    window->DrawList->AddText(ImVec2(rect.Min.x + 15, (rect.Min.y + rect.Max.y) / 2 - label_size.y / 2), ImColor(1.0f, 1.0f, 1.0f, it_anim->second.text_opacity), name);

    window->DrawList->AddRectFilled(ImVec2(rect.Max.x + 5, rect.Min.y + 9), ImVec2(rect.Max.x + 8, rect.Max.y - 9), ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], it_anim->second.rect_opacity), 10.0f, ImDrawFlags_RoundCornersLeft);

    return pressed;
}

struct icon_tab_element {
    float element_opacity, glow_opacity, text_opacity;
};

// Category title renderer
void elements::category_title(const char* title)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImVec2 pos = window->DC.CursorPos;

    // Smaller font size for category title
    ImGui::PushFont(ImGui::GetFont());
    ImGui::SetWindowFontScale(0.75f);
    ImVec2 label_size = ImGui::CalcTextSize(title);

    // Draw category title with subtle styling
    window->DrawList->AddText(
        ImVec2(pos.x + 10, pos.y),
        ImColor(1.0f, 1.0f, 1.0f, 0.4f),
        title
    );

    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();

    // Subtle underline
    window->DrawList->AddLine(
        ImVec2(pos.x + 8, pos.y + label_size.y + 2),
        ImVec2(pos.x + 195, pos.y + label_size.y + 2),
        ImColor(1.0f, 1.0f, 1.0f, 0.05f),
        1.0f
    );

    ImGui::Dummy(ImVec2(0, label_size.y + 6));
}

// Icon-based tab (Neverlose style)
bool elements::icon_tab(const char* icon, const char* label, bool active)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    // Sizes
    const float tab_height = 28.0f;
    const float icon_size = 14.0f;
    const float spacing = 10.0f;

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 label_size = ImGui::CalcTextSize(label);
    ImVec2 rect_size = ImVec2(195, tab_height);

    ImRect rect(pos, pos + rect_size);

    ImGui::ItemSize(rect, style.FramePadding.y);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);

    // Animation data
    static std::map<ImGuiID, icon_tab_element> anim;
    auto& a = anim.try_emplace(id, icon_tab_element{ 0.f, 0.f, 0.f }).first->second;

    a.element_opacity = ImLerp(a.element_opacity, (active ? 0.08f : hovered ? 0.04f : 0.0f), 0.15f * (1.0f - g.IO.DeltaTime));
    a.glow_opacity = ImLerp(a.glow_opacity, (active ? 1.0f : 0.0f), 0.20f * (1.0f - g.IO.DeltaTime));
    a.text_opacity = ImLerp(a.text_opacity, (active ? 1.0f : hovered ? 0.7f : 0.5f), 0.10f * (1.0f - g.IO.DeltaTime));

    // Background when active/hovered
    if (a.element_opacity > 0.0f)
        window->DrawList->AddRectFilled(
            rect.Min, rect.Max,
            ImColor(1.0f, 1.0f, 1.0f, a.element_opacity),
            4.0f
        );

    // Accent bar on left when active
    if (a.glow_opacity > 0.0f)
        window->DrawList->AddRectFilled(
            ImVec2(rect.Min.x, rect.Min.y + 6),
            ImVec2(rect.Min.x + 2, rect.Max.y - 6),
            ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], a.glow_opacity),
            2.0f
        );

    // Icon with subtle glow when active
    ImVec2 icon_pos = ImVec2(rect.Min.x + 12, rect.Min.y + (tab_height - icon_size) * 0.5f);

    if (a.glow_opacity > 0.0f) {
        // Subtle icon glow
        window->DrawList->AddText(
            fonts::dna_font,
            icon_size,
            ImVec2(icon_pos.x + 0.5f, icon_pos.y + 0.5f),
            ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 0.3f * a.glow_opacity),
            icon
        );
    }

    window->DrawList->AddText(
        fonts::dna_font,
        icon_size,
        icon_pos,
        ImColor(
            active ? menu::menu_accent[0] : 1.0f,
            active ? menu::menu_accent[1] : 1.0f,
            active ? menu::menu_accent[2] : 1.0f,
            a.text_opacity
        ),
        icon
    );

    if (active) {
        window->DrawList->AddText(
            fonts::dna_font,
            icon_size,
            ImVec2(icon_pos.x + 1, icon_pos.y + 1),
            ImColor(
                active ? 255.f : 1.0f,
                active ? 255.f : 1.0f,
                active ? 255.f : 1.0f,
                a.text_opacity
            ),
            icon
        );
    }

    // Label
    ImVec2 text_pos = ImVec2(icon_pos.x + icon_size + spacing, rect.Min.y + (tab_height - label_size.y) * 0.5f);
    window->DrawList->AddText(
        text_pos,
        ImColor(1.0f, 1.0f, 1.0f, a.text_opacity),
        label
    );

    return pressed;
}