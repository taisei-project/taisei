/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "util.h"
#include "stage.h"
#include "global.h"
#include "video.h"
#include "resource/bgm.h"
#include "replay/demoplayer.h"
#include "replay/stage.h"
#include "replay/state.h"
#include "replay/struct.h"
#include "config.h"
#include "player.h"
#include "menu/ingamemenu.h"
#include "menu/gameovermenu.h"
#include "audio/audio.h"
#include "log.h"
#include "stagetext.h"
#include "stagedraw.h"
#include "stageobjects.h"
#include "eventloop/eventloop.h"
#include "common_tasks.h"
#include "stageinfo.h"
#include "dynstage.h"

typedef struct StageFrameState {
	StageInfo *stage;
	ResourceGroup *rg;
	CallChain cc;
	CoSched sched;
	Replay *quicksave;
	bool quicksave_is_automatic;
	bool quickload_requested;
	bool was_skipping;
	uint32_t dynstage_generation;
	int transition_delay;
	int desync_check_freq;
	uint16_t last_replay_fps;
	float view_shake;
	int bgm_start_time;
	double bgm_start_pos;
} StageFrameState;

static StageFrameState *_current_stage_state;  // TODO remove this shitty hack

#define BGM_FADE_LONG (2.0 * FADE_TIME / (double)FPS)
#define BGM_FADE_SHORT (FADE_TIME / (double)FPS)

static inline bool is_quickloading(StageFrameState *fstate) {
	return fstate->quicksave && fstate->quicksave == global.replay.input.replay;
}

static inline bool should_skip_frame(StageFrameState *fstate) {
	return
		is_quickloading(fstate) || (
			global.replay.input.replay &&
			global.replay.input.play.skip_frames > 0
		);
}

bool stage_is_demo_mode(void) {
	return global.replay.input.replay && global.replay.input.play.demo_mode;
}

static void sync_bgm(StageFrameState *fstate) {
	if(stage_is_demo_mode()) {
		return;
	}

	double t = fstate->bgm_start_pos + (global.frames - fstate->bgm_start_time) / (double)FPS;
	audio_bgm_seek_realtime(t);
}

static void recover_after_skip(StageFrameState *fstate) {
	if(fstate->was_skipping) {
		fstate->was_skipping = false;
		sync_bgm(fstate);
		audio_sfx_set_enabled(true);
	}
}

#ifdef HAVE_SKIP_MODE

// TODO refactor this unholy mess
// somehow reconcile with what's implemented for quickloads and demos.

static struct {
	const char *skip_to_bookmark;
	bool skip_to_dialog;
	bool was_skip_mode;
} skip_state;

void _stage_bookmark(const char *name) {
	log_debug("Bookmark [%s] reached at %i", name, global.frames);

	if(skip_state.skip_to_bookmark && !strcmp(skip_state.skip_to_bookmark, name)) {
		skip_state.skip_to_bookmark = NULL;
		global.plr.iddqd = false;
	}
}

DEFINE_EXTERN_TASK(stage_bookmark) {
	_stage_bookmark(ARGS.name);
}

bool stage_is_skip_mode(void) {
	return skip_state.skip_to_bookmark || skip_state.skip_to_dialog;
}

static void skipstate_init(void) {
	skip_state.skip_to_dialog = env_get_int("TAISEI_SKIP_TO_DIALOG", 0);
	skip_state.skip_to_bookmark = env_get_string_nonempty("TAISEI_SKIP_TO_BOOKMARK", NULL);
}

static LogicFrameAction skipstate_handle_frame(void) {
	if(skip_state.skip_to_dialog && dialog_is_active(global.dialog)) {
		skip_state.skip_to_dialog = false;
		global.plr.iddqd = false;
	}

	bool skip_mode = stage_is_skip_mode();

	if(!skip_mode && skip_state.was_skip_mode) {
		sync_bgm(_current_stage_state);
	}

	skip_state.was_skip_mode = skip_mode;

	if(skip_mode) {
		return LFRAME_SKIP_ALWAYS;
	}

	if(gamekeypressed(KEY_SKIP)) {
		return LFRAME_SKIP;
	}

	return LFRAME_WAIT;
}

static void skipstate_shutdown(void) {
	memset(&skip_state, 0, sizeof(skip_state));
}

#else

INLINE LogicFrameAction skipstate_handle_frame(void) { return LFRAME_WAIT; }
INLINE void skipstate_init(void) { }
INLINE void skipstate_shutdown(void) { }

#endif

