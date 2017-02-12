/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef MENUCOMMON_H
#define MENUCOMMON_H

#include "menu.h"

void start_game(void *arg);
void draw_menu_selector(float x, float y, float w, float h, float t);
void draw_menu_title(MenuData *m, char *title);
void draw_menu_list(MenuData *m, float x, float y, void (*draw)(void*, int, int));
void animate_menu_list(MenuData *m);

#endif
