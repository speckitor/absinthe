#pragma once

#define ABSINTHE_CURSOR_MOD WLR_MODIFIER_ALT
#define ABSINTHE_CURSOR_MOVE_BUTTON BTN_LEFT
#define ABSINTHE_CURSOR_RESIZE_BUTTON BTN_RIGHT

#define ABSINTHE_TOPLEVEL_BORDER_WIDTH 1

static const float focused_border_color[4] = {0.64, 0.75, 0.55, 1.0};
static const float urgent_border_color[4] = {0.84, 0.47, 0.5, 1.0};
static const float unfocused_border_color[4] = {0.39, 0.42, 0.46, 1.0};

#define ABSINTHE_MAIN_STACK_SIZE 1;
#define ABSINTHE_MAIN_STACK_WIDTH 0.5;

#define ABSINTHE_OUTPUT_GAP 10
#define ABSINTHE_LAYOUT_GAP 10
