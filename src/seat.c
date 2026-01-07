#include <wayland-server-core.h>
#include <wlr/types/wlr_data_device.h>

#include "types.h"

void seat_request_cursor(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, request_cursor);
    struct wlr_seat_pointer_request_set_cursor_event *event = data;
    struct wlr_seat_client *client = server->seat->pointer_state.focused_client;

    if (client == event->seat_client)
        wlr_cursor_set_surface(server->cursor, event->surface, event->hotspot_x, event->hotspot_y);
}

void seat_pointer_focus_change(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, pointer_focus_change);
    struct wlr_seat_pointer_focus_change_event *event = data;

    if (!event->new_surface)
        wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
}

void seat_request_set_selection(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, request_set_selection);
    struct wlr_seat_request_set_selection_event *event = data;

    wlr_seat_set_selection(server->seat, event->source, event->serial);
}
