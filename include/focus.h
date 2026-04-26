#pragma once

#include "types.h"

void focus_toplevel(struct absinthe_toplevel *toplevel);

struct absinthe_toplevel *focus_get_topmost(struct absinthe_server *server);

struct absinthe_toplevel *focus_get_first_tiled(struct absinthe_output *output);
struct absinthe_toplevel *focus_get_last_tiled(struct absinthe_output *output);

void focus_next(struct absinthe_server *server);
void focus_prev(struct absinthe_server *server);
