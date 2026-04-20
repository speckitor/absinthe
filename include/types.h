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

#ifdef XWAYLAND
#include <wlr/xwayland.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#endif

#include "config.h"

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

enum absinthe_toplevel_type {
    ABSINTHE_TOPLEVEL_XDG,
    ABSINTHE_TOPLEVEL_LAYER_SHELL,
    ABSINTHE_TOPLEVEL_X11,
};

enum absinthe_layers {
    ABSINTHE_LAYER_BACKGROUND,
    ABSINTHE_LAYER_BOTTOM,
    ABSINTHE_LAYER_TILE,
    ABSINTHE_LAYER_FLOAT,
    ABSINTHE_LAYER_TOP,
    ABSINTHE_LAYER_FULLSCREEN,
    ABSINTHE_LAYER_OVERLAY,
    ABSINTHE_LAYER_LOCK,
    ABSINTHE_LAYERS_COUNT,
};

struct absinthe_output;

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
    struct wlr_xdg_decoration_manager_v1 *xdg_decoration_mgr;
    struct wl_listener new_xdg_decoration;

#ifdef XWAYLAND
    struct wlr_xwayland *xwayland;
    struct wl_listener xwayland_new_surface;
    struct wl_listener xwayland_ready;
#endif

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
    struct wlr_box grabbed_geometry;
    uint32_t grab_x, grab_y;
    enum absinthe_cursor_resize_corner cursor_resize_corner;

    struct wl_list toplevels;
    struct wl_list focus_stack;
    struct absinthe_toplevel *focused_toplevel;

    struct absinthe_output *focused_output;
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
    struct wlr_box geometry;
    struct wlr_output *wlr_output;
    struct wl_listener frame;
    struct wl_listener request_state;
    struct wl_listener destroy;

    float main_stack_width;
    float main_stack_size;
};

struct absinthe_toplevel {
    enum absinthe_toplevel_type type;

    struct wl_list link;
    struct wl_list flink;
    struct absinthe_server *server;
    struct absinthe_output *output;
    struct wlr_scene_tree *scene_tree;
    struct wlr_scene_tree *scene_surface;

    int32_t border_width;
    struct wlr_scene_rect *border[4];
    struct wlr_xdg_toplevel_decoration_v1 *decoration;

    bool floating;
    bool tiled;
    bool fullscreen;
    bool performing_resize;

    struct wlr_box geometry;
    struct wlr_box prev_geometry;

    union {
        struct wlr_xdg_toplevel *xdg;
        struct wlr_xwayland_surface *x11;
    } toplevel;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener commit;
    struct wl_listener destroy;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_maximize;
    struct wl_listener request_fullscreen;
    struct wl_listener decoration_request_mode;
    struct wl_listener decoration_destroy;

#ifdef XWAYLAND
    struct wl_listener xwayland_activate;
    struct wl_listener xwayland_associate;
    struct wl_listener xwayland_dissociate;
    struct wl_listener xwayland_configure;
    struct wl_listener xwayland_set_hints;
#endif
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
