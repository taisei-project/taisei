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

void backtomain(void*);

void create_stage_menu(MenuData *m) {
	char title[STGMENU_MAX_TITLE_LENGTH];
	int i;
	
	create_menu(m);
	m->type = MT_Transient;
	// TODO: I think ALL menus should use the title field, but I don't want to screw with it right now.
	m->title = "Stage Select";
	
	for(i = 0; stages[i].loop; ++i) if(!stages[i].hidden) {
		snprintf(title, STGMENU_MAX_TITLE_LENGTH, "%d. %s", i + 1, stages[i].title);
		add_menu_entry(m, title, start_story, &(stages[i]));
	}
	
	add_menu_separator(m);
	add_menu_entry(m, "Back", backtomain, m);
}

void draw_stage_menu(MenuData *m) {
	draw_options_menu_bg(m);
	
	int w, h;
	TTF_SizeText(_fonts.mainmenu, m->title, &w, &h);
	draw_text(AL_Right, (w + 10) * (1-m->fade), 30, m->title, _fonts.mainmenu);
	
	glPushMatrix();
	glTranslatef(100, 100, 0);
	
	glPushMatrix();
	glTranslatef(SCREEN_W/2 - 100, m->drawdata[2], 0);
	glScalef(SCREEN_W - 200, 20, 1);
	glColor4f(0,0,0,0.5);
	
	draw_quad();
	glPopMatrix();
	
	int i;
	for(i = 0; i < m->ecount; i++) {
		MenuEntry *e = &(m->entries[i]);
		
		e->drawdata += 0.2 * (10*(i == m->cursor) - e->drawdata);
		
		if(e->action == NULL)
			glColor4f(0.5, 0.5, 0.5, 0.7);
		else if(i == m->cursor)
			glColor4f(1,1,0,0.7);
		else
			glColor4f(1, 1, 1, 0.7);
		
		if(e->name)
			draw_text(AL_Left, 20 - e->drawdata, 20*i, e->name, _fonts.standard);
	}
	
	glPopMatrix();
	m->drawdata[2] += (20*m->cursor - m->drawdata[2])/10.0;
	
	fade_out(m->fade);
}

int stage_menu_loop(MenuData *m) {
	return menu_loop(m, NULL, draw_stage_menu);
}
