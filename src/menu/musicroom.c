/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "musicroom.h"

#include "audio/audio.h"
#include "common.h"
#include "options.h"
#include "progress.h"
#include "renderer/api.h"
#include "resource/font.h"
#include "resource/resource.h"
#include "video.h"

enum {
	MSTATE_UNLOCKED = 1,
	MSTATE_CONFIRM = 2,
	MSTATE_PLAYING = 4,

	MSTATE_COMMENT_BG_VISIBLE = MSTATE_UNLOCKED | MSTATE_CONFIRM | MSTATE_PLAYING,
	MSTATE_COMMENT_VISIBLE = MSTATE_UNLOCKED | MSTATE_PLAYING,
	MSTATE_TITLE_VISIBLE = MSTATE_UNLOCKED | MSTATE_PLAYING,
};

typedef struct MusicEntryParam {
	BGM *bgm;
	ShaderProgram *text_shader;
	uint8_t state;
} MusicEntryParam;

typedef struct MusicRoomContext {
	ResourceGroup rg;
} MusicRoomContext;

static void musicroom_logic(MenuData *m) {
	float prev_selector_x = m->drawdata[0];
	float prev_selector_w = m->drawdata[1];
	animate_menu_list(m);

	MenuEntry *cursor_entry = dynarray_get_ptr(&m->entries, m->cursor);
	if(cursor_entry->arg) {
		MusicEntryParam *p = cursor_entry->arg;
		float selector_w = SCREEN_W - 200;
		m->drawdata[0] = prev_selector_x;
		m->drawdata[1] = prev_selector_w;
		fapproach_asymptotic_p(&m->drawdata[0], 0.5 * selector_w, 0.1, 1e-5);
		fapproach_asymptotic_p(&m->drawdata[1], 2 * selector_w, 0.1, 1e-5);
		fapproach_asymptotic_p(&m->drawdata[3], (p->state & MSTATE_COMMENT_BG_VISIBLE) != 0, 0.2, 1e-5);
	} else {
		fapproach_asymptotic_p(&m->drawdata[3], 0, 0.2, 1e-5);
	}

	BGM *current_bgm = audio_bgm_current();

	dynarray_foreach(&m->entries, int i, MenuEntry *e, {
		MusicEntryParam *p = e->arg;

		if(p) {
			if(i != m->cursor && e->drawdata / 10.0 <= 0.02) {
				p->state &= ~MSTATE_CONFIRM;
			}

			if(current_bgm && current_bgm == p->bgm) {
				p->state |= MSTATE_PLAYING;
			} else {
				p->state &= ~MSTATE_PLAYING;
			}
		}
	});
}

static void musicroom_draw_item(MenuEntry *e, int i, int cnt, void *ctx) {
	if(!e->name) {
		return;
	}

	MusicEntryParam *p = e->arg;

	if(!p) {
		text_draw(e->name, &(TextParams) {
			.pos = { 20 - e->drawdata, 20 * i },
			.shader = "text_default",
		});

		return;
	}

	char buf[16];
	const char *title = p->state & MSTATE_TITLE_VISIBLE ? e->name : "???????";
	Color clr = *r_color_current();
	TextParams tparams = {
		.pos = { 20 - e->drawdata, 20 * i },
		.shader_ptr = p->text_shader,
		.font_ptr = res_font("standard"),
		.color = &clr,
	};
	bool kerning_saved = font_get_kerning_enabled(tparams.font_ptr);

	tparams.pos.x += text_draw("№ ", &tparams);
	font_set_kerning_enabled(tparams.font_ptr, false);
	snprintf(buf, sizeof(buf), " %i  ", i + 1);
	tparams.pos.x += text_width(tparams.font_ptr, " 99  ", 0);
	tparams.align = ALIGN_RIGHT;
	tparams.pos.x += text_draw(buf, &tparams);
	tparams.align = ALIGN_LEFT;
	font_set_kerning_enabled(tparams.font_ptr, kerning_saved);

	if(!(p->state & MSTATE_TITLE_VISIBLE)) {
		clr.r *= 0.5;
		clr.g *= 0.5;
		clr.b *= 0.5;
	}

	tparams.pos.x += text_draw(title, &tparams);

	if(p->state & MSTATE_PLAYING) {
		color_mul(&clr, RGBA(0.1, 0.6, 0.8, 0.8));
		text_draw(_("Now playing"), &(TextParams) {
			.pos = { SCREEN_W - 200, 20 * i },
			.shader_ptr = p->text_shader,
			.align = ALIGN_RIGHT,
			.color = &clr,
		});
	}
}

