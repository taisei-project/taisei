/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "transition.h"
#include "util/callchain.h"
#include "dynarray.h"

#include <SDL_events.h>

#define IMENU_BLUR 0.05

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
	MF_AlwaysProcessInput = 4,  // the menu will process input when it's fading out
	MF_NoDemo = 8,              // demo playback is disabled while in this menu
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

	DYNAMIC_ARRAY(MenuEntry) entries;

	int frames;

	int state;
	int transition_in_time;
	int transition_out_time;
	float fade;

	TransitionRule transition;

	float drawdata[4];

	void *context;

	MenuProc begin;
	MenuProc draw;
	MenuProc input;
	MenuProc logic;
	MenuProc end;

	CallChain cc;
};

MenuData *alloc_menu(void);
void free_menu(MenuData *menu);
void close_menu(MenuData *menu);
void kill_menu(MenuData *menu);

// You will get a pointer to the MenuData in callchain result.
// WARNING: Unless the menu state is MS_Dead at the time your callback is invoked, it may be called again!
// This must be accounted for, otherwise demons will literally fly out of your nose.
// No joke. Not *might*, they actually *will*. I wouldn't recommend testing that out.
void enter_menu(MenuData *menu, CallChain next);

MenuEntry *add_menu_entry(MenuData *menu, const char *name, MenuAction action, void *arg);
MenuEntry *add_menu_entry_f(MenuData *menu, char *name, MenuAction action, void *arg, int flags);
void add_menu_separator(MenuData *menu);

void menu_input(MenuData *menu);
void menu_no_input(MenuData *menu);

float menu_fade(MenuData *menu);

bool menu_input_handler(SDL_Event *event, void *arg);
