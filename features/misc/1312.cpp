#include <windows.h>
#include <string>
#include <vector>
#include "../visuals/visuals.hpp"

// Ссылки на переменные из твоего чита
extern std::string strartist;
extern std::string strtitle;

// Просто объявляем, что такая функция существует в другом файле
extern void apply_clan_tag(const char* tag, const char* name);

std::string get_current_media_info() {
    mplayer.Lock();
    std::string current_title = strtitle;
    std::string current_artist = strartist;
    mplayer.Unlock();

    if (current_title.empty())
        return "whatever";

    if (!current_artist.empty() && current_title.find(" - ") == std::string::npos) {
        return current_artist + " - " + current_title;
    }
    return current_title;
}

std::string animate_tag(std::string text) {
    static int offset = 0;
    std::string display = "          " + text + "          ";
    if (offset >= (int)display.length()) offset = 0;
    std::string part = display.substr(offset, 10);
    offset++;
    return part;
}