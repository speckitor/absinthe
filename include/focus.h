#ifndef __FOCUS_H_
#define __FOCUS_H_

#include "types.h"

void unfocus_toplevel(absn_toplevel *toplevel);
void focus_toplevel(absn_toplevel *toplevel);
absn_toplevel *focus_get_topmost(absn_server *server);

#endif
