/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "menu.h"
#include "global.h"

void add_menu_entry(MenuData *menu, char *name, void (*action)(void *), void *arg) {
	menu->entries = realloc(menu->entries, (++menu->ecount)*sizeof(MenuEntry));
	MenuEntry *e = &(menu->entries[menu->ecount-1]);
	memset(e, 0, sizeof(MenuEntry));
	
	e->name = malloc(strlen(name)+1);
	strcpy(e->name, name);
	e->action = action;
	e->arg = arg;
}

void add_menu_separator(MenuData *menu) {
	menu->entries = realloc(menu->entries, (++menu->ecount)*sizeof(MenuEntry));
	memset(&(menu->entries[menu->ecount-1]), 0, sizeof(MenuEntry));
}

void destroy_menu(MenuData *menu) {
	int i;
		
	for(i = 0; i < menu->ecount; i++) {
		MenuEntry *e = &(menu->entries[i]);
		
		free(e->name);
	}
	
	free(menu->entries);
}

void create_menu(MenuData *menu) {
	memset(menu, 0, sizeof(MenuData));
	
	menu->selected = -1;
	menu->quitdelay = FADE_TIME;
}

void close_menu(MenuData *menu) {
	menu->quitframe = menu->frames;
}

void kill_menu(MenuData *menu) {
	menu->state = MS_Dead;
}

float menu_fade(MenuData *menu) {
	if(menu->frames < menu->quitdelay)
		return 1.0 - menu->frames/(float)menu->quitdelay;
	
	if(menu->quitframe && menu->frames >= menu->quitframe)
		return (menu->frames - menu->quitframe)/(float)menu->quitdelay;
	
	return 0.0;
}

void menu_key_action(MenuData *menu, int sym) {
	Uint8 *keys = SDL_GetKeyState(NULL);
		
	if(sym == tconfig.intval[KEY_DOWN] || sym == SDLK_DOWN) {
		do {
			if(++menu->cursor >= menu->ecount)
				menu->cursor = 0;
		} while(menu->entries[menu->cursor].action == NULL);
	} else if(sym == tconfig.intval[KEY_UP] || sym == SDLK_UP) {
		do {
			if(--menu->cursor < 0)
				menu->cursor = menu->ecount - 1;
		} while(menu->entries[menu->cursor].action == NULL);
	} else if((sym == tconfig.intval[KEY_SHOT] || (sym == SDLK_RETURN && !keys[SDLK_LALT] && !keys[SDLK_RALT])) && menu->entries[menu->cursor].action) {
		menu->selected = menu->cursor;
		
		close_menu(menu);
	} else if(sym == SDLK_ESCAPE && menu->flags & MF_Abortable) {
		close_menu(menu);
		menu->selected = -1;
	}
}

void menu_input(MenuData *menu) {
	SDL_Event event;
	
	while(SDL_PollEvent(&event)) {
		int sym = event.key.keysym.sym;
				
		global_processevent(&event);
		if(event.type == SDL_KEYDOWN)
			menu_key_action(menu,sym);
	}
}

void menu_logic(MenuData *menu) {
	menu->frames++;
	
	if(menu->quitframe && menu->frames >= menu->quitframe)
		menu->state = MS_FadeOut;
		
	if(menu->quitframe && menu->frames >= menu->quitframe + menu->quitdelay) {
		menu->state = MS_Dead;
		if(menu->selected != -1 && menu->entries[menu->selected].action != NULL) {
			if(!(menu->flags & MF_Transient)) {
				menu->state = MS_Normal;
				menu->quitframe = 0;
			}
			
			menu->entries[menu->selected].action(menu->entries[menu->selected].arg);			
		}
	}
}

int menu_loop(MenuData *menu, void (*input)(MenuData*), void (*draw)(MenuData*), void (*end)(MenuData*)) {
	set_ortho();
	while(menu->state != MS_Dead) {
		menu_logic(menu);
		
		if(menu->state != MS_FadeOut) {
			if(input)
				input(menu);
			else
				menu_input(menu);
		}	
		
		draw(menu);
		SDL_GL_SwapBuffers();
		frame_rate(&menu->lasttime);
	}
	
	if(end)
		end(menu);
	destroy_menu(menu);
		
	return menu->selected;
}

void draw_menu_selector(float x, float y, float w, float h, float t) {
	Texture *bg = get_tex("part/smoke");
	glPushMatrix();
	glTranslatef(x, y, 0);
	glScalef(w, h, 1);
	glRotatef(t*2,0,0,1);
	glColor4f(0,0,0,0.5);
	draw_texture_p(0,0,bg);
	glPopMatrix();
}

void draw_menu_title(MenuData *m, char *title) {
	draw_text(AL_Right, (stringwidth(title, _fonts.mainmenu) + 10) * (1.0-menu_fade(m)), 30, title, _fonts.mainmenu);
}