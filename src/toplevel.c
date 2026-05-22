#include <wayland-server-core.h>

#include "config.h"
#include "layers.h"
#include "layout.h"
#include "toplevel.h"
#include "types.h"
#include "xdg-shell-protocol.h"

/*
 * if toplevel is unmanaged we should not
 * move or resize it and just draw it
 * where it wants
 */
bool toplevel_is_unmanaged(absn_toplevel *toplevel)
{
#ifdef XWAYLAND
    if (toplevel->type == TOPLEVEL_X11) {
        return toplevel->xw->override_redirect;
    }
#endif
    return false;
}

bool toplevel_wants_focus(absn_toplevel *toplevel)
{
#ifdef XWAYLAND
    return toplevel_is_unmanaged(toplevel) &&
           wlr_xwayland_surface_override_redirect_wants_focus(toplevel->xw) &&
           wlr_xwayland_surface_icccm_input_model(toplevel->xw) !=
               WLR_ICCCM_INPUT_MODEL_NONE;
#endif
}

/* used only to get initial window size */
void toplevel_get_geom(absn_toplevel *toplevel)
{
#ifdef XWAYLAND
    if (toplevel->type == TOPLEVEL_X11) {
        toplevel->geom.x = toplevel->xw->x;
        toplevel->geom.y = toplevel->xw->y;
        toplevel->geom.width = toplevel->xw->width;
        toplevel->geom.height = toplevel->xw->height;
    } else
#endif
    {
        toplevel->geom = toplevel->xdg->base->geometry;
    }
}

void toplevel_update_borders_geom(absn_toplevel *toplevel)
{
    int32_t bw = toplevel->bw;

    if (toplevel->geom.width - 2 * bw < 0 ||
        toplevel->geom.height - 2 * bw < 0) {
        return;
    }

    wlr_scene_node_set_position(&toplevel->scene_tree->node, toplevel->geom.x,
                                toplevel->geom.y);
    wlr_scene_node_set_position(&toplevel->scene_surface->node, bw, bw);

    wlr_scene_rect_set_size(toplevel->border[0], toplevel->geom.width - 2 * bw,
                            bw);
    wlr_scene_rect_set_size(toplevel->border[1], toplevel->geom.width - 2 * bw,
                            bw);
    wlr_scene_rect_set_size(toplevel->border[2], bw, toplevel->geom.height);
    wlr_scene_rect_set_size(toplevel->border[3], bw, toplevel->geom.height);

    wlr_scene_node_set_position(&toplevel->border[0]->node, bw, 0);
    wlr_scene_node_set_position(&toplevel->border[1]->node, bw,
                                toplevel->geom.height - bw);
    wlr_scene_node_set_position(&toplevel->border[2]->node, 0, 0);
    wlr_scene_node_set_position(&toplevel->border[3]->node,
                                toplevel->geom.width - bw, 0);
}

void toplevel_set_pos(absn_toplevel *toplevel, int32_t x, int32_t y)
{
    toplevel->geom.x = x;
    toplevel->geom.y = y;
    wlr_scene_node_set_position(&toplevel->scene_tree->node, x, y);
}

static struct wlr_scene_buffer *find_buffer(struct wlr_scene_tree *tree)
{
    struct wlr_scene_node *node;

    wl_list_for_each(node, &tree->children, link) {

        if (node->type == WLR_SCENE_NODE_BUFFER) {
            return wlr_scene_buffer_from_node(node);
        }

        if (node->type == WLR_SCENE_NODE_TREE) {
            struct wlr_scene_buffer *buffer =
                find_buffer(wlr_scene_tree_from_node(node));

            if (buffer) {
                return buffer;
            }
        }
    }

