/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "charselect.h"
#include "menu.h"
#include "mainmenu.h"
#include "common.h"
#include "global.h"
#include "video.h"

#define SELECTED_SUBSHOT(m) ((intptr_t)PLR_SHOT_A + (intptr_t)(m)->context)

static void set_player_mode(MenuData *m, void *p) {
	progress.game_settings.character = (CharacterID)(uintptr_t)p;
	progress.game_settings.shotmode = SELECTED_SUBSHOT(m);
}

static void char_menu_input(MenuData*);

static void update_char_menu(MenuData *menu) {
	for(int i = 0; i < menu->ecount; i++) {
		menu->entries[i].drawdata += 0.08*((menu->cursor != i) - menu->entries[i].drawdata);
	}
}

MenuData* create_char_menu(void) {
	MenuData *m = alloc_menu();

	m->input = char_menu_input;
	m->draw = draw_char_menu;
	m->logic = update_char_menu;
	m->transition = TransFadeBlack;
	m->flags = MF_Abortable;
	m->context = (void*)(intptr_t)progress.game_settings.shotmode;

	for(uintptr_t i = 0; i < NUM_CHARACTERS; ++i) {
		add_menu_entry(m, NULL, set_player_mode, (void*)i)->transition = TransFadeBlack;

		if(i == progress.game_settings.character) {
			m->cursor = i;
		}
	}

	return m;
}

void draw_char_menu(MenuData *menu) {
	CullFaceMode cull_saved = r_cull_current();

	draw_main_menu_bg(menu, SCREEN_W/2+100, 0, 0.1*(1-menu->entries[menu->cursor].drawdata));
	draw_menu_title(menu, "Select Character");

	r_mat_push();
	r_color4(0, 0, 0, 0.7);
	r_mat_translate(0, SCREEN_H/2, 0);
	r_mat_scale(700, SCREEN_H, 1);
	r_shader_standard_notex();
	//r_draw_quad();
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
		draw_sprite(SCREEN_W/2+260+200*menu->entries[i].drawdata, 2*SCREEN_H/3, spr);

		r_mat_push();
		r_mat_translate(SCREEN_W/4, SCREEN_H/3, 0);

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
			.pos = { 60-20*o, 30 },
			.shader = "text_default",
			.color = RGBA(o, o, o, o),
		});

		r_mat_pop();
	}

	r_mat_push();
	r_mat_translate(SCREEN_W/4, SCREEN_H/3, 0);

	ShotModeID current_subshot = SELECTED_SUBSHOT(menu);

	for(ShotModeID shot = PLR_SHOT_A; shot < NUM_SHOT_MODES_PER_CHARACTER; shot++) {
		PlayerMode *mode = plrmode_find(current_char, shot);
		assume(mode != NULL);

		if(shot == current_subshot) {
			r_color4(0.9, 0.6, 0.2, 1);
		} else {
			r_color4(1, 1, 1, 1);
		}

		text_draw(mode->name, &(TextParams) {
			.align = ALIGN_CENTER,
			.pos = { 0, 200 + 40 * (shot - PLR_SHOT_A) },
			.shader = "text_default",
		});
	}

	r_mat_pop();

	float o = 0.3*sin(menu->frames/20.0)+0.5;
	o *= 1-menu->entries[menu->cursor].drawdata;
	r_color4(o, o, o, o);

	for(int i = 0; i <= 1; i++) {
		r_mat_push();

		r_mat_translate(30 + 340*i, SCREEN_H/3+10, 0);
		r_mat_scale(0.5,0.7,1);

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

static bool char_menu_input_handler(SDL_Event *event, void *arg) {
	MenuData *menu = arg;
	TaiseiEvent type = TAISEI_EVENT(event->type);

	if(type == TE_MENU_CURSOR_RIGHT) {
		play_ui_sound("generic_shot");
		menu->cursor++;
	} else if(type == TE_MENU_CURSOR_LEFT) {
		play_ui_sound("generic_shot");
		menu->cursor--;
	} else if(type == TE_MENU_CURSOR_DOWN) {
		play_ui_sound("generic_shot");
		menu->context = (void*)(SELECTED_SUBSHOT(menu) + 1);
	} else if(type == TE_MENU_CURSOR_UP) {
		play_ui_sound("generic_shot");
		menu->context = (void*)(SELECTED_SUBSHOT(menu) - 1);
	} else if(type == TE_MENU_ACCEPT) {
		play_ui_sound("shot_special1");
		menu->selected = menu->cursor;
		close_menu(menu);
	} else if(type == TE_MENU_ABORT) {
		play_ui_sound("hit");
		close_menu(menu);
	}

	menu->cursor = (menu->cursor % menu->ecount) + menu->ecount*(menu->cursor < 0);

	intptr_t ss = SELECTED_SUBSHOT(menu);
	ss = (ss % NUM_SHOT_MODES_PER_CHARACTER) + NUM_SHOT_MODES_PER_CHARACTER * (ss < 0);
	menu->context = (void*)ss;

	return false;
}

static void char_menu_input(MenuData *menu) {
	events_poll((EventHandler[]){
		{ .proc = char_menu_input_handler, .arg = menu },
		{ NULL }
	}, EFLAG_MENU);
}
