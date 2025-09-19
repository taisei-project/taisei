/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "menu.h"
#include "resource/resource.h"

MenuData *create_main_menu(void);
void draw_main_menu_bg(MenuData *m, double center_x, double center_y, double R, const char *tex1, const char *tex2);
void draw_main_menu(MenuData *m);
void main_menu_update_practice_menus(void);
void draw_loading_screen(void);
void menu_preload(ResourceGroup *rg);
