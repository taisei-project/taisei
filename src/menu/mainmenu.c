/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "mainmenu.h"
#include "menu.h"

#include "common.h"
#include "options.h"
#include "stageselect.h"
#include "replayview.h"
#include "savereplay.h"
#include "spellpractice.h"
#include "stagepractice.h"
#include "difficultyselect.h"

#include "global.h"
#include "video.h"
#include "stage.h"
#include "version.h"

void enter_options(MenuData *menu, void *arg) {
	MenuData m;
	create_options_menu(&m);
	menu_loop(&m);
}

void enter_stagemenu(MenuData *menu, void *arg) {
	MenuData m;
	create_stage_menu(&m);
	menu_loop(&m);
}

void enter_replayview(MenuData *menu, void *arg) {
	MenuData m;
	create_replayview_menu(&m);
	menu_loop(&m);
}

void enter_spellpractice(MenuData *menu, void *arg) {
	MenuData m;
	create_spell_menu(&m);
	menu_loop(&m);
}

void enter_stagepractice(MenuData *menu, void *arg) {
	MenuData m;

	do {
		create_difficulty_menu(&m);

		if(menu_loop(&m) < 0) {
			return;
		}

		create_stgpract_menu(&m, global.diff);
		menu_loop(&m);
	} while(m.selected < 0 || m.selected == m.ecount - 1);
}

static MenuEntry *spell_practice_entry;
static MenuEntry *stage_practice_entry;

void main_menu_update_practice_menus(void) {
	spell_practice_entry->action = NULL;
	stage_practice_entry->action = NULL;

	for(StageInfo *stg = stages; stg->procs && (!spell_practice_entry->action || !stage_practice_entry->action); ++stg) {
		if(stg->type == STAGE_SPELL) {
			StageProgress *p = stage_get_progress_from_info(stg, D_Any, false);

			if(p && p->unlocked) {
				spell_practice_entry->action = enter_spellpractice;
			}
		} else if(stg->type == STAGE_STORY) {
			for(Difficulty d = D_Easy; d <= D_Lunatic; ++d) {
				StageProgress *p = stage_get_progress_from_info(stg, d, false);

				if(p && p->unlocked) {
					stage_practice_entry->action = enter_stagepractice;
				}
			}
		}
	}
}

void begin_main_menu(MenuData *m) {
	start_bgm("menu");
}

void create_main_menu(MenuData *m) {
	create_menu(m);
	m->draw = draw_main_menu;
	m->begin = begin_main_menu;

	add_menu_entry(m, "Start Story", start_game, NULL);
	add_menu_entry(m, "Start Extra", NULL, NULL);
	add_menu_entry(m, "Stage Practice", enter_stagepractice, NULL);
	add_menu_entry(m, "Spell Practice", enter_spellpractice, NULL);
#ifdef DEBUG
	add_menu_entry(m, "Select Stage", enter_stagemenu, NULL);
#endif
	add_menu_entry(m, "Replays", enter_replayview, NULL);
	add_menu_entry(m, "Options", enter_options, NULL);
	add_menu_entry(m, "Quit", menu_commonaction_close, NULL)->transition = TransFadeBlack;;

	stage_practice_entry = m->entries + 2;
	spell_practice_entry = m->entries + 3;
	main_menu_update_practice_menus();
}

void draw_main_menu_bg(MenuData* menu) {
	glColor4f(1,1,1,1);
	//draw_texture(SCREEN_W/2, SCREEN_H/2, "mainmenu/mainmenubgbg");
	//glColor4f(1,1,1,0.95 + 0.05*sin(menu->frames/100.0));

	draw_texture(SCREEN_W/2, SCREEN_H/2, "mainmenu/mainmenubg");
	glColor4f(1,1,1,1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void draw_main_menu(MenuData *menu) {
	draw_main_menu_bg(menu);

	draw_texture(150.5, 100, "mainmenu/logo");

	glPushMatrix();
	glTranslatef(0, SCREEN_H-270, 0);

	Texture *bg = get_tex("part/smoke");
	glPushMatrix();
	glTranslatef(50 + menu->drawdata[1]/2, menu->drawdata[2], 0);	// 135
	glScalef(menu->drawdata[1]/100.0, 0.5, 1);
	glRotatef(menu->frames*2,0,0,1);
	glColor4f(0,0,0,0.5);
	draw_texture_p(0,0,bg);
	glPopMatrix();

	int i;

	menu->drawdata[1] += (stringwidth(menu->entries[menu->cursor].name, _fonts.mainmenu) - menu->drawdata[1])/10.0;
	menu->drawdata[2] += (35*menu->cursor - menu->drawdata[2])/10.0;

	for(i = 0; i < menu->ecount; i++) {
		float s = 5*sin(menu->frames/80.0 + 20*i);
		menu->entries[i].drawdata += 0.2 * ((i == menu->cursor) - menu->entries[i].drawdata);

		if(menu->entries[i].action == NULL) {
			glColor4f(0.2,0.3,0.5,0.7);
		} else {
			//glColor4f(1,1,1,0.7);
			float a = 1 - menu->entries[i].drawdata;
			glColor4f(1, 0.7 + a, 0.4 + a, 0.7);
		}

		draw_text(AL_Left, 50 + s, 35*i, menu->entries[i].name, _fonts.mainmenu);
	}

	glPopMatrix();

	glColor4f(1,1,1,1);
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

		glDisable(GL_CULL_FACE);
		glPushMatrix();
		glTranslatef(posx,posy,0);
		glScalef(0.2,0.2,0.2);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glRotatef(2*(t%period),rx,ry,rz);
		draw_texture(0,0,"part/petal");
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_CULL_FACE);
		glPopMatrix();
	}

	char version[32];
	snprintf(version, sizeof(version), "v%s", TAISEI_VERSION);
	draw_text(AL_Right,SCREEN_W-5,SCREEN_H-10,version,_fonts.small);
}

void draw_loading_screen(void) {
	preload_resource(RES_TEXTURE, "loading", RESF_PERMANENT);
	set_ortho();
	draw_texture(SCREEN_W/2, SCREEN_H/2, "loading");
	draw_text(AL_Right,SCREEN_W-5,SCREEN_H-10,TAISEI_VERSION,_fonts.small);
	SDL_GL_SwapWindow(video.window);
}

void menu_preload(void) {
	difficulty_preload();

	preload_resources(RES_TEXTURE, RESF_PERMANENT,
		"mainmenu/mainmenubg",
		"mainmenu/logo",
		"part/smoke",
		"part/petal",
		"dialog/marisa",
		"dialog/youmu",
		"charselect_arrow",
	NULL);

	preload_resources(RES_SFX, RESF_PERMANENT | RESF_OPTIONAL,
		"generic_shot",
		"shot_special1",
		"hit",
	NULL);

	preload_resources(RES_BGM, RESF_PERMANENT | RESF_OPTIONAL,
		"menu",
	NULL);
}
