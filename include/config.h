#ifndef __CONFIG_H_
#define __CONFIG_H_

#include "callbacks.h"
#include "types.h"

#define CURSOR_MOD	     WLR_MODIFIER_ALT
#define CURSOR_MOVE_BUTTON   BTN_LEFT
#define CURSOR_RESIZE_BUTTON BTN_RIGHT

#define TOPLEVEL_BW	     1

static const float focused_bc[4] = { 0.0, 0.0, 1.0, 1.0 };
static const float urgent_bc[4] = { 1.0, 0.0, 0.0, 1.0 };
static const float unfocused_bc[4] = { 0.28, 0.28, 0.28, 1.0 };

#define MSTACK_SIZE  1
#define MSTACK_WIDTH 0.5

#define OUTPUT_GAP   0
#define LAYOUT_GAP   0

#define ALT	     WLR_MODIFIER_ALT
#define CTRL	     WLR_MODIFIER_CTRL
#define SHIFT	     WLR_MODIFIER_SHIFT
#define LOGO	     WLR_MODIFIER_LOGO

static const absn_keybind keybinds[] = {
	{ ALT, XKB_KEY_Return, &run, { .v = "alacritty" } },
	{ ALT, XKB_KEY_q, kill_focus, { 0 } },
	{ ALT, XKB_KEY_j, cycle_focus, { .i = +1 } },
	{ ALT, XKB_KEY_k, cycle_focus, { .i = -1 } },
	{ ALT, XKB_KEY_f, toggle_fullscreen, { 0 } },
	{ ALT, XKB_KEY_h, increase_master_count, { .i = +1 } },
	{ ALT, XKB_KEY_l, increase_master_count, { .i = -1 } },
	{ ALT | SHIFT, XKB_KEY_h, increase_master_width, { .f = -0.05 } },
	{ ALT | SHIFT, XKB_KEY_l, increase_master_width, { .f = +0.05 } },
	{ ALT, XKB_KEY_E, quit, { 0 } },
};

#endif