static void stage_start(StageInfo *stage) {
	global.frames = 0;
	global.gameover = 0;
	global.voltage_threshold = 0;

	player_stage_pre_init(&global.plr);

	stats_stage_reset(&global.plr.stats);

	if(stage->type == STAGE_SPELL) {
		global.is_practice_mode = true;
		global.plr.lives = 0;
		global.plr.bombs = 0;
	} else if(global.is_practice_mode) {
		global.plr.lives = PLR_STGPRACTICE_LIVES;
		global.plr.bombs = PLR_STGPRACTICE_BOMBS;
	}

	if(global.is_practice_mode) {
		global.plr.power_stored = config_get_int(CONFIG_PRACTICE_POWER);
	}

	global.plr.power_stored = iclamp(global.plr.power_stored, 0, PLR_MAX_POWER_STORED);

	reset_all_sfx();
}

static bool ingame_menu_interrupts_bgm(void) {
	return global.stage->type != STAGE_SPELL;
}

typedef struct IngameMenuContext {
	CallChain next;
	BGM *saved_bgm;
	double saved_bgm_pos;
	bool bgm_interrupted;
} IngameMenuContext;

static void setup_ingame_menu_bgm(IngameMenuContext *ctx, BGM *bgm) {
	if(ingame_menu_interrupts_bgm()) {
		ctx->bgm_interrupted = true;

		if(bgm) {
			ctx->saved_bgm = audio_bgm_current();
			ctx->saved_bgm_pos = audio_bgm_tell();
			audio_bgm_play(bgm, true, 0, 1);
		} else {
			audio_bgm_pause();
		}
	}
}

static void resume_bgm(IngameMenuContext *ctx) {
	if(ctx->bgm_interrupted) {
		if(ctx->saved_bgm) {
			audio_bgm_play(ctx->saved_bgm, true, ctx->saved_bgm_pos, 0.5);
			ctx->saved_bgm = NULL;
		} else {
			audio_bgm_resume();
		}

		ctx->bgm_interrupted = false;
	}
}

static void stage_leave_ingame_menu(CallChainResult ccr) {
	IngameMenuContext *ctx = ccr.ctx;
	MenuData *m = ccr.result;

	if(m->state != MS_Dead) {
		return;
	}

	if(global.gameover > 0) {
		stop_all_sfx();

		if(ctx->bgm_interrupted) {
			audio_bgm_stop(global.gameover == GAMEOVER_RESTART ? BGM_FADE_SHORT : BGM_FADE_LONG);
		}
	} else {
		resume_bgm(ctx);
		events_emit(TE_GAME_PAUSE_STATE_CHANGED, false, NULL, NULL);
	}

	resume_all_sfx();

	run_call_chain(&ctx->next, NULL);
	mem_free(ctx);
}

static void stage_enter_ingame_menu(MenuData *m, BGM *bgm, CallChain next) {
	events_emit(TE_GAME_PAUSE_STATE_CHANGED, true, NULL, NULL);
	auto ctx = ALLOC(IngameMenuContext, { .next = next });
	setup_ingame_menu_bgm(ctx, bgm);
	pause_all_sfx();
	enter_menu(m, CALLCHAIN(stage_leave_ingame_menu, ctx));
}

void stage_pause(void) {
	if(global.gameover == GAMEOVER_TRANSITIONING || stage_is_skip_mode()) {
		return;
	}

	MenuData *m;

	if(global.replay.input.replay) {
		m = create_ingame_menu_replay();
	} else {
		m = create_ingame_menu();
	}

	stage_enter_ingame_menu(m, NULL, NO_CALLCHAIN);
}

void stage_gameover(void) {
	if(global.stage->type == STAGE_SPELL && config_get_int(CONFIG_SPELLSTAGE_AUTORESTART)) {
		global.gameover = GAMEOVER_RESTART;
		return;
	}

	BGM *bgm = NULL;

	if(ingame_menu_interrupts_bgm()) {
		bgm = res_bgm("gameover");
		progress_unlock_bgm("gameover");
	}

	stage_enter_ingame_menu(create_gameover_menu(), bgm, NO_CALLCHAIN);
}

static bool stage_input_common(SDL_Event *event, void *arg) {
	TaiseiEvent type = TAISEI_EVENT(event->type);
	int32_t code = event->user.code;

	switch(type) {
		case TE_GAME_KEY_DOWN:
			switch(code) {
				case KEY_STOP:
					stage_finish(GAMEOVER_DEFEAT);
					return true;

				case KEY_RESTART:
					stage_finish(GAMEOVER_RESTART);
					return true;
			}

			break;

		case TE_GAME_PAUSE:
			stage_pause();
			break;

		default:
			break;
	}

	return false;
}

static bool stage_input_key_filter(KeyIndex key, bool is_release) {
	if(key == KEY_HAHAIWIN) {
		IF_DEBUG(
			if(!is_release) {
				stage_finish(GAMEOVER_WIN);
			}
		);

		return false;
	}

	IF_NOT_DEBUG(
		if(
			key == KEY_IDDQD ||
			key == KEY_POWERUP ||
			key == KEY_POWERDOWN
		) {
			return false;
		}
	);

	if(stage_is_cleared()) {
		if(key == KEY_SHOT) {
			if(
				global.gameover == GAMEOVER_SCORESCREEN &&
				global.frames - global.gameover_time > GAMEOVER_SCORE_DELAY * 2
			) {
				if(!is_release) {
					stage_finish(GAMEOVER_WIN);
				}
			}
		}

		if(key == KEY_BOMB || key == KEY_SPECIAL) {
			return false;
		}
	}

	return true;
}

