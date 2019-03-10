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

static void musicroom_logic(MenuData *m) {
	animate_menu_list(m);
	fapproach_asymptotic_p(&m->drawdata[3], m->entries[m->cursor].arg != NULL, 0.2, 1e-5);
}

static void musicroom_draw_item(MenuEntry *e, int i, int cnt) {
	if(!e->name) {
		return;
	}

	text_draw(e->name, &(TextParams) {
		.pos = { 20 - e->drawdata, 20 * i },
		.shader = "text_default",
	});

	if(
		e->arg &&
		current_bgm.music &&
		current_bgm.music->meta &&
		current_bgm.music->meta == get_resource_data(RES_BGM_METADATA, e->arg, RESF_OPTIONAL)
	) {
		text_draw("Now playing", &(TextParams) {
			.pos = { SCREEN_W - 200, 20 * i },
			.shader = "text_default",
			.align = ALIGN_RIGHT,
			.color = RGBA(0.1, 0.6, 0.8, 0.8),
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
		MusicMetadata *meta = get_resource_data(RES_BGM_METADATA, e->arg, RESF_OPTIONAL);

		if(meta && meta->comment) {
			comment = meta->comment;
		} else {
			comment = "No comment available";
		}

		text_draw_wrapped(comment, SCREEN_W - text_x * 2, &(TextParams) {
			.pos = { text_x, text_y },
			.font_ptr = text_font,
			.shader_ptr = text_shader,
			.color = RGBA(a, a, a, a),
		});

		if(meta->artist) {
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
	preload_resource(RES_BGM, arg, RESF_OPTIONAL);
	start_bgm(arg);
}

static void add_bgm(MenuData *m, const char *bgm) {
	if(progress_is_bgm_unlocked(bgm)) {
		MusicMetadata *meta = get_resource_data(RES_BGM_METADATA, bgm, RESF_OPTIONAL | RESF_PRELOAD);
		MenuEntry *e = add_menu_entry(m, (meta && meta->title) ? meta->title : "Unknown track", action_play_bgm, (void*)bgm);
		e->transition = NULL;
	} else {
		add_menu_entry(m, "???????", NULL, NULL);
	}
}

MenuData* create_musicroom_menu(void) {
	MenuData *m = alloc_menu();
	m->logic = musicroom_logic;
	m->draw = musicroom_draw;
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
