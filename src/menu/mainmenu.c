/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "mainmenu.h"
#include "menu.h"
#include "global.h"

void quit_menu(void *arg) {
	MenuData *m = arg;
	m->quit = 2;
}

void start_story(void *arg) {
// 	MenuData m;
// 	create_difficulty_menu(&m);
// 	difficulty_menu_loop(m);
// 	
// 	create_char_menu(&m);
// 	character_menu_loop(m);
	
	stage0_loop();
}

void start_extra(void *arg) {
}

void start_options(void *arg) {
}

void create_main_menu(MenuData *m) {
	create_menu(m);
	
	m->type = MT_Persistent;
	
	add_menu_entry(m, "Start Story", start_story, NULL);
	add_menu_entry(m, "Start Extra", start_extra, NULL);
	add_menu_entry(m, "Options", start_options, NULL);
	add_menu_entry(m, "Quit", quit_menu, m);
}


void draw_main_menu(MenuData *menu) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, SCREEN_W, SCREEN_H, 0, -10, 10);
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_DEPTH_TEST);
	
	draw_texture(SCREEN_W/2, SCREEN_H/2, "mainmenubgbg");
	glColor4f(1,1,1,0.6 + 0.1*sin(menu->frames/100.0));
	draw_texture(SCREEN_W/2, SCREEN_H/2, "mainmenubg");
	
	
	glColor4f(1,1,1,0.7);
	draw_texture(SCREEN_W/2+40, SCREEN_H/2, "gate");
	glColor4f(1,1,1,0.2 + 0.2*sin(menu->frames/70.0));
	draw_texture(SCREEN_W/2+40, SCREEN_H/2, "gate_pi");
	
	
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
		
		glColor4f(1,1,1,0.7 + 0.1*sin(menu->frames/70.0));
		draw_text(AlLeft, 50 + s, 35*i, menu->entries[i].name, _fonts.mainmenu);		
	}
	
	glPopMatrix();
	
	menu->drawdata[2] += (35*menu->cursor - menu->drawdata[2])/10.0;
	
	fade_out(menu->fade);
	
	glColor4f(1,1,1,1);	
}

void main_menu_loop(MenuData *menu) {
	while(menu->quit != 2) {
		menu_logic(menu);
		menu_input(menu);
		
		draw_main_menu(menu);
		SDL_GL_SwapBuffers();
		SDL_Delay(16);
	}
	destroy_menu(menu);
}