static void musicroom_draw(MenuData *m) {
	r_state_push();
	draw_options_menu_bg(m);
	draw_menu_title(m, _("Music Room"));
	draw_menu_list(m, 100, 100, musicroom_draw_item, SCREEN_H / GOLDEN_RATIO, NULL);

	float comment_height = SCREEN_H * (1 - 1 / GOLDEN_RATIO);
	float comment_alpha = (1 - menu_fade(m)) * m->drawdata[3];
	float comment_offset = smoothstep(0, 1, (1 - comment_alpha)) * comment_height;

	r_shader_standard_notex();
	r_mat_mv_push();
	r_mat_mv_translate(SCREEN_W * 0.5, SCREEN_H - comment_height * 0.5 + comment_offset, 0);
	r_mat_mv_scale(SCREEN_W, comment_height, 1);
	r_color4(0, 0, 0, 0.6 * comment_alpha);
	r_draw_quad();
	r_mat_mv_pop();
	r_state_pop();

	Font *const text_font = res_font("standard");
	ShaderProgram *const text_shader = res_shader("text_default");
	const float text_x = 50;
	const float text_y = SCREEN_H - comment_height + font_get_lineskip(text_font) * 1.5 + comment_offset;

	dynarray_foreach_elem(&m->entries, MenuEntry *e, {
		float a = e->drawdata / 10.0 * comment_alpha;

		if(a < 0.05 || !e->arg) {
			continue;
		}

		const char *comment;
		MusicEntryParam *p = e->arg;
		Color *clr = RGBA(a, a, a, a);

		if(p->state & MSTATE_CONFIRM) {
			comment = (
				_("\nYou have not unlocked this track yet!\n\n"
				  "If you wish to hear it anyway, please select it again to confirm.")
			);
			clr->g *= 0.3;
			clr->b *= 0.2;
		} else if(!(p->state & MSTATE_COMMENT_VISIBLE)) {
			continue;
		} else if(!(comment = _(bgm_get_comment(p->bgm)))) {
			comment = _("\nNo comment available");
		}

		text_draw_wrapped(comment, SCREEN_W - text_x * 2, &(TextParams) {
			.pos = { text_x + 0.5*(SCREEN_W - text_x * 2), text_y },
			.font_ptr = text_font,
			.shader_ptr = text_shader,
			.color = clr,
			.align = ALIGN_CENTER,
		});

		if(p->state & MSTATE_COMMENT_VISIBLE) {
			const char *artist = bgm_get_artist(p->bgm);

			if(artist) {
				const char *prefix = "— ";
				char buf[strlen(prefix) + strlen(artist) + 1];
				strcpy(buf, prefix);
				strcat(buf, artist);

				text_draw(buf, &(TextParams) {
					.pos = { SCREEN_W - text_x, SCREEN_H + comment_offset - font_get_lineskip(text_font) },
					.font_ptr = text_font,
					.shader_ptr = text_shader,
					.color = RGBA(a, a, a, a),
					.align = ALIGN_RIGHT,
				});
			}
		}
	});
}

static void action_play_bgm(MenuData *m, void *arg) {
	MusicEntryParam *p = arg;
	assume(p != NULL);

	if(p->state & (MSTATE_CONFIRM | MSTATE_UNLOCKED)) {
		p->state &= ~MSTATE_CONFIRM;
		audio_bgm_play(p->bgm, true, 0, 0);
	} else if (!(p->state & MSTATE_PLAYING)) {
		p->state |= MSTATE_CONFIRM;
	}
}

static void add_bgm(MenuData *m, const char *bgm_name, bool preload) {
	MusicRoomContext *ctx = m->context;

	if(preload) {
		res_group_preload(&ctx->rg, RES_BGM, RESF_OPTIONAL, bgm_name, NULL);
		return;
	}

	BGM *bgm = res_bgm(bgm_name);
	const char *title = bgm ? bgm_get_title(bgm) : NULL;

	if(!title) {
		title = _("Unknown track");
	}

	auto p = ALLOC(MusicEntryParam, {
		.bgm = bgm,
		.text_shader = res_shader("text_default"),
	});

	if(progress_is_bgm_unlocked(bgm_name)) {
		p->state |= MSTATE_UNLOCKED;
	}

	MenuEntry *e = add_menu_entry(m, title, action_play_bgm, p);
	e->transition = NULL;
}

static void musicroom_free(MenuData *m) {
	MusicRoomContext *ctx = m->context;
	res_group_release(&ctx->rg);
	mem_free(ctx);

	dynarray_foreach_elem(&m->entries, MenuEntry *e, {
		mem_free(e->arg);
	});
}

MenuData *create_musicroom_menu(void) {
	auto ctx = ALLOC(MusicRoomContext);
	res_group_init(&ctx->rg);

	MenuData *m = alloc_menu();
	m->logic = musicroom_logic;
	m->draw = musicroom_draw;
	m->end = musicroom_free;
	m->transition = TransFadeBlack;
	m->flags = MF_Abortable;
	m->context = ctx;

	for(int preload = 1; preload >= 0; --preload) {
		add_bgm(m, "intro", preload);
		add_bgm(m, "menu", preload);
		add_bgm(m, "stage1", preload);
		add_bgm(m, "stage1boss", preload);
		add_bgm(m, "stage2", preload);
		add_bgm(m, "stage2boss", preload);
		add_bgm(m, "stage3", preload);
		add_bgm(m, "stage3boss", preload);
		add_bgm(m, "scuttle", preload);
		add_bgm(m, "stage4", preload);
		add_bgm(m, "stage4boss", preload);
		add_bgm(m, "stage5", preload);
		add_bgm(m, "stage5boss", preload);
		add_bgm(m, "bonus0", preload);
		add_bgm(m, "stage6", preload);
		add_bgm(m, "stage6boss_phase1", preload);
		add_bgm(m, "stage6boss_phase2", preload);
		add_bgm(m, "stage6boss_phase3", preload);
		add_bgm(m, "ending", preload);
		add_bgm(m, "credits", preload);
		add_bgm(m, "gameover", preload);
	}

	add_menu_separator(m);
	add_menu_entry(m, "Back", menu_action_close, NULL);

	while(!dynarray_get(&m->entries, m->cursor).action) {
		++m->cursor;
	}

	return m;
}
