/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "menu.h"

void create_main_menu(MenuData *m);
void draw_main_menu_bg(MenuData *m);
void draw_main_menu(MenuData *m);
void main_menu_update_practice_menus(void);
void draw_loading_screen(void);
void menu_preload(void);
