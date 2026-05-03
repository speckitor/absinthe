#ifndef __SERVER_H_
#define __SERVER_H_

#include <wayland-server-core.h>

void new_output(struct wl_listener *listener, void *data);

void new_xdg_toplevel(struct wl_listener *listener, void *data);
void new_xdg_popup(struct wl_listener *listener, void *data);
void new_xdg_decoration(struct wl_listener *listener, void *data);

void new_layer_surface(struct wl_listener *listener, void *data);

#ifdef XWAYLAND
void xwayland_ready(struct wl_listener *listener, void *data);
void xwayland_new_surface(struct wl_listener *listener, void *data);
#endif

void cursor_motion(struct wl_listener *listener, void *data);
void cursor_motion_abs(struct wl_listener *listener, void *data);
void cursor_button(struct wl_listener *listener, void *data);
void cursor_axis(struct wl_listener *listener, void *data);
void cursor_frame(struct wl_listener *listener, void *data);

void new_input(struct wl_listener *listener, void *data);

#endif
