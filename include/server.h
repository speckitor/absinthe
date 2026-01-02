#pragma once

#include <wayland-server-core.h>

void server_new_output(struct wl_listener *listener, void *data);

void server_new_xdg_toplevel(struct wl_listener *listener, void *data);

void server_new_xdg_popup(struct wl_listener *listener, void *data);

void server_cursor_motion(struct wl_listener *listener, void *data);
void server_cursor_motion_absolute(struct wl_listener *listener, void *data);
void server_cursor_button(struct wl_listener *listener, void *data);
void server_cursor_axis(struct wl_listener *listener, void *data);
void server_cursor_frame(struct wl_listener *listener, void *data);

void server_new_input(struct wl_listener *listener, void *data);
