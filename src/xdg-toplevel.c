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

    int32_t borders_width = 2 * toplevel->border_width;

    if (toplevel->toplevel.xdg->base->initial_commit) {
        /* Let toplevel set preferred size */
        toplevel->geometry.width = toplevel->toplevel.xdg->base->geometry.width + borders_width;
        toplevel->geometry.height = toplevel->toplevel.xdg->base->geometry.height + borders_width;

        /* Forse server side decoration mode */
        if (toplevel->decoration)
            xdg_decoration_request_mode(&toplevel->decoration_request_mode, toplevel->decoration);
        toplevel->resizing = wlr_xdg_toplevel_set_size(toplevel->toplevel.xdg, 0, 0);
        return;
    }
    /* Check for size because we did't set it on initial commit */
    toplevel->geometry.width = toplevel->toplevel.xdg->base->geometry.width + borders_width;
    toplevel->geometry.height = toplevel->toplevel.xdg->base->geometry.height + borders_width;

    /* Update borders and position only after client prepared new buffer */
    absinthe_toplevel_set_position(toplevel, toplevel->geometry.x, toplevel->geometry.y);
    absinthe_toplevel_update_borders_geometry(toplevel);
    if (toplevel->resizing && toplevel->resizing <= toplevel->toplevel.xdg->base->current.configure_serial)
        toplevel->resizing = 0;
}
