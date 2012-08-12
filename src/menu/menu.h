/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef MENU_H
#define MENU_H

#define IMENU_BLUR 0.05
#define TS_KR_DELAY SDL_DEFAULT_REPEAT_DELAY
#define TS_KR_INTERVAL (SDL_DEFAULT_REPEAT_INTERVAL*2)

enum {	
	FADE_TIME = 15
};

typedef  void (*MenuAction)(void*);

typedef struct {
	char *name;
	MenuAction action;
	void *arg;
	
	int flags;
	
	float drawdata;	
} MenuEntry;

typedef enum MenuFlag { 
	MF_Transient = 1, // whether to close on selection or not.
	MF_Abortable = 2,
	
	MF_InstantSelect = 4
} MenuType;

enum MenuState{
	MS_Normal = 0,
	MS_FadeOut,
	MS_Dead
};

typedef struct MenuData{
	int flags;
	
	int cursor;
	int selected;
	
	MenuEntry *entries;
	int ecount;
	
	int frames;
	int lasttime;
		
	int state;
	int quitframe;
	int quitdelay;
	
	float drawdata[4];	
	
	void *context;
} MenuData;

void add_menu_entry(MenuData *menu, char *name, MenuAction action, void *arg);
void add_menu_entry_f(MenuData *menu, char *name, MenuAction action, void *arg, int flags);

void add_menu_separator(MenuData *menu);
void create_menu(MenuData *menu);
void destroy_menu(MenuData *menu);

void menu_logic(MenuData *menu);
void menu_input(MenuData *menu);

void close_menu(MenuData *menu); // softly close menu (should be used in most cases)
void kill_menu(MenuData *menu); // quit action for persistent menus

void menu_key_action(MenuData *menu, int sym);

int menu_loop(MenuData *menu, void (*input)(MenuData*), void (*draw)(MenuData*), void (*end)(MenuData*));

float menu_fade(MenuData *menu);

void draw_menu_selector(float x, float y, float w, float h, float t);
void draw_menu_title(MenuData *m, char *title);
#endif
