#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "absinthe-toplevel.h"
#include "focus.h"
#include "types.h"

void focus_toplevel(struct absinthe_toplevel *toplevel)
{
	if (!toplevel)
		return;

	struct absinthe_server *server = toplevel->server;
	struct wlr_seat *seat = server->seat;
	struct wlr_surface *prev_surface = seat->keyboard_state.focused_surface;
	struct wlr_surface *surface;
#ifdef XWAYLAND
	if (toplevel->type == ABSINTHE_TOPLEVEL_X11)
		surface = toplevel->xwayland_surface->surface;
	else
#endif
		surface = toplevel->xdg_toplevel->base->surface;

	if (surface == prev_surface)
		return;

	if (prev_surface) {
		struct wlr_xdg_toplevel *prev_toplevel = wlr_xdg_toplevel_try_from_wlr_surface(prev_surface);
		if (prev_toplevel) {
			wlr_xdg_toplevel_set_activated(prev_toplevel, false);
			absinthe_toplevel_set_border_color(prev_toplevel->base->data, unfocused_border_color);
		}

		struct wlr_xwayland_surface *prev_xwayland_surface =
		    wlr_xwayland_surface_try_from_wlr_surface(prev_surface);
		if (prev_xwayland_surface)
			absinthe_toplevel_set_border_color(prev_surface->data, unfocused_border_color);
	}

	toplevel->server->focused_toplevel = toplevel;

	struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);
	wlr_scene_node_raise_to_top(&toplevel->scene_tree->node);
	wl_list_remove(&toplevel->flink);
	wl_list_insert(&server->focus_stack, &toplevel->flink);
	if (toplevel->type != ABSINTHE_TOPLEVEL_X11)
		wlr_xdg_toplevel_set_activated(toplevel->xdg_toplevel, true);
	absinthe_toplevel_set_border_color(toplevel, focused_border_color);

	if (keyboard)
		wlr_seat_keyboard_notify_enter(seat, surface, keyboard->keycodes, keyboard->num_keycodes,
					       &keyboard->modifiers);
}

struct absinthe_toplevel *focus_get_topmost(struct absinthe_server *server)
{
	struct absinthe_toplevel *toplevel;
	wl_list_for_each(toplevel, &server->focus_stack, flink)
	{
		if (toplevel)
			return toplevel;
	}
	return NULL;
}

void focus_next(struct absinthe_server *server)
{
	struct absinthe_toplevel *toplevel = focus_get_topmost(server);
	if (!toplevel)
		return;

	struct absinthe_toplevel *next;
	wl_list_for_each(next, &toplevel->link, link)
	{
		if (&next->link == &toplevel->server->toplevels)
			continue;
		break;
	}
	focus_toplevel(next);
}

void focus_prev(struct absinthe_server *server)
{
	struct absinthe_toplevel *toplevel = focus_get_topmost(server);
	if (!toplevel)
		return;

	struct absinthe_toplevel *prev;
	wl_list_for_each_reverse(prev, &toplevel->link, link)
	{
		if (&prev->link == &toplevel->server->toplevels)
			continue;
		break;
	}
	focus_toplevel(prev);
}
