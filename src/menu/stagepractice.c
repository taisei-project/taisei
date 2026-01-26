/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "stagepractice.h"

#include "bitarray.h"
#include "color.h"
#include "common.h"

#include "menu/mainmenu.h"
#include "i18n/i18n.h"
#include "portrait.h"
#include "progress.h"
#include "renderer/api.h"
#include "resource/font.h"
#include "stageinfo.h"
#include "video.h"

static void draw_stgpract_menu(MenuData *m) {
	const char *bosses[][2] = {
		{"cirno",  PORTRAIT_STATIC_FACE_SPRITE_NAME(cirno, angry)},
		{"hina", PORTRAIT_STATIC_FACE_SPRITE_NAME(hina, serious)},
		{"wriggle", PORTRAIT_STATIC_FACE_SPRITE_NAME(wriggle, proud)},
		{"kurumi", PORTRAIT_STATIC_FACE_SPRITE_NAME(kurumi, tsun)},
		{"iku", PORTRAIT_STATIC_FACE_SPRITE_NAME(iku, smile)},
		{"elly", PORTRAIT_STATIC_FACE_SPRITE_NAME(elly, smug)},
	};

	draw_main_menu_bg(m, -100, 0, 0.06, "menu/mainmenubg", "menu/mainmenubg");
	draw_menu_title(m, _("Stage Practice"));
	ShaderProgram *text_shader = res_shader("text_default");
	Font *font_big = res_font("big");

	dynarray_foreach(&m->entries, int i, MenuEntry *e, {
		assert(i < ARRAY_SIZE(bosses));
		StageInfo *stg = e->arg;

		float ioff = i - m->drawdata[2]/20;
		float x = SCREEN_W/2.0 - 100 - 5*ioff*ioff;
		float y = SCREEN_H/2.0 - 10 + 70*ioff + tanh(2*ioff) * (10 - e->drawdata) * 11;
		char title[STAGE_MAX_TITLE_SIZE];
		stagetitle_format_localized(&stg->title, sizeof(title), title);


		if(e->drawdata/10 > 1e-3) {
			Sprite *spr = portrait_get_base_sprite(bosses[i][0], NULL);
			float p = e->drawdata/10;
			SpriteParams portrait_params = {
				.pos = { SCREEN_W/2.0 + 180, SCREEN_H - spr->h * 0.5 },
				.sprite_ptr = spr,
				.shader_ptr = res_shader("sprite_silhouette"),
				.color = RGBA_MUL_ALPHA(0, 0, 0, p),
			};

			r_mat_mv_push();
			r_mat_mv_translate(x, y*0.6, 0);
			r_mat_mv_scale(1+0.2*p,1+0.2*p,1);
			r_mat_mv_translate(-x, -y*0.6, 0);
			r_draw_sprite(&portrait_params);
			r_mat_mv_pop();
			portrait_params.shader_ptr = res_shader("sprite_default");
			portrait_params.color = RGBA_MUL_ALPHA(1, 1, 1, p);

			StageProgress *prog = stageinfo_get_progress(stg, progress.game_settings.difficulty, false);
			if(e->action != NULL && prog && prog->global.num_cleared > 0) {
				r_draw_sprite(&portrait_params);
				portrait_params.sprite_ptr = res_sprite(bosses[i][1]);
				r_draw_sprite(&portrait_params);
			}
		}

		const char *subtitle = _(stg->subtitle);
		float ia = 1 - 0.3 * fabsf(ioff);
		Color clr = *RGBA_MUL_ALPHA(1, 1, 1, ia);
		if(e->action == NULL) {
			clr = *RGBA_MUL_ALPHA(0.1, 0.1, 0.1, ia);
			for(char *p = title; *p; p++) {
				*p = '?';
			}
			subtitle = _("??????");
		}

		r_color(&clr);
		text_draw(title, &(TextParams) {
			.pos = { x, y },
			.shader_ptr = text_shader,
			.font_ptr = font_big,
			.align = ALIGN_CENTER,
		});

		text_draw(subtitle, &(TextParams) {
			.pos = { x, y + 35 },
			.shader_ptr = text_shader,
			.align = ALIGN_CENTER,
		});
	});

	r_color3(1,1,1);
}

MenuData* create_stgpract_menu(Difficulty diff) {
	MenuData *m = alloc_menu();

	m->draw = draw_stgpract_menu;
	m->logic = animate_menu_list;
	m->flags = MF_Abortable;
	m->transition = TransFadeBlack;

	int n = stageinfo_get_num_stages();
	for(int i = 0; i < n; ++i) {
		StageInfo *stg = stageinfo_get_by_index(i);

		if(stg->type != STAGE_STORY) {
			break;
		}

		StageProgress *p = stageinfo_get_progress(stg, diff, false);

		if(p && p->unlocked) {
			add_menu_entry(m, "", start_game_no_difficulty_menu, stg);
		} else {
			add_menu_entry(m, "", NULL, stg);
		}
	}

	while(!dynarray_get(&m->entries, m->cursor).action) {
		++m->cursor;
	}

	return m;
}
