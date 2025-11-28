/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "spellpractice.h"

#include "bitarray.h"
#include "boss.h"
#include "color.h"
#include "common.h"
#include "difficulty.h"
#include "options.h"
#include "i18n/i18n.h"

#include "plrmodes.h"
#include "progress.h"
#include "renderer/api.h"
#include "resource/font.h"
#include "stageinfo.h"
#include "video.h"

static void spell_menu_draw_item(MenuEntry *e, int i, int cnt, void *ctx) {
	int y = 20 * i;

	ShaderProgram *text_shader = res_shader("text_default");
	if(e->arg == NULL) {
		if(e->name != NULL) {
			text_draw(_(e->name), &(TextParams) {
				.pos = { 30, y },
				.shader_ptr = text_shader,
			});
		}
		return;
	}

	MenuData *m = ctx;

	StageInfo *stg = e->arg;
	StageProgress *p = stageinfo_get_progress(stg, D_Any, false);

	// messy code duplication from menu/common.c
	float offset = smoothmin(0, SCREEN_H * 0.8 - 100 - m->drawdata[2], 80);
	float pr = offset + y;
	if(pr < -y-100 || pr > SCREEN_H+10)
		return;
	float a = e->drawdata * 0.1;
	float o = (pr < 0 ? 1-pr/(-100-10) : 1);


	Color clr;
	if(p && p->unlocked) {
		float ia = 1-a;
		clr = *RGBA_MUL_ALPHA(0.9 + ia * 0.1, 0.6 + ia * 0.4, 0.2 + ia * 0.8, (0.7 + 0.3 * a)*o);
	} else {
		clr = *RGBA_MUL_ALPHA(0.5, 0.5, 0.5, 0.8);
	}
	char title[128];
	snprintf(title, sizeof(title), _("№ %s"), stg->title + strlen("Spell "));

	Color second_clr = clr;
	color_mul_scalar(&second_clr, 0.7);

	text_draw(title, &(TextParams) {
		.pos = { 20 - text_width(res_font("standard"), title, 0), y },
		.color = &second_clr,
		.shader_ptr = text_shader,
	});

	if(p && p->unlocked) {
		text_draw(_(stg->subtitle), &(TextParams) {
			.pos = { 30, y },
			.color = &clr,
			.shader_ptr = text_shader,
		});
		text_draw(difficulty_name(stg->difficulty), &(TextParams) {
			.pos = { 40 + text_width(res_font("standard"), _(stg->subtitle), 0), y },
			.color = &second_clr,
			.shader_ptr = text_shader,
		});
	} else {
		float width = 400;
		r_state_push();
		r_shader_standard_notex();

		r_mat_mv_push();
		r_mat_mv_translate(30 + width / 2, y - 3, 0);
		r_color(&second_clr);
		r_mat_mv_scale(width, 1.3, 1);
		r_draw_quad();
		r_mat_mv_pop();
		r_state_pop();
	}

}

