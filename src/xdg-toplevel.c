#include <stdlib.h>

#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "types.h"
#include "xdg-decoration.h"
#include "absinthe-toplevel.h"

void xdg_toplevel_commit(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, commit);

    if (toplevel->xdg_toplevel->base->initial_commit) {
        // Let client set the preferred size
        wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, 0, 0);

        // Forse server side decoration mode
        if (toplevel->decoration)
            xdg_decoration_request_mode(&toplevel->decoration_request_mode, toplevel->decoration);
    } else {
        // Check for size because we did't set it on initial commit
        if (toplevel->geometry.width == 0 || toplevel->geometry.height == 0) {
            int32_t bw = toplevel->border_width;
            toplevel->geometry.width = toplevel->xdg_toplevel->base->geometry.width + 2 * bw;
            toplevel->geometry.height = toplevel->xdg_toplevel->base->geometry.height + 2 * bw;
        }

        absinthe_toplevel_set_position(toplevel, toplevel->geometry.x, toplevel->geometry.y);
        absinthe_toplevel_update_borders_geometry(toplevel);
        toplevel->performing_resize = false;
    }
}

void xdg_toplevel_map(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, map);

    for (int i = 0; i < 4; ++i) {
        toplevel->border[i] = wlr_scene_rect_create(toplevel->scene_tree, 0, 0, unfocused_border_color);
        toplevel->border[i]->node.data = toplevel;
    }

    toplevel->border_width = ABSINTHE_WINDOW_BORDER_WIDTH;

    absinthe_toplevel_set_border_color(toplevel, unfocused_border_color);
    absinthe_toplevel_update_borders_geometry(toplevel);

    toplevel->output = toplevel->server->focused_output;
    toplevel->fullscreen = false;

    wl_list_insert(&toplevel->server->toplevels, &toplevel->link);
    wl_list_insert(&toplevel->server->focus_stack, &toplevel->flink);
}

void xdg_toplevel_unmap(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, unmap);

    wlr_scene_node_destroy(&toplevel->scene_tree->node);
    
    wl_list_remove(&toplevel->link);
}

void xdg_toplevel_destroy(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, destroy);

    wl_list_remove(&toplevel->map.link);
    wl_list_remove(&toplevel->unmap.link);
    wl_list_remove(&toplevel->commit.link);
    wl_list_remove(&toplevel->destroy.link);
    wl_list_remove(&toplevel->request_move.link);
    wl_list_remove(&toplevel->request_resize.link);
    wl_list_remove(&toplevel->request_maximize.link);
    wl_list_remove(&toplevel->request_fullscreen.link);

    free(toplevel);
}

void xdg_toplevel_request_move(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, request_maximize);
    if (toplevel->xdg_toplevel->base->initialized)
        wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
}

void xdg_toplevel_request_resize(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, request_maximize);
    if (toplevel->xdg_toplevel->base->initialized)
        wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
}

void xdg_toplevel_request_maximize(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, request_maximize);
    if (toplevel->xdg_toplevel->base->initialized)
        wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
}

void xdg_toplevel_request_fullscreen(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, request_fullscreen);
    absinthe_toplevel_set_fullscreen(toplevel, toplevel->xdg_toplevel->requested.fullscreen);
}
