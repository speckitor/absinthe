#include <stdlib.h>

#include "types.h"

void output_frame(struct wl_listener *listener, void *data)
{
    struct absinthe_output *output = wl_container_of(listener, output, frame);
    struct wlr_scene *scene = output->server->scene;

    struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(scene, output->wlr_output);

    wlr_scene_output_commit(scene_output, NULL);

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

void output_layout_change(struct wl_listener *listener, void *data)
{
    struct absinthe_server *server = wl_container_of(listener, server, output_layout_change);
    struct wlr_output_configuration_v1 *config = wlr_output_configuration_v1_create();

    struct wlr_output_configuration_head_v1 *config_head;
    struct absinthe_output *output;
    wl_list_for_each(output, &server->outputs, link) {
        if (output->wlr_output->enabled) continue;

        config_head = wlr_output_configuration_head_v1_create(config, output->wlr_output);
        config_head->state.enabled = false;
        wlr_output_layout_remove(server->output_layout, output->wlr_output);
    }

    wl_list_for_each(output, &server->outputs, link) {
        if (!output->wlr_output->enabled || !wlr_output_layout_get(server->output_layout, output->wlr_output)) continue;

        wlr_output_layout_add_auto(server->output_layout, output->wlr_output);
        wlr_output_layout_get_box(server->output_layout, output->wlr_output, &output->geometry);
    }

    wlr_output_manager_v1_set_configuration(server->output_mgr, config);
}
