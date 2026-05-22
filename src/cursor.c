#include <wayland-server-core.h>

#include "cursor.h"
#include "layout.h"
#include "toplevel.h"
#include "types.h"

/*
 * returns surface at given cursor coordinates
 * and coordinates inside of it to process input event
 */
void client_from_coords(absn_server *server, double x, double y,
                        struct wlr_surface **rsurface,
                        absn_toplevel **rtoplevel,
                        absn_layer_surface **rlayer_surface, double *rx,
                        double *ry)
{
    struct wlr_scene_node *pnode = NULL, *node = NULL;
    struct wlr_scene_buffer *buffer = NULL;
    struct wlr_surface *surface = NULL;

    absn_layer_surface *layer_surface = NULL;
    absn_toplevel *toplevel = NULL;

    for (int i = LAYERS_COUNT - 1; i > 0; --i) {
        node = wlr_scene_node_at(&server->layers[i]->node, x, y, rx, ry);
        if (!node) {
            continue;
        }

        if (node->type == WLR_SCENE_NODE_BUFFER) {
            buffer = wlr_scene_buffer_from_node(node);
            surface = wlr_scene_surface_try_from_buffer(buffer)->surface;
        }

        for (pnode = node; pnode && !toplevel; pnode = &pnode->parent->node)
            toplevel = pnode->data;

        if (toplevel && toplevel->type == LAYER_SURFACE) {
            toplevel = NULL;
            layer_surface = pnode->data;
        }
    }

    if (rsurface)
        *rsurface = surface;
    if (rtoplevel)
        *rtoplevel = toplevel;
    if (rlayer_surface)
        *rlayer_surface = layer_surface;
}

void reset_cursor_mode(absn_server *server)
{
    server->cursor_mode = CURSOR_PASSTHROUGH;
    wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
    if (server->focused_toplevel) {
        wlr_xdg_toplevel_set_resizing(server->focused_toplevel->xdg, false);
    }
}

static void process_cursor_move(absn_server *server)
{
    struct absn_toplevel *toplevel = server->focused_toplevel;

    if (!toplevel) {
        return;
    }

    if (toplevel->fullscreen) {
        toplevel->prev_geom = toplevel->geom;
        toplevel_set_fullscreen(toplevel, false);
    }

    uint32_t new_x, new_y;
    new_x = server->cursor->x - server->grab_x + server->grab_geom.x;
    new_y = server->cursor->y - server->grab_y + server->grab_geom.y;
    toplevel_set_pos(toplevel, new_x, new_y);

    if (!toplevel->floating) {
        toplevel_set_floating(toplevel, true);
    }

    if (toplevel->output != server->focused_output) {
        toplevel->output = server->focused_output;
        toplevel->workspace = toplevel->output->workspace;
    }
}

static void apply_resize(absn_toplevel *toplevel, struct wlr_box *new_geom)
{
    if (toplevel->type == TOPLEVEL_XDG) {
        int32_t min_width = toplevel->xdg->current.min_width;
        int32_t min_height = toplevel->xdg->current.min_height;

        int32_t max_width = toplevel->xdg->current.max_width;
        int32_t max_height = toplevel->xdg->current.max_height;

        if (max_width == 0) {
            max_width = 10000;
        }

        if (max_height == 0) {
            max_height = 10000;
        }

        if (!(new_geom->width >= min_width && new_geom->width <= max_width)) {
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

static void process_cursor_resize(absn_server *server)
{
    struct absn_toplevel *toplevel = server->focused_toplevel;

    if (!toplevel) {
        return;
    }

    if (toplevel->resizing) {
        return;
    }

    if (toplevel->fullscreen) {
        toplevel_set_fullscreen(toplevel, false);
    }

    if (!toplevel->floating) {
        toplevel_set_floating(toplevel, true);
    }

    int32_t new_x, new_y, new_width, new_height;
    new_x = server->grab_geom.x;
    new_y = server->grab_geom.y;
    new_width = server->grab_geom.width;
    new_height = server->grab_geom.height;

    int32_t dx = server->cursor->x - server->grab_x;
    int32_t dy = server->cursor->y - server->grab_y;

    if (dx == 0 && dy == 0) {
        return;
    }

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

void process_cursor_motion(absn_server *server, uint32_t time)
{
    double x, y;
    struct wlr_surface *surface = NULL;
    client_from_coords(server, server->cursor->x, server->cursor->y, &surface,
                       NULL, NULL, &x, &y);
    struct wlr_seat *seat = server->seat;

    if (server->cursor_mode == CURSOR_MOVE) {
        process_cursor_move(server);
        return;
    } else if (server->cursor_mode == CURSOR_RESIZE) {
        process_cursor_resize(server);
        return;
    }

    if (surface) {
        wlr_seat_pointer_notify_enter(seat, surface, x, y);
        wlr_seat_pointer_notify_motion(seat, time, x, y);
    } else {
        wlr_seat_pointer_clear_focus(seat);
    }
}
