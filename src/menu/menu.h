/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef MENU_H
#define MENU_H

#define IMENU_BLUR 0.05
#define FADE_TIME 15

typedef struct {
	char *name;
	void (*action)(void* arg);
	void *arg;
	float drawdata;
} MenuEntry;

typedef enum MenuType { // whether to close on selection or not.
	MT_Transient, // ingame menus are for the transient people.
	MT_Persistent
} MenuType;

typedef struct MenuData{
	MenuType type;
	
	int cursor;
	int selected;
	
	MenuEntry *entries;
	int ecount;
	
	int frames;
	int lasttime;
		
	int quit;
	float fade;
	
	float drawdata[4];
	
	void *context;
	void (*ondestroy)(void*);
} MenuData;

void add_menu_entry(MenuData *menu, char *name, void (*action)(void *), void *arg);
void create_menu(MenuData *menu);
void destroy_menu(MenuData *menu);

void menu_logic(MenuData *menu);
void menu_input(MenuData *menu);

int menu_loop(MenuData *menu, void (*input)(MenuData*), void (*draw)(MenuData*));


#endif
