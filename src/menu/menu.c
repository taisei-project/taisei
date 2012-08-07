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
	
	if(menu->ondestroy)
		menu->ondestroy(menu);
	
	for(i = 0; i < menu->ecount; i++) {
		MenuEntry *e = &(menu->entries[i]);
		
		free(e->name);
		if(e->freearg)
			e->freearg(e->arg);
	}
	
	free(menu->entries);
}

void create_menu(MenuData *menu) {
	memset(menu, 0, sizeof(MenuData));
	
	menu->fade = 1;
	menu->selected = -1;
}

static void key_action(MenuData *menu, int sym) {
	Uint8 *keys = SDL_GetKeyState(NULL);
	
	int look_i_can_hardcode_annoying_menu_hacks_too = menu->title && !strcmp(menu->title, "Save replay?");
	
	if(sym == tconfig.intval[KEY_DOWN]  || sym == SDLK_DOWN || look_i_can_hardcode_annoying_menu_hacks_too &&
	  (sym == tconfig.intval[KEY_RIGHT] || sym == SDLK_RIGHT)) {
		do {
			if(++menu->cursor >= menu->ecount)
				menu->cursor = 0;
		} while(menu->entries[menu->cursor].action == NULL);
	} else if(sym == tconfig.intval[KEY_UP]   || sym == SDLK_UP || look_i_can_hardcode_annoying_menu_hacks_too &&
			 (sym == tconfig.intval[KEY_LEFT] || sym == SDLK_LEFT)) {
		do {
			if(--menu->cursor < 0)
				menu->cursor = menu->ecount - 1;
		} while(menu->entries[menu->cursor].action == NULL);
	} else if((sym == tconfig.intval[KEY_SHOT] || (sym == SDLK_RETURN && !keys[SDLK_LALT] && !keys[SDLK_LALT])) && menu->entries[menu->cursor].action) {
		menu->quit = 1;
		menu->selected = menu->cursor;
	} else if(sym == SDLK_ESCAPE && (menu->type == MT_Transient || menu->abortable)) {
		menu->quit = 1;
		menu->abort = 1;
	}
}

void menu_input(MenuData *menu) {
	SDL_Event event;
	
	while(SDL_PollEvent(&event)) {
		int sym = event.key.keysym.sym;
				
		global_processevent(&event);
		if(event.type == SDL_KEYDOWN)
			key_action(menu,sym);
	}
}

void menu_logic(MenuData *menu) {
	int hax;
	menu->frames++;
	
	if(menu->quit == 1 && menu->fade < 1.0)
		menu->fade += 1.0/FADE_TIME;
	if(menu->quit == 0 && menu->fade > 0)
		menu->fade -= 1.0/FADE_TIME;
	
	if(menu->fade < 0)
		menu->fade = 0;
	if(menu->fade > 1)
		menu->fade = 1;
	
	if(menu->quit == 1 && (menu->fade >= 1.0 || (hax = (!menu->abort && menu->instantselect)))) {
		if(hax)
			menu->fade = 0;
		global.whitefade = 0;
		menu->quit = menu->type == MT_Transient ? 2 : 0;
		
		if(menu->abort && menu->abortaction)
			menu->abortaction(menu->abortarg);
		else if(menu->selected != -1 && menu->entries[menu->selected].action != NULL)
			menu->entries[menu->selected].action(menu->entries[menu->selected].arg);
	}
}

int menu_loop(MenuData *menu, void (*input)(MenuData*), void (*draw)(MenuData*)) {
	set_ortho();
	while(menu->quit != 2) {
		menu_logic(menu);
		if(input)
			input(menu);
		else
			menu_input(menu);
		
		draw(menu);
		SDL_GL_SwapBuffers();
		frame_rate(&menu->lasttime);
	}
	destroy_menu(menu);
	
	return menu->selected;
}
