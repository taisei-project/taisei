/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "common.h"

#include "audio/audio.h"
#include "charselect.h"
#include "difficultyselect.h"
#include "gamesession.h"
#include "i18n/i18n.h"
#include "mainmenu.h"
#include "menu.h"
#include "progress.h"
#include "replay/struct.h"
#include "resource/font.h"
#include "taisei_app.h"
#include "video.h"

typedef struct StartGameContext {
	GameSession session;
	StageInfo *first_stage;
	MenuData *diff_menu;
	MenuData *char_menu;
	Replay replay;
	Difficulty difficulty;
	bool practice_mode;
} StartGameContext;

static void start_game_do_pick_character(CallChainResult ccr);
static void start_game_do_enter_game(CallChainResult ccr);
static void start_game_do_cleanup(CallChainResult ccr);

static void start_game_internal(MenuData *menu, StageInfo *info, bool difficulty_menu) {
	auto ctx = ALLOC(StartGameContext);

	if(info == NULL) {
		ctx->practice_mode = false;
		ctx->first_stage = stageinfo_get_by_id(1);
	} else {
		ctx->practice_mode = (info->type != STAGE_EXTRA);
		ctx->first_stage = info;
	}

	Difficulty stagediff = info ? info->difficulty : D_Any;

	CallChain cc_pick_character = CALLCHAIN(start_game_do_pick_character, ctx);

	if(stagediff == D_Any) {
		if(difficulty_menu) {
			ctx->diff_menu = create_difficulty_menu(~0);
			enter_menu(ctx->diff_menu, cc_pick_character);
		} else {
			ctx->difficulty = progress.game_settings.difficulty;
			run_call_chain(&cc_pick_character, NULL);
		}
	} else {
		ctx->difficulty = stagediff;
		run_call_chain(&cc_pick_character, NULL);
	}
}

static void start_game_do_pick_character(CallChainResult ccr) {
	StartGameContext *ctx = ccr.ctx;
	MenuData *prev_menu = ccr.result;

	if(prev_menu) {
		if(prev_menu->state == MS_Dead) {
			start_game_do_cleanup(ccr);
			return;
		}

		// came here from the difficulty menu - update our setting
		ctx->difficulty = progress.game_settings.difficulty;
	}

	assert(ctx->char_menu == NULL);
	ctx->char_menu = create_char_menu();
	enter_menu(ctx->char_menu, CALLCHAIN(start_game_do_enter_game, ctx));
}

static void kill_aux_menus(StartGameContext *ctx) {
	kill_menu(ctx->char_menu);
	kill_menu(ctx->diff_menu);
	ctx->char_menu = ctx->diff_menu = NULL;
}

static void start_game_do_enter_game(CallChainResult ccr) {
	StartGameContext *ctx = ccr.ctx;
	MenuData *prev_menu = ccr.result;

	if(prev_menu && prev_menu->state == MS_Dead) {
		assert(prev_menu == ctx->char_menu);
		ctx->char_menu = NULL;

		if(!ctx->diff_menu) {
			start_game_do_cleanup(ccr);
		}

		return;
	}

	taisei_commit_persistent_data(true);
	kill_aux_menus(ctx);

	if(game_session_init(&ctx->session, &(GameSettings) {
		.init_stage = ctx->first_stage,
		.difficulty = ctx->difficulty,
		.player_char = progress.game_settings.character,
		.player_shot = progress.game_settings.shotmode,
		.practice_mode = ctx->practice_mode,
		.replay_out = &ctx->replay,
	})) {
		game_session_enter(&ctx->session, CALLCHAIN(start_game_do_cleanup, ctx));
		return;
	}

	start_game_do_cleanup(ccr);
}

static void start_game_do_cleanup(CallChainResult ccr) {
	StartGameContext *ctx = ccr.ctx;
	kill_aux_menus(ctx);
	game_session_deinit(&ctx->session);
	replay_reset(&ctx->replay);
	mem_free(ctx);
	main_menu_update_practice_menus();
	audio_bgm_play(res_bgm("menu"), true, 0, 0);
}