attr_nonnull_all
static Replay *create_quicksave_replay(ReplayStage *rstg_src) {
	ReplayStage *rstg = memdup(rstg_src, sizeof(*rstg));
	rstg->num_events = 0;
	memset(&rstg->events, 0, sizeof(rstg->events));

	dynarray_ensure_capacity(&rstg->events, rstg_src->events.num_elements + 1);
	dynarray_set_elements(&rstg->events, rstg_src->events.num_elements, rstg_src->events.data);

	replay_stage_event(rstg, global.frames, EV_RESUME, 0);
	replay_stage_update_final_stats(rstg, &global.plr.stats);

	auto rpy = ALLOC(Replay);
	rpy->stages.num_elements = rpy->stages.capacity = 1;
	rpy->stages.data = rstg;

	log_info("Created quicksave replay on frame %i", global.frames);
	return rpy;
}

static inline bool is_quicksave_allowed(void) {
#ifndef DEBUG
	if(!global.is_practice_mode) {
		return false;
	}
#endif

	if(global.gameover != GAMEOVER_NONE) {
		return false;
	}

	return true;
}

static void stage_do_quicksave(StageFrameState *fstate, bool isauto) {
	if(isauto && fstate->quicksave && !fstate->quicksave_is_automatic) {
		// Do not overwrite a manual quicksave with an auto quicksave
		return;
	}

	if(fstate->quicksave) {
		replay_reset(fstate->quicksave);
		mem_free(fstate->quicksave);
	}

	fstate->quicksave = create_quicksave_replay(global.replay.output.stage);
	fstate->quicksave_is_automatic = isauto;
}

static void stage_do_quickload(StageFrameState *fstate) {
	if(fstate->quicksave) {
		fstate->quickload_requested = true;
	} else {
		log_info("No active quicksave");
	}
}

static bool stage_input_handler_gameplay(SDL_Event *event, void *arg) {
	StageFrameState *fstate = NOT_NULL(arg);

	if(event->type == SDL_KEYDOWN && !event->key.repeat && is_quicksave_allowed()) {
		if(event->key.keysym.scancode == config_get_int(CONFIG_KEY_QUICKSAVE)) {
			stage_do_quicksave(fstate, false);
		} else if(event->key.keysym.scancode == config_get_int(CONFIG_KEY_QUICKLOAD)) {
			stage_do_quickload(fstate);
		}

		return false;
	}

	TaiseiEvent type = TAISEI_EVENT(event->type);
	int32_t code = event->user.code;

	if(stage_input_common(event, arg)) {
		return false;
	}

	if(
		(type == TE_GAME_KEY_DOWN || type == TE_GAME_KEY_UP) &&
		code == KEY_SHOT &&
		config_get_int(CONFIG_SHOT_INVERTED)
	) {
		type = type == TE_GAME_KEY_DOWN ? TE_GAME_KEY_UP : TE_GAME_KEY_DOWN;
	}

	ReplayState *rpy = &global.replay.output;

	switch(type) {
		case TE_GAME_KEY_DOWN:
			if(stage_input_key_filter(code, false)) {
				player_event(&global.plr, NULL, rpy, EV_PRESS, code);
			}
			break;

		case TE_GAME_KEY_UP:
			if(stage_input_key_filter(code, true)) {
				player_event(&global.plr, NULL, rpy, EV_RELEASE, code);
			}
			break;

		default: break;
	}

	return false;
}

static bool stage_input_handler_replay(SDL_Event *event, void *arg) {
	stage_input_common(event, arg);
	return false;
}

static bool stage_input_handler_demo(SDL_Event *event, void *arg) {
	if(event->type == SDL_KEYDOWN && !event->key.repeat) {
		goto exit;
	}

	switch(TAISEI_EVENT(event->type)) {
		case TE_GAME_KEY_DOWN:
		case TE_GAMEPAD_BUTTON_DOWN:
		case TE_GAMEPAD_AXIS_DIGITAL:
		exit:
			stage_finish(GAMEOVER_ABORT);
	}

	return false;
}

struct replay_event_arg {
	ReplayState *st;
	ReplayEvent *resume_event;
};

static void handle_replay_event(ReplayEvent *e, void *arg) {
	struct replay_event_arg *a = NOT_NULL(arg);

	if(UNLIKELY(a->resume_event != NULL)) {
		log_warn(
			"Got replay event [%i:%02x:%04x] after resume event in the same frame, ignoring",
			e->frame, e->type, e->value
		);
		return;
	}

	switch(e->type) {
		case EV_OVER:
			global.gameover = GAMEOVER_DEFEAT;
			break;

		case EV_RESUME:
			a->resume_event = e;
			break;

		default:
			player_event(&global.plr, a->st, &global.replay.output, e->type, e->value);
			break;
	}
}

