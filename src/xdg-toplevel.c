#include <stdlib.h>
#include <assert.h>

#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "types.h"
#include "absinthe-toplevel.h"
#include "layout.h"
#include "output.h"
#include "xdg-decoration.h"

void xdg_toplevel_commit(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, commit);

	if (toplevel->toplevel.xdg->base->initial_commit) {
		wlr_xdg_toplevel_set_activated(toplevel->toplevel.xdg, false);

		/* Forse server side decoration mode */
		if (toplevel->decoration)
			xdg_decoration_request_mode(&toplevel->decoration_request_mode, toplevel->decoration);
		toplevel->resizing = wlr_xdg_toplevel_set_size(toplevel->toplevel.xdg, 0, 0);
		return;
	}

	/* Update borders and position only after client prepared new buffer */
	absinthe_toplevel_set_position(toplevel, toplevel->geometry.x, toplevel->geometry.y);
	absinthe_toplevel_update_borders_geometry(toplevel);
	if (toplevel->resizing && toplevel->resizing <= toplevel->toplevel.xdg->base->current.configure_serial)
		toplevel->resizing = 0;
}
