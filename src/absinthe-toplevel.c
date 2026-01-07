#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "types.h"
#include "xdg-toplevel.h"

struct absinthe_toplevel *absinthe_toplevel_at(struct absinthe_server *server, double lx, double ly, struct wlr_surface **surface, double *sx, double *sy)
{
    struct wlr_scene_node *node = wlr_scene_node_at(&server->scene->tree.node, lx, ly, sx, sy);
    if (!node || node->type != WLR_SCENE_NODE_BUFFER) {
        return NULL;
    }

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
}

void absinthe_toplevel_set_position(struct absinthe_toplevel *toplevel, int32_t x, int32_t y)
{
    wlr_scene_node_set_position(&toplevel->scene_tree->node, x, y);
}

void absinthe_toplevel_set_size(struct absinthe_toplevel *toplevel, int32_t width, int32_t height)
{
    int bw = ABSINTHE_BORDER_WIDTH;

    if (width < 0 || height < 0)
        return;

    toplevel->geometry.width = width;
    toplevel->geometry.height = height;

    wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, width, height);
    wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
}

void absinthe_toplevel_set_border_color(struct absinthe_toplevel *toplevel, const float color[4])
{
    for (int i = 0; i < 4; ++i) {
        wlr_scene_rect_set_color(toplevel->border[i], color);
    }
}

void absinthe_toplevel_update_borders_geometry(struct absinthe_toplevel *toplevel)
{
    int bw = ABSINTHE_BORDER_WIDTH;

    if (toplevel->geometry.width - 2 * bw < 0 || toplevel->geometry.height - 2 * bw < 0)
        return;

	wlr_scene_node_set_position(&toplevel->scene_tree->node, toplevel->geometry.x, toplevel->geometry.y);
	wlr_scene_node_set_position(&toplevel->scene_surface->node, bw, bw);

    wlr_scene_rect_set_size(toplevel->border[0], toplevel->geometry.width - 2 * bw, bw);
    wlr_scene_rect_set_size(toplevel->border[1], toplevel->geometry.width - 2 * bw, bw);
    wlr_scene_rect_set_size(toplevel->border[2], bw, toplevel->geometry.height);
    wlr_scene_rect_set_size(toplevel->border[3], bw, toplevel->geometry.height);

	wlr_scene_node_set_position(&toplevel->border[0]->node, bw, 0);
	wlr_scene_node_set_position(&toplevel->border[1]->node, bw, toplevel->geometry.height - bw);
	wlr_scene_node_set_position(&toplevel->border[2]->node, 0, 0);
	wlr_scene_node_set_position(&toplevel->border[3]->node, toplevel->geometry.width - bw, 0);
}
