/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "global.h"
#include "menu.h"
#include "options.h"
#include "mainmenu.h"
#include "replayview.h"
#include "plrmodes.h"
#include "video.h"
#include "common.h"
#include "replay/state.h"
#include "replay/struct.h"

// Type of MenuData.context
typedef struct ReplayviewContext {
	MenuData *submenu;
	MenuData *next_submenu;
	double sub_fade;
} ReplayviewContext;

// Type of MenuEntry.arg (which should be renamed to context, probably...)
typedef struct ReplayviewItemContext {
	Replay *replay;
	char *replayname;
} ReplayviewItemContext;

static MenuData* replayview_sub_messagebox(MenuData *parent, const char *message);

static void replayview_set_submenu(MenuData *parent, MenuData *submenu) {
	ReplayviewContext *ctx = parent->context;

	if(ctx->submenu) {
		ctx->submenu->state = MS_Dead;
		ctx->next_submenu = submenu;
	} else {
		ctx->submenu = submenu;
	}

	if(submenu != NULL) {
		// submenu->context = ctx;
		assert(submenu->context == ctx);
	}
}

typedef struct {
	Replay *rpy;
	int stgnum;
} startrpy_arg_t;

static void on_replay_finished(CallChainResult ccr) {
	audio_bgm_play(res_bgm("menu"), true, 0, 0);
}

static void really_start_replay(void *varg) {
	startrpy_arg_t arg = *(startrpy_arg_t*)varg;
	free(varg);
	replay_play(arg.rpy, arg.stgnum, CALLCHAIN(on_replay_finished, NULL));
}

static void start_replay(MenuData *menu, void *arg) {
	ReplayviewItemContext *ictx = arg;
	ReplayviewContext *mctx = menu->context;

	int stagenum = 0;

	if(mctx->submenu) {
		stagenum = mctx->submenu->cursor;
	}

	Replay *rpy = ictx->replay;

	if(!replay_load(rpy, ictx->replayname, REPLAY_READ_EVENTS)) {
		replayview_set_submenu(menu, replayview_sub_messagebox(menu, "Failed to load replay events"));
		return;
	}

	ReplayStage *stg = dynarray_get_ptr(&rpy->stages, stagenum);
	char buf[64];

	if(!stageinfo_get_by_id(stg->stage)) {
		replay_destroy_events(rpy);
		snprintf(buf, sizeof(buf), "Can't replay this stage: unknown stage ID %X", stg->stage);
		replayview_set_submenu(menu, replayview_sub_messagebox(menu, buf));
		return;
	}

	if(!plrmode_find(stg->plr_char, stg->plr_shot)) {
		replay_destroy_events(rpy);
		snprintf(buf, sizeof(buf), "Can't replay this stage: unknown player character/mode %X/%X", stg->plr_char, stg->plr_shot);
		replayview_set_submenu(menu, replayview_sub_messagebox(menu, buf));
		return;
	}

	set_transition_callback(TransFadeBlack, FADE_TIME, FADE_TIME, really_start_replay,
		memdup(&(startrpy_arg_t) {
			.rpy = ictx->replay,
			.stgnum = stagenum
		}, sizeof(startrpy_arg_t))
	);
}

static void replayview_draw_stagemenu(MenuData*);
static void replayview_draw_messagebox(MenuData*);

static MenuData* replayview_sub_stageselect(MenuData *parent, ReplayviewItemContext *ictx) {
	MenuData *m = alloc_menu();
	Replay *rpy = ictx->replay;

	m->draw = replayview_draw_stagemenu;
	m->flags = MF_Transient | MF_Abortable;
	m->transition = NULL;
	m->context = parent->context;

	dynarray_foreach_elem(&rpy->stages, ReplayStage *rstg, {
		uint16_t stage_id = rstg->stage;
		StageInfo *stg = stageinfo_get_by_id(stage_id);

		if(LIKELY(stg != NULL)) {
			add_menu_entry(m, stg->title, start_replay, ictx);
		} else {
			char label[64];
			snprintf(label, sizeof(label), "Unknown stage %X", stage_id);
			add_menu_entry(m, label, menu_action_close, NULL);
		}
	});

	return m;
}

static MenuData* replayview_sub_messagebox(MenuData *parent, const char *message) {
	MenuData *m = alloc_menu();
	m->draw = replayview_draw_messagebox;
	m->flags = MF_Transient | MF_Abortable;
	m->transition = NULL;
	m->context = parent->context;
	add_menu_entry(m, message, menu_action_close, NULL);
	return m;
}

static void replayview_run(MenuData *menu, void *arg) {
	ReplayviewItemContext *ctx = arg;
	Replay *rpy = ctx->replay;

	if(rpy->stages.num_elements > 1) {
		replayview_set_submenu(menu, replayview_sub_stageselect(menu, ctx));
	} else {
		start_replay(menu, ctx);
	}
}

