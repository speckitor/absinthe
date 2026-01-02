#include <stdlib.h>

#include <wayland-server-core.h>
#include <xkbcommon/xkbcommon.h>

#include "types.h"

void keyboard_handle_modifiers(struct wl_listener *listener, void *data)
{
    struct absinthe_keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);

    wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr_keyboard);
    wlr_seat_keyboard_notify_modifiers(keyboard->server->seat, &keyboard->wlr_keyboard->modifiers);
}

static bool keyboard_handle_keybind(struct absinthe_server *server, xkb_keysym_t keysym)
{
    switch (keysym) {
    case XKB_KEY_Escape:
        wl_display_terminate(server->display);
        break;
    default:
        return false;
    }
    return true;
}

void keyboard_handle_key(struct wl_listener *listener, void *data)
{
    struct absinthe_keyboard *keyboard = wl_container_of(listener, keyboard, key);
    struct wlr_keyboard_key_event *event = data;

    uint32_t keycode = event->keycode + 8;
    uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);
    const xkb_keysym_t *syms;
    int nsyms = xkb_state_key_get_syms(keyboard->wlr_keyboard->xkb_state, keycode, &syms);

    bool handled = false;
    if ((modifiers & WLR_MODIFIER_ALT) && (event->state == WL_KEYBOARD_KEY_STATE_PRESSED)) {
        for (int i = 0; i < nsyms; ++i) {
            handled = keyboard_handle_keybind(keyboard->server, syms[i]);
        }
    }

    if (!handled) {
        wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr_keyboard);
        wlr_seat_keyboard_notify_key(keyboard->server->seat, event->time_msec, event->keycode, event->state);
    }
}

void keyboard_handle_destroy(struct wl_listener *listener, void *data)
{
    struct absinthe_keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);

    wl_list_remove(&keyboard->modifiers.link);
    wl_list_remove(&keyboard->key.link);
    wl_list_remove(&keyboard->destroy.link);
    wl_list_remove(&keyboard->link);

    free(keyboard);
}
