/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "mainmenu.h"
#include "menu.h"
#include "submenus.h"

#include "common.h"

#include "savereplay.h"
#include "stagepractice.h"
#include "difficultyselect.h"
#include "charselect.h"

#include "global.h"
#include "video.h"
#include "stage.h"
#include "version.h"
#include "plrmodes.h"

static MenuEntry *spell_practice_entry;
static MenuEntry *stage_practice_entry;

void main_menu_update_practice_menus(void) {
	spell_practice_entry->action = NULL;
	stage_practice_entry->action = NULL;

	int n = stageinfo_get_num_stages();
	for(int i = 0; i < n; ++i) {
		StageInfo *stg = stageinfo_get_by_index(i);

		if(stg->type == STAGE_SPELL) {
			StageProgress *p = stageinfo_get_progress(stg, D_Any, false);

			if(p && p->unlocked) {
				spell_practice_entry->action = menu_action_enter_spellpractice;
				if(stage_practice_entry->action) {
					break;
				}
			}
		} else if(stg->type == STAGE_STORY) {
			for(Difficulty d = D_Easy; d <= D_Lunatic; ++d) {
				StageProgress *p = stageinfo_get_progress(stg, d, false);

				if(p && p->unlocked) {
					stage_practice_entry->action = menu_action_enter_stagepractice;
					if(spell_practice_entry->action) {
						break;
					}
				}
			}
		}
	}
}

static void begin_main_menu(MenuData *m) {
	audio_bgm_play(res_bgm("menu"), true, 0, 0);
}

static void update_main_menu(MenuData *menu) {
	menu->drawdata[1] += 0.1*(menu->cursor-menu->drawdata[1]);

	dynarray_foreach(&menu->entries, int i, MenuEntry *e, {
		e->drawdata += 0.2 * ((i == menu->cursor) - e->drawdata);
	});
}

attr_unused
static bool main_menu_input_handler(SDL_Event *event, void *arg) {
	MenuData *m = arg;
	TaiseiEvent te = TAISEI_EVENT(event->type);
	static hrtime_t last_abort_time = 0;

	if(te == TE_MENU_ABORT) {
		play_sfx_ui("hit");
		m->cursor = m->entries.num_elements - 1;
		hrtime_t t = time_get();

		if(t - last_abort_time < HRTIME_RESOLUTION/5 && last_abort_time > 0) {
			m->selected = m->cursor;
			close_menu(m);
		}

		last_abort_time = t;
		return true;
	}

	return menu_input_handler(event, arg);
}

attr_unused
static void main_menu_input(MenuData *m) {
	events_poll((EventHandler[]){
		{ .proc = main_menu_input_handler, .arg = m },
		{ NULL }
	}, EFLAG_MENU);
}

MenuData* create_main_menu(void) {
	MenuData *m = alloc_menu();

	m->begin = begin_main_menu;
	m->draw = draw_main_menu;
	m->logic = update_main_menu;

	ptrdiff_t stage_practice_idx, spell_practice_idx;

	add_menu_entry(m, "Start Story", start_game, NULL);
	add_menu_entry(m, "Start Extra", NULL, NULL);
	stage_practice_entry = add_menu_entry(m, "Stage Practice", menu_action_enter_stagepractice, NULL);
	stage_practice_idx = dynarray_indexof(&m->entries, stage_practice_entry);
	spell_practice_entry = add_menu_entry(m, "Spell Practice", menu_action_enter_spellpractice, NULL);
	spell_practice_idx = dynarray_indexof(&m->entries, spell_practice_entry);
#ifdef DEBUG
	add_menu_entry(m, "Select Stage", menu_action_enter_stagemenu, NULL);
#endif
	add_menu_entry(m, "Replays", menu_action_enter_replayview, NULL);
	add_menu_entry(m, "Media Room", menu_action_enter_media, NULL);
	add_menu_entry(m, "Options", menu_action_enter_options, NULL);
#ifndef __EMSCRIPTEN__
	add_menu_entry(m, "Quit", menu_action_close, NULL)->transition = TransFadeBlack;
	m->input = main_menu_input;
#endif

	stage_practice_entry = dynarray_get_ptr(&m->entries, stage_practice_idx);
	spell_practice_entry = dynarray_get_ptr(&m->entries, spell_practice_idx);
	main_menu_update_practice_menus();

	progress_unlock_bgm("menu");
	audio_bgm_play(res_bgm("menu"), true, 0, 0);

	return m;
}

void draw_main_menu_bg(MenuData* menu, double center_x, double center_y, double R, const char *tex1, const char *tex2) {
	r_color4(1, 1, 1, 1);
	r_shader("mainmenubg");
	r_uniform_float("R", R/(1-menu_fade(menu)));
	r_uniform_vec2("bg_translation", 0.001*menu->frames, 0);
	r_uniform_vec2("center", 0.5 + center_x / SCREEN_W, 0.5 + center_y / SCREEN_H);
	r_uniform_float("inv_aspect_ratio", 1.0 / VIDEO_ASPECT_RATIO);
	r_uniform_sampler("blend_mask", "cell_noise");
	r_uniform_sampler("tex2", tex2);
	fill_screen(tex1);
	r_shader_standard();
}

