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

struct ReplayviewItemContext;
typedef struct ReplayviewItemContext ReplayviewItemContext;

// Type of MenuData.context
typedef struct ReplayviewContext {
	ReplayviewItemContext *commonictx;
	MenuData *submenu;
	int pickedstage;
} ReplayviewContext;

// Type of MenuEntry.arg (which should be renamed to context, probably...)
typedef struct ReplayviewItemContext {
	MenuData *menu;
	Replay *replay;
	char *replayname;
} ReplayviewItemContext;

void start_replay(void *arg) {
	ReplayviewItemContext *ictx = arg;
	ReplayviewContext *mctx = ictx->menu->context;

	Replay *rpy = ictx->replay;

	if(!replay_load(rpy, ictx->replayname, REPLAY_READ_EVENTS)) {
		return;
	}

	replay_copy(&global.replay, ictx->replay, true);
	global.replaymode = REPLAY_PLAY;

	if(global.replay.numstages == 1){
		mctx->pickedstage = 0;
	}

	init_player(&global.plr);

	for(int i = mctx->pickedstage; i < global.replay.numstages; ++i) {
		ReplayStage *rstg = global.replay_stage = global.replay.stages+i;
		StageInfo *gstg = stage_get(rstg->stage);

		if(!gstg) {
			printf("start_replay(): Invalid stage %d in replay at %i skipped.\n", rstg->stage, i);
			continue;
		}

		gstg->loop();

		if(global.game_over == GAMEOVER_ABORT) {
			break;
		}

		global.game_over = 0;
	}

	start_bgm("bgm_menu");
	global.game_over = 0;
	global.replaymode = REPLAY_RECORD;
	replay_destroy(&global.replay);
	global.replay_stage = NULL;
}

MenuData* replayview_sub_stageselect(ReplayviewItemContext *ictx) {
	MenuData *m = malloc(sizeof(MenuData));
	Replay *rpy = ictx->replay;

	create_menu(m);
	m->context = ictx->menu->context;
	m->flags = MF_Transient | MF_Abortable;
	m->transition = 0;

	for(int i = 0; i < rpy->numstages; ++i) {
		add_menu_entry(m, stage_get(rpy->stages[i].stage)->title, start_replay, ictx)->transition = TransFadeBlack;
	}

	return m;
}

void replayview_run(void *arg) {
	ReplayviewItemContext *ctx = arg;
	ReplayviewContext *menuctx = ctx->menu->context;
	Replay *rpy = ctx->replay;

	if(rpy->numstages > 1) {
		menuctx->submenu = replayview_sub_stageselect(ctx);
	} else {
		start_replay(ctx);
	}
}

static void replayview_freearg(void *a) {
	ReplayviewItemContext *ctx = a;

	if(ctx->replay) {
		replay_destroy(ctx->replay);
		free(ctx->replay);
	}

	if(ctx->replayname) {
		free(ctx->replayname);
	}

	free(ctx);
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
	// this context is shared with the parent menu
	ReplayviewContext *ctx = m->context;

	float alpha = 1-menu_fade(m);
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

		if(e->action == NULL) {
			glColor4f(0.5, 0.5, 0.5, 0.5 * alpha);
		} else {
			float ia = 1-a;
			glColor4f(0.9 + ia * 0.1, 0.6 + ia * 0.4, 0.2 + ia * 0.8, (0.7 + 0.3 * a) * alpha);
		}

		if(i == m->cursor) {
			ctx->pickedstage = i;
		}

		draw_text(AL_Center, 0, 20*i, e->name, _fonts.standard);
	}

	glPopMatrix();
}

