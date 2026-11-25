/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "stage.h"

#include "audio/audio.h"
#include "common_tasks.h"  // IWYU pragma: keep
#include "config.h"
#include "dynstage.h"
#include "eventloop/eventloop.h"
#include "events.h"
#include "global.h"
#include "i18n/i18n.h"
#include "lasers/draw.h"
#include "log.h"
#include "memory/scratch.h"
#include "menu/gameovermenu.h"
#include "menu/ingamemenu.h"
#include "player.h"
#include "replay/demoplayer.h"
#include "replay/stage.h"
#include "replay/state.h"
#include "replay/struct.h"
#include "resource/bgm.h"
#include "stagedraw.h"
#include "stageinfo.h"
#include "stageobjects.h"
#include "stagetext.h"
#include "util/env.h"
#include "util/strbuf.h"
#include "watchdog.h"

struct StagePrivateState {
	GameSession *game;
	StageInstance *stage;
	ResourceGroup *rg;
	CallChain cc;
	CoSched sched;
	Replay *quicksave;
	uint32_t dynstage_generation;
	int transition_delay;
	int desync_check_freq;
	int bgm_start_time;
	double bgm_start_pos;
	uint16_t last_replay_fps;
	GameoverType finish_gameover;  // applied after the fade-out transition
	int gameover_time;
	bool quicksave_is_automatic;
	bool quickload_requested;
	bool was_skipping;
	bool paused;
};

static StagePrivateState *_current_stage_state;  // TODO remove this shitty hack

#define BGM_FADE_LONG (2.0 * FADE_TIME / (double)FPS)
#define BGM_FADE_SHORT (FADE_TIME / (double)FPS)

static inline bool is_quickloading(StagePrivateState *state) {
	return state->quicksave && state->quicksave == state->game->replay.input.replay;
}

static inline bool should_skip_frame(StagePrivateState *state) {
	GameSession *game = state->game;

	return
		is_quickloading(state) || (
			game->replay.input.replay &&
			game->replay.input.play.skip_frames > 0
		);
}

bool stage_is_demo_mode(void) {
	return game_is_demo_mode(&GAME);
}

static void sync_bgm(StagePrivateState *state) {
	if(game_is_demo_mode(state->game)) {
		return;
	}

	double t = state->bgm_start_pos + (global.frames - state->bgm_start_time) / (double)FPS;
	audio_bgm_seek_realtime(t);
}

