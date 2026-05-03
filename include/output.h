#ifndef __OUTPUT_H_
#define __OUTPUT_H_

#include <wayland-server-core.h>

#include "types.h"

void output_frame(struct wl_listener *listener, void *data);
void output_request_state(struct wl_listener *listener, void *data);
void output_destroy(struct wl_listener *listener, void *data);
void output_layout_change(struct wl_listener *listener, void *data);

void update_focused_output(struct absinthe_server *server);

#endif
