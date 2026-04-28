#pragma once

void absinthe_toplevel_map(struct wl_listener *listener, void *data);
void absinthe_toplevel_unmap(struct wl_listener *listener, void *data);
void absinthe_toplevel_destroy(struct wl_listener *listener, void *data);
void absinthe_toplevel_request_move(struct wl_listener *listener, void *data);
void absinthe_toplevel_request_resize(struct wl_listener *listener, void *data);
void absinthe_toplevel_request_maximize(struct wl_listener *listener, void *data);
void absinthe_toplevel_request_fullscreen(struct wl_listener *listener, void *data);
