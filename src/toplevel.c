#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "config.h"
#include "toplevel.h"
#include "types.h"
#include "xdg-shell-protocol.h"

/*
 * returns toplevel at given cursor coordinates,
 * its surface and coordinates inside of it
 * to process input event
 */
absn_toplevel *
toplevel_at(absn_server *server, double lx, double ly,
    struct wlr_surface **surface, double *sx, double *sy)
{
	struct wlr_scene_node *node =
	    wlr_scene_node_at(&server->scene->tree.node, lx, ly, sx, sy);
	if (!node) {
		return NULL;
	}

	switch (node->type) {
	case WLR_SCENE_NODE_BUFFER:
		struct wlr_scene_buffer *scene_buffer =
		    wlr_scene_buffer_from_node(node);
		struct wlr_scene_surface *scene_surface =
		    wlr_scene_surface_try_from_buffer(scene_buffer);
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

/*
 * if toplevel is unmanaged we should not
 * move or resize it and just draw it
 * where it wants
 */
bool
toplevel_is_unmanaged(absn_toplevel *toplevel)
{
#ifdef XWAYLAND
	if (toplevel->type == TOPLEVEL_X11)
		return toplevel->xw->override_redirect;
#endif
	return false;
}

/* used only to get initial window size */
void
toplevel_get_geom(absn_toplevel *toplevel)
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

static void
toplevel_update_borders_geom(absn_toplevel *toplevel)
{
	int32_t bw = toplevel->bw;

	if (toplevel->geom.width - 2 * bw < 0 ||
	    toplevel->geom.height - 2 * bw < 0)
		return;

	wlr_scene_node_set_position(&toplevel->scene_tree->node,
	    toplevel->geom.x, toplevel->geom.y);
	wlr_scene_node_set_position(&toplevel->scene_surface->node, bw, bw);

	wlr_scene_rect_set_size(toplevel->border[0],
	    toplevel->geom.width - 2 * bw, bw);
	wlr_scene_rect_set_size(toplevel->border[1],
	    toplevel->geom.width - 2 * bw, bw);
	wlr_scene_rect_set_size(toplevel->border[2], bw, toplevel->geom.height);
	wlr_scene_rect_set_size(toplevel->border[3], bw, toplevel->geom.height);

	wlr_scene_node_set_position(&toplevel->border[0]->node, bw, 0);
	wlr_scene_node_set_position(&toplevel->border[1]->node, bw,
	    toplevel->geom.height - bw);
	wlr_scene_node_set_position(&toplevel->border[2]->node, 0, 0);
	wlr_scene_node_set_position(&toplevel->border[3]->node,
	    toplevel->geom.width - bw, 0);
}

void
toplevel_set_pos(absn_toplevel *toplevel, int32_t x, int32_t y)
{
	toplevel->geom.x = x;
	toplevel->geom.y = y;
	wlr_scene_node_set_position(&toplevel->scene_tree->node, x, y);
}

void
toplevel_set_size(absn_toplevel *toplevel, int32_t width, int32_t height)
{
	if (width <= 2 * toplevel->bw || height <= 2 * toplevel->bw ||
	    (width == toplevel->geom.width && height == toplevel->geom.height))
		return;
	toplevel->geom.width = width;
	toplevel->geom.height = height;

	toplevel_update_borders_geom(toplevel);

	struct wlr_box clip = {
		.x = 0,
		.y = 0,
		.width = width - toplevel->bw,
		.height = height - toplevel->bw,
	};

	if (toplevel->type == TOPLEVEL_XDG) {
		if (wl_resource_get_version(toplevel->xdg->resource) >=
		    XDG_TOPLEVEL_CONFIGURE_BOUNDS_SINCE_VERSION)
			wlr_xdg_toplevel_set_bounds(toplevel->xdg, width,
			    height);
		toplevel->resizing = wlr_xdg_toplevel_set_size(toplevel->xdg,
		    width - 2 * toplevel->bw, height - 2 * toplevel->bw);
	}
#ifdef XWAYLAND
	else if (toplevel->type == TOPLEVEL_X11) {
		wlr_xwayland_surface_configure(toplevel->xw, toplevel->geom.x,
		    toplevel->geom.y, width - 2 * toplevel->bw,
		    height - 2 * toplevel->bw);
		/* manually update position */
		toplevel_set_pos(toplevel, toplevel->geom.x, toplevel->geom.y);
	}
#endif

	wlr_scene_subsurface_tree_set_clip(&toplevel->scene_surface->node,
	    &clip);
}

void
toplevel_set_geom(absn_toplevel *toplevel, struct wlr_box *geom)
{
	toplevel_set_pos(toplevel, geom->x, geom->y);
	toplevel_set_size(toplevel, geom->width, geom->height);
}

void
toplevel_set_fullscreen(absn_toplevel *toplevel, bool fullscreen)
{
	if (!toplevel || toplevel->fullscreen == fullscreen)
		return;

	absn_output *output = toplevel->server->focused_output;
	toplevel->fullscreen = fullscreen;
	wlr_xdg_toplevel_set_fullscreen(toplevel->xdg, fullscreen);

	if (fullscreen) {
		toplevel->prev_geom = toplevel->geom;
		toplevel->bw = 0;
		toplevel_set_geom(toplevel, &output->geom);
	} else {
		toplevel->bw = toplevel_is_unmanaged(toplevel) ? 0 :
								 TOPLEVEL_BW;
		toplevel_set_geom(toplevel, &toplevel->prev_geom);
	}

	toplevel_update_borders_geom(toplevel);
}

void
toplevel_set_border_color(absn_toplevel *toplevel, const float color[4])
{
	if (!toplevel)
		return;
	for (int i = 0; i < 4; ++i) {
		wlr_scene_rect_set_color(toplevel->border[i], color);
	}
}
