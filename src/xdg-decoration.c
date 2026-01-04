#include <wayland-server-core.h>

#include "types.h"

void xdg_decoration_request_mode(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, decoration_request_mode);
    if (toplevel->xdg_toplevel->base->initialized) {
        wlr_xdg_toplevel_decoration_v1_set_mode(toplevel->decoration, WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
    }
}

void xdg_decoration_destroy(struct wl_listener *listener, void *data)
{
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, decoration_destroy);

    wl_list_remove(&toplevel->decoration_request_mode.link);
    wl_list_remove(&toplevel->decoration_destroy.link);
}
