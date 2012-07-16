/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <https://github.com/nexAkari>
 */

#include <dirent.h>
#include "global.h"
#include "menu.h"
#include "options.h"
#include "mainmenu.h"
#include "replayview.h"
#include "paths/native.h"

void backtomain(void*);

void start_replay(void *arg) {
	replay_load(&global.replay, (char*)arg);
	StageInfo *s = stage_get(global.replay.stage);
	
	if(!s) {
		printf("Invalid stage %d in replay... wtf?!\n", global.replay.stage);
		return;
	}
	
	init_player(&global.plr);
	
	// XXX: workaround, doesn't even always work. DEBUG THIS.
	global.fps.show_fps = 0;
	
	global.replaymode = REPLAY_PLAY;
	s->loop();
	global.replaymode = REPLAY_RECORD;
	global.game_over = 0;
}

int strendswith(char *s, char *e) {
	int ls = strlen(s);
	int le = strlen(e);
	
	if(le > ls)
		return False;
	
	int i; for(i = ls - 1; i < le; ++i)
		if(s[i] != e[i])
			return False;
	
	return True;
}

int fill_replayview_menu(MenuData *m) {
	DIR *dir = opendir(get_replays_path());
	struct dirent *e;
	int rpys = 0;
	
	if(!dir) {
		printf("Could't read %s\n", get_replays_path());
		return -1;
	}
	
	char ext[5];
	snprintf(ext, 5, ".%s", REPLAY_EXTENSION);
	
	while((e = readdir(dir))) {
		if(!strendswith(e->d_name, ext))
			continue;
		
		int size = strlen(e->d_name) - strlen(ext);
		char *s = (char*)malloc(size);
		strncpy(s, e->d_name, size);
		s[size] = 0;
		
		add_menu_entry(m, s, start_replay, s);
		m->entries[m->ecount-1].freearg = True;
		++rpys;
	}
	
	closedir(dir);
	return rpys;
}

void create_replayview_menu(MenuData *m) {
	create_menu(m);
	m->type = MT_Transient;
	m->title = "Replays";
	
	int r = fill_replayview_menu(m);
	
	if(!r) {
		add_menu_entry(m, "No replays available. Play the game and record some!", backtomain, m);
	} else if(r < 0) {
		add_menu_entry(m, "There was a problem getting the replay list :(", backtomain, m);
	} else {
		add_menu_separator(m);
		add_menu_entry(m, "Back", backtomain, m);
	}
}

void draw_stage_menu(MenuData *m);
int replayview_menu_loop(MenuData *m) {
	return menu_loop(m, NULL, draw_stage_menu);
}

