#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "types.h"
#include "absinthe-toplevel.h"

void reset_cursor_mode(struct absinthe_server *server)
{
    server->cursor_mode = ABSINTHE_CURSOR_PASSTHROUGH;
    server->grabbed_toplevel = NULL;
}

static void process_cursor_move(struct absinthe_server *server) {
    struct absinthe_toplevel *toplevel = server->grabbed_toplevel;

    if (!toplevel) return;

    uint32_t new_x, new_y;
    new_x = server->cursor->x - server->grab_x + server->grabbed_geometry.x;
    new_y = server->cursor->y - server->grab_y + server->grabbed_geometry.y;
    wlr_scene_node_set_position(&toplevel->scene_tree->node, new_x, new_y);
}

static void process_cursor_resize(struct absinthe_server *server) {
    struct absinthe_toplevel *toplevel = server->grabbed_toplevel;

    if (!toplevel) return;

    int32_t new_x, new_y, new_width, new_height;
    new_x = server->grabbed_geometry.x;
    new_y = server->grabbed_geometry.y;
    new_width = server->grabbed_geometry.width;
    new_height = server->grabbed_geometry.height;

    switch (server->cursor_resize_corner) {
    case ABSINTHE_CURSOR_RESIZE_CORNER_TOP_LEFT:
        new_x += server->cursor->x - server->grab_x;
        new_y += server->cursor->y - server->grab_y;
        new_width -= server->cursor->x - server->grab_x;
        new_height -= server->cursor->y - server->grab_y;
        break;
    case ABSINTHE_CURSOR_RESIZE_CORNER_TOP_RIGHT:
        new_y += server->cursor->y - server->grab_y;
        new_width += server->cursor->x - server->grab_x;
        new_height -= server->cursor->y - server->grab_y;
        break;
    case ABSINTHE_CURSOR_RESIZE_CORNER_BOTTOM_LEFT:
        new_x += server->cursor->x - server->grab_x;
        new_width -= server->cursor->x - server->grab_x;
        new_height += server->cursor->y - server->grab_y;
        break;
    case ABSINTHE_CURSOR_RESIZE_CORNER_BOTTOM_RIGHT:
        new_width += server->cursor->x - server->grab_x;
        new_height += server->cursor->y - server->grab_y;
        break;
    default: // unreachable
        break;
    }

    if (new_width > 0 && new_height > 0) {
        toplevel->geometry.x = new_x;
        toplevel->geometry.y = new_y;

        absinthe_toplevel_set_size(toplevel, new_width, new_height);
    }
}

void process_cursor_motion(struct absinthe_server *server, uint32_t time)
{
    if (server->cursor_mode == ABSINTHE_CURSOR_MOVE) {
        process_cursor_move(server);
        return;
    } else if (server->cursor_mode == ABSINTHE_CURSOR_RESIZE) {
        process_cursor_resize(server);
        return;
    }

    double sx, sy;
    struct wlr_seat *seat = server->seat;
    struct wlr_surface *surface = NULL;
    struct absinthe_toplevel *toplevel = absinthe_toplevel_at(server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);
    if (!toplevel) {
        wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
    }
    if (surface) {
        wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
        wlr_seat_pointer_notify_motion(seat, time, sx, sy);
    } else {
        wlr_seat_pointer_clear_focus(seat);
    }
}
