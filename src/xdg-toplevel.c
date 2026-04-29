#include <assert.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "absinthe-toplevel.h"
#include "layout.h"
#include "output.h"
#include "types.h"
#include "xdg-decoration.h"

void xdg_toplevel_commit(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, commit);

	if (toplevel->xdg_toplevel->base->initial_commit) {
		wlr_xdg_toplevel_set_activated(toplevel->xdg_toplevel, false);

		/* Forse server side decoration mode */
		if (toplevel->decoration)
			xdg_decoration_request_mode(&toplevel->decoration_request_mode, toplevel->decoration);
		toplevel->resizing = wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, 0, 0);
		return;
	}

	if (toplevel->resizing && toplevel->resizing <= toplevel->xdg_toplevel->base->current.configure_serial)
		toplevel->resizing = 0;
}
