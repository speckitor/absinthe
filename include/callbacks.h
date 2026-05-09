#ifndef __KEYBINDS_CALLBACKS_H
#define __KEYBINDS_CALLBACKS_H

#include "types.h"

void run(absn_server *server, const absn_arg *arg);

void kill_focus(absn_server *server, const absn_arg *arg);
void cycle_focus(absn_server *server, const absn_arg *arg);
void toggle_fullscreen(absn_server *server, const absn_arg *arg);

void increase_master_width(absn_server *server, const absn_arg *arg);
void increase_master_count(absn_server *server, const absn_arg *arg);

void quit(absn_server *server, const absn_arg *arg);

#endif
