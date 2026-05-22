#include <stdlib.h>
#include <wlr/types/wlr_layer_shell_v1.h>

#include "focus.h"
#include "layers.h"
#include "types.h"
#include "xdg-popup.h"

void layer_surface_unmap(struct wl_listener *listener, void *data)
{
    UNUSED(data);
    absn_layer_surface *layer_surface =
        wl_container_of(listener, layer_surface, unmap);

    if (layer_surface->wlr->surface ==
        layer_surface->server->seat->keyboard_state.focused_surface) {
        focus_toplevel(focus_get_topmost(layer_surface->server));
    }
    if (layer_surface == layer_surface->server->exclusive_focus) {
        layer_surface->server->exclusive_focus = NULL;
    }

    layer_surface->mapped = false;
}

void layer_surface_commit(struct wl_listener *listener, void *data)
{
    UNUSED(data);
    absn_layer_surface *layer_surface =
        wl_container_of(listener, layer_surface, commit);
    struct wlr_layer_surface_v1 *surface = layer_surface->wlr;
    struct wlr_scene_tree *scene_layer =
        layer_surface->server->layers[layermap[surface->current.layer]];

    if (surface->initial_commit) {
        struct wlr_layer_surface_v1_state old_state = surface->current;
        surface->current = surface->pending;
        layers_arrange(layer_surface->output);
        surface->current = old_state;
        return;
    }

    if (surface->current.committed == 0 ||
        layer_surface->mapped == surface->surface->mapped) {
        return;
    }
    layer_surface->mapped = surface->surface->mapped;

    if (scene_layer != layer_surface->scene_tree->node.parent) {
        wlr_scene_node_reparent(&layer_surface->scene_tree->node, scene_layer);
        wl_list_remove(&layer_surface->link);
        wl_list_insert(&layer_surface->output->layers[surface->current.layer],
                       &layer_surface->link);
        bool on_top_layer =
            surface->current.layer == ZWLR_LAYER_SHELL_V1_LAYER_TOP;
        if (on_top_layer) {
            wlr_scene_node_reparent(
                &layer_surface->popups->node,
                layer_surface->output->server->layers[LAYER_TOP]);
        } else {
            wlr_scene_node_reparent(&layer_surface->popups->node, scene_layer);
        }
    }

    layers_arrange(layer_surface->output);
    wlr_surface_send_enter(surface->surface, surface->output);
}

void layer_surface_new_popup(struct wl_listener *listener, void *data)
{
    UNUSED(data);
    absn_layer_surface *layer_surface =
        wl_container_of(listener, layer_surface, new_popup);
    struct wlr_xdg_popup *xdg_popup = data;

    wlr_scene_xdg_surface_create(layer_surface->popups, xdg_popup->base);

    absn_popup *popup = calloc(1, sizeof(*popup));
    popup->wlr = xdg_popup;
    popup->parent_type = LAYER_SURFACE;
    popup->parent = layer_surface;

    LISTEN(popup->commit, xdg_popup_commit,
           xdg_popup->base->surface->events.commit);
}

void layer_surface_destroy(struct wl_listener *listener, void *data)
{
    UNUSED(data);
    absn_layer_surface *layer_surface =
        wl_container_of(listener, layer_surface, destroy);

    wl_list_remove(&layer_surface->link);
    wl_list_remove(&layer_surface->unmap.link);
    wl_list_remove(&layer_surface->commit.link);
    wl_list_remove(&layer_surface->new_popup.link);
    wl_list_remove(&layer_surface->destroy.link);
    wlr_scene_node_destroy(&layer_surface->scene_tree->node);

    free(layer_surface);
}
