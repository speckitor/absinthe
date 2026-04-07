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
    struct absinthe_toplevel *toplevel = wl_container_of(listener, toplevel, commit);

    if (toplevel->toplevel.xdg->base->initial_commit) {
        // Forse server side decoration mode
        if (toplevel->decoration)
            xdg_decoration_request_mode(&toplevel->decoration_request_mode, toplevel->decoration);

        // Let toplevel set preferred size
        // wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, 0, 0);
    } else {
        // Check for size because we did't set it on initial commit
        int32_t borders_width = 2 * toplevel->border_width;
        toplevel->geometry.width = toplevel->toplevel.xdg->base->geometry.width + borders_width;
        toplevel->geometry.height = toplevel->toplevel.xdg->base->geometry.height + borders_width;

        absinthe_toplevel_set_position(toplevel, toplevel->geometry.x, toplevel->geometry.y);
        absinthe_toplevel_update_borders_geometry(toplevel);
        toplevel->performing_resize = false;
    }
}
