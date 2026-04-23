#include <stdlib.h>

#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "types.h"
#include "absinthe-toplevel.h"
#include "output.h"
#include "layout.h"

bool absinthe_toplevel_is_x11(struct absinthe_toplevel *toplevel)
{
#ifdef XWAYLAND
    return toplevel->type == ABSINTHE_TOPLEVEL_X11;
#endif
    return false;
}

bool absinthe_toplevel_is_unmanaged(struct absinthe_toplevel *toplevel)
{
#ifdef XWAYLAND
    if (absinthe_toplevel_is_x11(toplevel))
        return toplevel->toplevel.x11->override_redirect;
#endif
    return false;
}

void absinthe_toplevel_update_geometry(struct absinthe_toplevel *toplevel)
{
#ifdef XWAYLAND
    if (absinthe_toplevel_is_x11(toplevel)) {
            toplevel->geometry.x = toplevel->toplevel.x11->x;
            toplevel->geometry.y = toplevel->toplevel.x11->y;
            toplevel->geometry.width = toplevel->toplevel.x11->width;
            toplevel->geometry.height = toplevel->toplevel.x11->height;
    } else 
#endif
    {
        toplevel->geometry = toplevel->toplevel.xdg->base->geometry;
    }
}

struct wlr_surface *absinthe_toplevel_surface(struct absinthe_toplevel *toplevel)
{
#ifdef XWAYLAND
    if (absinthe_toplevel_is_x11(toplevel))
        return toplevel->toplevel.x11->surface;
#endif
    return toplevel->toplevel.xdg->base->surface;
}

void absinthe_toplevel_map(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, map);

    toplevel->scene_tree = wlr_scene_tree_create(&toplevel->server->scene->tree);
    toplevel->scene_tree->node.data = toplevel;
    wlr_scene_node_set_enabled(&toplevel->scene_tree->node, false);

    toplevel->tiled = true;

    toplevel->border_width = absinthe_toplevel_is_unmanaged(toplevel)
        ? 0
        : ABSINTHE_TOPLEVEL_BORDER_WIDTH;

#ifdef XWAYLAND
    if (absinthe_toplevel_is_x11(toplevel)) {
        toplevel->scene_surface = wlr_scene_subsurface_tree_create(toplevel->scene_tree, toplevel->toplevel.x11->surface);
    } else
#endif
    {
        toplevel->scene_surface = wlr_scene_subsurface_tree_create(toplevel->scene_tree, toplevel->toplevel.xdg->base->surface);
    }
    toplevel->scene_surface->node.data = toplevel;

    absinthe_toplevel_update_geometry(toplevel);
    toplevel->border_width = absinthe_toplevel_is_unmanaged(toplevel)
        ? 0
        : ABSINTHE_TOPLEVEL_BORDER_WIDTH;

    for (int i = 0; i < 4; ++i) {
        toplevel->border[i] = wlr_scene_rect_create(toplevel->scene_tree, 0, 0, unfocused_border_color);
        toplevel->border[i]->node.data = toplevel;
    }

    update_focused_output(toplevel->server);
    toplevel->output = toplevel->server->focused_output;
    toplevel->fullscreen = false;

    wl_list_insert(&toplevel->server->toplevels, &toplevel->link);
    wl_list_insert(&toplevel->server->focus_stack, &toplevel->flink);

    layout_arrange(toplevel->output);
}

void absinthe_toplevel_unmap(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, unmap);

    if (toplevel == toplevel->server->focused_toplevel)
        toplevel->server->focused_toplevel = NULL;

    wl_list_remove(&toplevel->link);
    wl_list_remove(&toplevel->flink);

    wlr_scene_node_destroy(&toplevel->scene_tree->node);

    layout_arrange(toplevel->output);
}

void absinthe_toplevel_destroy(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, destroy);

#ifdef XWAYLAND
    if (absinthe_toplevel_is_x11(toplevel)) {
        wl_list_remove(&toplevel->xwayland_activate.link);
        wl_list_remove(&toplevel->xwayland_associate.link);
        wl_list_remove(&toplevel->xwayland_dissociate.link);
        wl_list_remove(&toplevel->xwayland_configure.link);
        wl_list_remove(&toplevel->xwayland_set_hints.link);
    } else
#endif
    {
        wl_list_remove(&toplevel->map.link);
        wl_list_remove(&toplevel->unmap.link);
        wl_list_remove(&toplevel->commit.link);
        wl_list_remove(&toplevel->request_move.link);
        wl_list_remove(&toplevel->request_resize.link);
    }

    wl_list_remove(&toplevel->destroy.link);
    wl_list_remove(&toplevel->request_maximize.link);
    wl_list_remove(&toplevel->request_fullscreen.link);

    free(toplevel);
}

void absinthe_toplevel_request_move(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, request_maximize);
    if (toplevel->toplevel.xdg->base->initialized)
        wlr_xdg_surface_schedule_configure(toplevel->toplevel.xdg->base);
}

void absinthe_toplevel_request_resize(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, request_maximize);
    if (toplevel->toplevel.xdg->base->initialized)
        wlr_xdg_surface_schedule_configure(toplevel->toplevel.xdg->base);
}

