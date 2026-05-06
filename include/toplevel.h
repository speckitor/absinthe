#ifndef __TOPLEVEL_H_
#define __TOPLEVEL_H_

#include "types.h"

absn_toplevel *toplevel_at(absn_server *server, double lx, double ly,
    struct wlr_surface **surface, double *sx, double *sy);

bool toplevel_is_unmanaged(absn_toplevel *toplevel);

void toplevel_get_geom(absn_toplevel *toplevel);

void toplevel_set_pos(absn_toplevel *toplevel, int32_t x, int32_t y);
void toplevel_set_size(absn_toplevel *toplevel, int32_t width, int32_t height);
void toplevel_set_geom(absn_toplevel *toplevel, struct wlr_box *geom);
void toplevel_set_fullscreen(absn_toplevel *toplevel, bool fullscreen);
void toplevel_set_border_color(absn_toplevel *toplevel, const float color[4]);

#endif
