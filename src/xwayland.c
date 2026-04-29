#ifdef XWAYLAND

#include "xwayland.h"
#include "absinthe-toplevel-handlers.h"
#include "absinthe-toplevel.h"
#include "types.h"
#include "xdg-toplevel.h"

void xwayland_activate(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, xwayland_activate);

	if (!absinthe_toplevel_is_unmanaged(toplevel))
		wlr_xwayland_surface_activate(toplevel->xwayland_surface, 1);
}

void xwayland_associate(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, xwayland_associate);

	toplevel->map.notify = absinthe_toplevel_map;
	wl_signal_add(&toplevel->xwayland_surface->surface->events.map, &toplevel->map);
	toplevel->unmap.notify = absinthe_toplevel_unmap;
	wl_signal_add(&toplevel->xwayland_surface->surface->events.unmap, &toplevel->unmap);
}

void xwayland_dissociate(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, xwayland_dissociate);

	wl_list_remove(&toplevel->map.link);
	wl_list_remove(&toplevel->unmap.link);
}

void xwayland_configure(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, xwayland_configure);
	struct wlr_xwayland_surface_configure_event *event = data;

	if (!toplevel->xwayland_surface->surface || !toplevel->xwayland_surface->surface->mapped) {
		wlr_xwayland_surface_configure(toplevel->xwayland_surface, event->x, event->y, event->width,
					       event->height);
		return;
	}

	if (absinthe_toplevel_is_unmanaged(toplevel)) {
		wlr_scene_node_set_position(&toplevel->scene_tree->node, event->x, event->y);
		wlr_xwayland_surface_configure(toplevel->xwayland_surface, event->x, event->y, event->width,
					       event->height);
		return;
	}
}

void xwayland_set_hints(struct wl_listener *listener, void *data)
{
	UNUSED(listener);
	UNUSED(data);
}
#endif
