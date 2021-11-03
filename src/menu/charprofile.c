/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "resource/resource.h"
#include "taisei.h"

#include "charprofile.h"
#include "portrait.h"
#include "common.h"
#include "progress.h"
#include "options.h"
#include "global.h"
#include "util/glm.h"
#include "video.h"

typedef struct CharacterEntryParam {
	ShaderProgram *text_shader;
	uint8_t state;
} CharacterEntryParam;

enum {
	PROFILE_REIMU,
	PROFILE_MARISA,
	PROFILE_YOUMU,
};

enum {
	S_NAME,
	S_FULLNAME,
	S_DESCRIPTION,
	S_FACE,
	NUM_FIELDS,
};

#define CHARNAME_LEN 32
#define NUM_PROFILES 3
static const char chardefs[NUM_PROFILES][NUM_FIELDS][CHARNAME_LEN] = {
	[PROFILE_REIMU] = {
		[S_NAME] = "reimu",
		[S_FULLNAME] = "Reimu Hakurei",
		[S_DESCRIPTION] = "Temporary Description 1",
		[S_FACE] = PORTRAIT_STATIC_FACE_SPRITE_NAME(reimu, normal),
	},
	[PROFILE_MARISA] = {
		[S_NAME] = "marisa",
		[S_FULLNAME] = "Marisa Kirisame",
		[S_DESCRIPTION] = "Temporary Description 2",
		[S_FACE] = PORTRAIT_STATIC_FACE_SPRITE_NAME(marisa, normal),
	},
	[PROFILE_YOUMU] = {
		[S_NAME] = "youmu",
		[S_FULLNAME] = "Youmu Konpaku",
		[S_DESCRIPTION] = "Temporary Description 3",
		[S_FACE] = PORTRAIT_STATIC_FACE_SPRITE_NAME(youmu, normal),
	},
	/* PORTRAIT_STATIC_FACE_SPRITE_NAME(cirno, normal), */
	/* PORTRAIT_STATIC_FACE_SPRITE_NAME(hina, normal), */
	/* PORTRAIT_STATIC_FACE_SPRITE_NAME(scuttle, normal), */
	/* PORTRAIT_STATIC_FACE_SPRITE_NAME(wriggle, normal), */
	/* PORTRAIT_STATIC_FACE_SPRITE_NAME(kurumi, normal), */
	/* PORTRAIT_STATIC_FACE_SPRITE_NAME(iku, normal), */
	/* PORTRAIT_STATIC_FACE_SPRITE_NAME(elly, normal), */
};

static void charprofile_logic(MenuData *m) {
	animate_menu_list(m);
	dynarray_foreach(&m->entries, int i, MenuEntry *e, {
		e->drawdata += 0.05 * ((m->cursor != i) - e->drawdata);
	});

	MenuEntry *cursor_entry = dynarray_get_ptr(&m->entries, m->cursor);

	Font *font = res_font("standard");
	char buf[256] = { 0 };
	double height = text_height(font, buf, 0) + font_get_lineskip(font) * 2;

	fapproach_asymptotic_p(&m->drawdata[1], 1 - cursor_entry->drawdata, 0.1, 1e-5);
	fapproach_asymptotic_p(&m->drawdata[2], height, 0.1, 1e-5);
}

static void charprofile_draw(MenuData *m) {
	assert(m->cursor < 3);
	draw_options_menu_bg(m);
	draw_menu_title(m, "Character Profiles");
	draw_menu_list(m, 100, 100, NULL, SCREEN_H);

	dynarray_foreach_idx(&m->entries, int j, {
		MenuEntry *e = dynarray_get_ptr(&m->entries, j);
		Sprite *spr = portrait_get_base_sprite(chardefs[j][S_NAME], NULL);  // TODO cache this

		float o = 1 - e->drawdata*2;
		float pbrightness = 0.6 + 0.4 * o;

		float pofs = fmax(0.0f, e->drawdata * 1.5f - 0.5f);
		pofs = glm_ease_back_in(pofs);

		SpriteParams portrait_params = {
			.pos = { SCREEN_W/2 + 240 + 320 * pofs, SCREEN_H - spr->h * 0.5 },
			.sprite_ptr = spr,
			.shader = "sprite_default",
			.color = RGBA(pbrightness, pbrightness, pbrightness, 1),
		};

		r_draw_sprite(&portrait_params);
		portrait_params.sprite_ptr = res_sprite(chardefs[j][S_FACE]);
		r_draw_sprite(&portrait_params);
		r_mat_mv_push();
		r_mat_mv_translate(SCREEN_W/4, SCREEN_H/3, 0);
		r_mat_mv_push();

		if(e->drawdata != 0) {
			r_mat_mv_translate(0, -300 * e->drawdata, 0);
			r_mat_mv_rotate(M_PI * e->drawdata, 1, 0, 0);
		}

		text_draw("name", &(TextParams) {
			.align = ALIGN_CENTER,
			.font = "big",
			.shader = "text_default",
			.color = RGBA(1, 1, 1, 1),
		});

		r_mat_mv_pop();

		text_draw("title", &(TextParams) {
			.align = ALIGN_CENTER,
			.pos = { 20*(1-o), 30 },
			.shader = "text_default",
			.color = RGBA(1, 1, 1, 1),
		});

		r_mat_mv_pop();
	});

}

static void action_show_character(MenuData *m, void *arg) {
	return;
}

static void add_character(MenuData *m, int i) {
	portrait_preload_base_sprite(chardefs[i][S_NAME], NULL, RESF_PERMANENT);
	portrait_preload_face_sprite(chardefs[i][S_NAME], "normal", RESF_PERMANENT);

	/* char *p = (char*)chardefs; */

	/* for(int i = 0; i < sizeof(chardefs) / CHARNAME_LEN; ++i) { */
	/* 	preload_resource(RES_SPRITE, p + i * CHARNAME_LEN, RESF_PERMANENT); */
	/* } */
	MenuEntry *e = add_menu_entry(m, chardefs[i][S_FULLNAME], action_show_character, NULL);
	e->transition = NULL;
}

static void charprofile_free(MenuData *m) {
	dynarray_foreach_elem(&m->entries, MenuEntry *e, {
		free(e->arg);
	});
}

MenuData *create_charprofile_menu(void) {
	MenuData *m = alloc_menu();

	m->draw = charprofile_draw;
	m->logic = charprofile_logic;
	m->end = charprofile_free;
	m->transition = TransFadeBlack;
	m->flags = MF_Abortable;

	for(int i = 0; i < NUM_PROFILES; i++) {
		add_character(m, i);
	}

	while(!dynarray_get(&m->entries, m->cursor).action) {
			++m->cursor;
	}

	return m;
}

