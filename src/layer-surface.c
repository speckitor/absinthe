#include "types.h"

void
layer_surface_map(struct wl_listener *listener, void *data)
{
	UNUSED(listener);
	UNUSED(data);
}

void
layer_surface_unmap(struct wl_listener *listener, void *data)
{
	UNUSED(listener);
	UNUSED(data);
}

void
layer_surface_commit(struct wl_listener *listener, void *data)
{
	UNUSED(listener);
	UNUSED(data);
}

void
layer_surface_new_popup(struct wl_listener *listener, void *data)
{
	UNUSED(listener);
	UNUSED(data);
}

void
layer_surface_destroy(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	absn_layer_surface *layer_surface = wl_container_of(listener,
	    layer_surface, destroy);

	wl_list_remove(&layer_surface->link);
	wl_list_remove(&layer_surface->map.link);
	wl_list_remove(&layer_surface->unmap.link);
	wl_list_remove(&layer_surface->commit.link);
	wl_list_remove(&layer_surface->new_popup.link);
	wl_list_remove(&layer_surface->destroy.link);
	wlr_scene_node_destroy(&layer_surface->scene_tree->node);

	free(layer_surface);
}
