#pragma once
#include <string>
#include <vector>
#include <variant>

struct GearSettingEntry {
    std::string label;
    std::variant<bool*, float*, int*> value;
    float float_min = 0.0f;
    float float_max = 1.0f;
    int int_min = 0;
    int int_max = 100;

    GearSettingEntry(const char* label, bool* b)
        : label(label), value(b) {}
    GearSettingEntry(const char* label, float* f, float min = 0.f, float max = 1.f)
        : label(label), value(f), float_min(min), float_max(max) {}
    GearSettingEntry(const char* label, int* i, int min = 0, int max = 100)
        : label(label), value(i), int_min(min), int_max(max) {}
};