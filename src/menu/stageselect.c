/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2011, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include "global.h"
#include "menu.h"
#include "options.h"
#include "stage.h"
#include "stageselect.h"
#include "mainmenu.h"

void create_stage_menu(MenuData *m) {
	char title[STGMENU_MAX_TITLE_LENGTH];
	int i;

	create_menu(m);
	m->flags = MF_Transient | MF_Abortable;

	for(i = 0; stages[i].loop; ++i) if(!stages[i].hidden) {
		snprintf(title, STGMENU_MAX_TITLE_LENGTH, "%s", stages[i].title);
		add_menu_entry(m, title, start_story, &(stages[i]));
	}

	add_menu_separator(m);
	add_menu_entry(m, "Back", (MenuAction)kill_menu, m);
}

void draw_menu_list(MenuData *m, float x, float y, void (*draw)(void*, int, int)) {
	glPushMatrix();
	float offset = ((((m->ecount+5) * 20) > SCREEN_H)? min(0, SCREEN_H * 0.7 - y - m->drawdata[2]) : 0);
	glTranslatef(x, y + offset, 0);

	draw_menu_selector(m->drawdata[0], m->drawdata[2], m->drawdata[1]/100.0, 0.2, m->frames);

	int i;
	for(i = 0; i < m->ecount; i++) {
		MenuEntry *e = &(m->entries[i]);
		e->drawdata += 0.2 * (10*(i == m->cursor) - e->drawdata);

		float p = offset + 20*i;

		if(p < -y-10 || p > SCREEN_H+10)
			continue;

		float a = e->drawdata * 0.1;
		float o = (p < 0? 1-p/(-y-10) : 1);
		if(e->action == NULL)
			glColor4f(0.5, 0.5, 0.5, 0.5*o);
		else {
			float ia = 1-a;
			glColor4f(0.9 + ia * 0.1, 0.6 + ia * 0.4, 0.2 + ia * 0.8, (0.7 + 0.3 * a)*o);
		}

		if(draw && i < m->ecount-1)
			draw(e, i, m->ecount);
		else if(e->name)
        	draw_text(AL_Left, 20 - e->drawdata, 20*i, e->name, _fonts.standard);
	}

	glPopMatrix();
}

void draw_stage_menu(MenuData *m) {
	MenuEntry *s = &(m->entries[m->cursor]);

	draw_options_menu_bg(m);
	draw_menu_title(m, "Stage Select");

	m->drawdata[0] += (stringwidth(s->name, _fonts.mainmenu)/2.0 - s->drawdata*1.5 - m->drawdata[0])/10.0;
	m->drawdata[1] += (stringwidth(s->name, _fonts.mainmenu) - m->drawdata[1])/10.0;
	m->drawdata[2] += (20*m->cursor - m->drawdata[2])/10.0;

	draw_menu_list(m, 100, 100, NULL);
}

int stage_menu_loop(MenuData *m) {
	return menu_loop(m, NULL, draw_stage_menu, NULL);
}
