#include <assert.h>
#include <stdlib.h>

#include <wlr/util/log.h>
#include <wlr/types/wlr_xcursor_manager.h>

#include "types.h"
#include "output.h"
#include "xdg-toplevel.h"
#include "xdg-popup.h"
#include "xdg-decoration.h"
#include "absinthe-toplevel.h"
#include "focus.h"
#include "keyboard.h"
#include "cursor.h"

#ifdef XWAYLAND
#include <wlr/xwayland.h>
#include "xwayland.h"
#endif

void server_new_output(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, new_output);
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

    struct absinthe_output *output = malloc(sizeof(*output));
    output->wlr_output = wlr_output;
    output->server = server;

    output->frame.notify = output_frame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);

    output->request_state.notify = output_request_state;
    wl_signal_add(&wlr_output->events.request_state, &output->request_state);

    output->destroy.notify = output_destroy;
    wl_signal_add(&wlr_output->events.destroy, &output->destroy);

    wl_list_insert(&server->outputs, &output->link);

    struct wlr_output_layout_output *l_layout = wlr_output_layout_add_auto(server->output_layout, output->wlr_output);
    struct wlr_scene_output *scene_output = wlr_scene_output_create(server->scene, wlr_output);
    wlr_scene_output_layout_add_output(server->scene_layout, l_layout, scene_output);
    wlr_output_layout_get_box(server->output_layout, output->wlr_output, &output->geometry);
}

void server_new_xdg_toplevel(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, new_xdg_toplevel);
    struct wlr_xdg_toplevel *xdg_toplevel = data;

    struct absinthe_toplevel *toplevel = malloc(sizeof(*toplevel));
    toplevel->server = server;
    toplevel->type = ABSINTHE_TOPLEVEL_XDG;
    toplevel->toplevel.xdg = xdg_toplevel;
    toplevel->scene_tree = wlr_scene_tree_create(&toplevel->server->scene->tree);
    toplevel->scene_tree->node.data = toplevel;
    toplevel->scene_surface = wlr_scene_xdg_surface_create(toplevel->scene_tree, xdg_toplevel->base);
    xdg_toplevel->base->data = toplevel;

    toplevel->map.notify = xdg_toplevel_map;
    wl_signal_add(&xdg_toplevel->base->surface->events.map, &toplevel->map);
    toplevel->unmap.notify = xdg_toplevel_unmap;
    wl_signal_add(&xdg_toplevel->base->surface->events.unmap, &toplevel->unmap);
    toplevel->commit.notify = xdg_toplevel_commit;
    wl_signal_add(&xdg_toplevel->base->surface->events.commit, &toplevel->commit);

    toplevel->destroy.notify = xdg_toplevel_destroy;
    wl_signal_add(&xdg_toplevel->events.destroy, &toplevel->destroy);

    toplevel->request_move.notify = xdg_toplevel_request_move;
    wl_signal_add(&xdg_toplevel->events.request_move, &toplevel->request_move);
    toplevel->request_resize.notify = xdg_toplevel_request_resize;
    wl_signal_add(&xdg_toplevel->events.request_resize, &toplevel->request_resize);
    toplevel->request_maximize.notify = xdg_toplevel_request_maximize;
    wl_signal_add(&xdg_toplevel->events.request_maximize, &toplevel->request_maximize);
    toplevel->request_fullscreen.notify = xdg_toplevel_request_fullscreen;
    wl_signal_add(&xdg_toplevel->events.request_fullscreen, &toplevel->request_fullscreen);
}

void server_new_xdg_popup(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, new_xdg_popup);
    struct wlr_xdg_popup *xdg_popup = data;

    struct absinthe_popup *popup = malloc(sizeof(*popup));
    popup->xdg_popup = xdg_popup;

    struct wlr_xdg_surface *parent = wlr_xdg_surface_try_from_wlr_surface(xdg_popup->parent);
    struct absinthe_toplevel *parent_toplevel = parent->data;
    struct wlr_scene_tree *parent_tree = parent_toplevel->scene_tree;
    xdg_popup->base->data = wlr_scene_xdg_surface_create(parent_tree, xdg_popup->base);

    popup->commit.notify = xdg_popup_commit;
    wl_signal_add(&xdg_popup->base->surface->events.commit, &popup->commit);

    popup->destroy.notify = xdg_popup_destroy;
    wl_signal_add(&xdg_popup->base->surface->events.destroy, &popup->destroy);
}

