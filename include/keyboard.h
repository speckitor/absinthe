#ifndef __KEYBOARD_H_
#define __KEYBOARD_H_

#include <wayland-server-core.h>

void handle_modifiers(struct wl_listener *listener, void *data);
void handle_key(struct wl_listener *listener, void *data);
void handle_destroy(struct wl_listener *listener, void *data);

#endif
