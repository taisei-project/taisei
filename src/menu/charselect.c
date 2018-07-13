/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "menu.h"
#include "options.h"
#include "common.h"
#include "global.h"
#include "video.h"

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

void update_char_menu(MenuData *menu) {
	for(int i = 0; i < menu->ecount; i++) {
		menu->entries[i].drawdata += 0.08*(1.0*(menu->cursor != i) - menu->entries[i].drawdata);
	}
}

void create_char_menu(MenuData *m) {
	create_menu(m);
	m->input = char_menu_input;
	m->draw = draw_char_menu;
	m->logic = update_char_menu;
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
	CullFaceMode cull_saved = r_cull_current();

	draw_options_menu_bg(menu);
	draw_menu_title(menu, "Select Character");

	r_mat_push();
	r_color4(0, 0, 0, 0.7);
	r_mat_translate(SCREEN_W/4*3, SCREEN_H/2, 0);
	r_mat_scale(300, SCREEN_H, 1);
	r_shader_standard_notex();
	r_draw_quad();
	r_shader_standard();
	r_mat_pop();

	CharacterID current_char = 0;

	for(int i = 0; i < menu->ecount; i++) {
		PlayerCharacter *pchar = plrchar_get((CharacterID)(uintptr_t)menu->entries[i].arg);
		assert(pchar != NULL);

		const char *spr = pchar->dialog_sprite_name;
		const char *name = pchar->full_name;
		const char *title = pchar->title;

		if(menu->cursor == i) {
			current_char = pchar->id;
		}

		float o = 1-menu->entries[i].drawdata*2;

		r_color4(o, o, o, o);
		draw_sprite(SCREEN_W/3-200*menu->entries[i].drawdata, 2*SCREEN_H/3, spr);

		r_mat_push();
		r_mat_translate(SCREEN_W/4*3, SCREEN_H/3, 0);

		r_mat_push();

		if(menu->entries[i].drawdata != 0) {
			r_mat_translate(0,-300*menu->entries[i].drawdata, 0);
			r_mat_rotate_deg(180*menu->entries[i].drawdata, 1,0,0);
		}

		text_draw(name, &(TextParams) {
			.align = ALIGN_CENTER,
			.font = "big",
			.shader = "text_default",
		});

		r_mat_pop();

		if(menu->entries[i].drawdata) {
			o = 1-menu->entries[i].drawdata*3;
		} else {
			o = 1;
		}

		text_draw(title, &(TextParams) {
			.align = ALIGN_CENTER,
			.pos = { 0, 70 },
			.shader = "text_default",
			.color = RGBA(o, o, o, o),
		});

		r_mat_pop();
	}

	r_mat_push();
	r_mat_translate(SCREEN_W/4*3, SCREEN_H/3, 0);

	for(int i = 0; i < mod->ecount; i++) {
		PlayerMode *mode = plrmode_find(current_char, (ShotModeID)(uintptr_t)mod->entries[i].arg);
		assert(mode != NULL);

		if(mod->cursor == i) {
			r_color4(0.9, 0.6, 0.2, 1);
		} else {
			r_color4(1, 1, 1, 1);
		}

		text_draw(mode->name, &(TextParams) {
			.align = ALIGN_CENTER,
			.pos = { 0, 200+40*i },
			.shader = "text_default",
		});
	}

	r_mat_pop();

	float o = 0.3*sin(menu->frames/20.0)+0.5;
	r_color4(o, o, o, o);

	for(int i = 0; i <= 1; i++) {
		r_mat_push();

		r_mat_translate(60 + (SCREEN_W/2 - 30)*i, SCREEN_H/2+80, 0);

		if(i) {
			r_mat_scale(-1,1,1);
			r_cull(CULL_FRONT);
		} else {
			r_cull(CULL_BACK);
		}

		draw_sprite(0, 0, "menu/arrow");
		r_mat_pop();
	}

	r_color3(1, 1, 1);
	r_cull(cull_saved);
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
		{ NULL }
	}, EFLAG_MENU);
}

void free_char_menu(MenuData *menu) {
	MenuData *mod = menu->context;
	destroy_menu(mod);
	free(mod);
}
