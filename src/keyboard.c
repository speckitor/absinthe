#include <stdlib.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>

#include "config.h"
#include "types.h"

void handle_modifiers(struct wl_listener *listener, void *data)
{
    UNUSED(data);
    absn_keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);

    wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr);
    wlr_seat_keyboard_notify_modifiers(keyboard->server->seat, &keyboard->wlr->modifiers);
}

static bool handle_keybind(absn_server *server, uint32_t mods, xkb_keysym_t keysym)
{
    int nkeyb = sizeof(keybinds) / sizeof(keybinds[0]);
    for (int i = 0; i < nkeyb; ++i) {
        if (CLEANMASK(keybinds[i].mods) == CLEANMASK(mods) &&
            xkb_keysym_to_lower(keybinds[i].keysym) == xkb_keysym_to_lower(keysym)) {
            keybinds[i].cb(server, &keybinds[i].arg);
            return true;
        }
    }
    return false;
}

void handle_key(struct wl_listener *listener, void *data)
{
    absn_keyboard *keyboard = wl_container_of(listener, keyboard, key);
    struct wlr_keyboard_key_event *event = data;

    /* translate keycode to xkbcommon */
    uint32_t code = event->keycode + 8;
    uint32_t mods = wlr_keyboard_get_modifiers(keyboard->wlr);
    const xkb_keysym_t *syms;
    int nsyms = xkb_state_key_get_syms(keyboard->wlr->xkb_state, code, &syms);

    bool handled = false;
    if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        for (int i = 0; i < nsyms; ++i) {
            handled = handle_keybind(keyboard->server, mods, syms[i]) || handled;
        }
    }

    if (handled) {
        return;
    }

    wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr);
    wlr_seat_keyboard_notify_key(keyboard->server->seat, event->time_msec, event->keycode, event->state);
}

void handle_destroy(struct wl_listener *listener, void *data)
{
    UNUSED(data);
    absn_keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);

    wl_list_remove(&keyboard->modifiers.link);
    wl_list_remove(&keyboard->key.link);
    wl_list_remove(&keyboard->destroy.link);
    wl_list_remove(&keyboard->link);

    free(keyboard);
}
