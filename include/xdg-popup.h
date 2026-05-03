#ifndef __XDG_POPUP_H_
#define __XDG_POPUP_H_

#include <wayland-server-core.h>

void xdg_popup_commit(struct wl_listener *listener, void *data);
void xdg_popup_destroy(struct wl_listener *listener, void *data);

#endif
