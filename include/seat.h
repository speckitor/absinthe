#ifndef __SEAT_H_
#define __SEAT_H_

#include <wayland-server-core.h>

void request_cursor(struct wl_listener *listener, void *data);
void pointer_focus_change(struct wl_listener *listener, void *data);
void request_set_selection(struct wl_listener *listener, void *data);

#endif
