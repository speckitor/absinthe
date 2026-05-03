#ifndef __FOCUS_H_
#define __FOCUS_H_

#include "types.h"

void focus_toplevel(struct absinthe_toplevel *toplevel);

struct absinthe_toplevel *focus_get_topmost(struct absinthe_server *server);

void focus_next(struct absinthe_server *server);
void focus_prev(struct absinthe_server *server);

#endif
