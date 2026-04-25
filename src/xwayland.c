#ifdef XWAYLAND

#include "types.h"
#include "absinthe-toplevel.h"
#include "xdg-toplevel.h"
#include "xwayland.h"

void xwayland_activate(struct wl_listener *listener, void *data)
{
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, xwayland_activate);

	if (!absinthe_toplevel_is_unmanaged(toplevel))
		wlr_xwayland_surface_activate(toplevel->toplevel.x11, 1);
}

void xwayland_associate(struct wl_listener *listener, void *data)
{
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, xwayland_associate);

	toplevel->map.notify = absinthe_toplevel_map;
	wl_signal_add(&toplevel->toplevel.x11->surface->events.map, &toplevel->map);
	toplevel->unmap.notify = absinthe_toplevel_unmap;
	wl_signal_add(&toplevel->toplevel.x11->surface->events.unmap, &toplevel->unmap);
}

void xwayland_dissociate(struct wl_listener *listener, void *data)
{
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, xwayland_dissociate);

	wl_list_remove(&toplevel->map.link);
	wl_list_remove(&toplevel->unmap.link);
}

void xwayland_configure(struct wl_listener *listener, void *data)
{
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, xwayland_configure);
	struct wlr_xwayland_surface_configure_event *event = data;

	if (!toplevel->toplevel.x11->surface || !toplevel->toplevel.x11->surface->mapped) {
		wlr_xwayland_surface_configure(toplevel->toplevel.x11, event->x, event->y, event->width, event->height);
		return;
	}

	if (absinthe_toplevel_is_unmanaged(toplevel)) {
		wlr_scene_node_set_position(&toplevel->scene_tree->node, event->x, event->y);
		wlr_xwayland_surface_configure(toplevel->toplevel.x11,
									   event->x, event->y, event->width, event->height);
		return;
	}
}

void xwayland_set_hints(struct wl_listener *listener, void *data) {}

#endif
