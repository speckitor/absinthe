#include <stdlib.h>

#include <wayland-server-core.h>

#include "types.h"

void xdg_toplevel_map(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, map);

    wl_list_insert(&toplevel->server->toplevels, &toplevel->link);
}

void xdg_toplevel_unmap(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, unmap);

    wl_list_remove(&toplevel->link);
}

void xdg_toplevel_commit(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, commit);

    if (toplevel->xdg_toplevel->base->initial_commit) {
        wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, 0, 0);
    }
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

void xdg_toplevel_request_move(struct wl_listener *listener, void *data) {}
void xdg_toplevel_request_resize(struct wl_listener *listener, void *data) {}

void xdg_toplevel_request_maximize(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, request_maximize);
    if (toplevel->xdg_toplevel->base->initialized) {
        wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
    }
}

void xdg_toplevel_request_fullscreen(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, request_fullscreen);
    if (toplevel->xdg_toplevel->base->initialized) {
        wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
    }
}
