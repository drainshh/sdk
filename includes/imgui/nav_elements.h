#pragma once

#include <cstdint>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <map>

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

extern ImFont* tab_icons;
inline static int expanded_tab_id = -1;
namespace elements {
    bool tab(const char* icon, bool boolean);
    bool tab(const char* icon, const char* label, bool active);
    bool tab(const char* icon, const char* label, bool active, int tab_index);
    bool subtab(const char* name, bool boolean);
    void category_title(const char* title);
    bool icon_tab(const char* icon, const char* label, bool active);
}
