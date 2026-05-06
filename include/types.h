#ifndef __TYPES_H_
#define __TYPES_H_

#include <linux/input-event-codes.h>
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_output_management_v1.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_shell.h>

#ifdef XWAYLAND
#include <wlr/xwayland.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#endif

#define MAX(A, B) (A) > (B) ? (A) : (B)
#define MIN(A, B) (A) < (B) ? (A) : (B)
/* macro for adding listener for event */
#define LISTEN(L, C, E)                    \
	do {                               \
		(L).notify = (C);          \
		wl_signal_add(&(E), &(L)); \
	} while (0);
#define UNUSED(X) (void)(X)

/* cursor mode */
enum {
	CURSOR_PASSTHROUGH,
	CURSOR_MOVE,
	CURSOR_RESIZE,
};

/* resizing corners */
enum {
	TOP_LEFT,
	TOP_RIGHT,
	BOTTOM_LEFT,
	BOTTOM_RIGHT,
};

/* toplevel types */
enum {
	TOPLEVEL_XDG,
	TOPLEVEL_X11,
};

/* layers for wlr layer shell */
enum {
	LAYER_BACKGROUND,
	LAYER_BOTTOM,
	LAYER_TILE,
	LAYER_FLOAT,
	LAYER_TOP,
	LAYER_FULLSCREEN,
	LAYER_OVERLAY,
	LAYER_LOCK,
	LAYERS_COUNT,
};

typedef struct absn_output absn_output;
typedef struct absn_toplevel absn_toplevel;

typedef struct {
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
	struct wlr_xdg_decoration_manager_v1 *xdg_deco_mgr;
	struct wl_listener new_xdg_deco;

	struct wlr_layer_shell *layer_shell;
	struct wl_listener new_layer_surface;
	struct wlr_scene_tree *layers[LAYERS_COUNT];

#ifdef XWAYLAND
	struct wlr_xwayland *xwayland;
	struct wl_listener xw_new_surface;
	struct wl_listener xw_ready;
#endif

	struct wlr_cursor *cursor;
	struct wlr_xcursor_manager *cursor_mgr;
	struct wl_listener cursor_motion;
	struct wl_listener cursor_motion_abs;
	struct wl_listener cursor_button;
	struct wl_listener cursor_axis;
	struct wl_listener cursor_frame;

	struct wlr_seat *seat;
	struct wl_listener new_input;
	struct wl_listener request_cursor;
	struct wl_listener pointer_focus_change;
	struct wl_listener request_set_selection;
	struct wl_list keyboards;

	int cursor_mode;
	int resize_corner;
	struct wlr_box grab_geom;
	int32_t grab_x, grab_y;

	struct wl_list toplevels;
	struct wl_list focus_stack;
	absn_toplevel *focused_toplevel;

	absn_output *focused_output;
	struct wl_list outputs;
	struct wl_listener new_output;
	struct wlr_output_layout *output_layout;
	struct wl_listener layout_change;
	struct wlr_output_manager_v1 *output_mgr;
	struct wl_listener mgr_apply;
	struct wl_listener mgr_test;
} absn_server;

struct absn_output {
	struct wl_list link;
	absn_server *server;

	struct wlr_box geom;
	struct wlr_box usable_area;

	struct wlr_output *wlr;
	struct wl_listener frame;
	struct wl_listener request_state;
	struct wl_listener destroy;

	float mstack_width;
	int mstack_count;
};

typedef struct {
	absn_server *server;
	absn_output *output;

	struct wlr_scene_tree *scene_tree;
	struct wlr_scene_tree *scene_layer;
	struct wlr_scene_tree *popups;

	struct wlr_layer_surface_v1 *wlr;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener commit;
	struct wl_listener destroy;
} absn_layer_surface;

struct absn_toplevel {
	struct wl_list link;
	struct wl_list flink; /* link for focus stack */
	absn_server *server;
	absn_output *output;

	struct wlr_scene_tree *scene_tree;
	struct wlr_scene_tree *scene_surface;

	int32_t bw;
	struct wlr_scene_rect *border[4];
	struct wlr_xdg_toplevel_decoration_v1 *deco;

	bool tiled, floating, fullscreen, maximized;
	bool urgent;
	uint32_t resizing;

	struct wlr_box geom;
	struct wlr_box prev_geom;

	int type;
	union {
		struct wlr_xdg_toplevel *xdg;
		struct wlr_xwayland_surface *xw;
	};

	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener commit;
	struct wl_listener destroy;
	struct wl_listener request_move;
	struct wl_listener request_resize;
	struct wl_listener request_maximize;
	struct wl_listener request_fullscreen;
	struct wl_listener deco_request_mode;
	struct wl_listener deco_destroy;

#ifdef XWAYLAND
	struct wl_listener xw_activate;
	struct wl_listener xw_associate;
	struct wl_listener xw_dissociate;
	struct wl_listener xw_configure;
	struct wl_listener xw_set_hints;
#endif
};

typedef struct {
	struct wlr_xdg_popup *wlr;
	struct wl_listener commit;
	struct wl_listener destroy;
} absn_popup;

typedef struct {
	struct wl_list link;
	absn_server *server;

	struct wlr_keyboard *wlr;
	struct wl_listener modifiers;
	struct wl_listener key;
	struct wl_listener destroy;
} absn_keyboard;

#endif /* __TYPES_H_ */
