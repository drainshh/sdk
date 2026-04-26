#pragma once

namespace features::visuals::pixelsurf_db {
	void on_create_move();
	void on_paint_traverse();
	void clear();
	/// One-shot: true once after a new pixelsurf path is written to pxdatabase.px (for HUD notification_system).
	bool consume_new_route_screen_notification();
}
