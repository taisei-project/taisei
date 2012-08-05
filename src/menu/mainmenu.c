/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "mainmenu.h"
#include "menu.h"

#include "difficulty.h"
#include "charselect.h"
#include "options.h"
#include "stageselect.h"
#include "replayview.h"

#include "global.h"
#include "stage.h"
#include "ending.h"
#include "credits.h"
#include "paths/native.h"

void quit_menu(void *arg) {
	MenuData *m = arg;
	m->quit = 2;
}

void start_story(void *arg) {
	MenuData m;

	init_player(&global.plr);
	
troll:
	create_difficulty_menu(&m);
	if(difficulty_menu_loop(&m) == -1)
		return;
	
	create_char_menu(&m);
	if(char_menu_loop(&m) == -1)
		goto troll;
	
	if(arg)
		((StageInfo*)arg)->loop();
	else {
		int i;
		for(i = 0; stages[i].loop; ++i)
			stages[i].loop();
	}
	
	global.game_over = 0;
	
	ending_loop();
	credits_loop();
}

void enter_options(void *arg) {
	MenuData m;
	create_options_menu(&m);
	options_menu_loop(&m);
}

void enter_stagemenu(void *arg) {
	MenuData m;
	create_stage_menu(&m);
	stage_menu_loop(&m);
}

void enter_replayview(void *arg) {
	MenuData m;
	create_replayview_menu(&m);
	replayview_menu_loop(&m);
}

void create_main_menu(MenuData *m) {
	create_menu(m);
	
	m->type = MT_Persistent;
	
	add_menu_entry(m, "Start Story", start_story, NULL);
	add_menu_entry(m, "Start Extra", NULL, NULL);
#ifdef DEBUG
	add_menu_entry(m, "Select Stage", enter_stagemenu, NULL);
#endif
	add_menu_entry(m, "Replays", enter_replayview, NULL);
	add_menu_entry(m, "Options", enter_options, NULL);
	add_menu_entry(m, "Quit", quit_menu, m);
}


void draw_main_menu_bg(MenuData* menu) {
	draw_texture(SCREEN_W/2, SCREEN_H/2, "mainmenu/mainmenubgbg");
	glColor4f(1,1,1,0.9 + 0.1*sin(menu->frames/100.0));
	
	draw_texture(SCREEN_W/2, SCREEN_H/2, "mainmenu/mainmenubg");
	glColor4f(1,1,1,1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void draw_main_menu(MenuData *menu) {
	draw_main_menu_bg(menu);
		
	draw_texture(150, 100, "mainmenu/logo");
	
	glPushMatrix();
	glTranslatef(0, SCREEN_H-200, 0);
	
	glPushMatrix();
	glTranslatef(135, menu->drawdata[2], 0);
	glScalef(270, 34, 1);
	
	glColor4f(0,0,0,0.7);
	
	draw_quad();
	
	glPopMatrix();
	
	int i;

	int grey = 2;
	
	for(i = 0; i < menu->ecount; i++) {
		float s = 5*sin(menu->frames/80.0 + 20*i);
				
		if(menu->entries[i].action == NULL && grey != 1) {
			glColor4f(0.5,0.5,0.5,0.7);
			grey = 1;
		} else if(grey != 0) {
			glColor4f(1,1,1,0.7);
			grey = 0;
		}
		
		draw_text(AL_Left, 50 + s, 35*i, menu->entries[i].name, _fonts.mainmenu);				
	}
	
	glPopMatrix();
	
	menu->drawdata[2] += (35*menu->cursor - menu->drawdata[2])/10.0;
	
	fade_out(menu->fade);
}

void main_menu_loop(MenuData *menu) {
	menu_loop(menu, NULL, draw_main_menu);
}
