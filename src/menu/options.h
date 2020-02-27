/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_menu_options_h
#define IGUARD_menu_options_h

#include "taisei.h"

#include "menu.h"

MenuData* create_options_menu(void);
MenuData* create_options_menu_trainer(void);
void draw_options_menu_bg(MenuData*);

#endif // IGUARD_menu_options_h
