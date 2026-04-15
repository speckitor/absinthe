#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "types.h"
#include "absinthe-toplevel.h"

void reset_cursor_mode(struct absinthe_server *server)
{
    server->cursor_mode = ABSINTHE_CURSOR_PASSTHROUGH;
    wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
    if (server->focused_toplevel) {
        wlr_xdg_toplevel_set_resizing(server->focused_toplevel->toplevel.xdg, false);
    }
}

static void process_cursor_move(struct absinthe_server *server) {
    struct absinthe_toplevel *toplevel = server->focused_toplevel;

    if (!toplevel)
        return;

    if (toplevel->fullscreen) {
        toplevel->prev_geometry = toplevel->geometry;
        absinthe_toplevel_set_fullscreen(toplevel, false);
    }

    uint32_t new_x, new_y;
    new_x = server->cursor->x - server->grab_x + server->grabbed_geometry.x;
    new_y = server->cursor->y - server->grab_y + server->grabbed_geometry.y;
    toplevel->geometry.x = new_x;
    toplevel->geometry.y = new_y;
    absinthe_toplevel_set_position(toplevel, new_x, new_y);
}

static void apply_resize(struct absinthe_toplevel *toplevel, struct wlr_box *new_geometry)
{
    int32_t borders_width = 2 * toplevel->border_width;

    int32_t min_width = toplevel->toplevel.xdg->current.min_width;
    int32_t min_height = toplevel->toplevel.xdg->current.min_height;

    int32_t max_width = toplevel->toplevel.xdg->current.max_width;
    int32_t max_height = toplevel->toplevel.xdg->current.max_height;

    if (max_width == 0)
        max_width = 10000;

    if (max_height == 0)
        max_height = 10000;

    if (new_geometry->width - borders_width >= min_width && new_geometry->width - borders_width <= max_width) {
        toplevel->geometry.x = new_geometry->x;
        toplevel->geometry.width = new_geometry->width;
        toplevel->performing_resize = true;
    }

    if (new_geometry->height - borders_width >= min_height && new_geometry->height - borders_width <= max_height) {
        toplevel->geometry.y = new_geometry->y;
        toplevel->geometry.height = new_geometry->height;
        toplevel->performing_resize = true;
    }

    if (toplevel->performing_resize)
        absinthe_toplevel_set_size(toplevel, toplevel->geometry.width - borders_width, toplevel->geometry.height - borders_width);
}

static void process_cursor_resize(struct absinthe_server *server) {
    struct absinthe_toplevel *toplevel = server->focused_toplevel;

    if (!toplevel)
        return;

    if (toplevel->performing_resize == true)
        return;

    if (toplevel->fullscreen)
        absinthe_toplevel_set_fullscreen(toplevel, false);

    int32_t new_x, new_y, new_width, new_height;
    new_x = server->grabbed_geometry.x;
    new_y = server->grabbed_geometry.y;
    new_width = server->grabbed_geometry.width;
    new_height = server->grabbed_geometry.height;

    int32_t dx = server->cursor->x - server->grab_x;
    int32_t dy = server->cursor->y - server->grab_y;

    if (dx == 0 && dy == 0)
        return;

    switch (server->cursor_resize_corner) {
    case ABSINTHE_CURSOR_RESIZE_CORNER_TOP_LEFT:
        new_x += dx;
        new_y += dy;
        new_width -= dx;
        new_height -= dy;
        break;
    case ABSINTHE_CURSOR_RESIZE_CORNER_TOP_RIGHT:
        new_y += dy;
        new_width += dx;
        new_height -= dy;
        break;
    case ABSINTHE_CURSOR_RESIZE_CORNER_BOTTOM_LEFT:
        new_x += dx;
        new_width -= dx;
        new_height += dy;
        break;
    case ABSINTHE_CURSOR_RESIZE_CORNER_BOTTOM_RIGHT:
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

void process_cursor_motion(struct absinthe_server *server, uint32_t time)
{
    double sx, sy;
    struct wlr_seat *seat = server->seat;
    struct wlr_surface *surface = NULL;
    struct absinthe_toplevel *toplevel = absinthe_toplevel_at(server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);

    if (server->cursor_mode == ABSINTHE_CURSOR_MOVE) {
        process_cursor_move(server);
        return;
    } else if (server->cursor_mode == ABSINTHE_CURSOR_RESIZE) {
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
