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
        int32_t cw = toplevel->xdg_toplevel->current.min_width;
        int32_t ch = toplevel->xdg_toplevel->current.min_height;

        int32_t w = MAX(cw, ABSINTHE_WINDOW_MIN_WIDTH);
        int32_t h = MAX(ch, ABSINTHE_WINDOW_MIN_HEIGHT);
        
        absinthe_toplevel_set_size(toplevel, w, h);

        if (toplevel->decoration)
            xdg_decoration_request_mode(&toplevel->decoration_request_mode, toplevel->decoration);
    } else {
        absinthe_toplevel_set_position(toplevel, toplevel->geometry.x, toplevel->geometry.y);
        absinthe_toplevel_update_borders_geometry(toplevel);
        toplevel->performing_resize = false;
    }
}

void xdg_toplevel_map(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, map);

    for (int i = 0; i < 4; ++i) {
        toplevel->border[i] = wlr_scene_rect_create(toplevel->scene_tree, 0, 0, bordercolor);
        toplevel->border[i]->node.data = toplevel;
    }

    absinthe_toplevel_set_border_color(toplevel, bordercolor);
    absinthe_toplevel_update_borders_geometry(toplevel);

    wl_list_insert(&toplevel->server->toplevels, &toplevel->link);
}

void xdg_toplevel_unmap(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, unmap);

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
    if (toplevel->xdg_toplevel->base->initialized)
        wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
}
