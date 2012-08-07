/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#ifndef SAVERPYMENU_H
#define SAVERPYMENU_H

#include "menu.h"

void save_rpy(void*);
void create_saverpy_menu(MenuData*);
int saverpy_menu_loop(MenuData*);

#endif
