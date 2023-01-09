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
#include "portrait.h"

#define SELECTED_SUBSHOT(m) (((CharMenuContext*)(m)->context)->subshot)
#define DESCRIPTION_WIDTH (SCREEN_W / 3 + 40)

enum {
	F_HAPPY,
	F_NORMAL,
	F_SMUG,
	F_SURPRISED,
	F_UNAMUSED,
	NUM_FACES,
};

#define FACENAME_LEN 32
static const char facedefs[NUM_CHARACTERS][NUM_FACES][FACENAME_LEN] = {
	[PLR_CHAR_REIMU]  = {
		[F_HAPPY]     = PORTRAIT_STATIC_FACE_SPRITE_NAME(reimu, happy),
		[F_NORMAL]    = PORTRAIT_STATIC_FACE_SPRITE_NAME(reimu, normal),
		[F_SMUG]      = PORTRAIT_STATIC_FACE_SPRITE_NAME(reimu, smug),
		[F_SURPRISED] = PORTRAIT_STATIC_FACE_SPRITE_NAME(reimu, surprised),
		[F_UNAMUSED]  = PORTRAIT_STATIC_FACE_SPRITE_NAME(reimu, unamused),
	},
	[PLR_CHAR_MARISA]  = {
		[F_HAPPY]     = PORTRAIT_STATIC_FACE_SPRITE_NAME(marisa, happy),
		[F_NORMAL]    = PORTRAIT_STATIC_FACE_SPRITE_NAME(marisa, normal),
		[F_SMUG]      = PORTRAIT_STATIC_FACE_SPRITE_NAME(marisa, smug),
		[F_SURPRISED] = PORTRAIT_STATIC_FACE_SPRITE_NAME(marisa, surprised),
		[F_UNAMUSED]  = PORTRAIT_STATIC_FACE_SPRITE_NAME(marisa, unamused),
	},
	[PLR_CHAR_YOUMU]  = {
		[F_HAPPY]     = PORTRAIT_STATIC_FACE_SPRITE_NAME(youmu, happy),
		[F_NORMAL]    = PORTRAIT_STATIC_FACE_SPRITE_NAME(youmu, normal),
		[F_SMUG]      = PORTRAIT_STATIC_FACE_SPRITE_NAME(youmu, smug),
		[F_SURPRISED] = PORTRAIT_STATIC_FACE_SPRITE_NAME(youmu, surprised),
		[F_UNAMUSED]  = PORTRAIT_STATIC_FACE_SPRITE_NAME(youmu, unamused),
	},
};

typedef struct CharMenuContext {
	int8_t subshot;
	int8_t char_draw_order[NUM_CHARACTERS];
	int8_t prev_selected_char;
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
	dynarray_foreach(&menu->entries, int i, MenuEntry *e, {
		e->drawdata += 0.05 * ((menu->cursor != i) - e->drawdata);
	});

	MenuEntry *cursor_entry = dynarray_get_ptr(&menu->entries, menu->cursor);

	PlayerCharacter *pchar = plrchar_get((CharacterID)(uintptr_t)cursor_entry->arg);
	assume(pchar != NULL);

	PlayerMode *m = plrmode_find(pchar->id, SELECTED_SUBSHOT(menu));
	assume(m != NULL);

	Font *font = res_font("standard");
	char buf[256] = { 0 };
	text_wrap(font, m->description, DESCRIPTION_WIDTH, buf, sizeof(buf));
	double height = text_height(font, buf, 0) + font_get_lineskip(font) * 2;

	fapproach_asymptotic_p(&menu->drawdata[0], SELECTED_SUBSHOT(menu) - PLR_SHOT_A, 0.1, 1e-5);
	fapproach_asymptotic_p(&menu->drawdata[1], 1 - cursor_entry->drawdata, 0.1, 1e-5);
	fapproach_asymptotic_p(&menu->drawdata[2], height, 0.1, 1e-5);
}

static void end_char_menu(MenuData *m) {
	mem_free(m->context);
}

static void transition_to_game(double fade) {
	fade_out(pow(fmax(0, (fade - 0.5) * 2), 2));
}

