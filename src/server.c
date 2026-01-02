#include <assert.h>
#include <stdlib.h>

#include <wlr/util/log.h>

#include "types.h"
#include "output.h"
#include "xdg-toplevel.h"
#include "xdg-popup.h"
#include "absinthe-toplevel.h"
#include "focus.h"
#include "keyboard.h"
#include "cursor.h"

void server_new_output(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;

    wlr_output_init_render(wlr_output, server->allocator, server->renderer);

    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);

    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    if (mode) {
        wlr_output_state_set_mode(&state, mode);
    }

    wlr_output_commit_state(wlr_output, &state);
    wlr_output_state_finish(&state);

    struct absinthe_output *output = calloc(1, sizeof(*output));
    output->wlr_output = wlr_output;
    output->server = server;

    output->frame.notify = output_frame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);

    output->request_state.notify = output_request_state;
    wl_signal_add(&wlr_output->events.request_state, &output->request_state);

    output->destroy.notify = output_destroy;
    wl_signal_add(&wlr_output->events.destroy, &output->destroy);

    struct wlr_output_layout_output *l_layout = wlr_output_layout_add_auto(server->output_layout, output->wlr_output);
    struct wlr_scene_output *scene_output = wlr_scene_output_create(server->scene, wlr_output);
    wlr_scene_output_layout_add_output(server->scene_layout, l_layout, scene_output);
}

void server_new_xdg_toplevel(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, new_xdg_toplevel);
    struct wlr_xdg_toplevel *xdg_toplevel = data;

    struct absinthe_toplevel *toplevel = calloc(1, sizeof(*toplevel));
    toplevel->server = server;
    toplevel->xdg_toplevel = xdg_toplevel;
    toplevel->scene_tree = wlr_scene_xdg_surface_create(&toplevel->server->scene->tree, xdg_toplevel->base);
    toplevel->scene_tree->node.data = toplevel;
    xdg_toplevel->base->data = toplevel->scene_tree;

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

    struct absinthe_popup *popup = calloc(1, sizeof(*popup));
    popup->xdg_popup = xdg_popup;

    struct wlr_xdg_surface *parent = wlr_xdg_surface_try_from_wlr_surface(xdg_popup->parent);
    assert(parent != NULL);
    struct wlr_scene_tree *parent_tree = parent->data;
    xdg_popup->base->data = wlr_scene_xdg_surface_create(parent_tree, xdg_popup->base);

    popup->commit.notify = xdg_popup_commit;
    wl_signal_add(&xdg_popup->base->surface->events.commit, &popup->commit);

    popup->destroy.notify = xdg_popup_destroy;
    wl_signal_add(&xdg_popup->base->surface->events.destroy, &popup->destroy);
}

void server_cursor_motion(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, cursor_motion);
    struct wlr_pointer_motion_event *event = data;
    wlr_cursor_move(server->cursor, &event->pointer->base, event->delta_x, event->delta_y);
    process_cursor_motion(server, event->time_msec);
}

void server_cursor_motion_absolute(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, cursor_motion_absolute);
    struct wlr_pointer_motion_absolute_event *event = data;
    wlr_cursor_warp_absolute(server->cursor, &event->pointer->base, event->x, event->y);
    process_cursor_motion(server, event->time_msec);
}

void server_cursor_button(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, cursor_button);
    struct wlr_pointer_button_event *event = data;

    wlr_seat_pointer_notify_button(server->seat, event->time_msec, event->button, event->state);
    if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
        reset_cursor_mode(server);
    } else {
        double sx, sy;
        struct wlr_surface *surface = NULL;
        struct absinthe_toplevel *toplevel = absinthe_toplevel_at(server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);
        focus_toplevel(toplevel);

        struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(server->seat);
        uint32_t mods = wlr_keyboard_get_modifiers(keyboard);
        if (mods & ABSINTHE_CURSOR_MOD) {
            if (event->button == ABSINTHE_CURSOR_MOVE_BUTTON) {
                server->cursor_mode = ABSINTHE_CURSOR_MOVE;
            } else if (event->button == ABSINTHE_CURSOR_RESIZE_BUTTON) {
                server->cursor_mode = ABSINTHE_CURSOR_RESIZE;
            }
        }
        if (toplevel) {
            server->grab_x = server->cursor->x;
            server->grab_y = server->cursor->y;

            int lx, ly;
            wlr_scene_node_coords(&toplevel->scene_tree->node, &lx, &ly);
            server->grabbed_toplevel_x = lx;
            server->grabbed_toplevel_y = ly;
            server->grabbed_toplevel_width = toplevel->xdg_toplevel->base->geometry.width;
            server->grabbed_toplevel_height = toplevel->xdg_toplevel->base->geometry.height;
            wlr_log(WLR_DEBUG, "%d, %d", server->grabbed_toplevel_width, server->grabbed_toplevel_width);
            server->grabbed_toplevel = toplevel;

            int width = toplevel->xdg_toplevel->base->geometry.width;
            int height = toplevel->xdg_toplevel->base->geometry.height;

            if ((int)server->grab_x > (lx + width / 2) && (int)server->grab_y > (ly + height / 2)) {
                server->cursor_resize_corner = ABSINTHE_CURSOR_RESIZE_CORNER_BOTTOM_RIGHT;
            } else if ((int)server->grab_x < (lx + width / 2) && (int)server->grab_y > (ly + height / 2)) {
                server->cursor_resize_corner = ABSINTHE_CURSOR_RESIZE_CORNER_BOTTOM_LEFT;
            } else if ((int)server->grab_x > (lx + width / 2) && (int)server->grab_y < (ly + height / 2)) {
                server->cursor_resize_corner = ABSINTHE_CURSOR_RESIZE_CORNER_TOP_RIGHT;
            } else {
                server->cursor_resize_corner = ABSINTHE_CURSOR_RESIZE_CORNER_TOP_LEFT;
            }
        }
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
    struct absinthe_keyboard *keyboard = calloc(1, sizeof(*keyboard));
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
