#ifndef __XWAYLAND_H_
#define __XWAYLAND_H_

#include <wayland-server-core.h>

void xwayland_activate(struct wl_listener *listener, void *data);
void xwayland_associate(struct wl_listener *listener, void *data);
void xwayland_dissociate(struct wl_listener *listener, void *data);
void xwayland_configure(struct wl_listener *listener, void *data);
void xwayland_set_hints(struct wl_listener *listener, void *data);

#endif
