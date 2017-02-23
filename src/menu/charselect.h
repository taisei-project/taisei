/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef CHARSELECT_H
#define CHARSELECT_H

#include "menu.h"

void create_char_menu(MenuData *m);
void draw_char_menu(MenuData *menu);
int char_menu_loop(MenuData *menu);


#endif
