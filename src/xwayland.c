#ifdef XWAYLAND
#include "types.h"
#include "absinthe-toplevel.h"
#include "xdg_toplevel.h"
#include "xwayland.h"

void xwayland_activate(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, xwayland_activate);

    if (!absinthe_toplevel_is_unmanaged(toplevel))
        wlr_xwayland_surface_activate(toplevel->toplevel.x11, 1);
}

void xwayland_associate(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, xwayland_activate);

    toplevel->map.notify = xdg_toplevel_map;
    wl_signal_add(&toplevel->toplevel.x11->events.map, &toplevel->map);
    toplevel->unmap.notify = xdg_toplevel_unmap;
    wl_signal_add(&toplevel->toplevel.x11->events.unmap, &toplevel->unmap);
}

void xwayland_dissociate(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, xwayland_activate);

    wl_list_remove(&toplevel->map.link);
    wl_list_remove(&toplevel->unmap.link);
}

void xwayland_configure(struct wl_listener *listener, void *data) {}

void xwayland_set_hints(struct wl_listener *listener, void *data) {}

#endif