void start_game(MenuData *m, void *arg) {
	start_game_internal(m, (StageInfo*)arg, true);
}

void start_game_no_difficulty_menu(MenuData *m, void *arg) {
	start_game_internal(m, (StageInfo*)arg, false);
}

void draw_menu_selector(float x, float y, float w, float h, float t) {
	Sprite *bg = res_sprite("part/smoke");
	r_mat_mv_push();
	r_mat_mv_translate(x, y, 0);
	r_mat_mv_scale(w / bg->w, h / bg->h, 1);
	r_mat_mv_rotate(t * 2 * DEG2RAD, 0, 0, 1);
	r_draw_sprite(&(SpriteParams) {
		.sprite = bg,
		.color = RGBA(0, 0, 0, 0.5 * (1 - transition.fade)),
		.shader = res_shader("sprite_default"),
	});
	r_mat_mv_pop();
}

void draw_menu_title(MenuData *m, const char *title) {
	text_draw(title, &(TextParams) {
		.pos = { (text_width(res_font("big"), title, 0) + 10) * (1.0 - menu_fade(m)), 30 },
		.align = ALIGN_RIGHT,
		.font = "big",
		.color = RGB(1, 1, 1),
		.shader_ptr = res_shader("text_default"),
	});
}

void draw_menu_list(MenuData *m, float x, float y, void (*draw)(MenuEntry*, int, int, void*), float scroll_threshold, void *userdata) {
	r_mat_mv_push();
	float offset = smoothmin(0, scroll_threshold * 0.8 - y - m->drawdata[2], 80);
	r_mat_mv_translate(x, y + offset, 0);

	draw_menu_selector(m->drawdata[0], m->drawdata[2], m->drawdata[1], 34, m->frames);
	ShaderProgram *text_shader = res_shader("text_default");

	dynarray_foreach(&m->entries, int i, MenuEntry *e, {
		float p = offset + 20*i;

		if(p < -y-10 || p > SCREEN_H+10)
			continue;

		float a = e->drawdata * 0.1;
		float o = (p < 0? 1-p/(-y-10) : 1);

		Color clr;

		if(e->action == NULL) {
			clr = RGBA_MUL_ALPHA(0.5, 0.5, 0.5, 0.5*o);
		} else {
			float ia = 1-a;
			clr = RGBA_MUL_ALPHA(0.9 + ia * 0.1, 0.6 + ia * 0.4, 0.2 + ia * 0.8, (0.7 + 0.3 * a)*o);
		}

		r_color(clr);

		if(draw && i < m->entries.num_elements-1) {
			draw(e, i, m->entries.num_elements, userdata);
		} else if(e->name) {
			text_draw(_(e->name), &(TextParams) {
				.pos = { 20 - e->drawdata, 20*i },
				.shader_ptr = text_shader,
				.color = clr,
			});
		}
	});

	r_mat_mv_pop();
}

void animate_menu_list_entry(MenuData *m, int i) {
	MenuEntry *e = dynarray_get_ptr(&m->entries, i);
	fapproach_asymptotic_p(&e->drawdata, 10 * (i == m->cursor), 0.2, 1e-4);
}

void animate_menu_list_entries(MenuData *m) {
	dynarray_foreach_idx(&m->entries, int i, {
		animate_menu_list_entry(m, i);
	});
}

void animate_menu_list(MenuData *m) {
	MenuEntry *s = dynarray_get_ptr(&m->entries, m->cursor);
	int w = text_width(res_font("standard"), s->name, 0);

	fapproach_asymptotic_p(&m->drawdata[0], 10 + w * 0.5, 0.1, 1e-5);
	fapproach_asymptotic_p(&m->drawdata[1], w * 2, 0.1, 1e-5);
	fapproach_asymptotic_p(&m->drawdata[2], 20 * m->cursor, 0.1, 1e-5);

	animate_menu_list_entries(m);
}

void menu_action_close(MenuData *menu, void *arg) {
	menu->state = MS_Dead;
}
