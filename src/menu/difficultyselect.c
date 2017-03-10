/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "difficulty.h"
#include "difficultyselect.h"
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

	add_menu_entry(m, "“All those bullets confuse me!”\nYou will be stuck here forever", set_difficulty, (void *)D_Easy);
	add_menu_entry(m, "“So this isn’t about shooting things?”\nSomewhat challenging", set_difficulty, (void *)D_Normal);
	add_menu_entry(m, "“Why can’t I beat this?”\nActually challenging", set_difficulty, (void *)D_Hard);
	add_menu_entry(m, "“What is pain?”\nAsian mode", set_difficulty, (void *)D_Lunatic);
}

void draw_difficulty_menu(MenuData *menu) {
	draw_options_menu_bg(menu);
	draw_menu_title(menu, "Select Difficulty");


	menu->drawdata[0] += (menu->cursor-menu->drawdata[0])*0.1;
	
	glColor4f(0.1*menu->drawdata[0],0,0,0.7);
	glPushMatrix();
	glTranslatef(SCREEN_W/2+30 - 25*menu->drawdata[0], SCREEN_H/3 + 90*(0.7*menu->drawdata[0]),0);
	glRotatef(4*menu->drawdata[0]-4,0,0,1);
	glPushMatrix();
	glScalef(SCREEN_W*1.5,120,1);
	draw_quad();
	glPopMatrix();
	glColor3f(1,1,1);
	draw_text(AL_Left, 40+35*menu->drawdata[0], -12, menu->entries[menu->cursor].name, _fonts.standard);

	glPopMatrix();

	int i;
	for(i = 0; i < menu->ecount; i++) {
		menu->entries[i].drawdata += 0.2 * (30*(i == menu->cursor) - menu->entries[i].drawdata);
	
		glPushMatrix();
		glTranslatef(SCREEN_W/2 - (int)menu->entries[i].drawdata+25*i-50, SCREEN_H/3 + 90*(i-0.3*menu->drawdata[0]),0);

		//glColor4f(0,0,0,1);

		glColor3f(1,1,1);
		draw_texture(0,0,difficulty_tex(D_Easy+i));
		glPopMatrix();
	}
}
