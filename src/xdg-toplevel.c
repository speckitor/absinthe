#include <stdlib.h>
#include <assert.h>

#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "types.h"
#include "absinthe-toplevel.h"
#include "layout.h"
#include "output.h"
#include "xdg-decoration.h"

void xdg_toplevel_commit(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, commit);

    if (toplevel->toplevel.xdg->base->initial_commit) {
        // Forse server side decoration mode
        if (toplevel->decoration)
            xdg_decoration_request_mode(&toplevel->decoration_request_mode, toplevel->decoration);

        // Let toplevel set preferred size
        // wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, 0, 0);
    } else {
        // Check for size because we did't set it on initial commit
        int32_t borders_width = 2 * toplevel->border_width;
        toplevel->geometry.width = toplevel->toplevel.xdg->base->geometry.width + borders_width;
        toplevel->geometry.height = toplevel->toplevel.xdg->base->geometry.height + borders_width;

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

    toplevel->border_width = ABSINTHE_TOPLEVEL_BORDER_WIDTH;

    update_focused_output(toplevel->server);
    toplevel->output = toplevel->server->focused_output;
    toplevel->fullscreen = false;

    wl_list_insert(&toplevel->server->toplevels, &toplevel->link);
    wl_list_insert(&toplevel->server->focus_stack, &toplevel->flink);

    layout_arrange(toplevel->output);
}

void xdg_toplevel_unmap(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, unmap);

    wlr_scene_node_destroy(&toplevel->scene_tree->node);

    wl_list_remove(&toplevel->link);
    wl_list_remove(&toplevel->flink);

    layout_arrange(toplevel->output);
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
    if (toplevel->toplevel.xdg->base->initialized)
        wlr_xdg_surface_schedule_configure(toplevel->toplevel.xdg->base);
}

void xdg_toplevel_request_resize(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, request_maximize);
    if (toplevel->toplevel.xdg->base->initialized)
        wlr_xdg_surface_schedule_configure(toplevel->toplevel.xdg->base);
}

void xdg_toplevel_request_maximize(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, request_maximize);
    if (toplevel->toplevel.xdg->base->initialized)
        wlr_xdg_surface_schedule_configure(toplevel->toplevel.xdg->base);
}

void xdg_toplevel_request_fullscreen(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, request_fullscreen);
    absinthe_toplevel_set_fullscreen(toplevel, toplevel->toplevel.xdg->requested.fullscreen);
}
