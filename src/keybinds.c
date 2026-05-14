#include <stdnoreturn.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "focus.h"
#include "layout.h"
#include "output.h"
#include "toplevel.h"
#include "types.h"

void run(absn_server *server, const absn_arg *arg)
{
    UNUSED(server);
    if (fork() == 0) {
        setsid();
        execl("/bin/sh", "sh", "-c", arg->v, NULL);
    }
}

void kill_focus(absn_server *server, const absn_arg *arg)
{
    UNUSED(arg);
    if (!server->focused_toplevel)
        return;

    absn_toplevel *focus = server->focused_toplevel;

#ifdef XWAYLAND
    if (focus->type == TOPLEVEL_X11) {
        wlr_xwayland_surface_close(focus->xw);
    } else
#endif
    {
        wlr_xdg_toplevel_send_close(focus->xdg);
    }
}

void cycle_focus(absn_server *server, const absn_arg *arg)
{
    absn_toplevel *toplevel = focus_get_topmost(server);
    if (!toplevel || toplevel->fullscreen)
        return;

    absn_toplevel *new_focus;
    if (arg->i > 0) {
        wl_list_for_each(new_focus, &toplevel->link, link)
        {
            if (toplevel->workspace == new_focus->workspace)
                break;
        }
    } else {
        wl_list_for_each_reverse(new_focus, &toplevel->link, link)
        {
            if (toplevel->workspace == new_focus->workspace)
                break;
        }
    }
    focus_toplevel(new_focus);
}

void swap_focus(absn_server *server, const absn_arg *arg)
{

    absn_toplevel *toplevel = focus_get_topmost(server);
    if (!toplevel || toplevel->fullscreen || toplevel->floating)
        return;

    absn_toplevel *temp = NULL;
    absn_toplevel *first = NULL;
    absn_toplevel *last = NULL;

    wl_list_for_each(temp, &server->toplevels, link) {
        if (temp->workspace == toplevel->workspace)
            last = temp;
        if (!first && temp->workspace == toplevel->workspace)
            first = temp;
    }

    if (first == last)
        return;

    absn_toplevel *swap = NULL;
    if (arg->i > 0) {
        if (toplevel == last) {
            wl_list_remove(&toplevel->link);
            wl_list_insert(&server->toplevels, &toplevel->link);
            goto arrange;
        }

        wl_list_for_each(swap, &toplevel->link, link)
        {
            if (toplevel->workspace == swap->workspace)
                break;
        }

        wl_list_remove(&toplevel->link);
        wl_list_insert(&swap->link, &toplevel->link);
    } else {
        if (toplevel == first) {
            wl_list_remove(&toplevel->link);
            wl_list_insert(server->toplevels.prev, &toplevel->link);
            goto arrange;
        }

        wl_list_for_each_reverse(swap, &toplevel->link, link)
        {
            if (toplevel->workspace == swap->workspace)
                break;
        }

        wl_list_remove(&swap->link);
        wl_list_insert(&toplevel->link, &swap->link);
    }

arrange:
    layout_arrange(toplevel->output);
}

void toggle_fullscreen(absn_server *server, const absn_arg *arg)
{
    UNUSED(arg);
    if (!server->focused_toplevel)
        return;

    absn_toplevel *focus = server->focused_toplevel;
    toplevel_set_fullscreen(focus, !focus->fullscreen);
}

void increase_master_width(absn_server *server, const absn_arg *arg)
{
    absn_workspace *workspace = server->focused_output->workspace;

    if (workspace->size + arg->f >= 1.0 || workspace->size + arg->f <= 0.0)
        return;

    workspace->size += arg->f;
    layout_arrange(workspace->output);
}

void increase_master_count(absn_server *server, const absn_arg *arg)
{
    absn_workspace *workspace = server->focused_output->workspace;

    if (workspace->count + arg->i <= 0)
        return;

    workspace->count += arg->i;
    layout_arrange(workspace->output);
}

void switch_workspace(absn_server *server, const absn_arg *arg)
{
    int i;
    for (i = 0; i < server->workspaces_count; ++i) {
        if (arg->v == server->workspaces[i].name)
            break; /* found it */
    }

    if (&server->workspaces[i] == server->focused_output->workspace)
        return;

    if (!server->workspaces[i].output) {
        server->workspaces[i].output = server->focused_output;
    } else if (server->focused_output == server->workspaces[i].output) {
        server->focused_output->workspace = &server->workspaces[i];
    } else {
        server->focused_output = server->workspaces[i].output;

        struct wlr_box *geom = &server->focused_output->geom;

        int32_t new_x = geom->x + (geom->width / 2);
        int32_t new_y = geom->y + (geom->height / 2);

        wlr_cursor_warp(server->cursor, NULL, new_x, new_y);
    }
    server->focused_output->workspace = &server->workspaces[i];

    absn_toplevel *focus = NULL;
    absn_toplevel *toplevel;
    wl_list_for_each(toplevel, &server->toplevels, link)
    {
        if (!focus && toplevel && toplevel->workspace == &server->workspaces[i]) {
            focus = toplevel; /* found it */
        }

        if (toplevel && toplevel->workspace == &server->workspaces[i] && toplevel->fullscreen) {
            /* overwrite if there is a fullscreen window */
            focus = toplevel;
            break;
        }
    }

    unfocus_toplevel(server->focused_toplevel);
    if (focus) {
        /* crazy way to make it focus client if it was focused before */
        struct wlr_surface *surface;
#ifdef XWAYLAND
        if (focus->type == TOPLEVEL_X11)
            surface = focus->xw->surface;
        else
#endif
            surface = focus->xdg->base->surface;

        if (surface == server->seat->keyboard_state.focused_surface)
            server->seat->keyboard_state.focused_surface = NULL;

        focus_toplevel(focus);
    }

    layout_arrange(server->focused_output);
}

void move_focus_to_workspace(absn_server *server, const absn_arg *arg)
{
    if (!server->focused_toplevel)
        return;

    if (server->focused_toplevel->fullscreen)
        toplevel_set_fullscreen(server->focused_toplevel, false);

    int i;
    for (i = 0; i < server->workspaces_count; ++i) {
        if (arg->v == server->workspaces[i].name)
            break; /* found it */
    }

    if (&server->workspaces[i] == server->focused_toplevel->workspace)
        return;

    if (!server->workspaces[i].output)
        server->workspaces[i].output = server->focused_output;
    server->focused_toplevel->output = server->workspaces[i].output;

    server->focused_toplevel->workspace = &server->workspaces[i];

    focus_toplevel(focus_get_topmost(server));

    layout_arrange(server->focused_output);
    layout_arrange(server->focused_toplevel->output);
}

void set_layout(absn_server *server, const absn_arg *arg)
{
    server->focused_output->workspace->layout = arg->i;
    layout_arrange(server->focused_output);
}

noreturn void quit(absn_server *server, const absn_arg *arg)
{
    UNUSED(arg);
    wl_display_terminate(server->display);
    exit(EXIT_SUCCESS);
}
