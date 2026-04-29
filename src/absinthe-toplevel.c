#include <stdlib.h>
#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "absinthe-toplevel.h"
#include "focus.h"
#include "layout.h"
#include "output.h"
#include "types.h"
#include "xdg-shell-protocol.h"

struct absinthe_toplevel *absinthe_toplevel_at(struct absinthe_server *server, double lx, double ly,
					       struct wlr_surface **surface, double *sx, double *sy)
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

bool absinthe_toplevel_is_unmanaged(struct absinthe_toplevel *toplevel)
{
#ifdef XWAYLAND
	if (toplevel->type == ABSINTHE_TOPLEVEL_X11)
		return toplevel->xwayland_surface->override_redirect;
#endif
	return false;
}

void absinthe_toplevel_update_geometry(struct absinthe_toplevel *toplevel)
{
#ifdef XWAYLAND
	if (toplevel->type == ABSINTHE_TOPLEVEL_X11) {
		toplevel->geometry.x = toplevel->xwayland_surface->x;
		toplevel->geometry.y = toplevel->xwayland_surface->y;
		toplevel->geometry.width = toplevel->xwayland_surface->width;
		toplevel->geometry.height = toplevel->xwayland_surface->height;
	} else
#endif
	{
		toplevel->geometry = toplevel->xdg_toplevel->base->geometry;
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

void absinthe_toplevel_set_position(struct absinthe_toplevel *toplevel, int32_t x, int32_t y)
{
	toplevel->geometry.x = x;
	toplevel->geometry.y = y;
	wlr_scene_node_set_position(&toplevel->scene_tree->node, x, y);
}

void absinthe_toplevel_set_size(struct absinthe_toplevel *toplevel, int32_t width, int32_t height)
{
	if (width <= 2 * toplevel->border_width || height <= 2 * toplevel->border_width ||
	    (width == toplevel->geometry.width && height == toplevel->geometry.height))
		return;

	toplevel->geometry.width = width;
	toplevel->geometry.height = height;

	if (toplevel->type == ABSINTHE_TOPLEVEL_XDG) {
		if (wl_resource_get_version(toplevel->xdg_toplevel->resource) >=
		    XDG_TOPLEVEL_CONFIGURE_BOUNDS_SINCE_VERSION)
			wlr_xdg_toplevel_set_bounds(toplevel->xdg_toplevel, width, height);
		toplevel->resizing = wlr_xdg_toplevel_set_size(
		    toplevel->xdg_toplevel, width - 2 * toplevel->border_width, height - 2 * toplevel->border_width);
	}
#ifdef XWAYLAND
	else if (toplevel->type == ABSINTHE_TOPLEVEL_X11) {
		wlr_xwayland_surface_configure(toplevel->xwayland_surface, toplevel->geometry.x, toplevel->geometry.y,
					       width - 2 * toplevel->border_width, height - 2 * toplevel->border_width);
		absinthe_toplevel_set_position(toplevel, toplevel->geometry.x + toplevel->border_width,
					       toplevel->geometry.y + toplevel->border_width);
	}
#endif

	struct wlr_box clip = {
	    .x = 0,
	    .y = 0,
	    .width = width - toplevel->border_width,
	    .height = height - toplevel->border_width,
	};
	wlr_scene_subsurface_tree_set_clip(&toplevel->scene_surface->node, &clip);

	absinthe_toplevel_update_borders_geometry(toplevel);
}

void absinthe_toplevel_set_fullscreen(struct absinthe_toplevel *toplevel, bool fullscreen)
{
	if (!toplevel || toplevel->fullscreen == fullscreen)
		return;

	struct absinthe_output *output = toplevel->server->focused_output;
	toplevel->fullscreen = fullscreen;
	wlr_xdg_toplevel_set_fullscreen(toplevel->xdg_toplevel, fullscreen);

	if (fullscreen) {
		toplevel->prev_geometry = toplevel->geometry;
		toplevel->border_width = 0;
		absinthe_toplevel_set_size(toplevel, output->geometry.width, output->geometry.height);
		absinthe_toplevel_set_position(toplevel, output->geometry.x, output->geometry.y);
	} else {
		toplevel->border_width = absinthe_toplevel_is_unmanaged(toplevel) ? 0 : ABSINTHE_TOPLEVEL_BORDER_WIDTH;
		absinthe_toplevel_set_size(toplevel, toplevel->prev_geometry.width, toplevel->prev_geometry.height);
		absinthe_toplevel_set_position(toplevel, toplevel->prev_geometry.x, toplevel->prev_geometry.y);
	}

	absinthe_toplevel_update_borders_geometry(toplevel);
}

void absinthe_toplevel_set_border_color(struct absinthe_toplevel *toplevel, const float color[4])
{
	for (int i = 0; i < 4; ++i) {
		wlr_scene_rect_set_color(toplevel->border[i], color);
	}
}
