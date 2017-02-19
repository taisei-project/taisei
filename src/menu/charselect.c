/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "menu.h"
#include "options.h"
#include "common.h"
#include "global.h"

void set_player(MenuData *m, void *p) {
	global.plr.cha = (Character) (uintptr_t) p;
}

void set_shotmode(MenuData *m, void *p) {
	global.plr.shot = (ShotMode) (uintptr_t) p;
}

void create_shottype_menu(MenuData *m) {
	create_menu(m);

	add_menu_entry(m, "Laser Sign|Mirror Sign", set_shotmode, (void *) YoumuOpposite);
	add_menu_entry(m, "Star Sign|Haunting Sign", set_shotmode, (void *) YoumuHoming);
}

void create_char_menu(MenuData *m) {
	create_menu(m);

	m->flags = MF_Abortable | MF_Transient;
	m->context = malloc(sizeof(MenuData));
	create_shottype_menu(m->context);

	add_menu_entry(m, "dialog/marisa|Kirisame Marisa|Black Magician", set_player, (void *)Marisa);
	add_menu_entry(m, "dialog/youmu|Konpaku Yōmu|Half-Phantom Girl", set_player, (void *)Youmu);
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

	char buf[128];
	int i;
	for(i = 0; i < menu->ecount; i++) {
		strlcpy(buf, menu->entries[i].name, sizeof(buf));

		char *tex = strtok(buf,"|");
		char *name = strtok(NULL, "|");
		char *title = strtok(NULL, "|");

		if(!(tex && name && title))
			continue;

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

		if(menu->entries[i].drawdata)
			glColor4f(1,1,1,1-menu->entries[i].drawdata*3);
		draw_text(AL_Center, 0, 70, title, _fonts.standard);

		strlcpy(buf, mod->entries[i].name, sizeof(buf));

		char *mari = strtok(buf, "|");
		char *youmu = strtok(NULL, "|");

		char *use = menu->entries[menu->cursor].arg == (void *)Marisa ? mari : youmu;

		if(menu->entries[i].drawdata)
			glColor4f(1,1,1,1);

		if(mod->cursor == i)
			glColor4f(0.9,0.6,0.2,1);
		draw_text(AL_Center, 0, 200+40*i, use, _fonts.standard);

		glPopMatrix();
	}
	glColor4f(1,1,1,0.3*sin(menu->frames/20.0)+0.5);

	for(i = 0; i <= 1; i++) {
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

void char_menu_input_event(EventType type, int state, void *arg) {
	MenuData *menu = arg;
	MenuData *mod  = menu->context;

	if(type == E_CursorRight) {
		play_ui_sound("generic_shot");
		menu->cursor++;
	} else if(type == E_CursorLeft) {
		play_ui_sound("generic_shot");
		menu->cursor--;
	} else if(type == E_CursorDown) {
		play_ui_sound("generic_shot");
		mod->cursor++;
	} else if(type == E_CursorUp) {
		play_ui_sound("generic_shot");
		mod->cursor--;
	} else if(type == E_MenuAccept) {
		play_ui_sound("shot_special1");
		mod->selected = mod->cursor;
		close_menu(mod);
		menu->selected = menu->cursor;
		close_menu(menu);

		// XXX: This needs a better fix
		set_shotmode(mod, mod->entries[mod->selected].arg);
	} else if(type == E_MenuAbort) {
		play_ui_sound("hit");
		close_menu(menu);
		close_menu(mod);
	}

	menu->cursor = (menu->cursor % menu->ecount) + menu->ecount*(menu->cursor < 0);
	mod->cursor = (mod->cursor % mod->ecount) + mod->ecount*(mod->cursor < 0);
}

void char_menu_input(MenuData *menu) {
	handle_events(char_menu_input_event, EF_Menu, menu);
}

void free_char_menu(MenuData *menu) {
	MenuData *mod = menu->context;
	destroy_menu(mod);
	free(mod);
}

int char_menu_loop(MenuData *menu) {
	return menu_loop(menu, char_menu_input, draw_char_menu, free_char_menu);
}
