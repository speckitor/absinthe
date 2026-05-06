#include <stdlib.h>
#include <wlr/util/log.h>

#include "focus.h"
#include "layout.h"
#include "toplevel.h"
#include "types.h"

void
output_frame(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct timespec now;
	absn_output *output = wl_container_of(listener, output, frame);
	struct wlr_scene *scene = output->server->scene;
	struct wlr_scene_output *scene_output =
	    wlr_scene_get_scene_output(scene, output->wlr);

	absn_toplevel *toplevel;
	wl_list_for_each(toplevel, &output->server->toplevels, link)
	{
		if (toplevel->resizing && toplevel->output == output)
			goto skip;
	}

	wlr_scene_output_commit(scene_output, NULL);

skip:
	clock_gettime(CLOCK_MONOTONIC, &now);
	wlr_scene_output_send_frame_done(scene_output, &now);
}

void
output_request_state(struct wl_listener *listener, void *data)
{
	struct absn_output *output = wl_container_of(listener, output,
	    request_state);
	const struct wlr_output_event_request_state *event = data;
	wlr_output_commit_state(output->wlr, event->state);
}

void
output_destroy(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absn_output *output = wl_container_of(listener, output,
	    request_state);

	wl_list_remove(&output->frame.link);
	wl_list_remove(&output->request_state.link);
	wl_list_remove(&output->destroy.link);
	wl_list_remove(&output->link);
	free(output);
}

void
output_layout_change(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	absn_server *server = wl_container_of(listener, server, layout_change);
	struct wlr_output_configuration_v1 *config =
	    wlr_output_configuration_v1_create();

	absn_toplevel *toplevel = NULL;
	absn_output *output;
	struct wlr_output_configuration_head_v1 *config_head;

	wl_list_for_each(output, &server->outputs, link)
	{
		if (output->wlr->enabled)
			continue;
		config_head = wlr_output_configuration_head_v1_create(config,
		    output->wlr);
		config_head->state.enabled = 0;
		wlr_output_layout_remove(server->output_layout, output->wlr);

		// closemon(m);
		// m->m = m->w = (struct wlr_box){0};
	}

	wl_list_for_each(output, &server->outputs, link)
	{
		if (output->wlr->enabled &&
		    !wlr_output_layout_get(server->output_layout, output->wlr))
			wlr_output_layout_add_auto(server->output_layout,
			    toplevel->output->wlr);
	}

	wl_list_for_each(output, &server->outputs, link)
	{
		if (!output->wlr->enabled)
			continue;
		config_head = wlr_output_configuration_head_v1_create(config,
		    output->wlr);

		wlr_output_layout_get_box(server->output_layout, output->wlr,
		    &output->geom);

		if ((toplevel = focus_get_topmost(server)) &&
		    toplevel->fullscreen)
			toplevel_set_size(toplevel, output->geom.width,
			    output->geom.height);

		config_head->state.x = output->geom.x;
		config_head->state.y = output->geom.y;

		if (!server->focused_output)
			server->focused_output = output;
	}

	if (server->focused_output && server->focused_output->wlr->enabled) {
		wl_list_for_each(toplevel, &server->toplevels, link)
		{
			if (!toplevel->output) {
				toplevel->output = server->focused_output;
			}
		}
		layout_arrange(server->focused_output);
	}

	focus_toplevel(focus_get_topmost(server));
	wlr_cursor_move(server->cursor, NULL, 0, 0);
	wlr_output_manager_v1_set_configuration(server->output_mgr, config);
}

void
update_focused_output(absn_server *server)
{
	absn_output *output;
	int32_t cursor_x = server->cursor->x;
	int32_t cursor_y = server->cursor->y;
	wl_list_for_each(output, &server->outputs, link)
	{
		bool cursor_in_output_x = cursor_x >= output->geom.x &&
		    cursor_x <= output->geom.x + output->geom.width;
		bool cursor_in_output_y = cursor_y >= output->geom.y &&
		    cursor_y <= output->geom.y + output->geom.height;
		if (cursor_in_output_x && cursor_in_output_y) {
			server->focused_output = output;
			break;
		}
	}
}
