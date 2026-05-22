#include <assert.h>
#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "toplevel.h"
#include "types.h"
#include "xdg-decoration.h"

void toplevel_commit(struct wl_listener *listener, void *data)
{
    UNUSED(data);
    absn_toplevel *toplevel = wl_container_of(listener, toplevel, commit);
    struct wlr_surface *surface = toplevel->xdg->base->surface;

    if (toplevel->xdg->base->initial_commit) {
        wlr_xdg_toplevel_set_activated(toplevel->xdg, false);

        /* forse server side decoration mode */
        if (toplevel->deco) {
            deco_request_mode(&toplevel->deco_request_mode, toplevel->deco);
        }
        /* let client set initial size */
        // wlr_xdg_toplevel_set_size(toplevel->xdg, 0, 0);
        return;
    }

    struct wlr_box clip = {
        .x = toplevel->xdg->base->geometry.x,
        .y = toplevel->xdg->base->geometry.y,
        .width = toplevel->geom.width - toplevel->bw,
        .height = toplevel->geom.height - toplevel->bw,
    };
    wlr_scene_subsurface_tree_set_clip(&toplevel->scene_surface->node, &clip);
}