static void leave_replay_mode(StageFrameState *fstate, ReplayState *rp_in) {
	replay_state_deinit(rp_in);
}

static void replay_input(StageFrameState *fstate) {
	if(!should_skip_frame(fstate)) {
		events_poll((EventHandler[]){
			{ .proc =
				stage_is_demo_mode()
					? stage_input_handler_demo
					: stage_input_handler_replay
			},
			{ NULL }
		}, EFLAG_GAME);
	}

	ReplayState *rp_in = &global.replay.input;

	if(UNLIKELY(rp_in->mode == REPLAY_NONE)) {
		return;
	}

	struct replay_event_arg a = { .st = rp_in };
	replay_state_play_advance(rp_in, global.frames, handle_replay_event, &a);
	player_applymovement(&global.plr);

	if(a.resume_event) {
		leave_replay_mode(fstate, rp_in);
	}
}

static void display_bgm_title(void) {
	BGM *bgm = audio_bgm_current();
	const char *title = bgm ? bgm_get_title(bgm) : NULL;

	if(title) {
		char txt[strlen(title) + 6];
		snprintf(txt, sizeof(txt), "BGM: %s", title);
		stagetext_add(txt, VIEWPORT_W-15 + I * (VIEWPORT_H-20), ALIGN_RIGHT, res_font("standard"), RGB(1, 1, 1), 30, 180, 35, 35);
	}
}

static bool stage_handle_bgm_change(SDL_Event *evt, void *a) {
	StageFrameState *fstate = NOT_NULL(a);
	fstate->bgm_start_time = global.frames;
	fstate->bgm_start_pos = audio_bgm_tell();

	if(dialog_is_active(global.dialog)) {
		INVOKE_TASK_WHEN(&global.dialog->events.fadeout_began, common_call_func, display_bgm_title);
	} else {
		display_bgm_title();
	}

	return false;
}

static void stage_input(StageFrameState *fstate) {
	if(stage_is_skip_mode()) {
		events_poll((EventHandler[]){
			{
				.proc = stage_handle_bgm_change,
				.event_type = MAKE_TAISEI_EVENT(TE_AUDIO_BGM_STARTED),
				.arg = fstate,
			},
			{NULL}
		}, EFLAG_NOPUMP);
	} else {
		events_poll((EventHandler[]){
			{ .proc = stage_input_handler_gameplay, .arg = fstate },
			{
				.proc = stage_handle_bgm_change,
				.event_type = MAKE_TAISEI_EVENT(TE_AUDIO_BGM_STARTED),
				.arg = fstate,
			},
			{NULL}
		}, EFLAG_GAME);
	}

	player_fix_input(&global.plr, &global.replay.output);
	player_applymovement(&global.plr);
}

void stage_clear_hazards_predicate(bool (*predicate)(EntityInterface *ent, void *arg), void *arg, ClearHazardsFlags flags) {
	bool force = flags & CLEAR_HAZARDS_FORCE;

	if(flags & CLEAR_HAZARDS_BULLETS) {
		for(Projectile *p = global.projs.first, *next; p; p = next) {
			next = p->next;

			if(!force && !projectile_is_clearable(p)) {
				continue;
			}

			if(!predicate || predicate(&p->ent, arg)) {
				clear_projectile(p, flags);
			}
		}
	}

	if(flags & CLEAR_HAZARDS_LASERS) {
		for(Laser *l = global.lasers.first, *next; l; l = next) {
			next = l->next;

			if(!force && !laser_is_clearable(l)) {
				continue;
			}

			if(!predicate || predicate(&l->ent, arg)) {
				clear_laser(l, flags);
			}
		}
	}
}

void stage_clear_hazards(ClearHazardsFlags flags) {
	stage_clear_hazards_predicate(NULL, NULL, flags);
}

static bool proximity_predicate(EntityInterface *ent, void *varg) {
	Circle *area = varg;

	switch(ent->type) {
		case ENT_TYPE_ID(Projectile): {
			Projectile *p = ENT_CAST(ent, Projectile);
			return cabs(p->pos - area->origin) < area->radius;
		}

		case ENT_TYPE_ID(Laser): {
			Laser *l = ENT_CAST(ent, Laser);
			return laser_intersects_circle(l, *area);
		}

		default: UNREACHABLE;
	}
}

static bool ellipse_predicate(EntityInterface *ent, void *varg) {
	Ellipse *e = varg;

	switch(ent->type) {
		case ENT_TYPE_ID(Projectile): {
			Projectile *p = ENT_CAST(ent, Projectile);
			return point_in_ellipse(p->pos, *e);
		}

		case ENT_TYPE_ID(Laser): {
			Laser *l = ENT_CAST(ent, Laser);
			return laser_intersects_ellipse(l, *e);
		}

		default: UNREACHABLE;
	}
}

