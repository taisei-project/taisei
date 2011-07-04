/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "menu.h"
#include "mainmenu.h"
#include "global.h"

void set_player(void *p) {
	plr_set_char(&global.plr, (Character) p);
}

void set_shotmode(void *p) {
	global.plr.shot = (ShotMode) p;
}

void create_char_menu(MenuData *m) {
	create_menu(m);
	
	add_menu_entry(m, "dialog/marisa|Kirisame Marisa|Black Magician", set_player, (void *)Marisa);
	add_menu_entry(m, "dialog/youmu|Konpaku Youmu|Half Ghost Girl", set_player, (void *)Youmu);
}

void create_shottype_menu(MenuData *m) {
	create_menu(m);
	
	add_menu_entry(m, "Foo Sign|Mirror Sign", set_shotmode, (void *) YoumuOpposite);
	add_menu_entry(m, "Bar Sign|Haunting Sign", set_shotmode, (void *) YoumuHoming);
}

void draw_char_menu(MenuData *menu, MenuData *mod) {
	draw_main_menu_bg(menu);
	draw_text(AL_Right, 220*(1-menu->fade), 30, "Player Select", _fonts.mainmenu);
	
	glPushMatrix();
	glColor4f(0,0,0,0.7);
	glTranslatef(SCREEN_W/4*3, 0, 0);
	
	glBegin(GL_QUADS);
	glVertex3f(-150,0, 0);
	glVertex3f(-150,SCREEN_H, 0);
	glVertex3f(150,SCREEN_H, 0);
	glVertex3f(150,0, 0);
	glEnd();
	glPopMatrix();
	
	char buf[128];
	int i;
	for(i = 0; i < menu->ecount; i++) {
		strncpy(buf, menu->entries[i].name, sizeof(buf));
		
		char *tex = strtok(buf,"|");
		char *name = strtok(NULL, "|");
		char *title = strtok(NULL, "|");
		
		if(!(tex && name && title))
			continue;
		
		menu->entries[i].drawdata += 0.08*(1.0*(menu->cursor != i) - menu->entries[i].drawdata);
		
		glColor4f(1,1,1,1-menu->entries[i].drawdata*2);
		draw_texture(SCREEN_W/3-200*menu->entries[i].drawdata, 2*SCREEN_H/3, tex);
		
		glPushMatrix();
		glTranslatef(SCREEN_W/4*3, SCREEN_H/3, 0);
		
		glPushMatrix();
		glTranslatef(0,-300*menu->entries[i].drawdata, 0);
		glRotatef(180*menu->entries[i].drawdata, 1,0,0);
		draw_text(AL_Center, 0, 0, name, _fonts.mainmenu);
		glPopMatrix();
		
		glColor4f(1,1,1,1-menu->entries[i].drawdata*3);
		draw_text(AL_Center, 0, 70, title, _fonts.standard);
		
		strncpy(buf, mod->entries[i].name, sizeof(buf));
		
		char *mari = strtok(buf, "|");
		char *youmu = strtok(NULL, "|");
		
		char *use = menu->entries[menu->cursor].arg == (void *)Marisa ? mari : youmu;
		
		glColor4f(1,1,1,1);
		if(mod->cursor == i)
			glColor4f(0.9,0.6,0.2,1);
		draw_text(AL_Center, 0, 200+40*i, use, _fonts.standard);
		
		glPopMatrix();
	}
	glColor4f(1,1,1,0.3*sin(menu->frames/20.0)+0.5);
	
	for(i = 0; i <= 1; i++) {
		glPushMatrix();		
		
		glTranslatef(60 + (SCREEN_W/2 - 30)*i, SCREEN_H/2+80, 0);
		glScalef(1-2*i,1,1);
		if(i) glCullFace(GL_FRONT);
		glBegin(GL_TRIANGLES);
		glVertex3f(0,0,0);
		glVertex3f(20,30,0);
		glVertex3f(20,-30,0);
		glEnd();
		
		glPopMatrix();
	}
	
	glCullFace(GL_BACK);
	glColor4f(1,1,1,1);
	
	fade_out(menu->fade);
}

void char_menu_input(MenuData *menu, MenuData *mod) {
	SDL_Event event;
	
	while(SDL_PollEvent(&event)) {
		int sym = event.key.keysym.sym;
		
		global_processevent(&event);
		if(event.type == SDL_KEYDOWN) {
			if(sym == tconfig.intval[KEY_RIGHT]) {
				menu->cursor++;
			} else if(sym == tconfig.intval[KEY_LEFT]) {
				menu->cursor--;
			} else if(sym == tconfig.intval[KEY_DOWN]) {
				mod->cursor++;
			} else if(sym == tconfig.intval[KEY_UP]) {
				mod->cursor--;
			} else if((sym == tconfig.intval[KEY_SHOT] || sym == SDLK_RETURN) && menu->entries[menu->cursor].action) {
				menu->quit = 1;
				menu->selected = menu->cursor;
				mod->quit = 1;
				mod->selected = mod->cursor;
				mod->fade = 1;
			} else if(sym == SDLK_ESCAPE && menu->type == MT_Transient) {
				menu->quit = 1;
			}
			
			menu->cursor = (menu->cursor % menu->ecount) + menu->ecount*(menu->cursor < 0);
			mod->cursor = (mod->cursor % mod->ecount) + mod->ecount*(mod->cursor < 0);
		} else if(event.type == SDL_QUIT) {
			exit(1);
		}
	}
}

int char_menu_loop(MenuData *menu) {
	set_ortho();
	
	MenuData mod;
	create_shottype_menu(&mod);
	
	while(menu->quit != 2) {
		menu_logic(menu);
		menu_logic(&mod);
		char_menu_input(menu, &mod);
		
		draw_char_menu(menu, &mod);
		SDL_GL_SwapBuffers();
		frame_rate(&menu->lasttime);
	}
	destroy_menu(menu);
	
	
	return menu->selected;
}