void absinthe_toplevel_request_maximize(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, request_maximize);
    if (toplevel->toplevel.xdg->base->initialized)
        wlr_xdg_surface_schedule_configure(toplevel->toplevel.xdg->base);
}

void absinthe_toplevel_request_fullscreen(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, request_fullscreen);
    absinthe_toplevel_set_fullscreen(toplevel, toplevel->toplevel.xdg->requested.fullscreen);
}

struct absinthe_toplevel *absinthe_toplevel_at(struct absinthe_server *server, double lx, double ly, struct wlr_surface **surface, double *sx, double *sy)
{
    struct wlr_scene_node *node = wlr_scene_node_at(&server->scene->tree.node, lx, ly, sx, sy);
    if (!node) {
        return NULL;
    }

    switch (node->type) {
    case WLR_SCENE_NODE_BUFFER:
        struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
        struct wlr_scene_surface *scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);
        if (!scene_surface) {
            return NULL;
        }

        *surface = scene_surface->surface;
        struct wlr_scene_tree *tree = node->parent;
        while (tree != NULL && tree->node.data == NULL) {
            tree = tree->node.parent;
        }
        return tree->node.data;
        break;
    case WLR_SCENE_NODE_RECT:
        return node->data;
        break;
    default:
        return NULL;
    }
}

void absinthe_toplevel_set_position(struct absinthe_toplevel *toplevel, int32_t x, int32_t y)
{
    wlr_scene_node_set_position(&toplevel->scene_tree->node, x, y);
}

void absinthe_toplevel_set_size(struct absinthe_toplevel *toplevel, int32_t width, int32_t height)
{
    if (width < 0 || height < 0)
        return;
    if (toplevel->type == ABSINTHE_TOPLEVEL_XDG) {
        toplevel->resizing = wlr_xdg_toplevel_set_size(toplevel->toplevel.xdg, width - 2 * toplevel->border_width, height - 2 * toplevel->border_width);
    }
#ifdef XWAYLAND
    else if (toplevel->type == ABSINTHE_TOPLEVEL_X11) {
        wlr_xwayland_surface_configure(toplevel->toplevel.x11,
                                       toplevel->geometry.x, toplevel->geometry.y, width - 2 * toplevel->border_width, height - 2 * toplevel->border_width);
        absinthe_toplevel_set_position(toplevel, toplevel->geometry.x, toplevel->geometry.y);
        absinthe_toplevel_update_borders_geometry(toplevel);
    }
#endif
}

void absinthe_toplevel_set_fullscreen(struct absinthe_toplevel *toplevel, bool fullscreen)
{
    if (!toplevel || toplevel->fullscreen == fullscreen)
        return;

    struct absinthe_output *output = toplevel->server->focused_output;
    toplevel->fullscreen = fullscreen;
    wlr_xdg_toplevel_set_fullscreen(toplevel->toplevel.xdg, fullscreen);

    if (fullscreen) {
        toplevel->prev_geometry = toplevel->geometry;
        toplevel->geometry = output->geometry;
        toplevel->border_width = 0;
        absinthe_toplevel_set_size(toplevel, toplevel->geometry.width, toplevel->geometry.height);
    } else {
        toplevel->geometry = toplevel->prev_geometry;
        toplevel->border_width = absinthe_toplevel_is_unmanaged(toplevel)
            ? 0
            : ABSINTHE_TOPLEVEL_BORDER_WIDTH;
        absinthe_toplevel_set_size(toplevel, toplevel->geometry.width, toplevel->geometry.height);
    }
}

void absinthe_toplevel_set_border_color(struct absinthe_toplevel *toplevel, const float color[4])
{
    for (int i = 0; i < 4; ++i) {
        wlr_scene_rect_set_color(toplevel->border[i], color);
    }
}

void absinthe_toplevel_update_borders_geometry(struct absinthe_toplevel *toplevel)
{
    int32_t border_width = toplevel->border_width;

    if (toplevel->geometry.width - 2 * border_width < 0 || toplevel->geometry.height - 2 * border_width < 0)
        return;

    wlr_scene_node_set_position(&toplevel->scene_tree->node, toplevel->geometry.x, toplevel->geometry.y);
    wlr_scene_node_set_position(&toplevel->scene_surface->node, border_width, border_width);

    wlr_scene_rect_set_size(toplevel->border[0], toplevel->geometry.width - 2 * border_width, border_width);
    wlr_scene_rect_set_size(toplevel->border[1], toplevel->geometry.width - 2 * border_width, border_width);
    wlr_scene_rect_set_size(toplevel->border[2], border_width, toplevel->geometry.height);
    wlr_scene_rect_set_size(toplevel->border[3], border_width, toplevel->geometry.height);

    wlr_scene_node_set_position(&toplevel->border[0]->node, border_width, 0);
    wlr_scene_node_set_position(&toplevel->border[1]->node, border_width, toplevel->geometry.height - border_width);
    wlr_scene_node_set_position(&toplevel->border[2]->node, 0, 0);
    wlr_scene_node_set_position(&toplevel->border[3]->node, toplevel->geometry.width - border_width, 0);
}