void stage_clear_hazards_at(cmplx origin, double radius, ClearHazardsFlags flags) {
	Circle area = { origin, radius };
	stage_clear_hazards_predicate(proximity_predicate, &area, flags);
}

void stage_clear_hazards_in_ellipse(Ellipse e, ClearHazardsFlags flags) {
	stage_clear_hazards_predicate(ellipse_predicate, &e, flags);
}

TASK(clear_dialog) {
	assert(global.dialog != NULL);
	// dialog_deinit() should've been called by dialog_end() at this point
	global.dialog = NULL;
}

void stage_begin_dialog(Dialog *d) {
	assert(global.dialog == NULL);
	global.dialog = d;
	dialog_init(d);
	INVOKE_TASK_WHEN(&d->events.fadeout_ended, clear_dialog);
}

static void stage_free(void) {
	delete_enemies(&global.enemies);
	delete_items();
	delete_lasers();

	delete_projectiles(&global.projs);
	delete_projectiles(&global.particles);

	if(global.dialog) {
		dialog_deinit(global.dialog);
		global.dialog = NULL;
	}

	if(global.boss) {
		free_boss(global.boss);
		global.boss = NULL;
	}

	lasers_shutdown();
	projectiles_free();
	stagetext_free();
}

static void stage_finalize(CallChainResult ccr) {
	global.gameover = (intptr_t)ccr.ctx;
}

void stage_finish(int gameover) {
	if(global.gameover == GAMEOVER_TRANSITIONING) {
		return;
	}

	int prev_gameover = global.gameover;
	global.gameover_time = global.frames;

	if(gameover == GAMEOVER_SCORESCREEN) {
		global.gameover = GAMEOVER_SCORESCREEN;
	} else {
		global.gameover = GAMEOVER_TRANSITIONING;
		CallChain cc = CALLCHAIN(stage_finalize, (void*)(intptr_t)gameover);
		set_transition(TransFadeBlack, FADE_TIME, FADE_TIME*2, cc);

		if(!stage_is_demo_mode()) {
			audio_bgm_stop(BGM_FADE_LONG);
		}
	}

	if(
		global.replay.input.replay == NULL &&
		prev_gameover != GAMEOVER_SCORESCREEN &&
		(gameover == GAMEOVER_SCORESCREEN || gameover == GAMEOVER_WIN)
	) {
		StageProgress *p = NOT_NULL(stageinfo_get_progress(global.stage, global.diff, true));
		progress_register_stage_cleared(p, global.plr.mode);
	}

	if(gameover == GAMEOVER_SCORESCREEN && stage_is_skip_mode()) {
		// don't get stuck in an infinite loop
		taisei_quit();
	}
}

static void stage_preload(StageInfo *si, ResourceGroup *rg) {
	difficulty_preload(rg);
	projectiles_preload(rg);
	player_preload(rg);
	items_preload(rg);
	boss_preload(rg);
	laserdraw_preload(rg);
	enemies_preload(rg);

	if(si->type != STAGE_SPELL) {
		res_group_preload(rg, RES_BGM, RESF_OPTIONAL, "gameover", NULL);
		dialog_preload(rg);
	}

	si->procs->preload(rg);
}

static void display_stage_title(StageInfo *info) {
	if(stage_is_demo_mode()) {
		return;
	}

	stagetext_add(info->title,    VIEWPORT_W/2 + I * (VIEWPORT_H/2-40), ALIGN_CENTER, res_font("big"), RGB(1, 1, 1), 50, 85, 35, 35);
	stagetext_add(info->subtitle, VIEWPORT_W/2 + I * (VIEWPORT_H/2),    ALIGN_CENTER, res_font("standard"), RGB(1, 1, 1), 60, 85, 35, 35);
}

TASK(start_bgm, { BGM *bgm; }) {
	_current_stage_state->bgm_start_time = global.frames + 1;
	audio_bgm_play(ARGS.bgm, true, 0, 0);
}

void stage_start_bgm(const char *bgm) {
	if(stage_is_demo_mode()) {
		return;
	}

	INVOKE_TASK_DELAYED(1, start_bgm, res_bgm(bgm));
}

void stage_set_voltage_thresholds(uint easy, uint normal, uint hard, uint lunatic) {
	switch(global.diff) {
		case D_Easy:    global.voltage_threshold = easy;    return;
		case D_Normal:  global.voltage_threshold = normal;  return;
		case D_Hard:    global.voltage_threshold = hard;    return;
		case D_Lunatic: global.voltage_threshold = lunatic; return;
		default: UNREACHABLE;
	}
}

bool stage_is_cleared(void) {
	return global.gameover == GAMEOVER_SCORESCREEN || global.gameover == GAMEOVER_TRANSITIONING;
}

