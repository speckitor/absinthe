#pragma once

#include <wayland-server-core.h>

void xdg_decoration_request_mode(struct wl_listener *listener, void *data);
void xdg_decoration_destroy(struct wl_listener *listener, void *data);
