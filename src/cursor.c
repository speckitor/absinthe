#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "layout.h"
#include "toplevel.h"
#include "types.h"

void
reset_cursor_mode(absn_server *server)
{
	server->cursor_mode = CURSOR_PASSTHROUGH;
	wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
	if (server->focused_toplevel) {
		wlr_xdg_toplevel_set_resizing(server->focused_toplevel->xdg,
		    false);
	}
}

static void
process_cursor_move(absn_server *server)
{
	struct absn_toplevel *toplevel = server->focused_toplevel;

	if (!toplevel)
		return;

	if (toplevel->fullscreen) {
		toplevel->prev_geom = toplevel->geom;
		toplevel_set_fullscreen(toplevel, false);
	}

	uint32_t new_x, new_y;
	new_x = server->cursor->x - server->grab_x + server->grab_geom.x;
	new_y = server->cursor->y - server->grab_y + server->grab_geom.y;
	toplevel_set_pos(toplevel, new_x, new_y);

	if (toplevel->tiled) {
		toplevel->tiled = false;
		layout_arrange(toplevel->output);
	}
}

static void
apply_resize(absn_toplevel *toplevel, struct wlr_box *new_geom)
{
	if (toplevel->type == TOPLEVEL_XDG) {
		int32_t min_width = toplevel->xdg->current.min_width;
		int32_t min_height = toplevel->xdg->current.min_height;

		int32_t max_width = toplevel->xdg->current.max_width;
		int32_t max_height = toplevel->xdg->current.max_height;

		if (max_width == 0)
			max_width = 10000;

		if (max_height == 0)
			max_height = 10000;

		if (!(new_geom->width >= min_width &&
			new_geom->width <= max_width)) {
			new_geom->width = toplevel->geom.width;
			new_geom->x = toplevel->geom.x;
		}

		if (!(new_geom->height >= min_height &&
			new_geom->height <= max_height)) {
			new_geom->height = toplevel->geom.height;
			new_geom->y = toplevel->geom.y;
		}
	}

	toplevel_set_geom(toplevel, new_geom);
}

static void
process_cursor_resize(absn_server *server)
{
	struct absn_toplevel *toplevel = server->focused_toplevel;

	if (!toplevel)
		return;

	if (toplevel->resizing)
		return;

	if (toplevel->fullscreen)
		toplevel_set_fullscreen(toplevel, false);

	if (toplevel->tiled) {
		toplevel->tiled = false;
		layout_arrange(toplevel->output);
	}

	int32_t new_x, new_y, new_width, new_height;
	new_x = server->grab_geom.x;
	new_y = server->grab_geom.y;
	new_width = server->grab_geom.width;
	new_height = server->grab_geom.height;

	int32_t dx = server->cursor->x - server->grab_x;
	int32_t dy = server->cursor->y - server->grab_y;

	if (dx == 0 && dy == 0)
		return;

	switch (server->resize_corner) {
	case TOP_LEFT:
		new_x += dx;
		new_y += dy;
		new_width -= dx;
		new_height -= dy;
		break;
	case TOP_RIGHT:
		new_y += dy;
		new_width += dx;
		new_height -= dy;
		break;
	case BOTTOM_LEFT:
		new_x += dx;
		new_width -= dx;
		new_height += dy;
		break;
	case BOTTOM_RIGHT:
		new_width += dx;
		new_height += dy;
		break;
	default: // unreachable
		break;
	}

	if (new_width > 0 && new_height > 0) {
		struct wlr_box new_geometry = {
			.x = new_x,
			.y = new_y,
			.width = new_width,
			.height = new_height,
		};

		apply_resize(server->focused_toplevel, &new_geometry);
	}
}

void
process_cursor_motion(absn_server *server, uint32_t time)
{
	double sx, sy;
	struct wlr_surface *surface = NULL;
	toplevel_at(server, server->cursor->x, server->cursor->y, &surface, &sx,
	    &sy);
	struct wlr_seat *seat = server->seat;

	if (server->cursor_mode == CURSOR_MOVE) {
		process_cursor_move(server);
		return;
	} else if (server->cursor_mode == CURSOR_RESIZE) {
		process_cursor_resize(server);
		return;
	}

	if (surface) {
		wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
		wlr_seat_pointer_notify_motion(seat, time, sx, sy);
	} else {
		wlr_seat_pointer_clear_focus(seat);
	}
}
