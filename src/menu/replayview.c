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
#include "common.h"

// Type of MenuData.context
typedef struct ReplayviewContext {
	MenuData *submenu;
	int pickedstage;
	double sub_fade;
} ReplayviewContext;

// Type of MenuEntry.arg (which should be renamed to context, probably...)
typedef struct ReplayviewItemContext {
	Replay *replay;
	char *replayname;
} ReplayviewItemContext;

void start_replay(MenuData *menu, void *arg) {
	ReplayviewItemContext *ictx = arg;
	ReplayviewContext *mctx = menu->context;

	Replay *rpy = ictx->replay;

	if(!replay_load(rpy, ictx->replayname, REPLAY_READ_EVENTS)) {
		return;
	}

	replay_play(ictx->replay, mctx->pickedstage);
	start_bgm("bgm_menu");
}

static void replayview_draw_stagemenu(MenuData*);

MenuData* replayview_sub_stageselect(MenuData *menu, ReplayviewItemContext *ictx) {
	MenuData *m = malloc(sizeof(MenuData));
	Replay *rpy = ictx->replay;

	create_menu(m);
	m->draw = replayview_draw_stagemenu;
	m->context = menu->context;
	m->flags = MF_Transient | MF_Abortable;
	m->transition = NULL;

	for(int i = 0; i < rpy->numstages; ++i) {
		add_menu_entry(m, stage_get(rpy->stages[i].stage)->title, start_replay, ictx)->transition = TransFadeBlack;
	}

	return m;
}

void replayview_run(MenuData *menu, void *arg) {
	ReplayviewItemContext *ctx = arg;
	ReplayviewContext *menuctx = menu->context;
	Replay *rpy = ctx->replay;

	if(rpy->numstages > 1) {
		menuctx->submenu = replayview_sub_stageselect(menu, ctx);
	} else {
		start_replay(menu, ctx);
	}
}

static void replayview_freearg(void *a) {
	ReplayviewItemContext *ctx = a;
	replay_destroy(ctx->replay);
	free(ctx->replay);
	free(ctx->replayname);
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

	float alpha = 1 - ctx->sub_fade;
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

	float sizes[] = {1.2, 1.45, 0.8, 0.8, 0.75};
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
				strlcpy(tmp, rpy->playername, sizeof(tmp));
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
				if(rpy->numstages == 1) {
					StageInfo *stg = stage_get(rpy->stages[0].stage);

					if(stg) {
						snprintf(tmp, sizeof(tmp), "%s", stg->title);
					} else {
						snprintf(tmp, sizeof(tmp), "?????");
					}
				} else {
					snprintf(tmp, sizeof(tmp), "%i stages", rpy->numstages);
				}
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

static void replayview_logic(MenuData *m) {
	ReplayviewContext *ctx = m->context;

	if(ctx->submenu) {
		MenuData *sm = ctx->submenu;

		if(sm->state == MS_Dead) {
			if(ctx->sub_fade == 1.0) {
				destroy_menu(sm);
				ctx->submenu = NULL;
				return;
			}

			ctx->sub_fade = approach(ctx->sub_fade, 1.0, 1.0/FADE_TIME);
		} else {
			ctx->sub_fade = approach(ctx->sub_fade, 0.0, 1.0/FADE_TIME);
		}
	}
}

static void replayview_draw(MenuData *m) {
	ReplayviewContext *ctx = m->context;

	draw_options_menu_bg(m);
	draw_menu_title(m, "Replays");

	m->drawdata[0] = SCREEN_W/2 - 100;
	m->drawdata[1] = (SCREEN_W - 200)*1.75;
	m->drawdata[2] += (20*m->cursor - m->drawdata[2])/10.0;

	draw_menu_list(m, 100, 100, replayview_drawitem);

	if(ctx->submenu) {
		ctx->submenu->draw(ctx->submenu);
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
		log_warn("Could't read %s", get_replays_path());
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

		ictx->replay = rpy;
		ictx->replayname = malloc(strlen(e->d_name) + 1);
		strcpy(ictx->replayname, e->d_name);

		add_menu_entry(m, " ", replayview_run, ictx)->transition = rpy->numstages < 2 ? TransFadeBlack : NULL;
		++rpys;
	}

	closedir(dir);

	if(m->entries) {
		qsort(m->entries, m->ecount, sizeof(MenuEntry), replayview_cmp);
	}

	return rpys;
}

void replayview_menu_input(MenuData *m) {
	ReplayviewContext *ctx = (ReplayviewContext*)m->context;
	MenuData *sub = ctx->submenu;

	if(transition.state == TRANS_FADE_IN) {
		menu_no_input(m);
	} else {
		if(sub && sub->state != MS_Dead) {
			sub->input(sub);
		} else {
			menu_input(m);
		}
	}
}

void replayview_free(MenuData *m) {
	if(m->context) {
		free(m->context);
		m->context = NULL;
	}

	for(int i = 0; i < m->ecount; i++) {
		if(m->entries[i].action == replayview_run) {
			replayview_freearg(m->entries[i].arg);
		}
	}
}

void create_replayview_menu(MenuData *m) {
	create_menu(m);
	m->logic = replayview_logic;
	m->input = replayview_menu_input;
	m->draw = replayview_draw;
	m->end = replayview_free;
	m->transition = TransMenuDark;

	ReplayviewContext *ctx = malloc(sizeof(ReplayviewContext));
	memset(ctx, 0, sizeof(ReplayviewContext));

	m->context = ctx;
	m->flags = MF_Abortable;

	int r = fill_replayview_menu(m);

	if(!r) {
		add_menu_entry(m, "No replays available. Play the game and record some!", menu_commonaction_close, NULL);
	} else if(r < 0) {
		add_menu_entry(m, "There was a problem getting the replay list :(", menu_commonaction_close, NULL);
	} else {
		add_menu_separator(m);
		add_menu_entry(m, "Back", menu_commonaction_close, NULL);
	}
}
