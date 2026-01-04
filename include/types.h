#pragma once

#include <linux/input-event-codes.h>

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/allocator.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output_management_v1.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>

#define ABSINTHE_CURSOR_MOD WLR_MODIFIER_ALT
#define ABSINTHE_CURSOR_MOVE_BUTTON BTN_LEFT
#define ABSINTHE_CURSOR_RESIZE_BUTTON BTN_RIGHT

enum absinthe_cursor_mode {
    ABSINTHE_CURSOR_PASSTHROUGH,
    ABSINTHE_CURSOR_MOVE,
    ABSINTHE_CURSOR_RESIZE,
};

enum absinthe_cursor_resize_corner {
    ABSINTHE_CURSOR_RESIZE_CORNER_TOP_LEFT,
    ABSINTHE_CURSOR_RESIZE_CORNER_TOP_RIGHT,
    ABSINTHE_CURSOR_RESIZE_CORNER_BOTTOM_LEFT,
    ABSINTHE_CURSOR_RESIZE_CORNER_BOTTOM_RIGHT,
};

struct absinthe_server {
    struct wl_display *display;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
    struct wlr_compositor *compositor;
    struct wlr_scene *scene;
    struct wlr_scene_output_layout *scene_layout;

    struct wlr_xdg_shell *xdg_shell;
    struct wl_listener new_xdg_toplevel;
    struct wl_listener new_xdg_popup;
    struct wl_list toplevels;
    struct wlr_xdg_decoration_manager_v1 *xdg_decoration_mgr;
    struct wl_listener new_xdg_decoration;

    struct wlr_cursor *cursor;
    struct wlr_xcursor_manager *cursor_mgr;
    struct wl_listener cursor_motion;
    struct wl_listener cursor_motion_absolute;
    struct wl_listener cursor_button;
    struct wl_listener cursor_axis;
    struct wl_listener cursor_frame;

    struct wlr_seat *seat;
    struct wl_listener new_input;
    struct wl_listener request_cursor;
    struct wl_listener pointer_focus_change;
    struct wl_listener request_set_selection;
    struct wl_list keyboards;
    enum absinthe_cursor_mode cursor_mode;
    struct absinthe_toplevel *grabbed_toplevel;
    struct wlr_box grabbed_box;
    uint32_t grab_x, grab_y;
    enum absinthe_cursor_resize_corner cursor_resize_corner;

    struct wl_list outputs;
    struct wl_listener new_output;
    struct wlr_output_layout *output_layout;
    struct wl_listener output_layout_change;
    struct wlr_output_manager_v1 *output_mgr;
    struct wl_listener output_mgr_apply;
    struct wl_listener output_mgr_test;
};

struct absinthe_output {
    struct wl_list link;
    struct absinthe_server *server;
    struct wlr_output *wlr_output;
    struct wl_listener frame;
    struct wl_listener request_state;
    struct wl_listener destroy;
};

struct absinthe_toplevel {
    struct wl_list link;
    struct absinthe_server *server;
    struct wlr_scene_tree *scene_tree;
    struct wlr_xdg_toplevel *xdg_toplevel;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener commit;
    struct wl_listener destroy;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_maximize;
    struct wl_listener request_fullscreen;
    struct wlr_xdg_toplevel_decoration_v1 *decoration;
    struct wl_listener decoration_request_mode;
    struct wl_listener decoration_destroy;
};

struct absinthe_popup {
    struct wlr_xdg_popup *xdg_popup;
    struct wl_listener commit;
    struct wl_listener destroy;
};

struct absinthe_keyboard {
    struct wl_list link;
    struct absinthe_server *server;
    struct wlr_keyboard *wlr_keyboard;
    struct wl_listener modifiers;
    struct wl_listener key;
    struct wl_listener destroy;
};
