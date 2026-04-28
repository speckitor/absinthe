#include <stdlib.h>
#include <wayland-server-core.h>

#include "types.h"

void xdg_popup_commit(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_popup *popup = wl_container_of(listener, popup, commit);

	if (popup->xdg_popup->base->initial_commit)
		wlr_xdg_surface_schedule_configure(popup->xdg_popup->base);
}

void xdg_popup_destroy(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_popup *popup = wl_container_of(listener, popup, destroy);

	wl_list_remove(&popup->commit.link);
	wl_list_remove(&popup->destroy.link);

	free(popup);
}
