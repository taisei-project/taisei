/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "musicroom.h"
#include "resource/resource.h"
#include "resource/bgm_metadata.h"
#include "resource/font.h"
#include "audio/audio.h"
#include "progress.h"
#include "common.h"
#include "options.h"
#include "renderer/api.h"
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
	MusicMetadata *bgm_meta;
	const char *bgm;
	ShaderProgram *text_shader;
	uint8_t state;
} MusicEntryParam;

static void musicroom_logic(MenuData *m) {
	float prev_selector_x = m->drawdata[0];
	float prev_selector_w = m->drawdata[1];
	animate_menu_list(m);

	if(m->entries[m->cursor].arg) {
		MusicEntryParam *p = m->entries[m->cursor].arg;
		float selector_w = SCREEN_W - 200;
		m->drawdata[0] = prev_selector_x;
		m->drawdata[1] = prev_selector_w;
		fapproach_asymptotic_p(&m->drawdata[0], 0.5 * selector_w, 0.1, 1e-5);
		fapproach_asymptotic_p(&m->drawdata[1], 2 * selector_w, 0.1, 1e-5);
		fapproach_asymptotic_p(&m->drawdata[3], (p->state & MSTATE_COMMENT_BG_VISIBLE) != 0, 0.2, 1e-5);
	} else {
		fapproach_asymptotic_p(&m->drawdata[3], 0, 0.2, 1e-5);
	}

	for(int i = 0; i < m->ecount; ++i) {
		MenuEntry *e = m->entries + i;
		MusicEntryParam *p = e->arg;

		if(p) {
			if(i != m->cursor && e->drawdata / 10.0 <= 0.02) {
				p->state &= ~MSTATE_CONFIRM;
			}

			if(
				current_bgm.music &&
				current_bgm.music->meta &&
				current_bgm.music->meta == p->bgm_meta
			) {
				p->state |= MSTATE_PLAYING;
			} else {
				p->state &= ~MSTATE_PLAYING;
			}
		}
	}
}

