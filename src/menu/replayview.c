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
#include "stageselect.h"

// I give up.
static MenuData *replayview;
static int pickedstage;

void start_replay(void *arg) {
	ReplayStage *rstg;
	StageInfo 	*gstg;
	int i;
	
	replay_copy(&global.replay, (Replay*)arg);
	global.replaymode = REPLAY_PLAY;
	
	if(global.replay.stgcount == 1)
		pickedstage = 0;
	
	init_player(&global.plr);
	for(i = pickedstage; i < global.replay.stgcount; ++i) {
		replay_select(&global.replay, i);
		rstg = global.replay.current;
		gstg = stage_get(rstg->stage);
		
		if(!gstg) {
			printf("start_replay(): Invalid stage %d in replay at %i skipped.\n", rstg->stage, i);
			continue;
		}
		
		gstg->loop();
		
		if(global.game_over == GAMEOVER_ABORT)
			break;
		global.game_over = 0;
	}
	
	global.game_over = 0;
	global.replaymode = REPLAY_RECORD;
	replay_destroy(&global.replay);
	
	replayview->instantselect = False;
}

MenuData* replayview_stageselect(Replay *rpy) {
	MenuData *m = malloc(sizeof(MenuData));
	int i;
	
	create_menu(m);
	m->context = rpy;
	
	for(i = 0; i < rpy->stgcount; ++i) {
		add_menu_entry(m, stage_get(rpy->stages[i].stage)->title, start_replay, rpy);
	}
	
	return m;
}

void replayview_run(void *arg) {
	Replay *rpy = arg;
	if(rpy->stgcount > 1)
		replayview->context = replayview_stageselect(rpy);
	else
		start_replay(rpy);
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

static void replayview_draw_stagemenu(MenuData *m) {
	float alpha = 1 - m->fade;
	int i;
	
	float height = (1+m->ecount) * 20;
	float width  = 100;
	
	glPushMatrix();
	glTranslatef(SCREEN_W*0.5, SCREEN_H*0.5, 0);
	glScalef(width, height, 1);
	glColor4f(0.1, 0.1, 0.1, 0.7 * alpha);
	draw_quad();
	glPopMatrix();
	
	glPushMatrix();
	glTranslatef(SCREEN_W*0.5, (SCREEN_H-(m->ecount-1)*20)*0.5, 0);
	
	for(i = 0; i < m->ecount; ++i) {
		MenuEntry *e = &(m->entries[i]);
		float a = e->drawdata += 0.2 * ((i == m->cursor) - e->drawdata);
		
		if(e->action == NULL)
			glColor4f(0.5, 0.5, 0.5, 0.5 * alpha);
		else {
			float ia = 1-a;
			glColor4f(0.9 + ia * 0.1, 0.6 + ia * 0.4, 0.2 + ia * 0.8, (0.7 + 0.3 * a) * alpha);
		}
		
		if(i == m->cursor)
			pickedstage = i;
		
		draw_text(AL_Center, 0, 20*i, e->name, _fonts.standard);
	}
	
	glPopMatrix();
}

static void replayview_draw(MenuData *m) {
	draw_stage_menu(m);
	
	if(m->context) {
		MenuData *sm = m->context;
		menu_logic(sm);
		if(sm->quit == 2) {
			destroy_menu(sm);
			m->context = NULL;
		} else {
			fade_out(0.3 * (1 - sm->fade));
			replayview_draw_stagemenu(sm);
			
			if(!sm->abort && sm->quit == 1)
				m->fade = sm->fade;
		}
	}
}

static void replayview_drawitem(void *n, int item, int cnt) {
	MenuEntry *e = (MenuEntry*)n;
	Replay *rpy = (Replay*)e->arg;
	float sizes[] = {1.2, 1.45, 0.8, 0.8, 0.65};
	
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
				time_t t = rpy->stages[0].seed;
				struct tm* timeinfo = localtime(&t);
				strftime(tmp, 128, "%Y-%m-%d %H:%M", timeinfo);
				break;
			
			case 1:
				a = AL_Center;
				strncpy(tmp, rpy->playername, 128);
				break;
			
			case 2:
				plrmode_repr(tmp, 128, rpy->stages[0].plr_char, rpy->stages[0].plr_shot);
				tmp[0] = tmp[0] - 'a' + 'A';
				break;
			
			case 3:
				snprintf(tmp, 128, difficulty_name(rpy->stages[0].diff));
				break;
			
			case 4:
				a = AL_Right;
				if(rpy->stgcount == 1)
					snprintf(tmp, 128, "Stage %i", rpy->stages[0].stage);
				else
					snprintf(tmp, 128, "%i stages", rpy->stgcount);
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
	return ((Replay*)(((MenuEntry*)b)->arg))->stages[0].seed - ((Replay*)(((MenuEntry*)a)->arg))->stages[0].seed;
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
		if(!replay_load(rpy, e->d_name)) {
			free(rpy);
			continue;
		}
		
		add_menu_entry(m, " ", replayview_run, rpy);
		m->entries[m->ecount-1].freearg = replayview_freearg;
		m->entries[m->ecount-1].draw = replayview_drawitem;
		++rpys;
	}
	
	closedir(dir);
	
	qsort(m->entries, m->ecount, sizeof(MenuEntry), replayview_cmp);
	return rpys;
}

void replayview_abort(void *a) {
	MenuData *m = a;
	m->quit = 2;
	
	if(m->context) {	// will unlikely get here
		MenuData *sm = m->context;
		destroy_menu(sm);
		m->context = NULL;
	}
}

void create_replayview_menu(MenuData *m) {
	create_menu(m);
	m->type = MT_Persistent;
	m->abortable = True;
	m->abortaction = replayview_abort;
	m->abortarg = m;
	m->title = "Replays";
	replayview = m;
	
	int r = fill_replayview_menu(m);
	
	if(!r) {
		add_menu_entry(m, "No replays available. Play the game and record some!", replayview_abort, m);
	} else if(r < 0) {
		add_menu_entry(m, "There was a problem getting the replay list :(", replayview_abort, m);
	} else {
		add_menu_separator(m);
		add_menu_entry(m, "Back", replayview_abort, m);
	}
	
	printf("create_replayview_menu()\n");
}

void replayview_menu_input(MenuData *m) {
	if(m->context)
		menu_input((MenuData*)m->context);
	else
		menu_input(m);
	
	if(m->selected >= 0)
		m->instantselect = m->selected != m->ecount-1 && (((Replay*)m->entries[m->selected].arg)->stgcount > 1);
}

int replayview_menu_loop(MenuData *m) {
	return menu_loop(m, replayview_menu_input, replayview_draw);
}