static void replayview_freearg(void *a) {
	ReplayviewItemContext *ctx = a;
	replay_reset(ctx->replay);
	free(ctx->replay);
	free(ctx->replayname);
	free(ctx);
}

static void replayview_draw_submenu_bg(float width, float height, float alpha) {
	r_state_push();
	r_mat_mv_push();
	r_mat_mv_translate(SCREEN_W*0.5, SCREEN_H*0.5, 0);
	r_mat_mv_scale(width, height, 1);
	alpha *= 0.7;
	r_color4(0.1 * alpha, 0.1 * alpha, 0.1 * alpha, alpha);
	r_shader_standard_notex();
	r_draw_quad();
	r_mat_mv_pop();
	r_state_pop();
}

static void replayview_draw_messagebox(MenuData* m) {
	// this context is shared with the parent menu
	ReplayviewContext *ctx = m->context;
	MenuEntry *e = dynarray_get_ptr(&m->entries, 0);
	float alpha = 1 - ctx->sub_fade;
	float height = font_get_lineskip(res_font("standard")) * 2;
	float width  = text_width(res_font("standard"), e->name, 0) + 64;
	replayview_draw_submenu_bg(width, height, alpha);

	text_draw(e->name, &(TextParams) {
		.align = ALIGN_CENTER,
		.color = RGBA_MUL_ALPHA(0.9, 0.6, 0.2, alpha),
		.pos = { SCREEN_W*0.5, SCREEN_H*0.5 },
		.shader = "text_default",
	});
}

static void replayview_draw_stagemenu(MenuData *m) {
	// this context is shared with the parent menu
	ReplayviewContext *ctx = m->context;
	float alpha = 1 - ctx->sub_fade;
	float height = (1 + m->entries.num_elements) * 20;
	float width  = 180;

	replayview_draw_submenu_bg(width, height, alpha);

	r_mat_mv_push();
	r_mat_mv_translate(SCREEN_W*0.5, (SCREEN_H - (m->entries.num_elements - 1) * 20) * 0.5, 0);

	dynarray_foreach(&m->entries, int i, MenuEntry *e, {
		float a = e->drawdata;
		Color clr;

		if(e->action == NULL) {
			clr = *RGBA_MUL_ALPHA(0.5, 0.5, 0.5, 0.5 * alpha);
		} else {
			float ia = 1-a;
			clr = *RGBA_MUL_ALPHA(0.9 + ia * 0.1, 0.6 + ia * 0.4, 0.2 + ia * 0.8, (0.7 + 0.3 * a) * alpha);
		}

		text_draw(e->name, &(TextParams) {
			.align = ALIGN_CENTER,
			.pos = { 0, 20*i },
			.color = &clr,
			.shader = "text_default",
		});
	});

	r_mat_mv_pop();
}

static void replayview_drawitem(MenuEntry *e, int item, int cnt, void *ctx) {
	ReplayviewItemContext *ictx = e->arg;

	if(!ictx) {
		return;
	}

	Replay *rpy = ictx->replay;

	float sizes[] = { 1.2, 2.2, 0.5, 0.55, 0.55 };
	int columns = sizeof(sizes)/sizeof(float), i, j;
	float base_size = (SCREEN_W - 110.0) / columns;

	ReplayStage *first_stage = dynarray_get_ptr(&rpy->stages, 0);
	time_t t = first_stage->start_time;
	struct tm *timeinfo = localtime(&t);

	for(i = 0; i < columns; ++i) {
		char tmp[128];
		float csize = sizes[i] * base_size;
		float o = 0;

		for(j = 0; j < i; ++j)
			o += sizes[j] * base_size;

		Alignment a = ALIGN_CENTER;

		switch(i) {
			case 0:
				a = ALIGN_LEFT;
				strftime(tmp, sizeof(tmp), "%Y-%m-%d  %H:%M", timeinfo);
				break;

			case 1:
				a = ALIGN_LEFT;
				strlcpy(tmp, rpy->playername, sizeof(tmp));
				break;

			case 2: {
				a = ALIGN_RIGHT;
				PlayerMode *plrmode = plrmode_find(first_stage->plr_char, first_stage->plr_shot);

				if(plrmode == NULL) {
					strlcpy(tmp, "?????", sizeof(tmp));
				} else {
					plrmode_repr(tmp, sizeof(tmp), plrmode, false);
				}

				break;
			}

			case 3:
				a = ALIGN_CENTER;
				snprintf(tmp, sizeof(tmp), "%s", difficulty_name(first_stage->diff));
				break;

			case 4:
				a = ALIGN_LEFT;
				if(rpy->stages.num_elements == 1) {
					StageInfo *stg = stageinfo_get_by_id(first_stage->stage);

					if(stg) {
						snprintf(tmp, sizeof(tmp), "%s", stg->title);
					} else {
						snprintf(tmp, sizeof(tmp), "?????");
					}
				} else {
					snprintf(tmp, sizeof(tmp), "%i stages", rpy->stages.num_elements);
				}
				break;
		}

		switch(a) {
			case ALIGN_CENTER: o += csize * 0.5 - text_width(res_font("standard"), tmp, 0) * 0.5; break;
			case ALIGN_RIGHT:  o += csize - text_width(res_font("standard"), tmp, 0);             break;
			default:                                                                              break;
		}

		text_draw(tmp, &(TextParams) {
			.pos = { o + 10, 20 * item },
			.shader = "text_default",
			.max_width = csize,
		});
	}
}

