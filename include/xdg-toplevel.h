#ifndef __XDG_TOPLEVEL_H_
#define __XDG_TOPLEVEL_H_

#include <wayland-server-core.h>

void toplevel_commit(struct wl_listener *listener, void *data);

#endif
