#include <assert.h>
#include <stdlib.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/log.h>

#include "config.h"
#include "cursor.h"
#include "focus.h"
#include "keyboard.h"
#include "layer-surface.h"
#include "output.h"
#include "toplevel-handlers.h"
#include "toplevel.h"
#include "types.h"
#include "xdg-decoration.h"
#include "xdg-popup.h"
#include "xdg-toplevel.h"

#ifdef XWAYLAND
#include <wlr/xwayland.h>

#include "xwayland.h"
#endif

void
new_output(struct wl_listener *listener, void *data)
{
	struct absinthe_server *server = wl_container_of(listener, server,
	    new_output);
	struct wlr_output *wlr_output = data;

	wlr_output_init_render(wlr_output, server->allocator, server->renderer);

	struct wlr_output_state state;
	wlr_output_state_init(&state);
	wlr_output_state_set_enabled(&state, true);

	struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
	if (mode)
		wlr_output_state_set_mode(&state, mode);

	wlr_output_commit_state(wlr_output, &state);
	wlr_output_state_finish(&state);

	struct absinthe_output *output = calloc(1, sizeof(*output));
	output->wlr = wlr_output;
	output->server = server;

	LISTEN(output->frame, output_frame, wlr_output->events.frame);
	LISTEN(output->request_state, output_request_state,
	    wlr_output->events.request_state);
	LISTEN(output->destroy, output_destroy, wlr_output->events.destroy);

	wl_list_insert(&server->outputs, &output->link);

	struct wlr_output_layout_output *l_layout =
	    wlr_output_layout_add_auto(server->output_layout, output->wlr);
	struct wlr_scene_output *scene_output =
	    wlr_scene_output_create(server->scene, wlr_output);
	wlr_scene_output_layout_add_output(server->scene_layout, l_layout,
	    scene_output);
	wlr_output_layout_get_box(server->output_layout, output->wlr,
	    &output->geom);

	output->mstack_size = MSTACK_SIZE;
	output->mstack_width = MSTACK_WIDTH;
}

void
new_xdg_toplevel(struct wl_listener *listener, void *data)
{
	struct absinthe_server *server = wl_container_of(listener, server,
	    new_xdg_toplevel);
	struct wlr_xdg_toplevel *xdg_toplevel = data;

	struct absinthe_toplevel *toplevel = calloc(1, sizeof(*toplevel));
	toplevel->type = TOPLEVEL_XDG;
	toplevel->server = server;
	toplevel->xdg = xdg_toplevel;
	toplevel->xdg->base->data = toplevel;

	toplevel->commit.notify = toplevel_commit;
	wl_signal_add(&xdg_toplevel->base->surface->events.commit,
	    &toplevel->commit);

	toplevel->map.notify = toplevel_map;
	wl_signal_add(&xdg_toplevel->base->surface->events.map, &toplevel->map);
	toplevel->unmap.notify = toplevel_unmap;
	wl_signal_add(&xdg_toplevel->base->surface->events.unmap,
	    &toplevel->unmap);

	toplevel->destroy.notify = toplevel_destroy;
	wl_signal_add(&xdg_toplevel->events.destroy, &toplevel->destroy);

	toplevel->request_move.notify = toplevel_request_move;
	wl_signal_add(&xdg_toplevel->events.request_move,
	    &toplevel->request_move);
	toplevel->request_resize.notify = toplevel_request_resize;
	wl_signal_add(&xdg_toplevel->events.request_resize,
	    &toplevel->request_resize);
	toplevel->request_maximize.notify = toplevel_request_maximize;
	wl_signal_add(&xdg_toplevel->events.request_maximize,
	    &toplevel->request_maximize);
	toplevel->request_fullscreen.notify = toplevel_request_fullscreen;
	wl_signal_add(&xdg_toplevel->events.request_fullscreen,
	    &toplevel->request_fullscreen);
}

void
new_xdg_popup(struct wl_listener *listener, void *data)
{
	struct absinthe_server *server = wl_container_of(listener, server,
	    new_xdg_popup);
	struct wlr_xdg_popup *xdg_popup = data;

	struct absinthe_popup *popup = calloc(1, sizeof(*popup));
	popup->wlr = xdg_popup;

	struct wlr_xdg_surface *parent = wlr_xdg_surface_try_from_wlr_surface(
	    xdg_popup->parent);
	struct absinthe_toplevel *parent_toplevel = parent->data;
	struct wlr_scene_tree *parent_tree = parent_toplevel->scene_tree;
	xdg_popup->base->data = wlr_scene_xdg_surface_create(parent_tree,
	    xdg_popup->base);

	popup->commit.notify = xdg_popup_commit;
	wl_signal_add(&xdg_popup->base->surface->events.commit, &popup->commit);

	popup->destroy.notify = xdg_popup_destroy;
	wl_signal_add(&xdg_popup->base->surface->events.destroy,
	    &popup->destroy);
}