void server_new_xdg_decoration(struct wl_listener *listener, void *data)
{
    struct wlr_xdg_toplevel_decoration_v1 *xdg_decoration = data;
    struct absinthe_toplevel *toplevel = xdg_decoration->toplevel->base->data;
    toplevel->decoration = xdg_decoration;

    toplevel->decoration_request_mode.notify = xdg_decoration_request_mode;
    wl_signal_add(&xdg_decoration->events.request_mode, &toplevel->decoration_request_mode);
    toplevel->decoration_destroy.notify = xdg_decoration_destroy;
    wl_signal_add(&xdg_decoration->events.destroy, &toplevel->decoration_destroy);

    xdg_decoration_request_mode(&toplevel->decoration_request_mode, xdg_decoration);

    toplevel->destroy.notify = xdg_toplevel_destroy;
    wl_signal_add(&toplevel->toplevel.xdg->events.destroy, &toplevel->destroy);
}

#ifdef XWAYLAND
void server_xwayland_ready(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, xwayland_ready);
    struct wlr_xcursor *xcursor;

    wlr_xwayland_set_seat(server->xwayland, server->seat);

    if ((xcursor = wlr_xcursor_manager_get_xcursor(server->cursor_mgr, "default", 1)))
		wlr_xwayland_set_cursor(server->xwayland,
				xcursor->images[0]->buffer, xcursor->images[0]->width * 4,
				xcursor->images[0]->width, xcursor->images[0]->height,
				xcursor->images[0]->hotspot_x, xcursor->images[0]->hotspot_y);
}

void server_xwayland_new_surface(struct wl_listener *listener, void *data)
{
    struct wlr_xwayland_surface *surface = data;
    struct absinthe_toplevel *toplevel = malloc(sizeof(*toplevel));

    toplevel->type = ABSINTHE_TOPLEVEL_X11;
    toplevel->toplevel.x11 = surface;
    toplevel->border_width = absinthe_toplevel_is_unmanaged(toplevel)
        ? 0
        : ABSINTHE_TOPLEVEL_BORDER_WIDTH;

    toplevel->destroy.notify = xdg_toplevel_destroy;
    wl_signal_add(&surface->events.destroy, &toplevel->destroy);
    toplevel->request_maximize.notify = xdg_toplevel_request_maximize;
    wl_signal_add(&surface->events.request_maximize, &toplevel->request_maximize);
    toplevel->request_fullscreen.notify = xdg_toplevel_request_fullscreen;
    wl_signal_add(&surface->events.request_fullscreen, &toplevel->request_fullscreen);

    toplevel->xwayland_activate.notify = xwayland_activate;
    wl_signal_add(&surface->events.request_activate, &toplevel->xwayland_activate);
    toplevel->xwayland_associate.notify = xwayland_associate;
    wl_signal_add(&surface->events.associate, &toplevel->xwayland_associate);
    toplevel->xwayland_dissociate.notify = xwayland_dissociate;
    wl_signal_add(&surface->events.dissociate, &toplevel->xwayland_dissociate);
    toplevel->xwayland_configure.notify = xwayland_configure;
    wl_signal_add(&surface->events.request_configure, &toplevel->xwayland_configure);
    toplevel->xwayland_set_hints.notify = xwayland_set_hints;
    wl_signal_add(&surface->events.set_hints, &toplevel->xwayland_set_hints);
}
#endif

void server_cursor_motion(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, cursor_motion);
    struct wlr_pointer_motion_event *event = data;
    update_focused_output(server);
    wlr_cursor_move(server->cursor, &event->pointer->base, event->delta_x, event->delta_y);
    process_cursor_motion(server, event->time_msec);
}

void server_cursor_motion_absolute(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, cursor_motion_absolute);
    struct wlr_pointer_motion_absolute_event *event = data;
    update_focused_output(server);
    wlr_cursor_warp_absolute(server->cursor, &event->pointer->base, event->x, event->y);
    process_cursor_motion(server, event->time_msec);
}

