/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "menu.h"
#include "gamesession.h"

typedef struct IngameMenuContext {
	const char *title;
	GameSession *game;
} IngameMenuContext;

void draw_ingame_menu_bg(MenuData *menu, float f);
void draw_ingame_menu(MenuData *menu);

MenuData *create_ingame_menu(GameSession *game);
MenuData *create_ingame_menu_replay(GameSession *game);

void update_ingame_menu(MenuData *menu);

void restart_game(MenuData *m, void *arg);
