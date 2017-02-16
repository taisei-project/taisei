/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "mainmenu.h"
#include "menu.h"

#include "common.h"
#include "options.h"
#include "stageselect.h"
#include "replayview.h"
#include "savereplay.h"

#include "global.h"
#include "stage.h"
#include "paths/native.h"

void enter_options(MenuData *menu, void *arg) {
	MenuData m;
	create_options_menu(&m);
	options_menu_loop(&m);
}

void enter_stagemenu(MenuData *menu, void *arg) {
	MenuData m;
	create_stage_menu(&m);
	stage_menu_loop(&m);
}

void enter_replayview(MenuData *menu, void *arg) {
	MenuData m;
	create_replayview_menu(&m);
	replayview_menu_loop(&m);
}

void create_main_menu(MenuData *m) {
	create_menu(m);

	add_menu_entry(m, "Start Story", start_game, NULL);
	add_menu_entry(m, "Start Extra", NULL, NULL);
#ifdef DEBUG
	add_menu_entry(m, "Select Stage", enter_stagemenu, NULL);
#endif
	add_menu_entry(m, "Replays", enter_replayview, NULL);
	add_menu_entry(m, "Options", enter_options, NULL);
	add_menu_entry(m, "Quit", (MenuAction)kill_menu, m);
}


void draw_main_menu_bg(MenuData* menu) {
	glColor4f(1,1,1,1);
	draw_texture(SCREEN_W/2, SCREEN_H/2, "mainmenu/mainmenubgbg");
	glColor4f(1,1,1,0.95 + 0.05*sin(menu->frames/100.0));

	draw_texture(SCREEN_W/2, SCREEN_H/2, "mainmenu/mainmenubg");
	glColor4f(1,1,1,1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void draw_main_menu(MenuData *menu) {
	draw_main_menu_bg(menu);

	draw_texture(150, 100, "mainmenu/logo");

	glPushMatrix();
	glTranslatef(0, SCREEN_H-200, 0);

	Texture *bg = get_tex("part/smoke");
	glPushMatrix();
	glTranslatef(50 + menu->drawdata[1]/2, menu->drawdata[2], 0);	// 135
	glScalef(menu->drawdata[1]/100.0, 0.5, 1);
	glRotatef(menu->frames*2,0,0,1);
	glColor4f(0,0,0,0.5);
	draw_texture_p(0,0,bg);
	glPopMatrix();

	int i;

	menu->drawdata[1] += (stringwidth(menu->entries[menu->cursor].name, _fonts.mainmenu) - menu->drawdata[1])/10.0;
	menu->drawdata[2] += (35*menu->cursor - menu->drawdata[2])/10.0;

	for(i = 0; i < menu->ecount; i++) {
		float s = 5*sin(menu->frames/80.0 + 20*i);
		menu->entries[i].drawdata += 0.2 * ((i == menu->cursor) - menu->entries[i].drawdata);

		if(menu->entries[i].action == NULL) {
			glColor4f(0.5,0.5,0.5,0.7);
		} else {
			//glColor4f(1,1,1,0.7);
			float a = 1 - menu->entries[i].drawdata;
			glColor4f(1, 0.7 + a, 0.4 + a, 0.7);
		}

		draw_text(AL_Left, 50 + s, 35*i, menu->entries[i].name, _fonts.mainmenu);
	}

	glPopMatrix();
}

void main_menu_loop(MenuData *menu) {
	set_transition(TransLoader, -1, FADE_TIME*2);
	menu_loop(menu, NULL, draw_main_menu, NULL);
}
