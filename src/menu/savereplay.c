/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include <time.h>
#include "savereplay.h"
#include "global.h"
#include "replay.h"
#include "plrmodes.h"

void save_rpy(void *a) {
	Replay *rpy = &global.replay;
	char strtime[128], name[128];
	time_t rawtime;
	struct tm * timeinfo;
	
	// time when the game was *initiated*
	rawtime = (time_t)rpy->seed;
	timeinfo = localtime(&rawtime);
	strftime(strtime, 128, "%Y%m%d_%H-%M-%S_%Z", timeinfo);
	
	char prepr[16];
	plrmode_repr(prepr, 16, rpy->plr_char, rpy->plr_shot);
	snprintf(name, 128, "taisei_stg%d_%s_%s", rpy->stage, prepr, strtime);
	
	replay_save(rpy, name);
}

void dont_save_rpy(void *a) {
	//((MenuData*)a)->quit = 2;
	((MenuData*)a)->selected = 0;
}

MenuData* create_saverpy_menu() {
	MenuData *m = malloc(sizeof(MenuData));
	create_menu(m);
	m->context = "Save replay?";
	
	add_menu_entry(m, "Yes",	save_rpy,		m);
	add_menu_entry(m, "No",		dont_save_rpy,	m);
	
	return m;
}
