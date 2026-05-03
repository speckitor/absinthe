#ifndef __LAYER_SURFACE_H_
#define __LAYER_SURFACE_H_

#include <wayland-server-core.h>

void layer_surface_map(struct wl_listener *listener, void *data);
void layer_surface_unmap(struct wl_listener *listener, void *data);
void layer_surface_commit(struct wl_listener *listener, void *data);
void layer_surface_destroy(struct wl_listener *listener, void *data);

#endif