void
new_xdg_decoration(struct wl_listener *listener, void *data)
{
	UNUSED(listener);
	struct wlr_xdg_toplevel_decoration_v1 *deco = data;
	struct absinthe_toplevel *toplevel = deco->toplevel->base->data;
	toplevel->deco = deco;

	LISTEN(toplevel->deco_request_mode, deco_request_mode,
	    deco->events.request_mode);
	LISTEN(toplevel->deco_destroy, deco_destroy, deco->events.destroy);

	/* Forse server side decoration mode */
	deco_request_mode(&toplevel->deco_request_mode, toplevel->deco);
}

void
new_layer_surface(struct wl_listener *listener, void *data)
{
	struct absinthe_server *server = wl_container_of(listener, server,
	    new_layer_surface);
	struct wlr_layer_surface_v1 *layer_surface = data;

	if (!layer_surface->output &&
	    !(layer_surface->output = server->focused_output->wlr)) {
		wlr_layer_surface_v1_destroy(layer_surface);
		return;
	}

	struct absinthe_layer_surface *layer = calloc(1, sizeof(*layer));
	LISTEN(layer->commit, layer_surface_commit,
	    layer_surface->surface->events.commit);
	LISTEN(layer->unmap, layer_surface_unmap,
	    layer_surface->surface->events.unmap);
	LISTEN(layer->destroy, layer_surface_destroy,
	    layer_surface->surface->events.destroy);
}

#ifdef XWAYLAND
void
xwayland_ready(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_server *server = wl_container_of(listener, server,
	    xw_ready);

	wlr_xwayland_set_seat(server->xwayland, server->seat);

	struct wlr_xcursor *xcursor;
	if ((xcursor = wlr_xcursor_manager_get_xcursor(server->cursor_mgr,
		 "default", 1))) {
		struct wlr_buffer *buffer = wlr_xcursor_image_get_buffer(
		    xcursor->images[0]);
		wlr_xwayland_set_cursor(server->xwayland, buffer,
		    xcursor->images[0]->hotspot_x,
		    xcursor->images[0]->hotspot_y);
	}
}

void
xwayland_new_surface(struct wl_listener *listener, void *data)
{
	struct absinthe_server *server = wl_container_of(listener, server,
	    xw_new_surface);
	struct wlr_xwayland_surface *surface = data;
	struct absinthe_toplevel *toplevel = calloc(1, sizeof(*toplevel));

	toplevel->type = TOPLEVEL_X11;
	toplevel->server = server;
	toplevel->xw = surface;
	toplevel->bw = toplevel_is_unmanaged(toplevel) ? 0 : TOPLEVEL_BW;
	surface->data = toplevel;

	LISTEN(toplevel->destroy, toplevel_destroy, surface->events.destroy);
	LISTEN(toplevel->request_maximize, toplevel_request_maximize,
	    surface->events.request_maximize);
	LISTEN(toplevel->request_fullscreen, toplevel_request_fullscreen,
	    surface->events.request_fullscreen);

	LISTEN(toplevel->xw_activate, xwayland_activate,
	    surface->events.request_activate);
	LISTEN(toplevel->xw_associate, xwayland_associate,
	    surface->events.associate);
	LISTEN(toplevel->xw_dissociate, xwayland_dissociate,
	    surface->events.dissociate);
	LISTEN(toplevel->xw_configure, xwayland_configure,
	    surface->events.request_configure);
	LISTEN(toplevel->xw_set_hints, xwayland_set_hints,
	    surface->events.set_hints);
}
#endif

void
cursor_motion(struct wl_listener *listener, void *data)
{
	struct absinthe_server *server = wl_container_of(listener, server,
	    cursor_motion);
	struct wlr_pointer_motion_event *event = data;
	update_focused_output(server);
	wlr_cursor_move(server->cursor, &event->pointer->base, event->delta_x,
	    event->delta_y);
	process_cursor_motion(server, event->time_msec);
}

void
cursor_motion_abs(struct wl_listener *listener, void *data)
{
	struct absinthe_server *server = wl_container_of(listener, server,
	    cursor_motion_abs);
	struct wlr_pointer_motion_absolute_event *event = data;
	update_focused_output(server);
	wlr_cursor_warp_absolute(server->cursor, &event->pointer->base,
	    event->x, event->y);
	process_cursor_motion(server, event->time_msec);
}

