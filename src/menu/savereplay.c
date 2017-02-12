/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include <time.h>
#include "savereplay.h"
#include "options.h"
#include "global.h"
#include "replay.h"
#include "plrmodes.h"
#include "common.h"

void save_rpy(void *a) {
	Replay *rpy = &global.replay;
	char strtime[128], name[128];
	time_t rawtime;
	struct tm * timeinfo;

	// time when the game was *initiated*
	rawtime = (time_t)rpy->stages[0].seed;
	timeinfo = localtime(&rawtime);
	strftime(strtime, 128, "%Y%m%d_%H-%M-%S_%Z", timeinfo);

	char prepr[16], drepr[16];
	plrmode_repr(prepr, 16, rpy->stages[0].plr_char, rpy->stages[0].plr_shot);
	strncpy(drepr, difficulty_name(rpy->stages[0].diff), 16);
	drepr[0] += 'a' - 'A';

	if(rpy->numstages > 1)
		snprintf(name, 128, "taisei_%s_%s_%s", strtime, prepr, drepr);
	else
		snprintf(name, 128, "taisei_%s_stg%d_%s_%s", strtime, rpy->stages[0].stage, prepr, drepr);

	replay_save(rpy, name);
}

void create_saverpy_menu(MenuData *m) {
	create_menu(m);

	m->flags = MF_Transient;

	add_menu_entry(m, "Yes",	save_rpy,		m);
	add_menu_entry(m, "No", (MenuAction) kill_menu, m);
}

void draw_saverpy_menu(MenuData *m) {
	int i;

	draw_options_menu_bg(m);

	draw_menu_selector(SCREEN_W/2 + 100 * m->drawdata[0] - 50, SCREEN_H/2, 163, 81, m->frames);

	glPushMatrix();
	glColor4f(1, 1, 1, 1);
	glTranslatef(SCREEN_W/2, SCREEN_H/2 - 100, 0);
	draw_text(AL_Center, 0, 0, "Save Replay?", _fonts.mainmenu);
	glTranslatef(0, 100, 0);

	m->drawdata[0] += (m->cursor - m->drawdata[0])/10.0;

	for(i = 0; i < m->ecount; i++) {
		MenuEntry *e = &(m->entries[i]);

		e->drawdata += 0.2 * (10*(i == m->cursor) - e->drawdata);
		float a = e->drawdata * 0.1;

		if(e->action == NULL)
			glColor4f(0.5, 0.5, 0.5, 0.5);
		else {
			float ia = 1-a;
			glColor4f(0.9 + ia * 0.1, 0.6 + ia * 0.4, 0.2 + ia * 0.8, 0.7 + 0.3 * a);
		}

		if(e->name)
			draw_text(AL_Center, -50 + 100 * i, 0, e->name, _fonts.mainmenu);
	}

	glPopMatrix();
}

void saverpy_input_event(EventType type, int state, void *arg) {
	if(type == E_CursorLeft)
		menu_event(E_CursorUp, state, arg);
	else if(type == E_CursorRight)
		menu_event(E_CursorDown, state, arg);
	else
		menu_event(type, state, arg);
}

void saverpy_menu_input(MenuData *menu) {
	handle_events(saverpy_input_event, EF_Menu, menu);
}

int saverpy_menu_loop(MenuData *m) {
	return menu_loop(m, saverpy_menu_input, draw_saverpy_menu, NULL);
}
