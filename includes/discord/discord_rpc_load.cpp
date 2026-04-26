#include "discord_rpc.h"
#include "../../menu/config/config.hpp"
#include "../../menu/menu.hpp"
#include "../../hooks/hooks.hpp"
#include "../../features/visuals/visuals.hpp"
#include <ctime>
#include <string>

// Переменные из твоего музыкального модуля
extern std::string strartist;
extern std::string strtitle;

void c_discord::initialize() {
    DiscordEventHandlers Handle;
    memset(&Handle, 0, sizeof(Handle));

    // Твой ID приложения
    Discord_Initialize("1474938263285272729", &Handle, 1, NULL);
}

void c_discord::update() {
    static bool init = false;
    if (!init) {
        initialize();
        init = true;
    }

    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));
    static auto elapsed = std::time(nullptr);

    // 1. Формируем строку музыки (Details)
    std::string details_text = "";
    
    mplayer.Lock();
    std::string current_title = strtitle;
    std::string current_artist = strartist;
    mplayer.Unlock();

    if (!current_title.empty()) {
        if (!current_artist.empty())
            details_text = current_artist + " - " + current_title;
        else
            details_text = current_title;
    }
    else {
        const auto& profile = menu::menu_variant_profile(menu::active_menu_variant());
        details_text = profile.watermark_label;
    }

    // 2. Формируем строку карты (State)
    std::string state_text = "Main Menu";

    if (interfaces::engine->is_in_game()) {
        const char* level_name = interfaces::engine->get_level_name();

        if (level_name && strlen(level_name) > 1) {
            std::string map_raw = level_name;
            size_t last_slash = map_raw.find_last_of("/\\");
            std::string clean_map = (last_slash != std::string::npos) ? map_raw.substr(last_slash + 1) : map_raw;

            state_text = "Map: " + clean_map;
        }
        else {
            state_text = "In Match";
        }
    }
    else {
        state_text = "tell me whatchu want bby";
    }

    // Передаем данные в Discord
    discordPresence.details = details_text.c_str();
    discordPresence.state = state_text.c_str();
    discordPresence.largeImageKey = "thefool"; // Убедись, что этот ключ есть в Dev Portal
    
    const auto& profile = menu::menu_variant_profile(menu::active_menu_variant());
    if (menu::experimental_femboy_theme_active) {
        discordPresence.largeImageText = profile.watermark_label;
        discordPresence.smallImageKey = "heart";
        discordPresence.smallImageText = profile.display_name;
    }
    else if (menu::experimental_fight_club_mode) {
        discordPresence.largeImageText = profile.watermark_label;
        discordPresence.smallImageKey = "soap"; // If you have a soap icon
        discordPresence.smallImageText = profile.naming_style;
    }
    else {
        discordPresence.largeImageText = profile.watermark_label;
        discordPresence.smallImageKey = "";
        discordPresence.smallImageText = "";
    }
    
    discordPresence.startTimestamp = elapsed;

    Discord_UpdatePresence(&discordPresence);
}

void c_discord::shutdown() {
    Discord_ClearPresence();
    Discord_Shutdown();
}
