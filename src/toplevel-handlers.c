#include <stdlib.h>
#include <wlr/util/edges.h>

#include "config.h"
#include "focus.h"
#include "layout.h"
#include "output.h"
#include "toplevel.h"

void toplevel_map(struct wl_listener *listener, void *data)
{
    UNUSED(data);
    absn_toplevel *toplevel = wl_container_of(listener, toplevel, map);
    absn_server *server = toplevel->server;

    toplevel->scene_tree = wlr_scene_tree_create(server->layers[LAYER_TILE]);
    toplevel->scene_tree->node.data = toplevel;
    wlr_scene_node_set_enabled(&toplevel->scene_tree->node, toplevel_is_unmanaged(toplevel));

    update_focused_output(toplevel->server);
    toplevel->output = toplevel->server->focused_output;
    toplevel->workspace = toplevel->output->workspace;
    toplevel->fullscreen = false;

#ifdef XWAYLAND
    if (toplevel->type == TOPLEVEL_X11) {
        toplevel->xw->surface->data = toplevel;
    }
#endif

    if (toplevel->type != TOPLEVEL_X11 &&
        wl_resource_get_version(toplevel->xdg->resource) >= XDG_TOPLEVEL_STATE_TILED_RIGHT_SINCE_VERSION) {
        wlr_xdg_toplevel_set_tiled(toplevel->xdg, WLR_EDGE_TOP | WLR_EDGE_BOTTOM | WLR_EDGE_LEFT | WLR_EDGE_RIGHT);
    }

#ifdef XWAYLAND
    if (toplevel->type == TOPLEVEL_X11) {
        toplevel->scene_surface = wlr_scene_subsurface_tree_create(toplevel->scene_tree, toplevel->xw->surface);
    } else
#endif
    {
        toplevel->scene_surface = wlr_scene_xdg_surface_create(toplevel->scene_tree, toplevel->xdg->base);
    }
    toplevel->scene_surface->node.data = toplevel;

    toplevel_get_geom(toplevel);
    toplevel->bw = toplevel_is_unmanaged(toplevel) ? 0 : TOPLEVEL_BW;

    if (toplevel_is_unmanaged(toplevel)) {
        toplevel_set_floating(toplevel, true);
        toplevel_set_size(toplevel, toplevel->geom.width, toplevel->geom.height);
        if (toplevel_wants_focus(toplevel)) {
            focus_toplevel(toplevel);
            server->exclusive_focus = toplevel;
        }
        goto unset_fullscreen;
    }

    for (int i = 0; i < 4; ++i) {
        toplevel->border[i] = wlr_scene_rect_create(toplevel->scene_tree, 0, 0, unfocused_bc);
        toplevel->border[i]->node.data = toplevel;
    }

    wl_list_insert(&toplevel->server->toplevels, &toplevel->link);
    wl_list_insert(&toplevel->server->focus_stack, &toplevel->flink);

    focus_toplevel(focus_get_topmost(toplevel->server));

    if (toplevel->type == TOPLEVEL_XDG && toplevel->xdg->parent) {
        toplevel->floating = 1;
    } else if (toplevel->type == TOPLEVEL_X11 && toplevel->xw->parent) {
        toplevel->floating = 1;
    }

unset_fullscreen:
    absn_toplevel *t;
    wl_list_for_each(t, &server->toplevels, link)
    {
        if (t->workspace == toplevel->workspace && t->fullscreen) {
            toplevel_set_fullscreen(t, false);
        }
    }

    layout_arrange(toplevel->output);
}

void toplevel_unmap(struct wl_listener *listener, void *data)
{
    UNUSED(data);
    absn_toplevel *toplevel = wl_container_of(listener, toplevel, unmap);

    if (toplevel == toplevel->server->focused_toplevel) {
        toplevel->server->focused_toplevel = NULL;
        toplevel->server->seat->keyboard_state.focused_surface = NULL;
    }

    if (toplevel == toplevel->server->exclusive_focus) {
        toplevel->server->exclusive_focus = NULL;
    }

    if (!toplevel_is_unmanaged(toplevel)) {
        wl_list_remove(&toplevel->link);
        wl_list_remove(&toplevel->flink);
    }

    layout_arrange(toplevel->output);
    focus_toplevel(focus_get_topmost(toplevel->server));

    wlr_scene_node_destroy(&toplevel->scene_tree->node);
}

void toplevel_destroy(struct wl_listener *listener, void *data)
{
    UNUSED(data);
    absn_toplevel *toplevel = wl_container_of(listener, toplevel, destroy);

#ifdef XWAYLAND
    if (toplevel->type == TOPLEVEL_X11) {
        wl_list_remove(&toplevel->xw_activate.link);
        wl_list_remove(&toplevel->xw_associate.link);
        wl_list_remove(&toplevel->xw_dissociate.link);
        wl_list_remove(&toplevel->xw_configure.link);
        wl_list_remove(&toplevel->xw_set_hints.link);
    } else
#endif
    {
        wl_list_remove(&toplevel->map.link);
        wl_list_remove(&toplevel->unmap.link);
        wl_list_remove(&toplevel->commit.link);
        wl_list_remove(&toplevel->request_move.link);
        wl_list_remove(&toplevel->request_resize.link);
    }

    wl_list_remove(&toplevel->destroy.link);
    wl_list_remove(&toplevel->request_maximize.link);
    wl_list_remove(&toplevel->request_fullscreen.link);

    free(toplevel);
}

void toplevel_request_move(struct wl_listener *listener, void *data)
{
    UNUSED(data);
    absn_toplevel *toplevel = wl_container_of(listener, toplevel, request_move);
    if (toplevel->xdg->base->initialized) {
        wlr_xdg_surface_schedule_configure(toplevel->xdg->base);
    }
}

void toplevel_request_resize(struct wl_listener *listener, void *data)
{
    UNUSED(data);
    absn_toplevel *toplevel = wl_container_of(listener, toplevel, request_resize);
    if (toplevel->xdg->base->initialized) {
        wlr_xdg_surface_schedule_configure(toplevel->xdg->base);
    }
}

void toplevel_request_maximize(struct wl_listener *listener, void *data)
{
    UNUSED(data);
    absn_toplevel *toplevel = wl_container_of(listener, toplevel, request_maximize);
    if (toplevel->xdg->base->initialized) {
        wlr_xdg_surface_schedule_configure(toplevel->xdg->base);
    }
}

void toplevel_request_fullscreen(struct wl_listener *listener, void *data)
{
    UNUSED(data);
    absn_toplevel *toplevel = wl_container_of(listener, toplevel, request_fullscreen);
    toplevel_set_fullscreen(toplevel, toplevel->xdg->requested.fullscreen);
}
