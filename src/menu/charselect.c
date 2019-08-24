/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "charselect.h"
#include "menu.h"
#include "mainmenu.h"
#include "common.h"
#include "global.h"
#include "video.h"
#include "util/glm.h"

#define SELECTED_SUBSHOT(m) (((CharMenuContext*)(m)->context)->subshot)
#define DESCRIPTION_WIDTH (SCREEN_W / 3 + 40)

typedef struct CharMenuContext {
	ShotModeID subshot;
	CharacterID char_draw_order[NUM_CHARACTERS];
	CharacterID prev_selected_char;
} CharMenuContext;

static void set_player_mode(MenuData *m, void *p) {
	progress.game_settings.character = (CharacterID)(uintptr_t)p;
	progress.game_settings.shotmode = SELECTED_SUBSHOT(m);
}

static void char_menu_input(MenuData*);

static void update_char_draw_order(MenuData *menu) {
	CharMenuContext *ctx = menu->context;

	for(int i = 0; i < NUM_CHARACTERS; ++i) {
		if(ctx->char_draw_order[i] == menu->cursor) {
			while(i) {
				ctx->char_draw_order[i] = ctx->char_draw_order[i - 1];
				ctx->char_draw_order[i - 1] = menu->cursor;
				--i;
			}

			break;
		}
	}
}

static void update_char_menu(MenuData *menu) {
	for(int i = 0; i < menu->ecount; i++) {
		menu->entries[i].drawdata += 0.05*((menu->cursor != i) - menu->entries[i].drawdata);
	}

	PlayerCharacter *pchar = plrchar_get((CharacterID)(uintptr_t)menu->entries[menu->cursor].arg);
	assume(pchar != NULL);

	PlayerMode *m = plrmode_find(pchar->id, SELECTED_SUBSHOT(menu));
	assume(m != NULL);

	Font *font = get_font("standard");
	char buf[256] = { 0 };
	text_wrap(font, m->description, DESCRIPTION_WIDTH, buf, sizeof(buf));
	double height = text_height(font, buf, 0) + font_get_lineskip(font) * 2;

	fapproach_asymptotic_p(&menu->drawdata[0], SELECTED_SUBSHOT(menu) - PLR_SHOT_A, 0.1, 1e-5);
	fapproach_asymptotic_p(&menu->drawdata[1], 1 - menu->entries[menu->cursor].drawdata, 0.1, 1e-5);
	fapproach_asymptotic_p(&menu->drawdata[2], height, 0.1, 1e-5);
}

static void end_char_menu(MenuData *m) {
	free(m->context);
}

static void transition_to_game(double fade) {
	fade_out(pow(max(0, (fade - 0.5) * 2), 2));
}

MenuData* create_char_menu(void) {
	MenuData *m = alloc_menu();

	m->input = char_menu_input;
	m->draw = draw_char_menu;
	m->logic = update_char_menu;
	m->end = end_char_menu;
	m->transition = TransFadeBlack;
	m->flags = MF_Abortable;

	CharMenuContext *ctx = calloc(1, sizeof(*ctx));
	ctx->subshot = progress.game_settings.shotmode;
	ctx->prev_selected_char = -1;
	m->context = ctx;

	for(uintptr_t i = 0; i < NUM_CHARACTERS; ++i) {
		MenuEntry *e = add_menu_entry(m, NULL, set_player_mode, (void*)i);
		e->transition = transition_to_game;
		e->drawdata = 1;

		if(i == progress.game_settings.character) {
			m->cursor = i;
		}
	}

	for(CharacterID c = 0; c < NUM_CHARACTERS; ++c) {
		ctx->char_draw_order[c] = c;
	}

	m->drawdata[1] = 1;

	return m;
}

