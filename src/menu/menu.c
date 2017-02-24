/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include <assert.h>
#include "menu.h"
#include "global.h"
#include "video.h"

MenuEntry *add_menu_entry(MenuData *menu, char *name, MenuAction action, void *arg) {
	menu->entries = realloc(menu->entries, (++menu->ecount)*sizeof(MenuEntry));
	MenuEntry *e = menu->entries + menu->ecount - 1;
	memset(e, 0, sizeof(MenuEntry));

	stralloc(&e->name, name);
	e->action = action;
	e->arg = arg;
	e->transition = menu->transition;

	return e;
}

void add_menu_separator(MenuData *menu) {
	menu->entries = realloc(menu->entries, (++menu->ecount)*sizeof(MenuEntry));
	memset(menu->entries + menu->ecount - 1, 0, sizeof(MenuEntry));
}

void destroy_menu(MenuData *menu) {
	for(int i = 0; i < menu->ecount; i++) {
		free(menu->entries[i].name);
	}

	free(menu->entries);
}

void create_menu(MenuData *menu) {
	memset(menu, 0, sizeof(MenuData));

	menu->selected = -1;
	menu->transition = TransMenu; // TransFadeBlack;
	menu->transition_in_time = FADE_TIME;
	menu->transition_out_time = FADE_TIME;
	menu->fade = 1.0;
	menu->logic = menu_logic;
	menu->input = menu_input;
}

void close_menu_finish(MenuData *menu) {
	menu->state = MS_Dead;

	if(menu->selected != -1 && menu->entries[menu->selected].action != NULL) {
		if(!(menu->flags & MF_Transient)) {
			menu->state = MS_Normal;
		}

		menu->entries[menu->selected].action(menu, menu->entries[menu->selected].arg);
	}
}

void close_menu(MenuData *menu) {
	TransitionRule trans = menu->transition;

	assert(menu->state != MS_Dead);
	menu->state = MS_FadeOut;

	if(menu->selected != -1) {
		trans = menu->entries[menu->selected].transition;
	}

	if(trans) {
		set_transition_callback(
			trans,
			menu->transition_in_time,
			menu->transition_out_time,
			(TransitionCallback)close_menu_finish, menu
		);
	} else {
		close_menu_finish(menu);
	}
}

float menu_fade(MenuData *menu) {
	return transition.fade;
}

void menu_event(EventType type, int state, void *arg) {
	MenuData *menu = arg;

	switch(type) {
		case E_CursorDown:
			play_ui_sound("generic_shot");
			do {
				if(++menu->cursor >= menu->ecount)
					menu->cursor = 0;
			} while(menu->entries[menu->cursor].action == NULL);
		break;

		case E_CursorUp:
			play_ui_sound("generic_shot");
			do {
				if(--menu->cursor < 0)
					menu->cursor = menu->ecount - 1;
			} while(menu->entries[menu->cursor].action == NULL);
		break;

		case E_MenuAccept:
			play_ui_sound("shot_special1");
			if(menu->entries[menu->cursor].action) {
				menu->selected = menu->cursor;
				close_menu(menu);
			}
		break;

		case E_MenuAbort:
			play_ui_sound("hit");
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

void menu_no_input(MenuData *menu) {
	handle_events(NULL, 0, NULL);
}

void menu_logic(MenuData *menu) {
	menu->frames++;
}

int menu_loop(MenuData *menu) {
	set_ortho();

	if(menu->begin) {
		menu->begin(menu);
	}

	while(menu->state != MS_Dead) {
		assert(menu->logic);
		menu->logic(menu);

		if(menu->state != MS_FadeOut || menu->flags & MF_AlwaysProcessInput) {
			assert(menu->input);
			menu->input(menu);
		} else {
			menu_no_input(menu);
		}

		assert(menu->draw);
		menu->draw(menu);
		draw_transition();

		SDL_GL_SwapWindow(video.window);
		frame_rate(&menu->lasttime);
	}

	if(menu->end) {
		menu->end(menu);
	}

	destroy_menu(menu);
	return menu->selected;
}
