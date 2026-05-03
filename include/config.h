#ifndef __CONFIG_H_
#define __CONFIG_H_

#define CURSOR_MOD	     WLR_MODIFIER_ALT
#define CURSOR_MOVE_BUTTON   BTN_LEFT
#define CURSOR_RESIZE_BUTTON BTN_RIGHT

#define TOPLEVEL_BW	     1

static const float focused_bc[4] = { 0.64, 0.75, 0.55, 1.0 };
static const float urgent_bc[4] = { 0.84, 0.47, 0.5, 1.0 };
static const float unfocused_bc[4] = { 0.39, 0.42, 0.46, 1.0 };

#define MSTACK_SIZE  1
#define MSTACK_WIDTH 0.5

#define OUTPUT_GAP   0
#define LAYOUT_GAP   0
#endif