void
cursor_button(struct wl_listener *listener, void *data)
{
	struct absinthe_server *server = wl_container_of(listener, server,
	    cursor_button);
	struct wlr_pointer_button_event *event = data;
	bool handled = false;

	if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
		reset_cursor_mode(server);
	} else {
		double sx, sy;
		struct wlr_surface *surface = NULL;
		struct absinthe_toplevel *toplevel = toplevel_at(server,
		    server->cursor->x, server->cursor->y, &surface, &sx, &sy);

		if (!toplevel)
			goto handle;

		struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(
		    server->seat);
		uint32_t mods = wlr_keyboard_get_modifiers(keyboard);
		if (mods & CURSOR_MOD) {
			if (event->button == CURSOR_MOVE_BUTTON) {
				server->cursor_mode = CURSOR_MOVE;
				wlr_cursor_set_xcursor(server->cursor,
				    server->cursor_mgr, "all-scroll");
				handled = true;
			} else if (event->button == CURSOR_RESIZE_BUTTON) {
				server->cursor_mode = CURSOR_RESIZE;
				handled = true;
			}
		}

		focus_toplevel(toplevel);
		server->grab_x = server->cursor->x;
		server->grab_y = server->cursor->y;

		int32_t lx, ly;
		wlr_scene_node_coords(&toplevel->scene_tree->node, &lx, &ly);
		server->grab_geom.x = lx;
		server->grab_geom.y = ly;
		server->grab_geom.width = toplevel->geom.width;
		server->grab_geom.height = toplevel->geom.height;

		if (server->cursor_mode != CURSOR_RESIZE)
			goto handle;

		int32_t width = toplevel->xdg->base->geometry.width;
		int32_t height = toplevel->xdg->base->geometry.height;

		if (server->grab_x > (lx + width / 2) &&
		    server->grab_y > (ly + height / 2)) {
			server->resize_corner = BOTTOM_RIGHT;
			wlr_cursor_set_xcursor(server->cursor,
			    server->cursor_mgr, "se-resize");
		} else if (server->grab_x < (lx + width / 2) &&
		    server->grab_y > (ly + height / 2)) {
			server->resize_corner = BOTTOM_LEFT;
			wlr_cursor_set_xcursor(server->cursor,
			    server->cursor_mgr, "sw-resize");
		} else if (server->grab_x > (lx + width / 2) &&
		    server->grab_y < (ly + height / 2)) {
			server->resize_corner = TOP_RIGHT;
			wlr_cursor_set_xcursor(server->cursor,
			    server->cursor_mgr, "ne-resize");
		} else {
			server->resize_corner = TOP_LEFT;
			wlr_cursor_set_xcursor(server->cursor,
			    server->cursor_mgr, "nw-resize");
		}

		wlr_xdg_toplevel_set_resizing(toplevel->xdg, true);
	}

handle:
	if (!handled) {
		wlr_seat_pointer_notify_button(server->seat, event->time_msec,
		    event->button, event->state);
	}
}

void
cursor_axis(struct wl_listener *listener, void *data)
{
	struct absinthe_server *server = wl_container_of(listener, server,
	    cursor_axis);
	struct wlr_pointer_axis_event *event = data;
	wlr_seat_pointer_notify_axis(server->seat, event->time_msec,
	    event->orientation, event->delta, event->delta_discrete,
	    event->source, event->relative_direction);
}

void
cursor_frame(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_server *server = wl_container_of(listener, server,
	    cursor_frame);
	wlr_seat_pointer_notify_frame(server->seat);
}

static void
new_keyboard(struct absinthe_server *server, struct wlr_input_device *device)
{
	struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(
	    device);
	struct absinthe_keyboard *keyboard = calloc(1, sizeof(*keyboard));
	keyboard->server = server;
	keyboard->wlr = wlr_keyboard;

	struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, NULL,
	    XKB_KEYMAP_COMPILE_NO_FLAGS);

	wlr_keyboard_set_keymap(wlr_keyboard, keymap);
	xkb_keymap_unref(keymap);
	xkb_context_unref(context);
	wlr_keyboard_set_repeat_info(wlr_keyboard, 25, 600);

	LISTEN(keyboard->modifiers, handle_modifiers,
	    wlr_keyboard->events.modifiers);
	LISTEN(keyboard->key, handle_key, wlr_keyboard->events.key);
	LISTEN(keyboard->destroy, handle_destroy,
	    wlr_keyboard->base.events.destroy);

	wlr_seat_set_keyboard(server->seat, wlr_keyboard);

	wl_list_insert(&server->keyboards, &keyboard->link);
}

static void
new_pointer(struct absinthe_server *server, struct wlr_input_device *device)
{
	wlr_cursor_attach_input_device(server->cursor, device);
}

void
new_input(struct wl_listener *listener, void *data)
{
	struct absinthe_server *server = wl_container_of(listener, server,
	    new_input);
	struct wlr_input_device *device = data;
	switch (device->type) {
	case WLR_INPUT_DEVICE_KEYBOARD:
		new_keyboard(server, device);
		break;
	case WLR_INPUT_DEVICE_POINTER:
		new_pointer(server, device);
	default:
		break;
	}

	uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
	if (!wl_list_empty(&server->keyboards)) {
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;
	}
	wlr_seat_set_capabilities(server->seat, caps);
}
