#include "types.h"

void client_from_coords(absn_server *server, double x, double y,
                        struct wlr_surface **rsurface,
                        absn_toplevel **rtoplevel,
                        absn_layer_surface **rlayer_surface, double *rx,
                        double *ry);

void reset_cursor_mode(absn_server *server);
void process_cursor_motion(absn_server *server, uint32_t time);