static void recover_after_skip(StagePrivateState *state) {
	if(state->was_skipping) {
		state->was_skipping = false;
		sync_bgm(state);
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

static SceneAction skipstate_handle_frame(StagePrivateState *state) {
	if(skip_state.skip_to_dialog && dialog_is_active(state->stage->dialog)) {
		skip_state.skip_to_dialog = false;
		global.plr.iddqd = false;
	}

	bool skip_mode = stage_is_skip_mode();

	if(!skip_mode && skip_state.was_skip_mode) {
		sync_bgm(_current_stage_state);
	}

	skip_state.was_skip_mode = skip_mode;

	if(skip_mode) {
		return SCENE_ACTION_SKIP_ALWAYS;
	}

	if(gamekeypressed(KEY_SKIP)) {
		return SCENE_ACTION_SKIP;
	}

	return SCENE_ACTION_WAIT;
}

static void skipstate_shutdown(void) {
	skip_state = (typeof(skip_state)) {};
}

#else

INLINE SceneAction skipstate_handle_frame(StagePrivateState *state) { return SCENE_ACTION_WAIT; }
INLINE void skipstate_init(void) { }
INLINE void skipstate_shutdown(void) { }

#endif

static void stage_start(GameSession *session, StageInfo *stage_info) {
	global.frames = 0;
	player_stage_pre_init(&global.plr);

	stats_stage_reset(&global.plr.stats);

	if(stage_info->type == STAGE_SPELL) {
		session->is_practice_mode = true;
		global.plr.lives = 0;
		global.plr.bombs = 0;
	} else if(session->is_practice_mode) {
		global.plr.lives = PLR_STGPRACTICE_LIVES;
		global.plr.bombs = PLR_STGPRACTICE_BOMBS;
	}

	if(session->is_practice_mode) {
		global.plr.power_stored = config_get_int(CONFIG_PRACTICE_POWER);
	}

	global.plr.power_stored = clamp(global.plr.power_stored, 0, PLR_MAX_POWER_STORED);

	reset_all_sfx();
}

static bool ingame_menu_interrupts_bgm(void) {
	return global.stage->type != STAGE_SPELL;
}

static bool is_cleared(StagePrivateState *state) {
	StageInstance *stage = state->stage;

	return stage->gameover == GAMEOVER_SCORESCREEN || stage->gameover == GAMEOVER_TRANSITIONING;
}

static inline bool is_quicksave_allowed(StagePrivateState *state) {
#ifndef DEBUG
	if(!state->game->is_practice_mode) {
		return false;
	}
#endif

	if(state->stage->gameover != GAMEOVER_NONE) {
		return false;
	}

	return true;
}

typedef struct IngameMenuCallContext {
	StagePrivateState *state;
	CallChain next;
	BGM *saved_bgm;
	double saved_bgm_pos;
	bool bgm_interrupted;
} IngameMenuCallContext;

static void setup_ingame_menu_bgm(IngameMenuCallContext *ctx, BGM *bgm) {
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

static void resume_bgm(IngameMenuCallContext *ctx) {
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
	IngameMenuCallContext *ctx = ccr.ctx;
	MenuData *m = ccr.result;

	if(m->state != MS_Dead) {
		return;
	}

	if(ctx->state->stage->gameover > 0) {
		stop_all_sfx();

		if(ctx->bgm_interrupted) {
			audio_bgm_stop(ctx->state->stage->gameover == GAMEOVER_RESTART ? BGM_FADE_SHORT : BGM_FADE_LONG);
		}
	} else {
		resume_bgm(ctx);
		events_emit(TE_GAME_PAUSE_STATE_CHANGED, false, NULL, NULL);
	}

	resume_all_sfx();

	run_call_chain(&ctx->next, NULL);
	mem_free(ctx);
}

static void stage_enter_ingame_menu(StagePrivateState *state, MenuData *m, BGM *bgm, CallChain next) {
	events_emit(TE_GAME_PAUSE_STATE_CHANGED, true, NULL, NULL);
	auto ctx = ALLOC(IngameMenuCallContext, { .state = state, .next = next });
	setup_ingame_menu_bgm(ctx, bgm);
	pause_all_sfx();
	enter_menu(m, CALLCHAIN(stage_leave_ingame_menu, ctx));
}

static void stage_unpause(CallChainResult ccr) {
	StagePrivateState *state = ccr.ctx;
	state->paused = false;
}

static void stage_pause(StagePrivateState *state) {
	GameSession *game = state->game;

	if(state->paused) {
		log_debug("Pause request ignored, already paused");
		return;
	}

	if(state->stage->gameover == GAMEOVER_TRANSITIONING || stage_is_skip_mode()) {
		return;
	}

	MenuData *m;

	if(game->replay.input.replay) {
		m = create_ingame_menu_replay(game);
	} else {
		m = create_ingame_menu(game);
	}

	state->paused = true;
	stage_enter_ingame_menu(state, m, NULL, CALLCHAIN(stage_unpause, state));
}

static void stage_do_quickload(StagePrivateState *state);
static void stage_finish_internal(StagePrivateState *state, GameoverType gameover);

static void stage_continue_game(StagePrivateState *state) {
	GameSession *game = state->game;

	log_info("The game is being continued...");
	player_event(&global.plr, &game->replay.input, &game->replay.output, EV_CONTINUE, 0);
}

typedef struct GameoverMenuResultContext {
	StagePrivateState *state;
	GameoverMenuAction action;
} GameoverMenuResultContext;

static void gameover_menu_result(CallChainResult ccr) {
	GameoverMenuResultContext *ctx = ccr.ctx;
	StagePrivateState *state = ctx->state;
	StageInstance *stage = state->stage;
	auto action = ctx->action;
	mem_free(ctx);

	switch(action) {
		case GAMEOVERMENU_ACTION_QUIT:
		case GAMEOVERMENU_ACTION_DEFAULT:
			if(global.plr.stats.total.continues_used >= MAX_CONTINUES) {
				stage->gameover = GAMEOVER_DEFEAT;
			} else {
				stage->gameover = GAMEOVER_ABORT;
			}
			break;

		case GAMEOVERMENU_ACTION_CONTINUE:
			stage_continue_game(state);
			break;

		case GAMEOVERMENU_ACTION_RESTART:
			stage->gameover = GAMEOVER_RESTART;
			break;

		case GAMEOVERMENU_ACTION_QUICKLOAD:
			stage_do_quickload(state);
			break;

		default: UNREACHABLE;
	}
}

void stage_gameover(void) {
	StagePrivateState *state = STAGE.priv;

	if(global.stage->type == STAGE_SPELL && config_get_int(CONFIG_SPELLSTAGE_AUTORESTART)) {
		if(state->quicksave) {
			stage_do_quickload(state);
		} else {
			state->stage->gameover = GAMEOVER_RESTART;
		}
		return;
	}

	BGM *bgm = NULL;

	if(ingame_menu_interrupts_bgm()) {
		bgm = res_bgm("gameover");
		progress_unlock_bgm("gameover");
	}

	auto ctx = ALLOC(GameoverMenuResultContext, { .state = state });
	auto menu = create_gameover_menu(&(GameoverMenuParams) {
		.game = state->game,
		.output = &ctx->action,
		.quickload_shown = is_quicksave_allowed(state),
		.quickload_enabled = state->quicksave != NULL,
	});

	stage_enter_ingame_menu(state, menu, bgm, CALLCHAIN(gameover_menu_result, ctx));
}

static bool stage_input_common(SDL_Event *event, void *arg) {
	StagePrivateState *state = NOT_NULL(arg);
	TaiseiEvent type = TAISEI_EVENT(event->type);
	int32_t code = event->user.code;

	switch(type) {
		case TE_GAME_KEY_DOWN:
			switch(code) {
				case KEY_STOP:
					stage_finish_internal(state, GAMEOVER_DEFEAT);
					return true;

				case KEY_RESTART:
					stage_finish_internal(state, GAMEOVER_RESTART);
					return true;
			}

			break;

		case TE_GAME_PAUSE:
			stage_pause(state);
			break;

		default:
			break;
	}

	return false;
}

static bool stage_input_key_filter(StagePrivateState *state, KeyIndex key, bool is_release) {
	if(key == KEY_HAHAIWIN) {
		IF_DEBUG(
			if(!is_release) {
				stage_finish_internal(state, GAMEOVER_WIN);
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

	if(is_cleared(state)) {
		if(key == KEY_SHOT) {
			if(
				state->stage->gameover == GAMEOVER_SCORESCREEN &&
				global.frames - state->gameover_time > GAMEOVER_SCORE_DELAY * 2
			) {
				if(!is_release) {
					stage_finish_internal(state, GAMEOVER_WIN);
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
	rstg->events = (typeof(rstg->events)) {};

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

static void stage_do_quicksave(StagePrivateState *state, bool isauto) {
	if(isauto && state->quicksave && !state->quicksave_is_automatic) {
		// Do not overwrite a manual quicksave with an auto quicksave
		return;
	}

	if(state->quicksave) {
		replay_reset(state->quicksave);
		mem_free(state->quicksave);
	}

	state->quicksave = create_quicksave_replay(state->game->replay.output.stage);
	state->quicksave_is_automatic = isauto;
}

static void stage_do_quickload(StagePrivateState *state) {
	if(state->quicksave) {
		state->quickload_requested = true;
	} else {
		log_info("No active quicksave");
	}
}

static bool stage_input_handler_gameplay(SDL_Event *event, void *arg) {
	StagePrivateState *state = NOT_NULL(arg);
	GameSession *game = state->game;

#ifdef DEBUG
	if(event->type == SDL_EVENT_KEY_DOWN && event->key.scancode == config_get_int(CONFIG_KEY_TIMESTOP)) {
		if(!event->key.repeat) {
			state->paused = !state->paused;
		}

		return false;
	}
#endif

	if(event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat && is_quicksave_allowed(state)) {
		if(event->key.scancode == config_get_int(CONFIG_KEY_QUICKSAVE)) {
			stage_do_quicksave(state, false);
		} else if(event->key.scancode == config_get_int(CONFIG_KEY_QUICKLOAD)) {
			stage_do_quickload(state);
		}

		return false;
	}

	TaiseiEvent type = TAISEI_EVENT(event->type);
	int32_t code = event->user.code;

	if(stage_input_common(event, arg)) {
		return false;
	}

	if(UNLIKELY(state->paused)) {
		return false;
	}

	if(
		(type == TE_GAME_KEY_DOWN || type == TE_GAME_KEY_UP) &&
		code == KEY_SHOT &&
		config_get_int(CONFIG_SHOT_INVERTED)
	) {
		type = type == TE_GAME_KEY_DOWN ? TE_GAME_KEY_UP : TE_GAME_KEY_DOWN;
	}

	ReplayState *rpy = &game->replay.output;

	switch(type) {
		case TE_GAME_KEY_DOWN:
			if(stage_input_key_filter(state, code, false)) {
				player_event(&global.plr, NULL, rpy, EV_PRESS, code);
			}
			break;

		case TE_GAME_KEY_UP:
			if(stage_input_key_filter(state, code, true)) {
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
	if(event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat) {
		goto exit;
	}

	switch(TAISEI_EVENT(event->type)) {
		case TE_GAME_KEY_DOWN:
		case TE_GAMEPAD_BUTTON_DOWN:
		case TE_GAMEPAD_AXIS_DIGITAL:
		exit:
			stage_finish_internal(NOT_NULL(arg), GAMEOVER_ABORT);
	}

	return false;
}

struct replay_event_arg {
	StagePrivateState *state;
	ReplayState *st;
	ReplayEvent *resume_event;
};

static void handle_replay_event(ReplayEvent *e, void *arg) {
	struct replay_event_arg *a = NOT_NULL(arg);
	GameSession *game = a->state->game;

	if(UNLIKELY(a->resume_event != NULL)) {
		log_warn(
			"Got replay event [%i:%02x:%04x] after resume event in the same frame, ignoring",
			e->frame, e->type, e->value
		);
		return;
	}

	switch(e->type) {
		case EV_OVER:
			a->state->stage->gameover = GAMEOVER_DEFEAT;
			break;

		case EV_RESUME:
			a->resume_event = e;
			break;

		default:
			player_event(&global.plr, a->st, &game->replay.output, e->type, e->value);
			break;
	}
}

static void leave_replay_mode(StagePrivateState *state, ReplayState *rp_in) {
	replay_state_deinit(rp_in);
}

static void replay_input(StagePrivateState *state) {
	GameSession *game = state->game;

	if(!should_skip_frame(state)) {
		events_poll((EventHandler[]){
			{
				.proc =
					game_is_demo_mode(game)
						? stage_input_handler_demo
						: stage_input_handler_replay,
				.arg = state,
			},
			{}
		}, EFLAG_GAME);
	}

	ReplayState *rp_in = &game->replay.input;

	if(UNLIKELY(rp_in->mode == REPLAY_NONE)) {
		return;
	}

	struct replay_event_arg a = { .state = state, .st = rp_in };
	replay_state_play_advance(rp_in, global.frames, handle_replay_event, &a);
	player_applymovement(&global.plr);

	if(a.resume_event) {
		leave_replay_mode(state, rp_in);
	}
}

static void display_bgm_title(void) {
	BGM *bgm = audio_bgm_current();
	const char *title = bgm ? _(bgm_get_title(bgm)) : NULL;

	if(title) {
		StringBuffer txt = { acquire_scratch_arena() };
		strbuf_printf(&txt, F_("BGM: %s"), title);
		stagetext_add(txt.start, VIEWPORT_W-15 + I * (VIEWPORT_H-20), ALIGN_RIGHT, res_font("standard"), RGB(1, 1, 1), 30, 180, 35, 35);
		release_scratch_arena(txt.arena);
	}
}

static bool stage_handle_bgm_change(SDL_Event *evt, void *a) {
	StagePrivateState *state = NOT_NULL(a);
	StageInstance *stage = state->stage;
	state->bgm_start_time = global.frames;
	state->bgm_start_pos = audio_bgm_tell();

	if(dialog_is_active(stage->dialog)) {
		INVOKE_TASK_WHEN(&stage->dialog->events.fadeout_began, common_call_func, display_bgm_title);
	} else {
		display_bgm_title();
	}

	return false;
}

static void stage_input(StagePrivateState *state) {
	GameSession *game = state->game;

	if(stage_is_skip_mode()) {
		events_poll((EventHandler[]){
			{
				.proc = stage_handle_bgm_change,
				.event_type = MAKE_TAISEI_EVENT(TE_AUDIO_BGM_STARTED),
				.arg = state,
			},
			{}
		}, EFLAG_NOPUMP);
	} else {
		events_poll((EventHandler[]){
			{ .proc = stage_input_handler_gameplay, .arg = state },
			{
				.proc = stage_handle_bgm_change,
				.event_type = MAKE_TAISEI_EVENT(TE_AUDIO_BGM_STARTED),
				.arg = state,
			},
			{}
		}, EFLAG_GAME);
	}

	if(UNLIKELY(state->paused)) {
		return;
	}

	player_fix_input(&global.plr, &game->replay.output);
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

	if(UNLIKELY(radius <= 0)) {
		return;
	}

	stage_clear_hazards_predicate(proximity_predicate, &area, flags);
}

void stage_clear_hazards_in_ellipse(Ellipse e, ClearHazardsFlags flags) {
	stage_clear_hazards_predicate(ellipse_predicate, &e, flags);
}

TASK(clear_dialog) {
	assert(STAGE.dialog != NULL);
	// dialog_deinit() should've been called by dialog_end() at this point
	STAGE.dialog = NULL;
}

void stage_begin_dialog(Dialog *d) {
	assert(STAGE.dialog == NULL);
	STAGE.dialog = d;
	dialog_init(d);
	INVOKE_TASK_WHEN(&d->events.fadeout_ended, clear_dialog);
}

static void stage_free(StagePrivateState *state) {
	StageInstance *stage = state->stage;

	delete_enemies(&global.enemies);
	delete_items();
	delete_lasers();

	delete_projectiles(&global.projs);
	delete_projectiles(&global.particles);

	if(stage->dialog) {
		dialog_deinit(stage->dialog);
		stage->dialog = NULL;
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
	StagePrivateState *state = ccr.ctx;
	state->stage->gameover = state->finish_gameover;
}

static void stage_finish_internal(StagePrivateState *state, GameoverType gameover) {
	StageInstance *stage = state->stage;
	GameSession *game = state->game;

	if(stage->gameover == GAMEOVER_TRANSITIONING) {
		return;
	}

	GameoverType prev_gameover = stage->gameover;
	state->gameover_time = global.frames;

	if(gameover == GAMEOVER_SCORESCREEN) {
		stage->gameover = GAMEOVER_SCORESCREEN;
	} else {
		stage->gameover = GAMEOVER_TRANSITIONING;
		state->finish_gameover = gameover;
		set_transition(TransFadeBlack, FADE_TIME, FADE_TIME*2, CALLCHAIN(stage_finalize, state));

		if(!game_is_demo_mode(game)) {
			audio_bgm_stop(BGM_FADE_LONG);
		}
	}

	if(
		game->replay.input.replay == NULL &&
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

void stage_finish(GameoverType gameover) {
	stage_finish_internal(STAGE.priv, gameover);
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

	char title[STAGE_MAX_TITLE_SIZE];
	stagetitle_format_localized(&info->title, sizeof(title), title);

	stagetext_add(title,          VIEWPORT_W/2 + I * (VIEWPORT_H/2-40), ALIGN_CENTER, res_font("big"), RGB(1, 1, 1), 50, 85, 35, 35);
	stagetext_add(_(info->subtitle), VIEWPORT_W/2 + I * (VIEWPORT_H/2),    ALIGN_CENTER, res_font("standard"), RGB(1, 1, 1), 60, 85, 35, 35);
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
		case D_Easy:    STAGE.voltage_threshold = easy;    return;
		case D_Normal:  STAGE.voltage_threshold = normal;  return;
		case D_Hard:    STAGE.voltage_threshold = hard;    return;
		case D_Lunatic: STAGE.voltage_threshold = lunatic; return;
		default: UNREACHABLE;
	}
}

bool stage_is_cleared(void) {
	return is_cleared(STAGE.priv);
}

static void stage_update_fps(StagePrivateState *state) {
	GameSession *game = state->game;

	if(game->replay.output.replay) {
		uint16_t replay_fps = (uint16_t)rint(global.fps.logic.fps);

		if(replay_fps != state->last_replay_fps) {
			replay_stage_event(game->replay.output.stage, global.frames, EV_FPS, replay_fps);
			state->last_replay_fps = replay_fps;
		}
	}
}

static void stage_give_clear_bonus(StagePrivateState *state, StageClearBonus *bonus) {
	StageInstance *stage = state->stage;
	GameSession *game = state->game;

	*bonus = (typeof(*bonus)) {};

	// FIXME: this is clunky...
	if(!game->is_practice_mode && stage->info->type == STAGE_STORY) {
		StageInfo *next = stageinfo_get_by_id(stage->info->id + 1);

		if(next == NULL || next->type != STAGE_STORY) {
			bonus->all_clear.base = global.plr.point_item_value * 100 + global.plr.points / 10;
			bonus->all_clear.diff_multiplier = difficulty_value(1.0, 1.1, 1.3, 1.6);
			bonus->all_clear.diff_bonus = bonus->all_clear.base * (bonus->all_clear.diff_multiplier - 1.0);
		}
	}

	if(stage->info->type == STAGE_STORY) {
		bonus->base = stage->info->id * 1000000;
	}

	bonus->voltage = max(0, (int)global.plr.voltage - (int)stage->voltage_threshold) * (global.plr.point_item_value / 25);
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

static void stage_replay_sync(StagePrivateState *state) {
	GameSession *game = state->game;

	uint16_t desync_check = (rng_u64() ^ global.plr.points) & 0xFFFF;
	ReplaySyncStatus rpsync = replay_state_check_desync(&game->replay.input, global.frames, desync_check);

	if(
		game->replay.output.stage &&
		state->desync_check_freq > 0 &&
		!(global.frames % state->desync_check_freq)
	) {
		replay_stage_event(game->replay.output.stage, global.frames, EV_CHECK_DESYNC, desync_check);
	}

	if(rpsync == REPLAY_SYNC_FAIL) {
		if(
			global.is_replay_verification &&
			!game->replay.output.stage
		) {
			exit(1);
		}

		if(state->quicksave && state->quicksave == game->replay.input.replay) {
			log_warn("Quicksave replay desynced; resuming prematurely!");
			leave_replay_mode(state, &game->replay.input);
		}
	}
}

static void process_input(StagePrivateState *state);

static SceneAction stage_scene_logic(void *arg) {
	StagePrivateState *state = arg;
	GameSession *session = state->game;
	StageInstance *stage = state->stage;

	stage_update_fps(state);

	if(watchdog_signaled() && !game_is_demo_mode(session)) {
		session->gameover = GAMEOVER_ABORT;
	}

	if(stage_is_skip_mode()) {
		global.plr.iddqd = true;
	}

	fapproach_p(&stage->view_shake, 0, 1);
	fapproach_asymptotic_p(&stage->view_shake, 0, 0.05, 1e-2);

	if(
		session->replay.input.replay == NULL &&
		state->dynstage_generation != dynstage_get_generation()
	) {
		log_info("Stages library updated, attempting to hot-reload");
		stage_do_quicksave(state, true);   // no-op if user has a manual save
		stage_do_quickload(state);
	}

	if(stage->gameover == GAMEOVER_TRANSITIONING) {
		// Usually stage_comain will do this
		events_poll(NULL, 0);
	} else if(state->paused) {
		process_input(state);
	} else {
		cosched_run_tasks(&state->sched);
		update_all_sfx();
		stage_replay_sync(state);

		global.frames++;

		/*
		 * TODO: Investigate why/if any of this needs to happen after global.frames++,
		 *       and possibly fix that.
		 */

		stagetext_update();

		if(session->replay.input.replay && session->gameover != GAMEOVER_TRANSITIONING) {
			ReplayStage *rstg = session->replay.input.stage;
			ReplayEvent *last_event = dynarray_get_ptr(
				&rstg->events, rstg->events.num_elements - 1);

			if(
				global.frames == last_event->frame - FADE_TIME &&
				last_event->type != EV_RESUME
			) {
				stage_finish_internal(state, GAMEOVER_DEFEAT);
			}
		}

		if(
			stage->gameover == GAMEOVER_SCORESCREEN &&
			global.frames - state->gameover_time == GAMEOVER_SCORE_DELAY
		) {
			StageClearBonus b;
			stage_give_clear_bonus(state, &b);
			stage_display_clear_screen(&b);
		}

		if(stage->info->type == STAGE_SPELL && !global.boss && !state->transition_delay) {
			state->transition_delay = 120;
		}
	}

	if(state->transition_delay) {
		if(!--state->transition_delay) {
			stage_finish_internal(state, GAMEOVER_WIN);
		}
	} else if(!(should_skip_frame(state) && game_is_demo_mode(session))) {
		update_transition();
	}

	if(session->replay.input.replay == NULL) {
		StageProgress *p = NOT_NULL(stageinfo_get_progress(stage->info, global.diff, true));
		progress_register_hiscore(p, global.plr.mode, global.plr.points);
	}

	if(state->quickload_requested) {
		log_info("Quickload initiated");
		return SCENE_ACTION_STOP;
	}

	if(stage->gameover > GAMEOVER_NONE) {
		return SCENE_ACTION_STOP;
	}

	if(should_skip_frame(state)) {
		state->was_skipping = true;
		return SCENE_ACTION_SKIP_ALWAYS;
	}

	recover_after_skip(state);

	SceneAction skipmode = skipstate_handle_frame(state);
	if(skipmode != SCENE_ACTION_WAIT) {
		return skipmode;
	}

	if(session->replay.input.replay && gamekeypressed(KEY_SKIP)) {
		return SCENE_ACTION_SKIP;
	}

	return SCENE_ACTION_WAIT;
}

static void stage_scene_render(void *arg) {
	StagePrivateState *priv = arg;
	GameSession *session = priv->game;

	rng_lock(&global.rand_game);
	rng_make_active(&global.rand_visual);
	BEGIN_DRAW_CODE();
	stage_draw_scene(session);
	END_DRAW_CODE();
	rng_unlock(&global.rand_game);
	rng_make_active(&global.rand_game);
	draw_transition();
}

static void stage_scene_end(void *ctx);

static void stage_stub_proc(void) { }
static void stage_preload_stub_proc(ResourceGroup *rg) { }

static void process_input(StagePrivateState *state) {
	GameSession *game = state->game;

	if(game->replay.input.replay != NULL) {
		replay_input(state);
	} else {
		stage_input(state);
	}
}

TASK(stage_comain, { StagePrivateState *state; }) {
	StagePrivateState *state = ARGS.state;
	StageInstance *stage = state->stage;

	stage->info->procs->begin();
	player_stage_post_init(&global.plr);

	if(global.stage->type != STAGE_SPELL) {
		display_stage_title(stage->info);
	}

	YIELD;

	for(;;YIELD) {
		process_input(state);
		process_boss(&global.boss);
		process_enemies(&global.enemies);
		process_projectiles(&global.projs, true);
		process_items();
		process_lasers();
		process_projectiles(&global.particles, false);

		if(stage->dialog) {
			dialog_update(stage->dialog);

			if((global.plr.inputflags & INFLAG_SKIP) && dialog_is_active(stage->dialog)) {
				dialog_page(stage->dialog);
			}
		}

		if(stage_is_skip_mode()) {
			if(dialog_is_active(stage->dialog)) {
				dialog_page(stage->dialog);
			}

			if(global.boss) {
				ent_damage(&global.boss->ent, &(DamageInfo) { 400, DMG_PLAYER_SHOT } );
			}
		}
	}
}

static void _stage_enter(
	GameSession *session, StageInfo *stage_info, ResourceGroup *rg, CallChain next,
	Replay *quickload, bool quicksave_is_automatic
) {
	assert(stage_info);
	assert(stage_info->procs);

	if(session->gameover == GAMEOVER_WIN) {
		session->gameover = GAMEOVER_NONE;
	} else if(session->gameover) {
		run_call_chain(&next, NULL);
		return;
	}

	uint32_t dynstage_generation = dynstage_get_generation();
	stageinfo_reload();

	#define STUB_PROC(proc, stub) do {\
		if(!stage_info->procs->proc) { \
			stage_info->procs->proc = stub; \
			log_debug(#proc " proc is missing"); \
		} \
	} while(0)

	STUB_PROC(preload, stage_preload_stub_proc);
	STUB_PROC(begin, stage_stub_proc);

	auto state = ALLOC(StagePrivateState, {
		.game = session,
		.stage = &session->stage,
		.cc = next,
		.quicksave = quickload,
		.quicksave_is_automatic = quicksave_is_automatic,
		.desync_check_freq = env_get("TAISEI_REPLAY_DESYNC_CHECK_FREQUENCY", FPS * 5),
		.dynstage_generation = dynstage_generation,
		.rg = rg,
	});

	auto stage = state->stage;
	*stage = (StageInstance) {
		.priv = state,
		.info = stage_info,
	};

	cosched_init(&state->sched, session);
	COEVENT_INIT_ARRAY(stage->events);

	if(quickload) {
		assert(session->replay.input.stage == NULL);
		ReplayStage *qload_stage = dynarray_get_ptr(&quickload->stages, 0);
		assert(qload_stage->stage == stage_info->id);
		replay_state_init_play(&session->replay.input, quickload, qload_stage);
	}

	// I really want to separate all of the game state from the global struct sometime
	global.stage = stage_info;

	ent_init();
	stage_objpools_init();
	stage_draw_preload(session, rg);
	stage_preload(stage_info, rg);
	stage_draw_init();
	lasers_init();

	rng_make_active(&global.rand_game);
	stage_start(session, stage_info);

	uint64_t start_time, seed;

	if(session->replay.input.replay) {
		ReplayStage *rstg = NOT_NULL(session->replay.input.stage);
		assert(stageinfo_get_by_id(rstg->stage) == stage_info);
		assert(!rstg->skip_frames || !quickload);

		start_time = rstg->start_time;
		seed = rstg->rng_seed;
		global.diff = rstg->diff;

		char title[STAGE_MAX_TITLE_SIZE];
		stagetitle_format_localized(&stage_info->title, sizeof(title), title);
		log_debug("REPLAY_PLAY mode: %d events, stage: \"%s\"", rstg->events.num_elements, title);
	} else {
		start_time = (uint64_t)time(0);
		seed = makeseed();

		StageProgress *p = NOT_NULL(stageinfo_get_progress(stage_info, global.diff, true));
		progress_register_stage_played(p, global.plr.mode);
	}

	rng_seed(&global.rand_game, seed);

	if(session->replay.input.replay) {
		player_init(&global.plr);
		replay_stage_sync_player_state(session->replay.input.stage, &global.plr);
	}

	if(session->replay.output.replay) {
		session->replay.output.stage = replay_stage_new(
			session->replay.output.replay,
			stage_info,
			start_time,
			seed,
			global.diff,
			&global.plr
		);
		player_init(&global.plr);
		replay_stage_sync_player_state(session->replay.output.stage, &global.plr);
	}

	plrmode_preload(global.plr.mode, rg);

	res_purge();

	_current_stage_state = state;

	skipstate_init();

	if(should_skip_frame(state)) {
		audio_sfx_set_enabled(false);
	}

	if(!is_quickloading(state)) {
		demoplayer_suspend();
	}

	SCHED_INVOKE_TASK(&state->sched, stage_comain, state);
	eventloop_enter(state, stage_scene_logic, stage_scene_render, stage_scene_end, FPS);
}

void stage_enter(GameSession *session, StageInfo *stage_info, ResourceGroup *rg, CallChain next) {
	_stage_enter(session, stage_info, rg, next, NULL, false);
}

static void stage_scene_end(void *ctx) {
	StagePrivateState *state = ctx;
	GameSession *session = state->game;
	StageInstance *stage = state->stage;
	assert(state == _current_stage_state);

	recover_after_skip(state);

	Replay *quicksave = state->quicksave;
	bool quicksave_is_automatic = state->quicksave_is_automatic;
	bool is_quickload = state->quickload_requested;

	if(is_quickload) {
		assume(quicksave != NULL);
	}

	if(session->replay.output.replay) {
		if(is_quickload) {
			// rollback this stage, as we're about to replay it
			session->replay.output.replay->stages.num_elements--;
			replay_stage_destroy_events(session->replay.output.stage);
		} else {
			replay_stage_event(session->replay.output.stage, global.frames, EV_OVER, 0);
			session->replay.output.stage->plr_points_final = global.plr.points;

			if(session->gameover == GAMEOVER_WIN) {
				session->replay.output.stage->flags |= REPLAY_SFLAG_CLEAR;
			}

			replay_stage_update_final_stats(session->replay.output.stage, &global.plr.stats);
		}
	}

	if(quicksave && !is_quickload) {
		replay_reset(quicksave);
		mem_free(quicksave);
	}

	coevent_signal(&stage->events.end);
	stage_draw_shutdown();
	COEVENT_CANCEL_ARRAY(stage->events);
	cosched_finish(&state->sched);
	stage_free(state);
	player_free(&global.plr);
	ent_shutdown();
	rng_make_active(&global.rand_visual);
	stop_all_sfx();

	taisei_commit_persistent_data(true);
	skipstate_shutdown();

	if(taisei_quit_requested()) {
		stage->gameover = GAMEOVER_ABORT;
	}

	session->gameover = stage->gameover;

	_current_stage_state = NULL;

	StageInfo *stginfo = stage->info;
	CallChain cc = state->cc;
	ResourceGroup *rg = state->rg;

	mem_free(state);
	*stage = (StageInstance) {};

	if(is_quickload) {
		_stage_enter(session, stginfo, rg, cc, quicksave, quicksave_is_automatic);
	} else {
		demoplayer_resume();
		run_call_chain(&cc, NULL);
	}
}

void stage_unlock_bgm(const char *bgm) {
	if(GAME.replay.input.replay == NULL && !global.plr.stats.total.continues_used) {
		progress_unlock_bgm(bgm);
	}
}

void stage_shake_view(float strength) {
	assume(strength >= 0);
	_current_stage_state->stage->view_shake += strength;
}

void stage_load_quicksave(void) {
	stage_do_quickload(NOT_NULL(_current_stage_state));
}

CoSched *stage_get_sched(void) {
	return &NOT_NULL(_current_stage_state)->sched;
}
