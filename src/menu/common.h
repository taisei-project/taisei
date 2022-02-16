/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "menu.h"

void start_game(MenuData *m, void *arg);
void start_game_no_difficulty_menu(MenuData *m, void *arg);
void draw_menu_selector(float x, float y, float w, float h, float t);
void draw_menu_title(MenuData *m, const char *title);
void draw_menu_list(MenuData *m, float x, float y, void (*draw)(MenuEntry*, int, int, void*), float scroll_threshold, void *userdata);
void animate_menu_list(MenuData *m);
void animate_menu_list_entries(MenuData *m);
void animate_menu_list_entry(MenuData *m, int i);
void menu_action_close(MenuData *menu, void *arg);