static void replayview_drawitem(void *n, int item, int cnt) {
	MenuEntry *e = (MenuEntry*)n;
	ReplayviewItemContext *ictx = e->arg;

	if(!ictx) {
		return;
	}

	Replay *rpy = ictx->replay;

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
				strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M", timeinfo);
				break;

			case 1:
				a = AL_Center;
				strncpy(tmp, rpy->playername, 128);
				break;

			case 2:
				plrmode_repr(tmp, sizeof(tmp), rpy->stages[0].plr_char, rpy->stages[0].plr_shot);
				tmp[0] = tmp[0] - 'a' + 'A';
				break;

			case 3:
				snprintf(tmp, sizeof(tmp), "%s", difficulty_name(rpy->stages[0].diff));
				break;

			case 4:
				a = AL_Right;
				if(rpy->numstages == 1)
					snprintf(tmp, sizeof(tmp), "Stage %i", rpy->stages[0].stage);
				else
					snprintf(tmp, sizeof(tmp), "%i stages", rpy->numstages);
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

static void replayview_draw(MenuData *m) {
	ReplayviewContext *ctx = m->context;

	draw_options_menu_bg(m);
	draw_menu_title(m, "Replays");

	m->drawdata[0] = SCREEN_W/2 - 100;
	m->drawdata[1] = (SCREEN_W - 200)*0.85;
	m->drawdata[2] += (20*m->cursor - m->drawdata[2])/10.0;

	draw_menu_list(m, 100, 100, replayview_drawitem);

	if(ctx->submenu) {
		MenuData *sm = ctx->submenu;
		menu_logic(sm);
		if(sm->state == MS_Dead) {
			destroy_menu(sm);
			ctx->submenu = NULL;
		} else {
			replayview_draw_stagemenu(sm);
		}
	}

}

int replayview_cmp(const void *a, const void *b) {
	ReplayviewItemContext *actx = ((MenuEntry*)a)->arg;
	ReplayviewItemContext *bctx = ((MenuEntry*)b)->arg;

	Replay *arpy = actx->replay;
	Replay *brpy = bctx->replay;

	return brpy->stages[0].seed - arpy->stages[0].seed;
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
		if(!replay_load(rpy, e->d_name, REPLAY_READ_META)) {
			free(rpy);
			continue;
		}

		ReplayviewItemContext *ictx = malloc(sizeof(ReplayviewItemContext));
		memset(ictx, 0, sizeof(ReplayviewItemContext));

		ictx->menu = m;
		ictx->replay = rpy;
		ictx->replayname = malloc(strlen(e->d_name) + 1);
		strcpy(ictx->replayname, e->d_name);

		add_menu_entry_f(m, " ", replayview_run, ictx, (rpy->numstages > 1)*MF_InstantSelect)->transition = rpy->numstages < 2 ? TransFadeBlack : NULL;
		++rpys;
	}

	closedir(dir);
	qsort(m->entries, m->ecount, sizeof(MenuEntry), replayview_cmp);
	return rpys;
}

void replayview_abort(void *a) {
	kill_menu(((ReplayviewItemContext*)a)->menu);
}

void create_replayview_menu(MenuData *m) {
	create_menu(m);

	ReplayviewContext *ctx = malloc(sizeof(ReplayviewContext));
	memset(ctx, 0, sizeof(ReplayviewContext));

	ctx->commonictx = malloc(sizeof(ReplayviewItemContext));
	memset(ctx->commonictx, 0, sizeof(ReplayviewItemContext));
	ctx->commonictx->menu = m;

	m->context = ctx;
	m->flags = MF_Abortable;

	int r = fill_replayview_menu(m);

	if(!r) {
		add_menu_entry(m, "No replays available. Play the game and record some!", replayview_abort, ctx->commonictx);
	} else if(r < 0) {
		add_menu_entry(m, "There was a problem getting the replay list :(", replayview_abort, ctx->commonictx);
	} else {
		add_menu_separator(m);
		add_menu_entry(m, "Back", replayview_abort, ctx->commonictx);
	}
}

void replayview_menu_input(MenuData *m) {
	ReplayviewContext *ctx = (ReplayviewContext*)m->context;
	menu_input(ctx->submenu ? ctx->submenu : m);
}

void replayview_free(MenuData *m) {
	if(m->context) {
		ReplayviewContext *ctx = m->context;

		if(ctx) {
			replayview_freearg(ctx->commonictx);
			ctx->commonictx = NULL;
		}

		free(m->context);
		m->context = NULL;
	}

	for(int i = 0; i < m->ecount; i++) {
		if(m->entries[i].action == replayview_run) {
			replayview_freearg(m->entries[i].arg);
		}
	}
}

int replayview_menu_loop(MenuData *m) {
	return menu_loop(m, replayview_menu_input, replayview_draw, replayview_free);
}