void server_cursor_button(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, cursor_button);
    struct wlr_pointer_button_event *event = data;
    bool handled = false;

    if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
        reset_cursor_mode(server);
    } else {
        double sx, sy;
        struct wlr_surface *surface = NULL;
        struct absinthe_toplevel *toplevel = absinthe_toplevel_at(server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);

        if (!toplevel)
            goto handle;

        struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(server->seat);
        uint32_t mods = wlr_keyboard_get_modifiers(keyboard);
        if (mods & ABSINTHE_CURSOR_MOD) {
            if (event->button == ABSINTHE_CURSOR_MOVE_BUTTON) {
                server->cursor_mode = ABSINTHE_CURSOR_MOVE;
                wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "all-scroll");
                handled = true;
            } else if (event->button == ABSINTHE_CURSOR_RESIZE_BUTTON) {
                server->cursor_mode = ABSINTHE_CURSOR_RESIZE;
                handled = true;
            }
        }

        focus_toplevel(toplevel);
        server->grab_x = server->cursor->x;
        server->grab_y = server->cursor->y;

        int32_t lx, ly;
        wlr_scene_node_coords(&toplevel->scene_tree->node, &lx, &ly);
        server->grabbed_geometry.x = lx;
        server->grabbed_geometry.y = ly;
        server->grabbed_geometry.width = toplevel->geometry.width;
        server->grabbed_geometry.height = toplevel->geometry.height;

        if (server->cursor_mode != ABSINTHE_CURSOR_RESIZE)
            goto handle;

        int32_t width = toplevel->toplevel.xdg->base->geometry.width;
        int32_t height = toplevel->toplevel.xdg->base->geometry.height;

        if (server->grab_x > (lx + width / 2) && server->grab_y > (ly + height / 2)) {
            server->cursor_resize_corner = ABSINTHE_CURSOR_RESIZE_CORNER_BOTTOM_RIGHT;
            wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "se-resize");
        } else if (server->grab_x < (lx + width / 2) && server->grab_y > (ly + height / 2)) {
            server->cursor_resize_corner = ABSINTHE_CURSOR_RESIZE_CORNER_BOTTOM_LEFT;
            wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "sw-resize");
        } else if (server->grab_x > (lx + width / 2) && server->grab_y < (ly + height / 2)) {
            server->cursor_resize_corner = ABSINTHE_CURSOR_RESIZE_CORNER_TOP_RIGHT;
            wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "ne-resize");
        } else {
            server->cursor_resize_corner = ABSINTHE_CURSOR_RESIZE_CORNER_TOP_LEFT;
            wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "nw-resize");
        }

        wlr_xdg_toplevel_set_resizing(toplevel->toplevel.xdg, true);
    }

handle:
    if (!handled) {
        wlr_seat_pointer_notify_button(server->seat, event->time_msec, event->button, event->state);
    }
}

void server_cursor_axis(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, cursor_axis);
    struct wlr_pointer_axis_event *event = data;
    wlr_seat_pointer_notify_axis(server->seat, event->time_msec, event->orientation, event->delta, event->delta_discrete, event->source, event->relative_direction);
}

void server_cursor_frame(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, cursor_frame);
    wlr_seat_pointer_notify_frame(server->seat);
}

static void server_new_keyboard(struct absinthe_server *server, struct wlr_input_device *device)
{
    struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(device);
    struct absinthe_keyboard *keyboard = malloc(sizeof(*keyboard));
    keyboard->server = server;
    keyboard->wlr_keyboard = wlr_keyboard;

    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);

    wlr_keyboard_set_keymap(wlr_keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
    wlr_keyboard_set_repeat_info(wlr_keyboard, 25, 600);

    keyboard->modifiers.notify = keyboard_handle_modifiers;
    wl_signal_add(&wlr_keyboard->events.modifiers, &keyboard->modifiers);
    keyboard->key.notify = keyboard_handle_key;
    wl_signal_add(&wlr_keyboard->events.key, &keyboard->key);
    keyboard->destroy.notify = keyboard_handle_destroy;
    wl_signal_add(&device->events.destroy, &keyboard->destroy);

    wlr_seat_set_keyboard(server->seat, wlr_keyboard);

    wl_list_insert(&server->keyboards, &keyboard->link);
}

static void server_new_pointer(struct absinthe_server *server, struct wlr_input_device *device)
{
    wlr_cursor_attach_input_device(server->cursor, device);
}

void server_new_input(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, new_input);
    struct wlr_input_device *device = data;
    switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
        server_new_keyboard(server, device);
        break;
    case WLR_INPUT_DEVICE_POINTER:
        server_new_pointer(server, device);
    default:
        break;
    }

    uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
    if (!wl_list_empty(&server->keyboards)) {
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    wlr_seat_set_capabilities(server->seat, caps);
}
