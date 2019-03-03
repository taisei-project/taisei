/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "mainmenu.h"
#include "menu.h"
#include "submenus.h"

#include "common.h"

#include "savereplay.h"
#include "stagepractice.h"
#include "difficultyselect.h"

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

	for(StageInfo *stg = stages; stg->procs && (!spell_practice_entry->action || !stage_practice_entry->action); ++stg) {
		if(stg->type == STAGE_SPELL) {
			StageProgress *p = stage_get_progress_from_info(stg, D_Any, false);

			if(p && p->unlocked) {
				spell_practice_entry->action = menu_action_enter_spellpractice;
			}
		} else if(stg->type == STAGE_STORY) {
			for(Difficulty d = D_Easy; d <= D_Lunatic; ++d) {
				StageProgress *p = stage_get_progress_from_info(stg, d, false);

				if(p && p->unlocked) {
					stage_practice_entry->action = menu_action_enter_stagepractice;
				}
			}
		}
	}
}

static void begin_main_menu(MenuData *m) {
	start_bgm("menu");
}

static void update_main_menu(MenuData *menu) {
	menu->drawdata[1] += (text_width(get_font("big"), menu->entries[menu->cursor].name, 0) - menu->drawdata[1])/10.0;
	menu->drawdata[2] += (35*menu->cursor - menu->drawdata[2])/10.0;

	for(int i = 0; i < menu->ecount; i++) {
		menu->entries[i].drawdata += 0.2 * ((i == menu->cursor) - menu->entries[i].drawdata);
	}
}

attr_unused
static bool main_menu_input_handler(SDL_Event *event, void *arg) {
	MenuData *m = arg;
	TaiseiEvent te = TAISEI_EVENT(event->type);
	static hrtime_t last_abort_time = 0;

	if(te == TE_MENU_ABORT) {
		play_ui_sound("hit");
		m->cursor = m->ecount - 1;
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

	add_menu_entry(m, "Start Story", start_game, NULL);
	add_menu_entry(m, "Start Extra", NULL, NULL);
	add_menu_entry(m, "Stage Practice", menu_action_enter_stagepractice, NULL);
	add_menu_entry(m, "Spell Practice", menu_action_enter_spellpractice, NULL);
#ifdef DEBUG
	add_menu_entry(m, "Select Stage", menu_action_enter_stagemenu, NULL);
#endif
	add_menu_entry(m, "Replays", menu_action_enter_replayview, NULL);
	add_menu_entry(m, "Options", menu_action_enter_options, NULL);
#ifndef __EMSCRIPTEN__
	add_menu_entry(m, "Quit", menu_action_close, NULL)->transition = TransFadeBlack;
	m->input = main_menu_input;
#endif

	stage_practice_entry = m->entries + 2;
	spell_practice_entry = m->entries + 3;
	main_menu_update_practice_menus();

	start_bgm("menu");

	return m;
}

void draw_main_menu_bg(MenuData* menu) {
	r_color4(1, 1, 1, 1);
	fill_screen("menu/mainmenubg");
}

void draw_main_menu(MenuData *menu) {
	draw_main_menu_bg(menu);
	draw_sprite(150.5, 100, "menu/logo");

	r_mat_push();
	r_mat_translate(0, SCREEN_H-270, 0);
	draw_menu_selector(50 + menu->drawdata[1]/2, menu->drawdata[2], 1.5 * menu->drawdata[1], 64, menu->frames);
	r_shader("text_default");

	float o = 0.7;

	for(int i = 0; i < menu->ecount; i++) {
		float s = 5*sin(menu->frames/80.0 + 20*i);

		if(menu->entries[i].action == NULL) {
			r_color4(0.2 * o, 0.3 * o, 0.5 * o, o);
		} else {
			float a = 1 - menu->entries[i].drawdata;
			r_color4(o, min(1, 0.7 + a) * o, min(1, 0.4 + a) * o, o);
		}

		text_draw(menu->entries[i].name, &(TextParams) {
			.pos = { 50 + s, 35*i },
			.font = "big",
			// .shader = "text_example",
			// .custom = time_get()
		});
	}

	r_mat_pop();

	bool cullcap_saved = r_capability_current(RCAP_CULL_FACE);
	r_disable(RCAP_CULL_FACE);
	r_color4(1, 1, 1, 0);
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

		r_mat_push();
		r_mat_translate(posx,posy,0);
		r_mat_scale(0.2,0.2,0.2);
		r_mat_rotate_deg(2*(t%period),rx,ry,rz);
		draw_sprite_batched(0,0,"part/petal");
		r_mat_pop();
	}

	r_shader("text_default");
	r_capability(RCAP_CULL_FACE, cullcap_saved);

	char version[32];
	snprintf(version, sizeof(version), "v%s", TAISEI_VERSION);
	text_draw(TAISEI_VERSION, &(TextParams) {
		.align = ALIGN_RIGHT,
		.pos = { SCREEN_W-5, SCREEN_H-10 },
		.font = "small",
	});

	r_shader_standard();
}

void draw_loading_screen(void) {
	preload_resource(RES_TEXTURE, "loading", RESF_PERMANENT);
	set_ortho(SCREEN_W, SCREEN_H);
	fill_screen("loading");
	/*
	text_draw(TAISEI_VERSION, &(TextParams) {
		.align = ALIGN_RIGHT,
		.pos = { SCREEN_W-5, SCREEN_H-10 },
		.font = "small",
		.shader = "text_default",
	});
	*/
	video_swap_buffers();
}

void menu_preload(void) {
	difficulty_preload();

	preload_resources(RES_FONT, RESF_PERMANENT,
		"big",
		"small",
	NULL);

	preload_resources(RES_TEXTURE, RESF_PERMANENT,
		"menu/mainmenubg",
	NULL);

	preload_resources(RES_SPRITE, RESF_PERMANENT,
		"part/smoke",
		"part/petal",
		"menu/logo",
		"menu/arrow",
		"star",
	NULL);

	preload_resources(RES_SHADER_PROGRAM, RESF_PERMANENT,
		"sprite_circleclipped_indicator",
	NULL);

	preload_resources(RES_SFX, RESF_PERMANENT | RESF_OPTIONAL,
		"generic_shot",
		"shot_special1",
		"hit",
	NULL);

	preload_resources(RES_BGM, RESF_PERMANENT | RESF_OPTIONAL,
		"menu",
	NULL);

	for(int i = 0; i < NUM_CHARACTERS; ++i) {
		preload_resource(RES_SPRITE, plrchar_get(i)->dialog_sprite_name, RESF_PERMANENT);
	}
}
