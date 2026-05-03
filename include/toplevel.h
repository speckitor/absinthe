#ifndef __TOPLEVEL_H_
#define __TOPLEVEL_H_

#include "types.h"

struct absinthe_toplevel *toplevel_at(struct absinthe_server *server, double lx,
    double ly, struct wlr_surface **surface, double *sx, double *sy);

bool toplevel_is_unmanaged(struct absinthe_toplevel *toplevel);

void toplevel_update_geom(struct absinthe_toplevel *toplevel);
void toplevel_update_borders_geom(struct absinthe_toplevel *toplevel);

void toplevel_set_pos(struct absinthe_toplevel *toplevel, int32_t x, int32_t y);
void toplevel_set_size(struct absinthe_toplevel *toplevel, int32_t width,
    int32_t height);
void toplevel_set_fullscreen(struct absinthe_toplevel *toplevel,
    bool fullscreen);
void toplevel_set_border_color(struct absinthe_toplevel *toplevel,
    const float color[4]);

#endif