static void stage_update_fps(StageFrameState *fstate) {
	if(global.replay.output.replay) {
		uint16_t replay_fps = (uint16_t)rint(global.fps.logic.fps);

		if(replay_fps != fstate->last_replay_fps) {
			replay_stage_event(global.replay.output.stage, global.frames, EV_FPS, replay_fps);
			fstate->last_replay_fps = replay_fps;
		}
	}
}

static void stage_give_clear_bonus(const StageInfo *stage, StageClearBonus *bonus) {
	memset(bonus, 0, sizeof(*bonus));

	// FIXME: this is clunky...
	if(!global.is_practice_mode && stage->type == STAGE_STORY) {
		StageInfo *next = stageinfo_get_by_id(stage->id + 1);

		if(next == NULL || next->type != STAGE_STORY) {
			bonus->all_clear.base = global.plr.point_item_value * 100 + global.plr.points / 10;
			bonus->all_clear.diff_multiplier = difficulty_value(1.0, 1.1, 1.3, 1.6);
			bonus->all_clear.diff_bonus = bonus->all_clear.base * (bonus->all_clear.diff_multiplier - 1.0);
		}
	}

	if(stage->type == STAGE_STORY) {
		bonus->base = stage->id * 1000000;
	}

	bonus->voltage = imax(0, (int)global.plr.voltage - (int)global.voltage_threshold) * (global.plr.point_item_value / 25);
	bonus->lives = global.plr.lives * global.plr.point_item_value * 5;

	// TODO: maybe a difficulty multiplier?

	bonus->total = (
		bonus->base +
		bonus->voltage +
		bonus->lives +
		bonus->graze +
		bonus->all_clear.base +
		bonus->all_clear.diff_bonus +
	0);
	player_add_points(&global.plr, bonus->total, global.plr.pos);
}

static void stage_replay_sync(StageFrameState *fstate) {
	uint16_t desync_check = (rng_u64() ^ global.plr.points) & 0xFFFF;
	ReplaySyncStatus rpsync = replay_state_check_desync(&global.replay.input, global.frames, desync_check);

	if(
		global.replay.output.stage &&
		fstate->desync_check_freq > 0 &&
		!(global.frames % fstate->desync_check_freq)
	) {
		replay_stage_event(global.replay.output.stage, global.frames, EV_CHECK_DESYNC, desync_check);
	}

	if(rpsync == REPLAY_SYNC_FAIL) {
		if(
			global.is_replay_verification &&
			!global.replay.output.stage
		) {
			exit(1);
		}

		if(fstate->quicksave && fstate->quicksave == global.replay.input.replay) {
			log_warn("Quicksave replay desynced; resuming prematurely!");
			leave_replay_mode(fstate, &global.replay.input);
		}
	}
}

static LogicFrameAction stage_logic_frame(void *arg) {
	StageFrameState *fstate = arg;
	StageInfo *stage = fstate->stage;

	stage_update_fps(fstate);

	if(stage_is_skip_mode()) {
		global.plr.iddqd = true;
	}

	fapproach_p(&fstate->view_shake, 0, 1);
	fapproach_asymptotic_p(&fstate->view_shake, 0, 0.05, 1e-2);

	if(
		global.replay.input.replay == NULL &&
		fstate->dynstage_generation != dynstage_get_generation()
	) {
		log_info("Stages library updated, attempting to hot-reload");
		stage_do_quicksave(fstate, true);   // no-op if user has a manual save
		stage_do_quickload(fstate);
	}

	if(global.gameover == GAMEOVER_TRANSITIONING) {
		// Usually stage_comain will do this
		events_poll(NULL, 0);
	} else {
		cosched_run_tasks(&fstate->sched);
		update_all_sfx();
		stage_replay_sync(fstate);

		global.frames++;

		/*
		 * TODO: Investigate why/if any of this needs to happen after global.frames++,
		 *       and possibly fix that.
		 */

		stagetext_update();

		if(global.replay.input.replay && global.gameover != GAMEOVER_TRANSITIONING) {
			ReplayStage *rstg = global.replay.input.stage;
			ReplayEvent *last_event = dynarray_get_ptr(
				&rstg->events, rstg->events.num_elements - 1);

			if(
				global.frames == last_event->frame - FADE_TIME &&
				last_event->type != EV_RESUME
			) {
				stage_finish(GAMEOVER_DEFEAT);
			}
		}

		if(
			global.gameover == GAMEOVER_SCORESCREEN &&
			global.frames - global.gameover_time == GAMEOVER_SCORE_DELAY
		) {
			StageClearBonus b;
			stage_give_clear_bonus(stage, &b);
			stage_display_clear_screen(&b);
		}

		if(stage->type == STAGE_SPELL && !global.boss && !fstate->transition_delay) {
			fstate->transition_delay = 120;
		}
	}

	if(fstate->transition_delay) {
		if(!--fstate->transition_delay) {
			stage_finish(GAMEOVER_WIN);
		}
	} else if(!(should_skip_frame(fstate) && stage_is_demo_mode())) {
		update_transition();
	}

	if(global.replay.input.replay == NULL) {
		StageProgress *p = NOT_NULL(stageinfo_get_progress(fstate->stage, global.diff, true));
		progress_register_hiscore(p, global.plr.mode, global.plr.points);
	}

	if(fstate->quickload_requested) {
		log_info("Quickload initiated");
		return LFRAME_STOP;
	}

	if(global.gameover > 0) {
		return LFRAME_STOP;
	}

	if(should_skip_frame(fstate)) {
		fstate->was_skipping = true;
		return LFRAME_SKIP_ALWAYS;
	}

	recover_after_skip(fstate);


	LogicFrameAction skipmode = skipstate_handle_frame();
	if(skipmode != LFRAME_WAIT) {
		return skipmode;
	}

	if(global.replay.input.replay && gamekeypressed(KEY_SKIP)) {
		return LFRAME_SKIP;
	}

	return LFRAME_WAIT;
}

