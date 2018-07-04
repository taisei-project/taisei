/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "global.h"
#include "menu.h"
#include "savereplay.h"
#include "difficultyselect.h"
#include "charselect.h"
#include "ending.h"
#include "credits.h"
#include "mainmenu.h"
#include "video.h"

static void start_game_internal(MenuData *menu, StageInfo *info, bool difficulty_menu) {
	MenuData m;
	Difficulty stagediff;
	bool restart;

	player_init(&global.plr);

	do {
		restart = false;
		stagediff = info ? info->difficulty : D_Any;

		if(stagediff == D_Any) {
			if(difficulty_menu) {
				create_difficulty_menu(&m);
				if(menu_loop(&m) == -1) {
					return;
				}

				global.diff = progress.game_settings.difficulty;
			} else {
				// assume global.diff is set up beforehand
			}
		} else {
			global.diff = stagediff;
		}

		create_char_menu(&m);
		if(menu_loop(&m) == -1) {
			if(stagediff != D_Any || !difficulty_menu) {
				return;
			}

			restart = true;
		}
	} while(restart);

	global.plr.mode = plrmode_find(
		progress.game_settings.character,
		progress.game_settings.shotmode
	);

	assert(global.plr.mode != NULL);

	global.replay_stage = NULL;
	replay_init(&global.replay);
	PlayerMode *mode = global.plr.mode;

	do {
		restart = false;

		if(info) {
			global.is_practice_mode = (info->type != STAGE_EXTRA);
			stage_loop(info);
		} else {
			global.is_practice_mode = false;
			for(StageInfo *s = stages; s->type == STAGE_STORY; ++s) {
				stage_loop(s);
			}
		}

		if(global.game_over == GAMEOVER_RESTART) {
			replay_destroy(&global.replay);
			replay_init(&global.replay);
			global.game_over = 0;
			player_init(&global.plr);
			global.plr.mode = mode;

			restart = true;
		}
	} while(restart);

	free_resources(false);
	ask_save_replay();

	global.replay_stage = NULL;

	if(global.game_over == GAMEOVER_WIN && !info) {
		ending_loop();
		credits_loop();
		free_resources(false);
	}

	start_bgm("menu");
	replay_destroy(&global.replay);
	main_menu_update_practice_menus();
	global.game_over = 0;
}

void start_game(MenuData *m, void *arg) {
	start_game_internal(m, (StageInfo*)arg, true);
}

void start_game_no_difficulty_menu(MenuData *m, void *arg) {
	start_game_internal(m, (StageInfo*)arg, false);
}

void draw_menu_selector(float x, float y, float w, float h, float t) {
	Sprite *bg = get_sprite("part/smoke");
	r_mat_push();
	r_mat_translate(x, y, 0);
	r_mat_scale(w / bg->w, h / bg->h, 1);
	r_mat_rotate_deg(t*2,0,0,1);
	r_color4(0,0,0,0.5 * (1 - transition.fade));
	draw_sprite(0, 0, "part/smoke");
	r_mat_pop();
}

void draw_menu_title(MenuData *m, char *title) {
	ShaderProgram *sh_prev = r_shader_current();
	r_shader("text_default");
	text_draw(title, &(TextParams) {
		.pos = { (text_width(get_font("big"), title, 0) + 10) * (1.0 - menu_fade(m)), 30 },
		.align = ALIGN_RIGHT,
		.font = "big",
		.color = rgb(1, 1, 1),
	});
	r_shader_ptr(sh_prev);
}

void draw_menu_list(MenuData *m, float x, float y, void (*draw)(void*, int, int)) {
	r_mat_push();
	float offset = ((((m->ecount+5) * 20) > SCREEN_H)? min(0, SCREEN_H * 0.7 - y - m->drawdata[2]) : 0);
	r_mat_translate(x, y + offset, 0);

	draw_menu_selector(m->drawdata[0], m->drawdata[2], m->drawdata[1], 34, m->frames);

	for(int i = 0; i < m->ecount; ++i) {
		MenuEntry *e = &(m->entries[i]);

		float p = offset + 20*i;

		if(p < -y-10 || p > SCREEN_H+10)
			continue;

		float a = e->drawdata * 0.1;
		float o = (p < 0? 1-p/(-y-10) : 1);
		if(e->action == NULL)
			r_color4(0.5, 0.5, 0.5, 0.5*o);
		else {
			float ia = 1-a;
			r_color4(0.9 + ia * 0.1, 0.6 + ia * 0.4, 0.2 + ia * 0.8, (0.7 + 0.3 * a)*o);
		}

		if(draw && i < m->ecount-1) {
			draw(e, i, m->ecount);
		} else if(e->name) {
			ShaderProgram *sh_prev = r_shader_current();
			r_shader("text_default");
			text_draw(e->name, &(TextParams) { .pos = { 20 - e->drawdata, 20*i } });
			r_shader_ptr(sh_prev);
		}
	}

	r_mat_pop();
}

void animate_menu_list_entry(MenuData *m, int i) {
	MenuEntry *e = &(m->entries[i]);
	e->drawdata += 0.2 * (10*(i == m->cursor) - e->drawdata);
}

void animate_menu_list_entries(MenuData *m) {
	for(int i = 0; i < m->ecount; ++i) {
		animate_menu_list_entry(m, i);
	}
}

void animate_menu_list(MenuData *m) {
	MenuEntry *s = m->entries + m->cursor;
	int w = text_width(get_font("standard"), s->name, 0);

	m->drawdata[0] += (10 + w/2.0 - m->drawdata[0])/10.0;
	m->drawdata[1] += (w*2 - m->drawdata[1])/10.0;
	m->drawdata[2] += (20*m->cursor - m->drawdata[2])/10.0;

	animate_menu_list_entries(m);
}

void menu_commonaction_close(MenuData *menu, void *arg) {
	menu->state = MS_Dead;
}
