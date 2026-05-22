#include <stdlib.h>

#include "types.h"

void xdg_popup_commit(struct wl_listener *listener, void *data)
{
    UNUSED(data);
    absn_popup *popup = wl_container_of(listener, popup, commit);
    struct wlr_box box;

    if (popup->parent_type == TOPLEVEL_XDG) {
        absn_toplevel *toplevel = popup->parent;
        box = toplevel->output->usable_area;
        box.x -= toplevel->geom.x;
        box.y -= toplevel->geom.y;
    } else {
        absn_layer_surface *layer_surface = popup->parent;
        box = layer_surface->output->geom;
        box.x -= layer_surface->popups->node.x;
        box.y -= layer_surface->popups->node.y;
    }

    wlr_xdg_popup_unconstrain_from_box(popup->wlr, &box);

    wl_list_remove(&popup->commit.link);
    free(popup);
}
