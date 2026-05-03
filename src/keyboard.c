#include <stdlib.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>

#include "focus.h"
#include "layout.h"
#include "toplevel.h"
#include "types.h"

void
handle_modifiers(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_keyboard *keyboard = wl_container_of(listener, keyboard,
	    modifiers);

	wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr);
	wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,
	    &keyboard->wlr->modifiers);
}

static bool
handle_keybind(struct absinthe_server *server, xkb_keysym_t keysym)
{
	switch (keysym) {
	case XKB_KEY_Escape:
		wl_display_terminate(server->display);
		break;
	case XKB_KEY_Return:
		if (fork() == 0)
			execl("/bin/sh", "sh", "-c", "ghostty", NULL);
		break;
	case XKB_KEY_r:
		if (fork() == 0)
			execl("/bin/sh", "sh", "-c", "wofi --show drun", NULL);
		break;
	case XKB_KEY_f:
		if (server->focused_toplevel)
			toplevel_set_fullscreen(server->focused_toplevel,
			    !server->focused_toplevel->fullscreen);
		break;
	case XKB_KEY_j:
		focus_next(server);
		break;
	case XKB_KEY_k:
		focus_prev(server);
		break;
	case XKB_KEY_h:
		if (server->focused_output &&
		    server->focused_output->mstack_width > 0.15) {
			server->focused_output->mstack_width -= 0.1;
			layout_arrange(server->focused_output);
		}
		break;
	case XKB_KEY_l:
		if (server->focused_output &&
		    server->focused_output->mstack_width < 0.9) {
			server->focused_output->mstack_width += 0.1;
			layout_arrange(server->focused_output);
		}
		break;
	case XKB_KEY_H:
		if (server->focused_output) {
			server->focused_output->mstack_size += 1;
			layout_arrange(server->focused_output);
		}
		break;
	case XKB_KEY_L:
		if (server->focused_output &&
		    server->focused_output->mstack_size > 1) {
			server->focused_output->mstack_size -= 1;
			layout_arrange(server->focused_output);
		}
		break;
	default:
		return false;
	}
	return true;
}

void
handle_key(struct wl_listener *listener, void *data)
{
	struct absinthe_keyboard *keyboard = wl_container_of(listener, keyboard,
	    key);
	struct wlr_keyboard_key_event *event = data;

	uint32_t keycode = event->keycode + 8;
	uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->wlr);
	const xkb_keysym_t *syms;
	int nsyms = xkb_state_key_get_syms(keyboard->wlr->xkb_state, keycode,
	    &syms);

	bool handled = false;
	if ((modifiers & WLR_MODIFIER_ALT) &&
	    (event->state == WL_KEYBOARD_KEY_STATE_PRESSED)) {
		for (int i = 0; i < nsyms; ++i) {
			handled = handle_keybind(keyboard->server, syms[i]);
		}
	}

	if (!handled) {
		wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr);
		wlr_seat_keyboard_notify_key(keyboard->server->seat,
		    event->time_msec, event->keycode, event->state);
	}
}

void
handle_destroy(struct wl_listener *listener, void *data)
{
	UNUSED(data);
	struct absinthe_keyboard *keyboard = wl_container_of(listener, keyboard,
	    modifiers);

	wl_list_remove(&keyboard->modifiers.link);
	wl_list_remove(&keyboard->key.link);
	wl_list_remove(&keyboard->destroy.link);
	wl_list_remove(&keyboard->link);

	free(keyboard);
}
