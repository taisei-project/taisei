/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2011, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */
 
#ifndef STGMENU_H
#define STGMENU_H

#define STGMENU_MAX_TITLE_LENGTH 128

void create_stage_menu(MenuData *m);
void draw_menu_list(MenuData *m, float x, float y, void (*draw)(void *, int, int));
int stage_menu_loop(MenuData *m);

#endif
