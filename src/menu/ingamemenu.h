/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "menu.h"

void draw_ingame_menu_bg(MenuData *menu, float f);
void draw_ingame_menu(MenuData *menu);

void create_ingame_menu(MenuData *menu);
void create_ingame_menu_replay(MenuData *m);

void update_ingame_menu(MenuData *menu);

void restart_game(MenuData *m, void *arg);
