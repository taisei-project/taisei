/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "menu.h"

typedef enum GameoverMenuAction {
	GAMEOVERMENU_ACTION_DEFAULT,
	GAMEOVERMENU_ACTION_RESTART,
	GAMEOVERMENU_ACTION_QUIT,
	GAMEOVERMENU_ACTION_CONTINUE,
	GAMEOVERMENU_ACTION_QUICKLOAD,
} GameoverMenuAction;

typedef struct GameoverMenuParams {
	GameoverMenuAction *output;
	bool quickload_shown;
	bool quickload_enabled;
} GameoverMenuParams;

MenuData *create_gameover_menu(const GameoverMenuParams *params)
	attr_nonnull_all attr_returns_allocated;