static RenderFrameAction stage_render_frame(void *arg) {
	StageFrameState *fstate = arg;
	StageInfo *stage = fstate->stage;

	if(stage_is_skip_mode()) {
		return RFRAME_DROP;
	}

	rng_lock(&global.rand_game);
	rng_make_active(&global.rand_visual);
	BEGIN_DRAW_CODE();
	stage_draw_scene(stage);
	END_DRAW_CODE();
	rng_unlock(&global.rand_game);
	rng_make_active(&global.rand_game);
	draw_transition();

	return RFRAME_SWAP;
}

static void stage_end_loop(void *ctx);

static void stage_stub_proc(void) { }
static void stage_preload_stub_proc(ResourceGroup *rg) { }

TASK(stage_comain, { StageFrameState *fstate; }) {
	StageFrameState *fstate = ARGS.fstate;
	StageInfo *stage = fstate->stage;

	stage->procs->begin();
	player_stage_post_init(&global.plr);

	if(global.stage->type != STAGE_SPELL) {
		display_stage_title(stage);
	}

	for(;;YIELD) {
		if(global.replay.input.replay != NULL) {
			replay_input(fstate);
		} else {
			stage_input(fstate);
		}

		process_boss(&global.boss);
		process_enemies(&global.enemies);
		process_projectiles(&global.projs, true);
		process_items();
		process_lasers();
		process_projectiles(&global.particles, false);

		if(global.dialog) {
			dialog_update(global.dialog);

			if((global.plr.inputflags & INFLAG_SKIP) && dialog_is_active(global.dialog)) {
				dialog_page(global.dialog);
			}
		}

		if(stage_is_skip_mode()) {
			if(dialog_is_active(global.dialog)) {
				dialog_page(global.dialog);
			}

			if(global.boss) {
				ent_damage(&global.boss->ent, &(DamageInfo) { 400, DMG_PLAYER_SHOT } );
			}
		}
	}
}

