/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef INGAMEMENU_H
#define INGAMEMENU_H

#include "menu.h"

void draw_ingame_menu_bg(MenuData *menu, float f);
void draw_ingame_menu(MenuData *menu);

void create_ingame_menu(MenuData *menu);
void create_ingame_menu_replay(MenuData *m);

void restart_game(MenuData *m, void *arg);

#endif
