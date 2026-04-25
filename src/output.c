#include <stdlib.h>

#include <wlr/util/log.h>

#include "types.h"

void output_frame(struct wl_listener *listener, void *data)
{
	struct absinthe_output *output = wl_container_of(listener, output, frame);
	struct wlr_scene *scene = output->server->scene;

	struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(scene, output->wlr_output);

	struct absinthe_toplevel *toplevel;
	wl_list_for_each(toplevel, &output->server->toplevels, link) {
		if (toplevel->resizing && toplevel->output == output)
			goto skip;
	}

	wlr_scene_output_commit(scene_output, NULL);

skip:
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	wlr_scene_output_send_frame_done(scene_output, &now);
}

void output_request_state(struct wl_listener *listener, void *data)
{
	struct absinthe_output *output = wl_container_of(listener, output, request_state);
	const struct wlr_output_event_request_state *event = data;
	wlr_output_commit_state(output->wlr_output, event->state);
}

void output_destroy(struct wl_listener *listener, void *data)
{
	struct absinthe_output *output = wl_container_of(listener, output, request_state);

	wl_list_remove(&output->frame.link);
	wl_list_remove(&output->request_state.link);
	wl_list_remove(&output->destroy.link);
	wl_list_remove(&output->link);
	free(output);
}

void outputs_update(struct wl_listener *listener, void *data)
{
	struct absinthe_server *server = wl_container_of(listener, server, output_layout_change);
	struct absinthe_output *output;
	wl_list_for_each(output, &server->outputs, link) {
		wlr_output_layout_get_box(server->output_layout, output->wlr_output, &output->geometry);
	}
}

void update_focused_output(struct absinthe_server *server)
{
	struct absinthe_output *output;
	int32_t cursor_x = server->cursor->x;
	int32_t cursor_y = server->cursor->y;
	wl_list_for_each(output, &server->outputs, link) {
		bool cursor_in_output_x = cursor_x >= output->geometry.x && cursor_x <= output->geometry.x + output->geometry.width;
		bool cursor_in_output_y = cursor_y >= output->geometry.y && cursor_y <= output->geometry.y + output->geometry.height;
		if (cursor_in_output_x && cursor_in_output_y) {
			server->focused_output = output;
			break;
		}
	}
}

