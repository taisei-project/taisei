/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef DIFFICULTY_H
#define DIFFICULTY_H

#include "menu.h"

void create_difficulty_menu(MenuData *menu);
void draw_difficulty_menu(MenuData *m);
void difficulty_menu_loop(MenuData *m);

#endif