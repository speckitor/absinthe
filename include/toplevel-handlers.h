#ifndef __TOPLEVLE_HANDLERS_H_
#define __TOPLEVLE_HANDLERS_H_

#include <wayland-server-core.h>

void toplevel_map(struct wl_listener *listener, void *data);
void toplevel_unmap(struct wl_listener *listener, void *data);
void toplevel_destroy(struct wl_listener *listener, void *data);
void toplevel_request_move(struct wl_listener *listener, void *data);
void toplevel_request_resize(struct wl_listener *listener, void *data);
void toplevel_request_maximize(struct wl_listener *listener, void *data);
void toplevel_request_fullscreen(struct wl_listener *listener, void *data);

#endif
