/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <stdbool.h>
#include "transition.h"

#define IMENU_BLUR 0.05

#include "events.h"

enum {
	FADE_TIME = 15
};

typedef struct MenuData MenuData;

typedef void (*MenuAction)(MenuData*, void*);
typedef bool (*MenuCallback)(MenuData*);
typedef void (*MenuProc)(MenuData*);

typedef struct MenuEntry {
	char *name;
	MenuAction action;
	void *arg;
	float drawdata;
	TransitionRule transition;
} MenuEntry;

enum MenuFlag {
	MF_Transient = 1,           // the menu will be automatically closed on selection
	MF_Abortable = 2,           // the menu can be closed with the escape key
	MF_AlwaysProcessInput = 4   // the menu will process input when it's fading out
};

enum MenuState {
	MS_Normal = 0,
	MS_FadeOut,
	MS_Dead
};

struct MenuData {
	int flags;

	int cursor;
	int selected;

	MenuEntry *entries;
	int ecount;

	int frames;

	int state;
	int transition_in_time;
	int transition_out_time;
	float fade;

	TransitionRule transition;

	float drawdata[4];

	void *context;

	MenuProc draw;
	MenuProc input;
	MenuProc logic;
	MenuProc begin;
	MenuProc end;
};

MenuEntry *add_menu_entry(MenuData *menu, char *name, MenuAction action, void *arg);
MenuEntry *add_menu_entry_f(MenuData *menu, char *name, MenuAction action, void *arg, int flags);

void add_menu_separator(MenuData *menu);
void create_menu(MenuData *menu);
void destroy_menu(MenuData *menu);

void menu_input(MenuData *menu);
void menu_no_input(MenuData *menu);

void close_menu(MenuData *menu);

void menu_key_action(MenuData *menu, int sym);

int menu_loop(MenuData *menu);

float menu_fade(MenuData *menu);

bool menu_input_handler(SDL_Event *event, void *arg);