static void _stage_enter(
	StageInfo *stage, ResourceGroup *rg, CallChain next, Replay *quickload, bool quicksave_is_automatic
) {
	assert(stage);
	assert(stage->procs);

	if(global.gameover == GAMEOVER_WIN) {
		global.gameover = 0;
	} else if(global.gameover) {
		run_call_chain(&next, NULL);
		return;
	}

	uint32_t dynstage_generation = dynstage_get_generation();
	stageinfo_reload();

	#define STUB_PROC(proc, stub) do {\
		if(!stage->procs->proc) { \
			stage->procs->proc = stub; \
			log_debug(#proc " proc is missing"); \
		} \
	} while(0)

	static const ShaderRule shader_rules_stub[1] = { NULL };

	STUB_PROC(preload, stage_preload_stub_proc);
	STUB_PROC(begin, stage_stub_proc);
	STUB_PROC(end, stage_stub_proc);
	STUB_PROC(draw, stage_stub_proc);
	STUB_PROC(shader_rules, (ShaderRule*)shader_rules_stub);

	if(quickload) {
		assert(global.replay.input.stage == NULL);
		ReplayStage *qload_stage = dynarray_get_ptr(&quickload->stages, 0);
		assert(qload_stage->stage == stage->id);
		replay_state_init_play(&global.replay.input, quickload, qload_stage);
	}

	// I really want to separate all of the game state from the global struct sometime
	global.stage = stage;

	ent_init();
	stage_objpools_init();
	stage_draw_preload(rg);
	stage_preload(stage, rg);
	stage_draw_init();
	lasers_init();

	rng_make_active(&global.rand_game);
	stage_start(stage);

	uint64_t start_time, seed;

	if(global.replay.input.replay) {
		ReplayStage *rstg = NOT_NULL(global.replay.input.stage);
		assert(stageinfo_get_by_id(rstg->stage) == stage);
		assert(!rstg->skip_frames || !quickload);

		start_time = rstg->start_time;
		seed = rstg->rng_seed;
		global.diff = rstg->diff;

		log_debug("REPLAY_PLAY mode: %d events, stage: \"%s\"", rstg->events.num_elements, stage->title);
	} else {
		start_time = (uint64_t)time(0);
		seed = makeseed();

		StageProgress *p = NOT_NULL(stageinfo_get_progress(stage, global.diff, true));
		progress_register_stage_played(p, global.plr.mode);
	}

	rng_seed(&global.rand_game, seed);

	if(global.replay.input.replay) {
		player_init(&global.plr);
		replay_stage_sync_player_state(global.replay.input.stage, &global.plr);
	}

	if(global.replay.output.replay) {
		global.replay.output.stage = replay_stage_new(
			global.replay.output.replay,
			stage,
			start_time,
			seed,
			global.diff,
			&global.plr
		);
		player_init(&global.plr);
		replay_stage_sync_player_state(global.replay.output.stage, &global.plr);
	}

	plrmode_preload(global.plr.mode, rg);

	res_purge();

	auto fstate = ALLOC(StageFrameState, {
		.stage = stage,
		.cc = next,
		.quicksave = quickload,
		.quicksave_is_automatic = quicksave_is_automatic,
		.desync_check_freq = env_get("TAISEI_REPLAY_DESYNC_CHECK_FREQUENCY", FPS * 5),
		.dynstage_generation = dynstage_generation,
		.rg = rg,
	});

	cosched_init(&fstate->sched);
	_current_stage_state = fstate;

	skipstate_init();

	if(should_skip_frame(fstate)) {
		audio_sfx_set_enabled(false);
	}

	if(!is_quickloading(fstate)) {
		demoplayer_suspend();
	}

	SCHED_INVOKE_TASK(&fstate->sched, stage_comain, fstate);
	eventloop_enter(fstate, stage_logic_frame, stage_render_frame, stage_end_loop, FPS);
}

void stage_enter(StageInfo *stage, ResourceGroup *rg, CallChain next) {
	_stage_enter(stage, rg, next, NULL, false);
}

void stage_end_loop(void *ctx) {
	StageFrameState *s = ctx;
	assert(s == _current_stage_state);

	recover_after_skip(s);

	Replay *quicksave = s->quicksave;
	bool quicksave_is_automatic = s->quicksave_is_automatic;
	bool is_quickload = s->quickload_requested;

	if(is_quickload) {
		assume(quicksave != NULL);
	}

	if(global.replay.output.replay) {
		if(is_quickload) {
			// rollback this stage, as we're about to replay it
			global.replay.output.replay->stages.num_elements--;
			replay_stage_destroy_events(global.replay.output.stage);
		} else {
			replay_stage_event(global.replay.output.stage, global.frames, EV_OVER, 0);
			global.replay.output.stage->plr_points_final = global.plr.points;

			if(global.gameover == GAMEOVER_WIN) {
				global.replay.output.stage->flags |= REPLAY_SFLAG_CLEAR;
			}

			replay_stage_update_final_stats(global.replay.output.stage, &global.plr.stats);
		}
	}

	if(quicksave && !is_quickload) {
		replay_reset(quicksave);
		mem_free(quicksave);
	}

	s->stage->procs->end();
	stage_draw_shutdown();
	cosched_finish(&s->sched);
	stage_free();
	player_free(&global.plr);
	ent_shutdown();
	rng_make_active(&global.rand_visual);
	stop_all_sfx();

	taisei_commit_persistent_data();
	skipstate_shutdown();

	if(taisei_quit_requested()) {
		global.gameover = GAMEOVER_ABORT;
	}

	_current_stage_state = NULL;

	StageInfo *stginfo = s->stage;
	CallChain cc = s->cc;
	ResourceGroup *rg = s->rg;

	mem_free(s);

	if(is_quickload) {
		_stage_enter(stginfo, rg, cc, quicksave, quicksave_is_automatic);
	} else {
		demoplayer_resume();
		run_call_chain(&cc, NULL);
	}
}

void stage_unlock_bgm(const char *bgm) {
	if(global.replay.input.replay == NULL && !global.plr.stats.total.continues_used) {
		progress_unlock_bgm(bgm);
	}
}

void stage_shake_view(float strength) {
	assume(strength >= 0);
	_current_stage_state->view_shake += strength;
}

float stage_get_view_shake_strength(void) {
	if(_current_stage_state) {
		return _current_stage_state->view_shake;
	}

	return 0;
}

void stage_load_quicksave(void) {
	stage_do_quickload(NOT_NULL(_current_stage_state));
}

CoSched *stage_get_sched(void) {
	return &NOT_NULL(_current_stage_state)->sched;
}
