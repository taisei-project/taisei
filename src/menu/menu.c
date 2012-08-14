/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "menu.h"
#include "global.h"

MenuEntry *add_menu_entry(MenuData *menu, char *name, MenuAction action, void *arg) {
	return add_menu_entry_f(menu, name, action, arg, 0);
}

MenuEntry *add_menu_entry_f(MenuData *menu, char *name, MenuAction action, void *arg, int flags) {
	menu->entries = realloc(menu->entries, (++menu->ecount)*sizeof(MenuEntry));
	MenuEntry *e = &(menu->entries[menu->ecount-1]);
	memset(e, 0, sizeof(MenuEntry));
	
	e->name = malloc(strlen(name)+1);
	strcpy(e->name, name);
	e->action = action;
	e->arg = arg;
	e->flags = flags;
	e->transition = menu->transition;
	return e;
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
	menu->transition = TransFadeBlack;
}
	
void close_menu(MenuData *menu) {
	TransitionRule trans = menu->transition;
		
	if(menu->selected != -1)
		trans = menu->entries[menu->selected].transition;
	
	set_transition(trans, menu->quitdelay, menu->quitdelay);	
	
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

void menu_event(EventType type, int state, void *arg) {
	MenuData *menu = arg;
	
	switch(type) {
		case E_CursorDown:
			do {
				if(++menu->cursor >= menu->ecount)
					menu->cursor = 0;
			} while(menu->entries[menu->cursor].action == NULL);
		break;
		
		case E_CursorUp:
			do {
				if(--menu->cursor < 0)
					menu->cursor = menu->ecount - 1;
			} while(menu->entries[menu->cursor].action == NULL);
		break;
		
		case E_MenuAccept:
			if(menu->entries[menu->cursor].action) {
				menu->selected = menu->cursor;
				close_menu(menu);
			}
		break;
		
		case E_MenuAbort:
			if(menu->flags & MF_Abortable) {
				menu->selected = -1;
				close_menu(menu);
			}
		break;
		
		default: break;
	}
}

void menu_input(MenuData *menu) {
	handle_events(menu_event, EF_Menu, (void*)menu);
}

void menu_logic(MenuData *menu) {
	menu->frames++;
	
	if(menu->quitframe && menu->frames >= menu->quitframe)
		menu->state = MS_FadeOut;
	
	if(menu->quitframe && menu->frames >= menu->quitframe + menu->quitdelay*!(menu->state & MF_InstantSelect || menu->selected != -1 && menu->entries[menu->selected].flags & MF_InstantSelect)) {
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
		if(!(menu->flags & MF_ManualDrawTransition))
			draw_transition();
		
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
	
