#include <wlr/util/edges.h>

#include "absinthe-toplevel.h"
#include "focus.h"
#include "layout.h"
#include "output.h"

void absinthe_toplevel_map(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, map);

	toplevel->scene_tree = wlr_scene_tree_create(&toplevel->server->scene->tree);
	toplevel->scene_tree->node.data = toplevel;
	wlr_scene_node_set_enabled(&toplevel->scene_tree->node, absinthe_toplevel_is_unmanaged(toplevel));

	toplevel->tiled = true;

	if (toplevel->type != ABSINTHE_TOPLEVEL_X11 &&
	    wl_resource_get_version(toplevel->toplevel.xdg->resource) >= XDG_TOPLEVEL_STATE_TILED_RIGHT_SINCE_VERSION) {
		wlr_xdg_toplevel_set_tiled(toplevel->toplevel.xdg,
					   WLR_EDGE_TOP | WLR_EDGE_BOTTOM | WLR_EDGE_LEFT | WLR_EDGE_RIGHT);
	} else {
		wlr_xdg_toplevel_set_maximized(toplevel->toplevel.xdg, true);
	}

	toplevel->border_width = absinthe_toplevel_is_unmanaged(toplevel) ? 0 : ABSINTHE_TOPLEVEL_BORDER_WIDTH;

#ifdef XWAYLAND
	if (toplevel->type == ABSINTHE_TOPLEVEL_X11) {
		toplevel->scene_surface =
		    wlr_scene_subsurface_tree_create(toplevel->scene_tree, toplevel->toplevel.x11->surface);
	} else
#endif
	{
		toplevel->scene_surface =
		    wlr_scene_subsurface_tree_create(toplevel->scene_tree, toplevel->toplevel.xdg->base->surface);
	}
	toplevel->scene_surface->node.data = toplevel;

	absinthe_toplevel_update_geometry(toplevel);
	toplevel->border_width = absinthe_toplevel_is_unmanaged(toplevel) ? 0 : ABSINTHE_TOPLEVEL_BORDER_WIDTH;

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
	focus_toplevel(focus_get_topmost(toplevel->server));
}

void absinthe_toplevel_unmap(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, unmap);

	if (toplevel == toplevel->server->focused_toplevel) {
		toplevel->server->focused_toplevel = NULL;
		toplevel->server->seat->keyboard_state.focused_surface = NULL;
	}

	if (toplevel->output == toplevel->server->focused_output && toplevel->tiled)
		focus_after_unmap(toplevel);

	wl_list_remove(&toplevel->link);
	wl_list_remove(&toplevel->flink);

	layout_arrange(toplevel->output);

	wlr_scene_node_destroy(&toplevel->scene_tree->node);
}

void absinthe_toplevel_destroy(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, destroy);

#ifdef XWAYLAND
	if (toplevel->type == ABSINTHE_TOPLEVEL_X11) {
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
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, request_maximize);
	if (toplevel->toplevel.xdg->base->initialized)
		wlr_xdg_surface_schedule_configure(toplevel->toplevel.xdg->base);
}

void absinthe_toplevel_request_resize(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, request_maximize);
	if (toplevel->toplevel.xdg->base->initialized)
		wlr_xdg_surface_schedule_configure(toplevel->toplevel.xdg->base);
}

void absinthe_toplevel_request_maximize(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, request_maximize);
	if (toplevel->toplevel.xdg->base->initialized)
		wlr_xdg_surface_schedule_configure(toplevel->toplevel.xdg->base);
}

void absinthe_toplevel_request_fullscreen(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, request_fullscreen);
	absinthe_toplevel_set_fullscreen(toplevel, toplevel->toplevel.xdg->requested.fullscreen);
}
