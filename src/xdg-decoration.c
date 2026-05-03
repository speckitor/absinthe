#include <wayland-server-core.h>

#include "types.h"

void
deco_request_mode(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel,
	    deco_request_mode);
	if (toplevel->xdg->base->initialized)
		wlr_xdg_toplevel_decoration_v1_set_mode(toplevel->deco,
		    WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
}

void
deco_destroy(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel,
	    deco_destroy);

	wl_list_remove(&toplevel->deco_request_mode.link);
	wl_list_remove(&toplevel->deco_destroy.link);
}
