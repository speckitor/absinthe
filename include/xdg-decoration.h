#ifndef __XDG_DECORATION_H_
#define __XDG_DECORATION_H_

#include <wayland-server-core.h>

void deco_request_mode(struct wl_listener *listener, void *data);
void deco_destroy(struct wl_listener *listener, void *data);

#endif
