#include "events.hpp"
#include "../misc/misc.hpp"
#include "../panorama/scaleform.hpp"
#pragma comment(lib, "Winmm.lib")

hooked_events events;

void hooked_events::fire_game_event(i_game_event* event) {
	if (!event)
		return;

	auto event_name = event->get_name();
	if (!event_name)
		return;

	g::last_event = event_name;
	debug::log("Event fired: %s", event_name);

	if (!strcmp(event_name, "player_hurt")) {
		features::misc::hit_info(event);
	}
	if (!strcmp(event_name, "player_death")) {
		features::misc::kill_say(event);
	}
	if (!strcmp(event_name, "round_prestart")) {
		recorder->forcestop();
	}
	if (strstr(event_name, "player_footstep")) {

	}
	if (strstr(event_name, "vote_cast")) {

	}
}

int hooked_events::get_event_debug_id(void) {
	return EVENT_DEBUG_ID_INIT;
}

void hooked_events::setup() {
	debug_id = EVENT_DEBUG_ID_INIT;
	interfaces::event_manager->add_listener(this, "player_hurt", false);
	interfaces::event_manager->add_listener(this, "player_death", false);
	interfaces::event_manager->add_listener(this, "item_purchase", false);
	interfaces::event_manager->add_listener(this, "player_footstep", false);
	interfaces::event_manager->add_listener(this, "round_prestart", false);
	interfaces::event_manager->add_listener(this, "item_equip", false);
	interfaces::event_manager->add_listener(this, "vote_cast", false);
	interfaces::event_manager->add_listener(this, "round_mvp", false);
	interfaces::event_manager->add_listener(this, "round_end", false);
	interfaces::event_manager->add_listener(this, "other_death", false);
	interfaces::event_manager->add_listener(this, "weapon_fire", false);
	interfaces::event_manager->add_listener(this, "bot_takeover", false);
	interfaces::event_manager->add_listener(this, "switch_team", false);
	interfaces::event_manager->add_listener(this, "round_start", false);
}

void hooked_events::unload() {
	interfaces::event_manager->remove_listener(this);
}