static void musicroom_draw_item(MenuEntry *e, int i, int cnt) {
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
		.font_ptr = get_font("standard"),
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
		text_draw("Now playing", &(TextParams) {
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
	draw_menu_title(m, "Music Room");
	draw_menu_list(m, 100, 100, musicroom_draw_item, SCREEN_H / GOLDEN_RATIO);

	float comment_height = SCREEN_H * (1 - 1 / GOLDEN_RATIO);
	float comment_alpha = (1 - menu_fade(m)) * m->drawdata[3];
	float comment_offset = smoothstep(0, 1, (1 - comment_alpha)) * comment_height;

	r_shader_standard_notex();
	r_mat_push();
	r_mat_translate(SCREEN_W * 0.5, SCREEN_H - comment_height * 0.5 + comment_offset, 0);
	r_mat_scale(SCREEN_W, comment_height, 1);
	r_color4(0, 0, 0, 0.6 * comment_alpha);
	r_draw_quad();
	r_mat_pop();
	r_state_pop();

	Font *const text_font = get_font("standard");
	ShaderProgram *const text_shader = r_shader_get("text_default");
	const float text_x = 50;
	const float text_y = SCREEN_H - comment_height + font_get_lineskip(text_font) * 1.5 + comment_offset;

	for(int i = 0; i < m->ecount; ++i) {
		MenuEntry *e = m->entries + i;
		float a = e->drawdata / 10.0 * comment_alpha;

		if(a < 0.05 || !e->arg) {
			continue;
		}

		const char *comment;
		MusicEntryParam *p = e->arg;
		MusicMetadata *meta = p->bgm_meta;
		Color *clr = RGBA(a, a, a, a);

		if(p->state & MSTATE_CONFIRM) {
			comment = (
				"\nYou have not unlocked this track yet!\n\n"
				"If you wish to hear it anyway, please select it again to confirm."
			);
			clr->g *= 0.3;
			clr->b *= 0.2;
		} else if(!(p->state & MSTATE_COMMENT_VISIBLE)) {
			continue;
		} else if(meta && meta->comment) {
			comment = meta->comment;
		} else {
			comment = "\nNo comment available";
		}

		text_draw_wrapped(comment, SCREEN_W - text_x * 2, &(TextParams) {
			.pos = { text_x + 0.5*(SCREEN_W - text_x * 2), text_y },
			.font_ptr = text_font,
			.shader_ptr = text_shader,
			.color = clr,
			.align = ALIGN_CENTER,
		});

		if(meta->artist && (p->state & MSTATE_COMMENT_VISIBLE)) {
			const char *prefix = "— ";
			char buf[strlen(prefix) + strlen(meta->artist) + 1];
			strcpy(buf, prefix);
			strcat(buf, meta->artist);

			text_draw(buf, &(TextParams) {
				.pos = { SCREEN_W - text_x, SCREEN_H + comment_offset - font_get_lineskip(text_font) },
				.font_ptr = text_font,
				.shader_ptr = text_shader,
				.color = RGBA(a, a, a, a),
				.align = ALIGN_RIGHT,
			});
		}
	}
}

static void action_play_bgm(MenuData *m, void *arg) {
	MusicEntryParam *p = arg;
	assume(p != NULL);

	if(p->state & (MSTATE_CONFIRM | MSTATE_UNLOCKED)) {
		p->state &= ~MSTATE_CONFIRM;
		preload_resource(RES_BGM, p->bgm, RESF_OPTIONAL);
		start_bgm(p->bgm);
	} else if (!(p->state & MSTATE_PLAYING)) {
		p->state |= MSTATE_CONFIRM;
	}
}

static void add_bgm(MenuData *m, const char *bgm) {
	MusicMetadata *meta = get_resource_data(RES_BGM_METADATA, bgm, RESF_OPTIONAL | RESF_PRELOAD);
	const char *title = (meta && meta->title) ? meta->title : "Unknown track";
	MusicEntryParam *p = calloc(1, sizeof(*p));
	p->bgm = bgm;
	p->bgm_meta = meta;
	p->text_shader = r_shader_get("text_default");

	if(progress_is_bgm_unlocked(bgm)) {
		p->state |= MSTATE_UNLOCKED;
	}

	MenuEntry *e = add_menu_entry(m, title, action_play_bgm, p);
	e->transition = NULL;
}

static void musicroom_free(MenuData *m) {
	for(MenuEntry *e = m->entries; e < m->entries + m->ecount; ++e) {
		free(e->arg);
	}
}

MenuData* create_musicroom_menu(void) {
	MenuData *m = alloc_menu();
	m->logic = musicroom_logic;
	m->draw = musicroom_draw;
	m->end = musicroom_free;
	m->transition = TransMenuDark;
	m->flags = MF_Abortable;

	add_bgm(m, "menu");
	add_bgm(m, "stage1");
	add_bgm(m, "stage1boss");
	add_bgm(m, "stage2");
	add_bgm(m, "stage2boss");
	add_bgm(m, "stage3");
	add_bgm(m, "stage3boss");
	add_bgm(m, "scuttle");
	add_bgm(m, "stage4");
	add_bgm(m, "stage4boss");
	add_bgm(m, "stage5");
	add_bgm(m, "stage5boss");
	add_bgm(m, "bonus0");
	add_bgm(m, "stage6");
	add_bgm(m, "stage6boss_phase1");
	add_bgm(m, "stage6boss_phase2");
	add_bgm(m, "stage6boss_phase3");
	add_bgm(m, "ending");
	add_bgm(m, "credits");
	add_menu_separator(m);
	add_menu_entry(m, "Back", menu_action_close, NULL);

	while(!m->entries[m->cursor].action) {
		++m->cursor;
	}

	return m;
}
