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

MenuData *create_char_menu(void);
void draw_char_menu(MenuData *menu);
void preload_char_menu(ResourceGroup *rg);