    return NULL;
}
void toplevel_set_size(absn_toplevel *toplevel, int32_t width, int32_t height)
{
    if (width <= 2 * toplevel->bw || height <= 2 * toplevel->bw ||
        (width == toplevel->geom.width && height == toplevel->geom.height)) {
        return;
    }
    toplevel->geom.width = width;
    toplevel->geom.height = height;

    struct wlr_scene_buffer *buffer = find_buffer(toplevel->scene_tree);
    if (buffer) {
        wlr_scene_buffer_set_dest_size(buffer, width, height);
    }

    int32_t bw = toplevel->bw;
    if (toplevel->type == TOPLEVEL_XDG) {
        if (wl_resource_get_version(toplevel->xdg->resource) >=
            XDG_TOPLEVEL_CONFIGURE_BOUNDS_SINCE_VERSION) {
            wlr_xdg_toplevel_set_bounds(toplevel->xdg, width, height);
        }
        wlr_xdg_toplevel_set_size(toplevel->xdg, width - 2 * bw, height - 2 * bw);
    }
#ifdef XWAYLAND
    else if (toplevel->type == TOPLEVEL_X11) {
        wlr_xwayland_surface_configure(toplevel->xw, toplevel->geom.x,
                                       toplevel->geom.y, width - 2 * bw,
                                       height - 2 * toplevel->bw);
        /* manually update position */
        toplevel_set_pos(toplevel, toplevel->geom.x, toplevel->geom.y);
    }
#endif

    struct wlr_box clip = {
        .x = 0,
        .y = 0,
        .width = width - 2 * bw,
        .height = height - 2 * bw,
    };

    wlr_scene_subsurface_tree_set_clip(&toplevel->scene_surface->node, &clip);

    toplevel_update_borders_geom(toplevel);
}

void toplevel_set_geom(absn_toplevel *toplevel, struct wlr_box *geom)
{
    toplevel_set_size(toplevel, geom->width, geom->height);
    toplevel_set_pos(toplevel, geom->x, geom->y);
}

void toplevel_set_floating(absn_toplevel *toplevel, bool floating)
{
    if (!toplevel || toplevel->floating == floating) {
        return;
    }

    toplevel->floating = floating;

    if (floating) {
        wlr_scene_node_reparent(&toplevel->scene_tree->node,
                                toplevel->server->layers[LAYER_FLOAT]);
    } else {
        wlr_scene_node_reparent(&toplevel->scene_tree->node,
                                toplevel->server->layers[LAYER_TILE]);
    }

    layout_arrange(toplevel->output);
}

void toplevel_set_fullscreen(absn_toplevel *toplevel, bool fullscreen)
{
    if (!toplevel || toplevel->fullscreen == fullscreen) {
        return;
    }

    absn_output *output = toplevel->server->focused_output;
    toplevel->fullscreen = fullscreen;
    if (toplevel->type == TOPLEVEL_XDG) {
        wlr_xdg_toplevel_set_fullscreen(toplevel->xdg, fullscreen);
    }

    if (fullscreen) {
        toplevel->prev_geom = toplevel->geom;
        toplevel->bw = 0;
        toplevel_set_geom(toplevel, &output->geom);

        wlr_scene_node_reparent(&toplevel->scene_tree->node,
                                toplevel->server->layers[LAYER_FULLSCREEN]);
    } else {
        toplevel->bw = toplevel_is_unmanaged(toplevel) ? 0 : TOPLEVEL_BW;
        toplevel_set_geom(toplevel, &toplevel->prev_geom);

        if (toplevel->floating) {
            wlr_scene_node_reparent(&toplevel->scene_tree->node,
                                    toplevel->server->layers[LAYER_FLOAT]);
        } else {
            wlr_scene_node_reparent(&toplevel->scene_tree->node,
                                    toplevel->server->layers[LAYER_TILE]);
        }
    }

    toplevel_update_borders_geom(toplevel);

    layout_arrange(toplevel->output);
    layers_arrange(toplevel->output);
}

void toplevel_set_border_color(absn_toplevel *toplevel, const float color[4])
{
    if (!toplevel) {
        return;
    }

    for (int i = 0; i < 4; ++i) {
        wlr_scene_rect_set_color(toplevel->border[i], color);
    }
}
