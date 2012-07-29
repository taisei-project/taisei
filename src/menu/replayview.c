/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <https://github.com/nexAkari>
 */

#include <dirent.h>
#include <time.h>
#include "global.h"
#include "menu.h"
#include "options.h"
#include "mainmenu.h"
#include "replayview.h"
#include "paths/native.h"
#include "plrmodes.h"
#include "video.h"

void backtomain(void*);

void start_replay(void *arg) {
	replay_copy(&global.replay, (Replay*)arg);
	StageInfo *s = stage_get(global.replay.stage);
	
	if(!s) {
		printf("Invalid stage %d in replay... wtf?!\n", global.replay.stage);
		return;
	}
	
	init_player(&global.plr);
		
	global.replaymode = REPLAY_PLAY;
	s->loop();
	global.replaymode = REPLAY_RECORD;
	global.game_over = 0;
}

static void replayview_freearg(void *a) {
	Replay *r = (Replay*)a;
	replay_destroy(r);
	free(r);
}

static void shorten(char *s, int width) {
	float sw = stringwidth(s, _fonts.standard);
	float len = strlen(s);
	float avgw = sw / len;
	int c = width / avgw, i;
	
	if(c > len)
		return;
	
	s[c+1] = 0;
	
	for(i = 0; i < 3; ++i)
		s[c - i] = '.';
}

static void replayview_drawitem(void *n, int item, int cnt) {
	MenuEntry *e = (MenuEntry*)n;
	Replay *rpy = (Replay*)e->arg;
	float sizes[] = {1.1, 1.6, 0.8, 0.8, 0.6};
	
	//draw_text(AL_Left, 20 - e->drawdata, 20*i, "lol replay omg", _fonts.standard);
	
	int columns = 5, i, j;
	for(i = 0; i < columns; ++i) {
		char tmp[128];
		int csize = sizes[i] * (SCREEN_W - 210)/columns;
		int o = 0;
		
		for(j = 0; j < i; ++j)
			o += sizes[j] * (SCREEN_W - 210)/columns;
		
		Alignment a = AL_Center;
		
		// hell yeah, another loop-switch sequence
		switch(i) {
			case 0:
				a = AL_Left;
				
				time_t t = rpy->seed;
				struct tm* timeinfo = localtime(&t);
				strftime(tmp, 128, "%y/%m/%d  %H:%M", timeinfo);
				
				break;
			
			case 1:
				a = AL_Center;
				strncpy(tmp, rpy->playername, 128);
				break;
			
			case 2:
				plrmode_repr(tmp, 128, rpy->plr_char, rpy->plr_shot);
				tmp[0] = tmp[0] - 'a' + 'A';
				break;
			
			case 3:
				snprintf(tmp, 128, difficulty_name(rpy->diff));
				break;
			
			case 4:
				a = AL_Right;
				snprintf(tmp, 128, "Stage %i", rpy->stage);
				break;
		}
		
		shorten(tmp, csize);
		switch(a) {
			case AL_Center:	o += csize * 0.5 - stringwidth(tmp, _fonts.standard) * 0.5;		break;
			case AL_Right:	o += csize - stringwidth(tmp, _fonts.standard);					break;
			default:																		break;
		}
		
		draw_text(AL_Left, o + 10, 20*item, tmp, _fonts.standard);
	}
}

int replayview_cmp(const void *a, const void *b) {
	return ((Replay*)(((MenuEntry*)b)->arg))->seed - ((Replay*)(((MenuEntry*)a)->arg))->seed;
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
		
		Replay *rpy = malloc(sizeof(Replay));
		replay_load(rpy, e->d_name);
		
		/*
		int size = strlen(e->d_name) - strlen(ext) + 1;
		char *s = (char*)malloc(size);
		strncpy(s, e->d_name, size);
		s[size-1] = 0;
		*/
		
		add_menu_entry(m, " ", start_replay, rpy);
		m->entries[m->ecount-1].freearg = replayview_freearg;
		m->entries[m->ecount-1].draw = replayview_drawitem;
		++rpys;
	}
	
	closedir(dir);
	
	qsort(m->entries, m->ecount, sizeof(MenuEntry), replayview_cmp);
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
	
	printf("create_replayview_menu()\n");
}

void draw_stage_menu(MenuData *m);
int replayview_menu_loop(MenuData *m) {
	return menu_loop(m, NULL, draw_stage_menu);
}

