/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef MENU_H
#define MENU_H

#include <stdbool.h>
#include "transition.h"

#define IMENU_BLUR 0.05

#include "events.h"

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
	TransitionRule transition;
} MenuEntry;

// enum EntryFlag {
//	MF_InstantSelect = 4
// };

enum MenuFlag {
	MF_Transient = 1, // whether to close on selection or not.
	MF_Abortable = 2,

	MF_InstantSelect = 4,
	MF_ManualDrawTransition = 8, // the menu will not call draw_transition() automatically
	MF_AlwaysProcessInput = 16 // the menu will process input even during fadeouts
};

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

	TransitionRule transition;

	float drawdata[4];

	void *context;
} MenuData;

MenuEntry *add_menu_entry(MenuData *menu, char *name, MenuAction action, void *arg);
MenuEntry *add_menu_entry_f(MenuData *menu, char *name, MenuAction action, void *arg, int flags);

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

void menu_event(EventType type, int state, void *arg);

#endif
