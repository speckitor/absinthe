#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "config.h"
#include "focus.h"
#include "toplevel.h"
#include "types.h"

void
focus_toplevel(absn_toplevel *toplevel)
{
	if (!toplevel)
		return;

	absn_server *server = toplevel->server;
	struct wlr_seat *seat = server->seat;
	struct wlr_surface *prev_surface = seat->keyboard_state.focused_surface;
	struct wlr_surface *surface;
#ifdef XWAYLAND
	if (toplevel->type == TOPLEVEL_X11)
		surface = toplevel->xw->surface;
	else
#endif
		surface = toplevel->xdg->base->surface;

	if (surface == prev_surface)
		return;

	if (prev_surface) {
		struct wlr_xdg_toplevel *prev_toplevel =
		    wlr_xdg_toplevel_try_from_wlr_surface(prev_surface);
		if (prev_toplevel) {
			wlr_xdg_toplevel_set_activated(prev_toplevel, false);
			toplevel_set_border_color(prev_toplevel->base->data,
			    unfocused_bc);
		}

#ifdef XWAYLAND
		struct wlr_xwayland_surface *prev_xwayland_surface =
		    wlr_xwayland_surface_try_from_wlr_surface(prev_surface);
		if (prev_xwayland_surface)
			toplevel_set_border_color(prev_xwayland_surface->data,
			    unfocused_bc);
#endif
	}

	toplevel->server->focused_toplevel = toplevel;

	struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);
	wlr_scene_node_raise_to_top(&toplevel->scene_tree->node);
	wl_list_remove(&toplevel->flink);
	wl_list_insert(&server->focus_stack, &toplevel->flink);
	if (toplevel->type != TOPLEVEL_X11)
		wlr_xdg_toplevel_set_activated(toplevel->xdg, true);
	toplevel_set_border_color(toplevel, focused_bc);

	if (keyboard)
		wlr_seat_keyboard_notify_enter(seat, surface,
		    keyboard->keycodes, keyboard->num_keycodes,
		    &keyboard->modifiers);
}

/*
 * get first toplevel on monitor
 * (last focused toplevel)
 */
absn_toplevel *
focus_get_topmost(absn_server *server)
{
	absn_toplevel *toplevel;
	wl_list_for_each(toplevel, &server->focus_stack, flink)
	{
		if (toplevel)
			return toplevel;
	}
	return NULL;
}

void
focus_next(absn_server *server)
{
	absn_toplevel *toplevel = focus_get_topmost(server);
	if (!toplevel)
		return;

	absn_toplevel *next;
	wl_list_for_each(next, &toplevel->link, link)
	{
		if (&next->link == &toplevel->server->toplevels)
			continue;
		break;
	}
	focus_toplevel(next);
}

void
focus_prev(absn_server *server)
{
	absn_toplevel *toplevel = focus_get_topmost(server);
	if (!toplevel)
		return;

	absn_toplevel *prev;
	wl_list_for_each_reverse(prev, &toplevel->link, link)
	{
		if (&prev->link == &toplevel->server->toplevels)
			continue;
		break;
	}
	focus_toplevel(prev);
}