MenuData* create_char_menu(void) {
	MenuData *m = alloc_menu();

	m->input = char_menu_input;
	m->draw = draw_char_menu;
	m->logic = update_char_menu;
	m->end = end_char_menu;
	m->transition = TransFadeBlack;
	m->flags = MF_Abortable;

	auto ctx = ALLOC(CharMenuContext, {
		.subshot = progress.game_settings.shotmode,
		.prev_selected_char = -1,
	});
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

	static const char *const prefixes[] = {
		"Intuition",
		"Science",
	};

	assert(menu->cursor < 3);
	PlayerCharacter *selected_char = plrchar_get((CharacterID)(uintptr_t)dynarray_get(&menu->entries, menu->cursor).arg);

	draw_main_menu_bg(menu, SCREEN_W/4+100, 0, 0.1 * (0.5 + 0.5 * menu->drawdata[1]), "menu/mainmenubg", selected_char->menu_texture_name);
	draw_menu_title(menu, "Select Character");

	CharacterID current_char = 0;

	dynarray_foreach_idx(&menu->entries, int j, {
		CharacterID i = ctx->char_draw_order[j];
		MenuEntry *e = dynarray_get_ptr(&menu->entries, i);

		PlayerCharacter *pchar = plrchar_get((CharacterID)(uintptr_t)e->arg);
		assert(pchar != NULL);
		assert(pchar->id == i);

		Sprite *spr = portrait_get_base_sprite(pchar->lower_name, NULL);  // TODO cache this
		const char *name = pchar->full_name;
		const char *title = pchar->title;

		if(menu->cursor == i) {
			current_char = pchar->id;
		}

		float o = 1 - e->drawdata*2;
		const char *face;

		if(menu->selected == i) {
			face = facedefs[i][SELECTED_SUBSHOT(menu) == PLR_SHOT_A ? F_HAPPY : F_SMUG];
		} else if(fabs(o - 1) < 1e-1) {
			face = facedefs[i][F_NORMAL];
		} else if(menu->cursor == i) {
			face = facedefs[i][F_SURPRISED];
		} else {
			face = facedefs[i][F_UNAMUSED];
		}

		float pofs = fmax(0.0f, e->drawdata * 1.5f - 0.5f);
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
		portrait_params.sprite_ptr = res_sprite(face);
		r_draw_sprite(&portrait_params);

		r_mat_mv_push();
		r_mat_mv_translate(SCREEN_W/4, SCREEN_H/3, 0);

		r_mat_mv_push();

		if(e->drawdata != 0) {
			r_mat_mv_translate(0, -300 * e->drawdata, 0);
			r_mat_mv_rotate(M_PI * e->drawdata, 1, 0, 0);
		}

		text_draw(name, &(TextParams) {
			.align = ALIGN_CENTER,
			.font = "big",
			.shader = "text_default",
			.color = RGBA(o, o, o, o),
		});

		r_mat_mv_pop();

		if(e->drawdata) {
			o = 1 - e->drawdata * 3;
		} else {
			o = 1;
		}

		text_draw(title, &(TextParams) {
			.align = ALIGN_CENTER,
			.pos = { 20*(1-o), 30 },
			.shader = "text_default",
			.color = RGBA(o, o, o, o),
		});

		r_mat_mv_pop();
	});

	r_mat_mv_push();
	r_mat_mv_translate(SCREEN_W/4, SCREEN_H/3, 0);

	ShotModeID current_subshot = SELECTED_SUBSHOT(menu);

	float f = menu->drawdata[0]-PLR_SHOT_A;
	float selbg_ofs = 200 + (100-70)*f-20*f - font_get_lineskip(res_font("standard")) * 0.7;

	r_color4(0, 0, 0, 0.5);
	r_shader_standard_notex();
	r_mat_mv_push();
	r_mat_mv_translate(-150, selbg_ofs + menu->drawdata[2] * 0.5, 0);
	r_mat_mv_scale(650, menu->drawdata[2], 1);
	r_draw_quad();
	r_shader_standard();
	r_mat_mv_pop();

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

	r_mat_mv_pop();

	float o = 0.3*sin(menu->frames/20.0)+0.5;
	o *= 1 - dynarray_get(&menu->entries, menu->cursor).drawdata;
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
		play_sfx_ui("generic_shot");
		menu->cursor++;
	} else if(type == TE_MENU_CURSOR_LEFT) {
		play_sfx_ui("generic_shot");
		menu->cursor--;
	} else if(type == TE_MENU_CURSOR_DOWN) {
		play_sfx_ui("generic_shot");
		ctx->subshot++;
	} else if(type == TE_MENU_CURSOR_UP) {
		play_sfx_ui("generic_shot");
		ctx->subshot--;
	} else if(type == TE_MENU_ACCEPT) {
		play_sfx_ui("shot_special1");
		menu->selected = menu->cursor;
		menu->transition_in_time = FADE_TIME * 1.5;
		menu->transition_out_time = FADE_TIME * 3;
		close_menu(menu);
	} else if(type == TE_MENU_ABORT) {
		play_sfx_ui("hit");
		close_menu(menu);
	}

	menu->cursor = (menu->cursor % menu->entries.num_elements) + menu->entries.num_elements * (menu->cursor < 0);
	ctx->subshot = (ctx->subshot % NUM_SHOT_MODES_PER_CHARACTER) + NUM_SHOT_MODES_PER_CHARACTER * (ctx->subshot < 0);

	if(menu->cursor != prev_cursor) {
		if(ctx->prev_selected_char != menu->cursor || dynarray_get(&menu->entries, menu->cursor).drawdata > 0.95) {
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

void preload_char_menu(void) {
	for(int i = 0; i < NUM_CHARACTERS; ++i) {
		PlayerCharacter *pchar = plrchar_get(i);
		portrait_preload_base_sprite(pchar->lower_name, NULL, RESF_PERMANENT);
		preload_resource(RES_TEXTURE, pchar->menu_texture_name, RESF_PERMANENT);
	}

	char *p = (char*)facedefs;

	for(int i = 0; i < sizeof(facedefs) / FACENAME_LEN; ++i) {
		preload_resource(RES_SPRITE, p + i * FACENAME_LEN, RESF_PERMANENT);
	}
}