static void draw_spell_menu_summary(MenuData *m) {
	MenuEntry *entry = dynarray_get_ptr(&m->entries, m->cursor);
	StageInfo *stg = entry->arg;
	if(stg == NULL) {
		return;
	}
	StageProgress *p = stageinfo_get_progress(stg, D_Any, false);
	if(p == NULL) {
		return;
	}

	Color clr = *RGBA(0.8,0.8,0.8,1.0);

	auto font = res_font("standard");
	r_state_push();

	ShaderProgram *text_shader = res_shader("text_default");
	r_mat_mv_push();
	r_mat_mv_translate(700-(entry->drawdata-10), 160, 0);

	int title_offset = -120;

	r_mat_mv_push();
	r_mat_mv_translate(130, 20*2, 0);
	r_mat_mv_scale(520, 120, 1);
	r_shader_standard_notex();
	r_color(RGBA(0,0,0,0.7));
	r_draw_quad();
	r_mat_mv_pop();

	r_shader_ptr(text_shader);
	const char *title = _("Spell Information");
	text_draw(title, &(TextParams) {
		.pos = { title_offset, 0 },
		.color = RGBA(1.0,1.0,1.0,1.0),
	});

	if(stg->spell != NULL) {
		const char *labels[] = {
			_("Type"),
			_("Health"),
			_("Duration"),
			_("Bonus Rank"),
		};
		char bufs[ARRAY_SIZE(labels)][256];
		strlcpy(bufs[0], attacktype_name(stg->spell->type), sizeof(bufs[0]));
		if(stg->spell->type == AT_SurvivalSpell) {
			snprintf(bufs[1], sizeof(bufs[1]), "∞");
		} else {
			snprintf(bufs[1], sizeof(bufs[1]), "%g", stg->spell->hp);
		}
		snprintf(bufs[2], sizeof(bufs[2]), _("%g s"), stg->spell->timeout);
		snprintf(bufs[3], sizeof(bufs[3]), "%d", stg->spell->bonus_rank);

		for(int i = 0; i < ARRAY_SIZE(labels); i++) {
			int y = 25 + 20 * i;

			text_draw(labels[i], &(TextParams) {
				.pos = { -5 - text_width(font, labels[i], 0), y },
				.color = &clr,
			});
			text_draw(bufs[i], &(TextParams) {
				.pos = { 5, y },
				.color = &clr,
			});
		}
	}

	r_mat_mv_translate(0, 180, 0);
	r_mat_mv_push();
	r_mat_mv_translate(130, 20*1.1*3, 0);
	r_mat_mv_scale(520, 170, 1);
	r_shader_standard_notex();
	r_color(RGBA(0,0,0,0.7));
	r_draw_quad();
	r_mat_mv_pop();

	r_shader_ptr(text_shader);

	title = _("Clears/Attempts");
	text_draw(title, &(TextParams) {
		.pos = { title_offset, 0 },
		.color = RGBA(1.0,1.0,1.0,1.0),
	});
	for(int plr_id = 0; plr_id < NUM_CHARACTERS; plr_id++) {
		for(int shot_id = 0; shot_id < NUM_SHOT_MODES_PER_CHARACTER; shot_id++) {
			auto plrmode = plrmode_find(plr_id, shot_id);
			char buf[256];
			plrmode_repr(buf, sizeof(buf), plrmode, false);

			int y = 25 + 20 * (shot_id + 1.1*NUM_SHOT_MODES_PER_CHARACTER * plr_id);

			Color labelclr = clr;
			auto modeprog = p->per_plrmode[plr_id][shot_id];
			if(modeprog.num_cleared == 0) {
				color_mul_scalar(&labelclr, 0.6);
			}

			text_draw(buf, &(TextParams) {
				.pos = { -5 - text_width(font, buf, 0), y },
				.color = &labelclr,
			});

			snprintf(buf, sizeof(buf), "%d/%d", modeprog.num_cleared, modeprog.num_played);
			text_draw(buf, &(TextParams) {
				.pos = { 5, y },
				.color = &labelclr,
			});
		}
	}

	r_mat_mv_pop();
	r_state_pop();
}

static void draw_spell_menu(MenuData *m) {
	draw_options_menu_bg(m);
	draw_menu_title(m, _("Spell Practice"));
	draw_menu_list(m, 100, 100, spell_menu_draw_item, SCREEN_H, m);
	draw_spell_menu_summary(m);
}

static void update_spell_menu(MenuData *m) {
	m->drawdata[0] += ((SCREEN_W/2 - 200) - m->drawdata[0])/10.0;
	m->drawdata[1] += ((SCREEN_W - 400) * 1.75 - m->drawdata[1])/10.0;
	m->drawdata[2] += (20*m->cursor - m->drawdata[2])/10.0;
	animate_menu_list_entries(m);
}

MenuData* create_spell_menu(void) {
	Difficulty lastdiff = D_Any;
	uint16_t lastgroup = 0;

	MenuData *m = alloc_menu();

	m->draw = draw_spell_menu;
	m->logic = update_spell_menu;
	m->flags = MF_Abortable;
	m->transition = TransFadeBlack;

	int n = stageinfo_get_num_stages();

	for(int i = 0; i < n; ++i) {
		StageInfo *stg = stageinfo_get_by_index(i);

		if(stg->type != STAGE_SPELL) {
			continue;
		}

		uint16_t group = stg->id & ~0xff;

		if(i && (lastgroup != group || stg->difficulty < lastdiff)) {
			add_menu_separator(m);
		}

		StageProgress *p = stageinfo_get_progress(stg, D_Any, false);

		if(p && p->unlocked) {
			add_menu_entry(m, "", start_game, stg);
		} else {
			add_menu_entry(m, "", NULL, stg);
		}

		lastdiff = stg->difficulty;
		lastgroup = group;
	}

	add_menu_separator(m);
	add_menu_entry(m, "Back", menu_action_close, NULL);

	while(!dynarray_get(&m->entries, m->cursor).action) {
		++m->cursor;
	}

	return m;
}
