#include <stdnoreturn.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "focus.h"
#include "layout.h"
#include "toplevel.h"
#include "types.h"

void
run(absn_server *server, const absn_arg *arg)
{
	UNUSED(server);
	if (fork() == 0) {
		setsid();
		execl("/bin/sh", "sh", "-c", arg->v, NULL);
	}
}

void
kill_focus(absn_server *server, const absn_arg *arg)
{
	UNUSED(arg);
	if (!server->focused_output)
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

void
cycle_focus(absn_server *server, const absn_arg *arg)
{
	absn_toplevel *toplevel = focus_get_topmost(server);
	if (!toplevel || toplevel->fullscreen)
		return;

	absn_toplevel *new_focus;
	if (arg->i > 0) {
		wl_list_for_each(new_focus, &toplevel->link, link)
		{
			if (&new_focus->link == &toplevel->server->toplevels)
				continue;
			break;
		}
	} else {
		wl_list_for_each_reverse(new_focus, &toplevel->link, link)
		{
			if (&new_focus->link == &toplevel->server->toplevels)
				continue;
			break;
		}
	}
	focus_toplevel(new_focus);
}

void
toggle_fullscreen(absn_server *server, const absn_arg *arg)
{
	UNUSED(arg);
	if (!server->focused_toplevel)
		return;

	absn_toplevel *focus = server->focused_toplevel;
	toplevel_set_fullscreen(focus, !focus->fullscreen);
}

void
increase_master_width(absn_server *server, const absn_arg *arg)
{
	if (!server->focused_output)
		return;

	absn_output *focus = server->focused_output;

	if (focus->mstack_width + arg->f >= 1.0 ||
	    focus->mstack_width + arg->f <= 0.0)
		return;

	focus->mstack_width += arg->f;
	layout_arrange(focus);
}

void
increase_master_count(absn_server *server, const absn_arg *arg)
{
	if (!server->focused_output)
		return;

	absn_output *focus = server->focused_output;

	if (focus->mstack_count + arg->i <= 0)
		return;

	focus->mstack_count += arg->i;
	layout_arrange(focus);
}

noreturn void
quit(absn_server *server, const absn_arg *arg)
{
	UNUSED(arg);
	wl_display_terminate(server->display);
	exit(EXIT_SUCCESS);
}