void draw_main_menu(MenuData *menu) {
	draw_main_menu_bg(menu, 0, 0, 0.05, "menu/mainmenubg", "stage1/cirnobg");
	r_state_push();

	float rot = sqrt(menu->frames/120.0);
	float rotfac = (1 - pow(menu_fade(menu), 2.0));

	r_draw_sprite(&(SpriteParams) {
		.sprite = "menu/logo",
		.pos = { SCREEN_W/2, SCREEN_H/2 },
		.shader = "sprite_default",
		.rotation.vector = { 0, -1, 0 },
		.rotation.angle = fmax(0, M_PI/1.5 - fmin(M_PI/1.5, rot) * rotfac),
		.color = color_mul_scalar(RGBA(1, 1, 1, 1), fmin(1, rot) * rotfac),
	});

	r_mat_mv_push();
	r_mat_mv_translate(0, SCREEN_H/2, 0);
	r_shader("text_default");

	float o = 0.7;

	dynarray_foreach(&menu->entries, int i, MenuEntry *e, {
		if(e->action == NULL) {
			r_color4(0.2 * o, 0.3 * o, 0.5 * o, o);
		} else {
			float a = 1 - e->drawdata;
			r_color4(o, fmin(1, 0.7 + a) * o, fmin(1, 0.4 + a) * o, o);
		}

		text_draw(e->name, &(TextParams) {
			.pos = { 50 - 15 * e->drawdata, 20 * (i - menu->drawdata[1]) },
			.font = "standard",
		});
	});

	r_mat_mv_pop();

	r_disable(RCAP_CULL_FACE);
	r_shader("sprite_default");

	for(int i = 0; i < 50; i++) { // who needs persistent state for a particle system?
		int period = 900;
		int t = menu->frames+100*i + 30*sin(35*i);
		int cycle = t/period;
		float posx = SCREEN_W+300+100*sin(100345*i)+200*sin(1003*i+13537*cycle)-(0.6+0.01*sin(35*i))*(t%period);
		float posy = 50+ 50*sin(503*i+14677*cycle)+0.8*(t%period);
		float rx = sin(56*i+2147*cycle);
		float ry = sin(913*i+137*cycle);
		float rz = sin(1303*i+89631*cycle);
		float r = sqrt(rx*rx+ry*ry+rz*rz);

		if(!r) {
			continue;
		}

		rx /= r;
		ry /= r;
		rz /= r;

		if(posx > SCREEN_W+20 || posy < -20 || posy > SCREEN_H+20)
			continue;

		r_draw_sprite(&(SpriteParams) {
			.sprite = "part/petal",
			.color = RGBA(1, 1, 1, 0),
			.pos = { posx, posy },
			.scale.both = 0.2,
			.rotation.angle = DEG2RAD * 2 * (t % period),
			.rotation.vector = { rx, ry, rz },
		});
	}

	r_enable(RCAP_CULL_FACE);

	char version[32];
	snprintf(version, sizeof(version), "v%s", TAISEI_VERSION);
	text_draw(TAISEI_VERSION, &(TextParams) {
		.align = ALIGN_RIGHT,
		.pos = { SCREEN_W-5, SCREEN_H-10 },
		.font = "small",
		.shader = "text_default",
		.color = RGBA(1, 1, 1, 1),
	});

	r_state_pop();
}

void draw_loading_screen(void) {
	res_preload(RES_TEXTURE, "loading", RESF_DEFAULT);
	res_preload(RES_SHADER_PROGRAM, "text_default", RESF_PERMANENT);

	set_ortho(SCREEN_W, SCREEN_H);
	fill_screen("loading");
	text_draw("Please wait warmly…", &(TextParams) {
		.align = ALIGN_CENTER,
		.pos = { SCREEN_W/2, SCREEN_H-20 },
		.font = "standard",
		.shader = "text_default",
		.color = RGBA(0.35, 0.35, 0.35, 0.35),
	});

	video_swap_buffers();
}

void menu_preload(void) {
	difficulty_preload();

	res_preload_multi(RES_FONT, RESF_PERMANENT,
		"big",
		"small",
	NULL);

	res_preload_multi(RES_TEXTURE, RESF_PERMANENT,
		"abstract_brown",
		"cell_noise",
		"stage1/cirnobg",
		"menu/mainmenubg",
	NULL);

	res_preload_multi(RES_SPRITE, RESF_PERMANENT,
		"part/smoke",
		"part/petal",
		"menu/logo",
		"menu/arrow",
		"star",
	NULL);

	res_preload_multi(RES_SHADER_PROGRAM, RESF_PERMANENT,
		"mainmenubg",
		"sprite_circleclipped_indicator",
	NULL);

	res_preload_multi(RES_SFX, RESF_PERMANENT | RESF_OPTIONAL,
		"generic_shot",
		"shot_special1",
		"hit",
	NULL);

	res_preload_multi(RES_BGM, RESF_PERMANENT | RESF_OPTIONAL,
		"menu",
	NULL);

	preload_char_menu();
}