void draw_char_menu(MenuData *menu) {
	CharMenuContext *ctx = menu->context;

	r_state_push();

	char *prefixes[] = {
		"Intuition",
		"Science",
	};

	assert(menu->cursor < 3);
	PlayerCharacter *selected_char = plrchar_get((CharacterID)(uintptr_t)menu->entries[menu->cursor].arg);

	draw_main_menu_bg(menu, SCREEN_W/4+100, 0, 0.1 * (0.5 + 0.5 * menu->drawdata[1]), "menu/mainmenubg", selected_char->menu_texture_name);
	draw_menu_title(menu, "Select Character");

	CharacterID current_char = 0;

	for(int j = 0; j < menu->ecount; j++) {
		CharacterID i = ctx->char_draw_order[j];

		PlayerCharacter *pchar = plrchar_get((CharacterID)(uintptr_t)menu->entries[i].arg);
		assert(pchar != NULL);
		assert(pchar->id == i);

		Sprite *spr = get_sprite(pchar->dialog_base_sprite_name);
		const char *name = pchar->full_name;
		const char *title = pchar->title;

		if(menu->cursor == i) {
			current_char = pchar->id;
		}

		float o = 1-menu->entries[i].drawdata*2;

		float pofs = max(0, menu->entries[i].drawdata * 1.5 - 0.5);
		pofs = glm_ease_back_in(pofs);

		if(i != menu->selected) {
			pofs = lerp(pofs, 1, menu_fade(menu));
		}

		float pbrightness = 0.6 + 0.4 * o;

		SpriteParams portrait_params = {
			.pos = { SCREEN_W/2 + 240 + 320 * pofs, SCREEN_H - spr->h * 0.5 },
			.sprite_ptr = spr,
			.shader = "sprite_default",
			.color = RGBA(pbrightness, pbrightness, pbrightness, 1),
			// .flip.x = true,
		};

		r_draw_sprite(&portrait_params);

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
			.color = RGBA(o, o, o, o),
		});

		r_mat_pop();

		if(menu->entries[i].drawdata) {
			o = 1-menu->entries[i].drawdata*3;
		} else {
			o = 1;
		}

		text_draw(title, &(TextParams) {
			.align = ALIGN_CENTER,
			.pos = { 20*(1-o), 30 },
			.shader = "text_default",
			.color = RGBA(o, o, o, o),
		});

		r_mat_pop();
	}

	r_mat_push();
	r_mat_translate(SCREEN_W/4, SCREEN_H/3, 0);

	ShotModeID current_subshot = SELECTED_SUBSHOT(menu);

	float f = menu->drawdata[0]-PLR_SHOT_A;
	float selbg_ofs = 200 + (100-70)*f-20*f - font_get_lineskip(get_font("standard")) * 0.7;

	r_color4(0, 0, 0, 0.5);
	r_shader_standard_notex();
	r_mat_push();
	r_mat_translate(-150, selbg_ofs + menu->drawdata[2] * 0.5, 0);
	r_mat_scale(650, menu->drawdata[2], 1);
	r_draw_quad();
	r_shader_standard();
	r_mat_pop();

	for(ShotModeID shot = PLR_SHOT_A; shot < NUM_SHOT_MODES_PER_CHARACTER; shot++) {
		PlayerMode *mode = plrmode_find(current_char, shot);
		assume(mode != NULL);
		int shotidx = shot-PLR_SHOT_A;

		float o = 1-fabs(f - shotidx);
		float al = 0.2+o;
		if(shot == current_subshot && shot == PLR_SHOT_A) {
			r_color4(0.9*al, 0.6*al, 0.2*al, 1*al);
		} else if(shot == current_subshot && shot == PLR_SHOT_B) {
			r_color4(0.2*al, 0.6*al, 0.9*al, 1*al);
		} else {
			r_color4(al, al, al, al);
		}

		char buf[64];
		snprintf(buf, sizeof(buf), "%s: %s", prefixes[shot - PLR_SHOT_A], mode->name);

		double y = 200 + (100-70*f)*shotidx-20*f;
		text_draw(buf, &(TextParams) {
			.align = ALIGN_CENTER,
			.pos = { 0, y},
			.shader = "text_default",
		});

		if(shot == current_subshot) {
			r_color4(o, o, o, o);
			text_draw_wrapped(mode->description, DESCRIPTION_WIDTH, &(TextParams) {
				.align = ALIGN_CENTER,
				.pos = { 0, y + 30 },
				.shader = "text_default",
			});
		}
	}

	r_mat_pop();

	float o = 0.3*sin(menu->frames/20.0)+0.5;
	o *= 1-menu->entries[menu->cursor].drawdata;
	r_shader("sprite_default");

	r_draw_sprite(&(SpriteParams) {
		.sprite = "menu/arrow",
		.pos = { 30, SCREEN_H/3+10 },
		.color = RGBA(o, o, o, o),
		.scale = { 0.5, 0.7 },
	});

	r_draw_sprite(&(SpriteParams) {
		.sprite = "menu/arrow",
		.pos = { 30 + 340, SCREEN_H/3+10 },
		.color = RGBA(o, o, o, o),
		.scale = { 0.5, 0.7 },
		.flip.x = true,
	});

	r_state_pop();
}

static bool char_menu_input_handler(SDL_Event *event, void *arg) {
	MenuData *menu = arg;
	CharMenuContext *ctx = menu->context;
	TaiseiEvent type = TAISEI_EVENT(event->type);

	int prev_cursor = menu->cursor;

	if(type == TE_MENU_CURSOR_RIGHT) {
		play_ui_sound("generic_shot");
		menu->cursor++;
	} else if(type == TE_MENU_CURSOR_LEFT) {
		play_ui_sound("generic_shot");
		menu->cursor--;
	} else if(type == TE_MENU_CURSOR_DOWN) {
		play_ui_sound("generic_shot");
		ctx->subshot++;
	} else if(type == TE_MENU_CURSOR_UP) {
		play_ui_sound("generic_shot");
		ctx->subshot--;
	} else if(type == TE_MENU_ACCEPT) {
		play_ui_sound("shot_special1");
		menu->selected = menu->cursor;
		menu->transition_in_time = FADE_TIME * 1.5;
		menu->transition_out_time = FADE_TIME * 3;
		close_menu(menu);
	} else if(type == TE_MENU_ABORT) {
		play_ui_sound("hit");
		close_menu(menu);
	}

	menu->cursor = (menu->cursor % menu->ecount) + menu->ecount * (menu->cursor < 0);
	ctx->subshot = (ctx->subshot % NUM_SHOT_MODES_PER_CHARACTER) + NUM_SHOT_MODES_PER_CHARACTER * (ctx->subshot < 0);

	if(menu->cursor != prev_cursor) {
		if(ctx->prev_selected_char != menu->cursor || menu->entries[menu->cursor].drawdata > 0.95) {
			ctx->prev_selected_char = prev_cursor;
			update_char_draw_order(menu);
		}
	}

	return false;
}

static void char_menu_input(MenuData *menu) {
	events_poll((EventHandler[]){
		{ .proc = char_menu_input_handler, .arg = menu },
		{ NULL }
	}, EFLAG_MENU);
}
