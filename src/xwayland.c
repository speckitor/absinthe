#ifdef XWAYLAND

#include "toplevel-handlers.h"
#include "toplevel.h"
#include "types.h"
#include "xwayland.h"

void
xwayland_activate(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel,
	    xw_activate);

	if (!toplevel_is_unmanaged(toplevel))
		wlr_xwayland_surface_activate(toplevel->xw, 1);
}

void
xwayland_associate(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel,
	    xw_associate);

	LISTEN(toplevel->map, toplevel_map, toplevel->xw->surface->events.map);
	LISTEN(toplevel->unmap, toplevel_unmap,
	    toplevel->xw->surface->events.unmap);
}

void
xwayland_dissociate(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel,
	    xw_dissociate);

	wl_list_remove(&toplevel->map.link);
	wl_list_remove(&toplevel->unmap.link);
}

void
xwayland_configure(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel,
	    xw_configure);
	struct wlr_xwayland_surface_configure_event *event = data;

	if (!toplevel->xw->surface || !toplevel->xw->surface->mapped) {
		wlr_xwayland_surface_configure(toplevel->xw, event->x, event->y,
		    event->width, event->height);
		return;
	}

	if (toplevel_is_unmanaged(toplevel)) {
		wlr_scene_node_set_position(&toplevel->scene_tree->node,
		    event->x, event->y);
		wlr_xwayland_surface_configure(toplevel->xw, event->x, event->y,
		    event->width, event->height);
		return;
	}
}

void
xwayland_set_hints(struct wl_listener *listener, void *data)
{
	UNUSED(listener);
	UNUSED(data);
}
#endif
