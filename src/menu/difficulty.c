/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "difficulty.h"
#include "options.h"
#include "common.h"
#include "global.h"

void set_difficulty(MenuData *m, void *d) {
	global.diff = (Difficulty) (uintptr_t) d;
}

void create_difficulty_menu(MenuData *m) {
	create_menu(m);
	m->draw = draw_difficulty_menu;
	m->transition = TransMenuDark;
	m->flags = MF_Transient | MF_Abortable;

	add_menu_entry(m, "Easy\nfor fearful fairies", set_difficulty, (void *)D_Easy);
	add_menu_entry(m, "Normal\nfor confident kappa", set_difficulty, (void *)D_Normal);
	add_menu_entry(m, "Hard\nfor omnipotent oni", set_difficulty, (void *)D_Hard);
	add_menu_entry(m, "Lunatic\nfor gods", set_difficulty, (void *)D_Lunatic);
}

void draw_difficulty_menu(MenuData *menu) {
	draw_options_menu_bg(menu);
	draw_menu_title(menu, "Select Difficulty");

	int i;
	for(i = 0; i < menu->ecount; i++) {
		menu->entries[i].drawdata += 0.2 * (30*(i == menu->cursor) - menu->entries[i].drawdata);

		glPushMatrix();
		glTranslatef(SCREEN_W/3 - menu->entries[i].drawdata, 200 + 70*i,0);

		glColor4f(1,1-0.1*i,1-0.1*i,0.7);
		glPushMatrix();
		glTranslatef(145,0,0);
		glScalef(310,60,1);

		draw_quad();
		glPopMatrix();

		glColor4f(0,0,0,1);

		if(i == menu->cursor)
			glColor3f(0.2,0,0.1);
		draw_text(AL_Left, 0, -15, menu->entries[i].name, _fonts.standard);
		glColor3f(1,1,1);
		glPopMatrix();
	}
}
