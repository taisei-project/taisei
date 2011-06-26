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

#include "global.h"

void quit_menu(void *arg) {
	MenuData *m = arg;
	m->quit = 2;
}

void start_story(void *arg) {
	MenuData m;

troll:
	create_difficulty_menu(&m);
	if(difficulty_menu_loop(&m) == -1)
		return;
	
	create_char_menu(&m);
	if(char_menu_loop(&m) == -1)
		goto troll;
	
	init_player(&global.plr, global.plrtype, global.plrmode);
	
	stage0_loop();
}

void create_main_menu(MenuData *m) {
	create_menu(m);
	
	m->type = MT_Persistent;
	
	add_menu_entry(m, "Start Story", start_story, NULL);
	add_menu_entry(m, "Start Extra", NULL, NULL);
	add_menu_entry(m, "Options", NULL, NULL);
	add_menu_entry(m, "Quit", quit_menu, m);
}


void draw_main_menu_bg(MenuData* menu) {
	draw_texture(SCREEN_W/2, SCREEN_H/2, "mainmenu/mainmenubgbg");
	glColor4f(1,1,1,0.6 + 0.1*sin(menu->frames/100.0));
	draw_texture(SCREEN_W/2, SCREEN_H/2, "mainmenu/mainmenubg");
	glColor4f(1,1,1,1);
}

void draw_main_menu(MenuData *menu) {
	draw_main_menu_bg(menu);
	
	glColor4f(1,1,1,0.7);
	draw_texture(SCREEN_W/2+40, SCREEN_H/2, "mainmenu/gate");
		
	glPushMatrix();
	glTranslatef(0, SCREEN_H-200, 0);
	
	glPushMatrix();
	glTranslatef(0, menu->drawdata[2], 0);
	glColor4f(0,0,0,0.5);
	glBegin(GL_QUADS);
		glVertex3f(0,-17, 0);
		glVertex3f(0,17, 0);
		glVertex3f(270, 17, 0);
		glVertex3f(270,-17, 0);
	glEnd();
	glPopMatrix();
	
	int i;

	for(i = 0; i < menu->ecount; i++) {
		float s = 5*sin(menu->frames/80.0 + 20*i);
		
		float grey = 1;
		if(menu->entries[i].action == NULL)
			grey = 0.5;
		
		glColor4f(grey,grey,grey,0.7);
		draw_text(AL_Left, 50 + s, 35*i, menu->entries[i].name, _fonts.mainmenu);		
	}
	
	glPopMatrix();
	
	menu->drawdata[2] += (35*menu->cursor - menu->drawdata[2])/10.0;
	
	fade_out(menu->fade);
	
	glColor4f(1,1,1,1);	
}

void main_menu_loop(MenuData *menu) {
	menu_loop(menu, NULL, draw_main_menu);
}