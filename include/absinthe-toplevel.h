#pragma once

#include "types.h"

struct absinthe_toplevel *absinthe_toplevel_at(struct absinthe_server *server, double lx, double ly,
					       struct wlr_surface **surface, double *sx, double *sy);

bool absinthe_toplevel_is_unmanaged(struct absinthe_toplevel *toplevel);

void absinthe_toplevel_update_geometry(struct absinthe_toplevel *toplevel);
void absinthe_toplevel_update_borders_geometry(struct absinthe_toplevel *toplevel);

void absinthe_toplevel_set_position(struct absinthe_toplevel *toplevel, int32_t x, int32_t y);
void absinthe_toplevel_set_size(struct absinthe_toplevel *toplevel, int32_t width, int32_t height);
void absinthe_toplevel_set_fullscreen(struct absinthe_toplevel *toplevel, bool fullscreen);
void absinthe_toplevel_set_border_color(struct absinthe_toplevel *toplevel, const float color[4]);