static void replayview_logic(MenuData *m) {
	ReplayviewContext *ctx = m->context;

	if(ctx->submenu) {
		MenuData *sm = ctx->submenu;

		dynarray_foreach(&sm->entries, int i, MenuEntry *e, {
			e->drawdata += 0.2 * ((i == sm->cursor) - e->drawdata);
		});

		if(sm->state == MS_Dead) {
			if(ctx->sub_fade == 1.0) {
				free_menu(sm);
				ctx->submenu = ctx->next_submenu;
				ctx->next_submenu = NULL;
				return;
			}

			ctx->sub_fade = approach(ctx->sub_fade, 1.0, 1.0/FADE_TIME);
		} else {
			ctx->sub_fade = approach(ctx->sub_fade, 0.0, 1.0/FADE_TIME);
		}
	}

	m->drawdata[0] = SCREEN_W/2 - 50;
	m->drawdata[1] = (SCREEN_W - 100)*1.75;
	m->drawdata[2] += (20*m->cursor - m->drawdata[2])/10.0;

	animate_menu_list_entries(m);
}

static void replayview_draw(MenuData *m) {
	ReplayviewContext *ctx = m->context;

	draw_options_menu_bg(m);
	draw_menu_title(m, "Replays");

	draw_menu_list(m, 50, 100, replayview_drawitem, SCREEN_H, NULL);

	if(ctx->submenu) {
		ctx->submenu->draw(ctx->submenu);
	}

	r_shader_standard();
}

static int replayview_cmp(const void *a, const void *b) {
	ReplayviewItemContext *actx = ((MenuEntry*)a)->arg;
	ReplayviewItemContext *bctx = ((MenuEntry*)b)->arg;

	Replay *arpy = actx->replay;
	Replay *brpy = bctx->replay;

	return dynarray_get(&brpy->stages, 0).start_time - dynarray_get(&arpy->stages, 0).start_time;
}

static int fill_replayview_menu(MenuData *m) {
	VFSDir *dir = vfs_dir_open("storage/replays");
	const char *filename;
	int rpys = 0;

	if(!dir) {
		log_warn("VFS error: %s", vfs_get_error());
		return -1;
	}

	char ext[5];
	snprintf(ext, 5, ".%s", REPLAY_EXTENSION);

	while((filename = vfs_dir_read(dir))) {
		if(!strendswith(filename, ext))
			continue;

		Replay *rpy = malloc(sizeof(Replay));
		if(!replay_load(rpy, filename, REPLAY_READ_META)) {
			free(rpy);
			continue;
		}

		ReplayviewItemContext *ictx = malloc(sizeof(ReplayviewItemContext));
		memset(ictx, 0, sizeof(ReplayviewItemContext));

		ictx->replay = rpy;
		ictx->replayname = strdup(filename);

		add_menu_entry(m, " ", replayview_run, ictx)->transition = /*rpy->numstages < 2 ? TransFadeBlack :*/ NULL;
		++rpys;
	}

	vfs_dir_close(dir);

	dynarray_qsort(&m->entries, replayview_cmp);

	return rpys;
}

static void replayview_menu_input(MenuData *m) {
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

static void replayview_free(MenuData *m) {
	if(m->context) {
		ReplayviewContext *ctx = m->context;

		free_menu(ctx->next_submenu);
		free_menu(ctx->submenu);
		free(m->context);
		m->context = NULL;
	}

	dynarray_foreach_elem(&m->entries, MenuEntry *e, {
		if(e->action == replayview_run) {
			replayview_freearg(e->arg);
		}
	});
}

MenuData* create_replayview_menu(void) {
	MenuData *m = alloc_menu();

	m->logic = replayview_logic;
	m->input = replayview_menu_input;
	m->draw = replayview_draw;
	m->end = replayview_free;
	m->transition = TransFadeBlack;

	ReplayviewContext *ctx = malloc(sizeof(ReplayviewContext));
	memset(ctx, 0, sizeof(ReplayviewContext));
	ctx->sub_fade = 1.0;

	m->context = ctx;
	m->flags = MF_Abortable;

	int r = fill_replayview_menu(m);

	if(!r) {
		add_menu_entry(m, "No replays available. Play the game and record some!", menu_action_close, NULL);
	} else if(r < 0) {
		add_menu_entry(m, "There was a problem getting the replay list :(", menu_action_close, NULL);
	} else {
		add_menu_separator(m);
		add_menu_entry(m, "Back", menu_action_close, NULL);
	}

	return m;
}
