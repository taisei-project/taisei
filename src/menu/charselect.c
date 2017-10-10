/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "menu.h"
#include "options.h"
#include "common.h"
#include "global.h"

void set_player(MenuData *m, void *p) {
	progress.game_settings.character = (CharacterID)(uintptr_t)p;
}

void set_shotmode(MenuData *m, void *p) {
	progress.game_settings.shotmode = (ShotModeID)(uintptr_t)p;
}

void create_shottype_menu(MenuData *m) {
	create_menu(m);
	m->transition = NULL;

	for(uintptr_t i = 0; i < NUM_SHOT_MODES_PER_CHARACTER; ++i) {
		add_menu_entry(m, NULL, set_shotmode, (void*)i);

		if(i == progress.game_settings.shotmode) {
			m->cursor = i;
		}
	}
}

void char_menu_input(MenuData*);
void draw_char_menu(MenuData*);
void free_char_menu(MenuData*);

void create_char_menu(MenuData *m) {
	create_menu(m);
	m->input = char_menu_input;
	m->draw = draw_char_menu;
	m->end = free_char_menu;
	m->transition = TransMenuDark;
	m->flags = MF_Abortable | MF_Transient;
	m->context = malloc(sizeof(MenuData));
	create_shottype_menu(m->context);

	for(uintptr_t i = 0; i < NUM_CHARACTERS; ++i) {
		add_menu_entry(m, NULL, set_player, (void*)i)->transition = TransFadeBlack;

		if(i == progress.game_settings.character) {
			m->cursor = i;
		}
	}
}

void draw_char_menu(MenuData *menu) {
	MenuData *mod = ((MenuData *)menu->context);

	draw_options_menu_bg(menu);
	draw_menu_title(menu, "Select Character");

	glPushMatrix();
	glColor4f(0,0,0,0.7);
	glTranslatef(SCREEN_W/4*3, SCREEN_H/2, 0);
	glScalef(300, SCREEN_H, 1);
	draw_quad();
	glPopMatrix();

	CharacterID current_char;

	for(int i = 0; i < menu->ecount; i++) {
		PlayerCharacter *pchar = plrchar_get((CharacterID)(uintptr_t)menu->entries[i].arg);
		assert(pchar != NULL);

		const char *tex = pchar->dialog_sprite_name;
		const char *name = pchar->full_name;
		const char *title = pchar->title;

		if(menu->cursor == i) {
			current_char = pchar->id;
		}

		menu->entries[i].drawdata += 0.08*(1.0*(menu->cursor != i) - menu->entries[i].drawdata);

		glColor4f(1,1,1,1-menu->entries[i].drawdata*2);
		draw_texture(SCREEN_W/3-200*menu->entries[i].drawdata, 2*SCREEN_H/3, tex);

		glPushMatrix();
		glTranslatef(SCREEN_W/4*3, SCREEN_H/3, 0);

		glPushMatrix();

		if(menu->entries[i].drawdata != 0) {
			glTranslatef(0,-300*menu->entries[i].drawdata, 0);
			glRotatef(180*menu->entries[i].drawdata, 1,0,0);
		}

		draw_text(AL_Center, 0, 0, name, _fonts.mainmenu);
		glPopMatrix();

		if(menu->entries[i].drawdata) {
			glColor4f(1,1,1,1-menu->entries[i].drawdata*3);
		} else {
			glColor4f(1,1,1,1);
		}

		draw_text(AL_Center, 0, 70, title, _fonts.standard);
		glPopMatrix();
	}

	glPushMatrix();
	glTranslatef(SCREEN_W/4*3, SCREEN_H/3, 0);

	for(int i = 0; i < mod->ecount; i++) {
		PlayerMode *mode = plrmode_find(current_char, (ShotModeID)(uintptr_t)mod->entries[i].arg);
		assert(mode != NULL);

		if(mod->cursor == i) {
			glColor4f(0.9,0.6,0.2,1);
		} else {
			glColor4f(1,1,1,1);
		}

		draw_text(AL_Center, 0, 200+40*i, mode->name, _fonts.standard);
	}

	glPopMatrix();
	glColor4f(1,1,1,0.3*sin(menu->frames/20.0)+0.5);

	for(int i = 0; i <= 1; i++) {
		glPushMatrix();

		glTranslatef(60 + (SCREEN_W/2 - 30)*i, SCREEN_H/2+80, 0);

		if(i) {
			glScalef(-1,1,1);
			glCullFace(GL_FRONT);
		}

		draw_texture(0,0,"charselect_arrow");

		glPopMatrix();

		if(i)
			glCullFace(GL_BACK);
	}

	glColor3f(1,1,1);
}

bool char_menu_input_handler(SDL_Event *event, void *arg) {
	MenuData *menu = arg;
	MenuData *mod  = menu->context;
	TaiseiEvent type = TAISEI_EVENT(event->type);

	if(type == TE_MENU_CURSOR_RIGHT) {
		play_ui_sound("generic_shot");
		menu->cursor++;
	} else if(type == TE_MENU_CURSOR_LEFT) {
		play_ui_sound("generic_shot");
		menu->cursor--;
	} else if(type == TE_MENU_CURSOR_DOWN) {
		play_ui_sound("generic_shot");
		mod->cursor++;
	} else if(type == TE_MENU_CURSOR_UP) {
		play_ui_sound("generic_shot");
		mod->cursor--;
	} else if(type == TE_MENU_ACCEPT) {
		play_ui_sound("shot_special1");
		mod->selected = mod->cursor;
		close_menu(mod);
		menu->selected = menu->cursor;
		close_menu(menu);

		// XXX: This needs a better fix
		set_shotmode(mod, mod->entries[mod->selected].arg);
	} else if(type == TE_MENU_ABORT) {
		play_ui_sound("hit");
		close_menu(menu);
		close_menu(mod);
	}

	menu->cursor = (menu->cursor % menu->ecount) + menu->ecount*(menu->cursor < 0);
	mod->cursor = (mod->cursor % mod->ecount) + mod->ecount*(mod->cursor < 0);

	return false;
}

void char_menu_input(MenuData *menu) {
	events_poll((EventHandler[]){
		{ .proc = char_menu_input_handler, .arg = menu },
		{NULL}
	}, EFLAG_MENU);
}

void free_char_menu(MenuData *menu) {
	MenuData *mod = menu->context;
	destroy_menu(mod);
	free(mod);
}
