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
#include "replay.h"
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

#ifdef DEBUG
	#define DPSTEST
	#include "stages/dpstest.h"
#endif

static size_t numstages = 0;
StageInfo *stages = NULL;

static void add_stage(uint16_t id, StageProcs *procs, StageType type, const char *title, const char *subtitle, AttackInfo *spell, Difficulty diff) {
	++numstages;
	stages = realloc(stages, numstages * sizeof(StageInfo));
	StageInfo *stg = stages + (numstages - 1);
	memset(stg, 0, sizeof(StageInfo));

	stg->id = id;
	stg->procs = procs;
	stg->type = type;
	stralloc(&stg->title, title);
	stralloc(&stg->subtitle, subtitle);
	stg->spell = spell;
	stg->difficulty = diff;
}

static void end_stages(void) {
	add_stage(0, NULL, 0, NULL, NULL, NULL, 0);
}

static void add_spellpractice_stage(StageInfo *s, AttackInfo *a, int *spellnum, uint16_t spellbits, Difficulty diff) {
	uint16_t id = spellbits | a->idmap[diff - D_Easy] | (s->id << 8);

	char *title = strfmt("Spell %d", ++(*spellnum));
	char *subtitle = strjoin(a->name, " ~ ", difficulty_name(diff), NULL);

	add_stage(id, s->procs->spellpractice_procs, STAGE_SPELL, title, subtitle, a, diff);

	free(title);
	free(subtitle);
}

static void add_spellpractice_stages(int *spellnum, bool (*filter)(AttackInfo*), uint16_t spellbits) {
	for(int i = 0 ;; ++i) {
		StageInfo *s = stages + i;

		if(s->type == STAGE_SPELL || !s->spell) {
			break;
		}

		for(AttackInfo *a = s->spell; a->name; ++a) {
			if(!filter(a)) {
				continue;
			}

			for(Difficulty diff = D_Easy; diff < D_Easy + NUM_SELECTABLE_DIFFICULTIES; ++diff) {
				if(a->idmap[diff - D_Easy] >= 0) {
					add_spellpractice_stage(s, a, spellnum, spellbits, diff);
					s = stages + i; // stages just got realloc'd, so we must update the pointer
				}
			}
		}
	}
}

static bool spellfilter_normal(AttackInfo *spell) {
	return spell->type != AT_ExtraSpell;
}

static bool spellfilter_extra(AttackInfo *spell) {
	return spell->type == AT_ExtraSpell;
}

#include "stages/corotest.h"

void stage_init_array(void) {
	int spellnum = 0;

//	         id  procs          type         title      subtitle                       spells                       diff
	add_stage(1, &stage1_procs, STAGE_STORY, "Stage 1", "Misty Lake",                  (AttackInfo*)&stage1_spells, D_Any);
	add_stage(2, &stage2_procs, STAGE_STORY, "Stage 2", "Walk Along the Border",       (AttackInfo*)&stage2_spells, D_Any);
	add_stage(3, &stage3_procs, STAGE_STORY, "Stage 3", "Through the Tunnel of Light", (AttackInfo*)&stage3_spells, D_Any);
	add_stage(4, &stage4_procs, STAGE_STORY, "Stage 4", "Forgotten Mansion",           (AttackInfo*)&stage4_spells, D_Any);
	add_stage(5, &stage5_procs, STAGE_STORY, "Stage 5", "Climbing the Tower of Babel", (AttackInfo*)&stage5_spells, D_Any);
	add_stage(6, &stage6_procs, STAGE_STORY, "Stage 6", "Roof of the World",           (AttackInfo*)&stage6_spells, D_Any);

#ifdef DPSTEST
	add_stage(0x40|0, &stage_dpstest_single_procs, STAGE_SPECIAL, "DPS Test", "Single target", NULL, D_Normal);
	add_stage(0x40|1, &stage_dpstest_multi_procs, STAGE_SPECIAL, "DPS Test", "Multiple targets", NULL, D_Normal);
	add_stage(0x40|2, &stage_dpstest_boss_procs, STAGE_SPECIAL, "DPS Test", "Boss", NULL, D_Normal);
#endif

	// generate spellpractice stages
	add_spellpractice_stages(&spellnum, spellfilter_normal, STAGE_SPELL_BIT);
	add_spellpractice_stages(&spellnum, spellfilter_extra, STAGE_SPELL_BIT | STAGE_EXTRASPELL_BIT);

#ifdef SPELL_BENCHMARK
	add_spellpractice_stage(stages, &stage1_spell_benchmark, &spellnum, STAGE_SPELL_BIT, D_Extra);
#endif

	add_stage(0xC0, &corotest_procs, STAGE_SPECIAL, "Coroutines!", "wow such concurrency very async", NULL, D_Any);
	add_stage(0xC1, &extra_procs, STAGE_SPECIAL, "Extra Stage", "Descent into Madness", NULL, D_Extra);

	end_stages();

#ifdef DEBUG
	for(int i = 0; stages[i].procs; ++i) {
		if(stages[i].type == STAGE_SPELL && !(stages[i].id & STAGE_SPELL_BIT)) {
			log_fatal("Spell stage has an ID without the spell bit set: 0x%04x", stages[i].id);
		}

		for(int j = 0; stages[j].procs; ++j) {
			if(i != j && stages[i].id == stages[j].id) {
				log_fatal("Duplicate ID %X in stages array, indices: %i, %i", stages[i].id, i, j);
			}
		}
	}
#endif
}

void stage_free_array(void) {
	for(StageInfo *stg = stages; stg->procs; ++stg) {
		free(stg->title);
		free(stg->subtitle);
		free(stg->progress);
	}

	free(stages);
}

// NOTE: This returns the stage BY ID, not by the array index!
StageInfo* stage_get(uint16_t n) {
	for(StageInfo *stg = stages; stg->procs; ++stg)
		if(stg->id == n)
			return stg;
	return NULL;
}

StageInfo* stage_get_by_spellcard(AttackInfo *spell, Difficulty diff) {
	if(!spell)
		return NULL;

	for(StageInfo *stg = stages; stg->procs; ++stg)
		if(stg->spell == spell && stg->difficulty == diff)
			return stg;

	return NULL;
}

StageProgress* stage_get_progress_from_info(StageInfo *stage, Difficulty diff, bool allocate) {
	// D_Any stages will have a separate StageProgress for every selectable difficulty.
	// Stages with a fixed difficulty setting (spellpractice, extra stage...) obviously get just one and the diff parameter is ignored.

	// This stuff must stay around until progress_save(), which happens on shutdown.
	// So do NOT try to free any pointers this function returns, that will fuck everything up.

	if(!stage) {
		return NULL;
	}

	bool fixed_diff = (stage->difficulty != D_Any);

	if(!fixed_diff && (diff < D_Easy || diff > D_Lunatic)) {
		return NULL;
	}

	if(!stage->progress) {
		if(!allocate) {
			return NULL;
		}

		size_t allocsize = sizeof(StageProgress) * (fixed_diff ? 1 : NUM_SELECTABLE_DIFFICULTIES);
		stage->progress = malloc(allocsize);
		memset(stage->progress, 0, allocsize);
	}

	return stage->progress + (fixed_diff ? 0 : diff - D_Easy);
}

StageProgress* stage_get_progress(uint16_t id, Difficulty diff, bool allocate) {
	return stage_get_progress_from_info(stage_get(id), diff, allocate);
}

static void stage_start(StageInfo *stage) {
	global.timer = 0;
	global.frames = 0;
	global.gameover = 0;
	global.shake_view = 0;
	global.voltage_threshold = 0;

	player_stage_pre_init(&global.plr);

	if(stage->type == STAGE_SPELL) {
		global.is_practice_mode = true;
		global.plr.lives = 0;
		global.plr.bombs = 0;
	} else if(global.is_practice_mode) {
		global.plr.lives = PLR_STGPRACTICE_LIVES;
		global.plr.bombs = PLR_STGPRACTICE_BOMBS;
	}

	if(global.is_practice_mode) {
		global.plr.power = config_get_int(CONFIG_PRACTICE_POWER);
	}

	if(global.plr.power < 0) {
		global.plr.power = 0;
	} else if(global.plr.power > PLR_MAX_POWER) {
		global.plr.power = PLR_MAX_POWER;
	}

	reset_sounds();
}

static bool ingame_menu_interrupts_bgm(void) {
	return global.stage->type != STAGE_SPELL;
}

static void stage_fade_bgm(void) {
	fade_bgm((FPS * FADE_TIME) / 2000.0);
}

static void stage_leave_ingame_menu(CallChainResult ccr) {
	MenuData *m = ccr.result;

	if(m->state != MS_Dead) {
		return;
	}

	if(global.gameover > 0) {
		stop_sounds();

		if(ingame_menu_interrupts_bgm() || global.gameover != GAMEOVER_RESTART) {
			stage_fade_bgm();
		}
	} else {
		resume_sounds();
		resume_bgm();
	}

	CallChain *cc = ccr.ctx;
	run_call_chain(cc, NULL);
	free(cc);
}

static void stage_enter_ingame_menu(MenuData *m, CallChain next) {
	if(ingame_menu_interrupts_bgm()) {
		stop_bgm(false);
	}

	pause_sounds();
	enter_menu(m, CALLCHAIN(stage_leave_ingame_menu, memdup(&next, sizeof(next))));
}

void stage_pause(void) {
	if(global.gameover == GAMEOVER_TRANSITIONING || taisei_is_skip_mode_enabled()) {
		return;
	}

	stage_enter_ingame_menu(
		(global.replaymode == REPLAY_PLAY
			? create_ingame_menu_replay
			: create_ingame_menu
		)(), NO_CALLCHAIN
	);
}

void stage_gameover(void) {
	if(global.stage->type == STAGE_SPELL && config_get_int(CONFIG_SPELLSTAGE_AUTORESTART)) {
		global.gameover = GAMEOVER_RESTART;
		return;
	}

	stage_enter_ingame_menu(create_gameover_menu(), NO_CALLCHAIN);
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

static bool stage_input_handler_gameplay(SDL_Event *event, void *arg) {
	TaiseiEvent type = TAISEI_EVENT(event->type);
	int32_t code = event->user.code;

	if(stage_input_common(event, arg)) {
		return false;
	}

	switch(type) {
		case TE_GAME_KEY_DOWN:
			if(stage_input_key_filter(code, false)) {
				player_event_with_replay(&global.plr, EV_PRESS, code);
			}
			break;

		case TE_GAME_KEY_UP:
			if(stage_input_key_filter(code, true)) {
				player_event_with_replay(&global.plr, EV_RELEASE, code);
			}
			break;

		case TE_GAME_AXIS_LR:
			player_event_with_replay(&global.plr, EV_AXIS_LR, (uint16_t)code);
			break;

		case TE_GAME_AXIS_UD:
			player_event_with_replay(&global.plr, EV_AXIS_UD, (uint16_t)code);
			break;

		default: break;
	}

	return false;
}

static bool stage_input_handler_replay(SDL_Event *event, void *arg) {
	stage_input_common(event, arg);
	return false;
}

static void replay_input(void) {
	ReplayStage *s = global.replay_stage;
	int i;

	events_poll((EventHandler[]){
		{ .proc = stage_input_handler_replay },
		{NULL}
	}, EFLAG_GAME);

	for(i = s->playpos; i < s->numevents; ++i) {
		ReplayEvent *e = s->events + i;

		if(e->frame != global.frames)
			break;

		switch(e->type) {
			case EV_OVER:
				global.gameover = GAMEOVER_DEFEAT;
				break;

			case EV_CHECK_DESYNC:
				s->desync_check = e->value;
				break;

			case EV_FPS:
				s->fps = e->value;
				break;

			default: {
				player_event(&global.plr, e->type, (int16_t)e->value, NULL, NULL);
				break;
			}
		}
	}

	s->playpos = i;
	player_applymovement(&global.plr);
}

static void stage_input(void) {
	events_poll((EventHandler[]){
		{ .proc = stage_input_handler_gameplay },
		{NULL}
	}, EFLAG_GAME);
	player_fix_input(&global.plr);
	player_applymovement(&global.plr);
}

#ifdef DEBUG
static const char *_skip_to_bookmark;
bool _skip_to_dialog;

void _stage_bookmark(const char *name) {
	log_debug("Bookmark [%s] reached at %i", name, global.frames);

	if(_skip_to_bookmark && !strcmp(_skip_to_bookmark, name)) {
		_skip_to_bookmark = NULL;
		global.plr.iddqd = false;
	}
}

DEFINE_EXTERN_TASK(stage_bookmark) {
	_stage_bookmark(ARGS.name);
}
#endif

static bool _stage_should_skip(void) {
#ifdef DEBUG
	if(_skip_to_bookmark || _skip_to_dialog) {
		return true;
	}
#endif
	return false;
}

static void stage_logic(void) {
	player_logic(&global.plr);

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

	if(_stage_should_skip()) {
		if(dialog_is_active(global.dialog)) {
			dialog_page(global.dialog);
		}

		if(global.boss) {
			ent_damage(&global.boss->ent, &(DamageInfo) { 400, DMG_PLAYER_SHOT } );
		}
	}

	update_sounds();

	global.frames++;

	if(!dialog_is_active(global.dialog) && (!global.boss || boss_is_fleeing(global.boss))) {
		global.timer++;
	}

	if(global.replaymode == REPLAY_PLAY &&
		global.frames == global.replay_stage->events[global.replay_stage->numevents-1].frame - FADE_TIME &&
		global.gameover != GAMEOVER_TRANSITIONING) {
		stage_finish(GAMEOVER_DEFEAT);
	}

	stagetext_update();
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
		case ENT_PROJECTILE: {
			Projectile *p = ENT_CAST(ent, Projectile);
			return cabs(p->pos - area->origin) < area->radius;
		}

		case ENT_LASER: {
			Laser *l = ENT_CAST(ent, Laser);
			return laser_intersects_circle(l, *area);
		}

		default: UNREACHABLE;
	}
}

static bool ellipse_predicate(EntityInterface *ent, void *varg) {
	Ellipse *e = varg;

	switch(ent->type) {
		case ENT_PROJECTILE: {
			Projectile *p = ENT_CAST(ent, Projectile);
			return point_in_ellipse(p->pos, *e);
		}

		case ENT_LASER: {
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

TASK(clear_dialog, NO_ARGS) {
	assert(global.dialog != NULL);
	// dialog_deinit() should've been called by dialog_end() at this point
	global.dialog = NULL;
}

TASK(dialog_fixup_timer, NO_ARGS) {
	// HACK: remove when global.timer is gone
	global.timer++;
}

void stage_begin_dialog(Dialog *d) {
	assert(global.dialog == NULL);
	global.dialog = d;
	dialog_init(d);
	INVOKE_TASK_WHEN(&d->events.fadeout_began, dialog_fixup_timer);
	INVOKE_TASK_WHEN(&d->events.fadeout_ended, clear_dialog);
}

static void stage_free(void) {
	delete_enemies(&global.enemies);
	delete_enemies(&global.plr.slaves);
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

	projectiles_free();
	lasers_free();
	stagetext_free();
}

static void stage_finalize(void *arg) {
	global.gameover = (intptr_t)arg;
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
		set_transition_callback(TransFadeBlack, FADE_TIME, FADE_TIME*2, stage_finalize, (void*)(intptr_t)gameover);
		stage_fade_bgm();
	}

	if(
		global.replaymode != REPLAY_PLAY &&
		prev_gameover != GAMEOVER_SCORESCREEN &&
		(gameover == GAMEOVER_SCORESCREEN || gameover == GAMEOVER_WIN)
	) {
		StageProgress *p = stage_get_progress_from_info(global.stage, global.diff, true);

		if(p) {
			++p->num_cleared;
			log_debug("Stage cleared %u times now", p->num_cleared);
		}
	}
}

static void stage_preload(void) {
	difficulty_preload();
	projectiles_preload();
	player_preload();
	items_preload();
	boss_preload();
	lasers_preload();
	enemies_preload();

	if(global.stage->type != STAGE_SPELL) {
		dialog_preload();
	}

	global.stage->procs->preload();
}

static void display_stage_title(StageInfo *info) {
	stagetext_add(info->title,    VIEWPORT_W/2 + I * (VIEWPORT_H/2-40), ALIGN_CENTER, get_font("big"), RGB(1, 1, 1), 50, 85, 35, 35);
	stagetext_add(info->subtitle, VIEWPORT_W/2 + I * (VIEWPORT_H/2),    ALIGN_CENTER, get_font("standard"), RGB(1, 1, 1), 60, 85, 35, 35);
}

static void display_bgm_title(void) {
	char txt[strlen(current_bgm.title) + 6];
	snprintf(txt, sizeof(txt), "BGM: %s", current_bgm.title);
	stagetext_add(txt, VIEWPORT_W-15 + I * (VIEWPORT_H-20), ALIGN_RIGHT, get_font("standard"), RGB(1, 1, 1), 30, 180, 35, 35);
}

void stage_start_bgm(const char *bgm) {
	char *old_title = NULL;

	if(current_bgm.title && global.stage->type == STAGE_SPELL) {
		old_title = strdup(current_bgm.title);
	}

	start_bgm(bgm);

	if(current_bgm.title && current_bgm.started_at >= 0 && (!old_title || strcmp(current_bgm.title, old_title))) {
		if(dialog_is_active(global.dialog)) {
			INVOKE_TASK_WHEN(&global.dialog->events.fadeout_began, common_call_func, display_bgm_title);
		} else {
			display_bgm_title();
		}
	}

	free(old_title);
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

typedef struct StageFrameState {
	StageInfo *stage;
	CallChain cc;
	CoSched sched;
	int transition_delay;
	int logic_calls;
	uint16_t last_replay_fps;
} StageFrameState;

static void stage_update_fps(StageFrameState *fstate) {
	if(global.replaymode == REPLAY_RECORD) {
		uint16_t replay_fps = (uint16_t)rint(global.fps.logic.fps);

		if(replay_fps != fstate->last_replay_fps) {
			replay_stage_event(global.replay_stage, global.frames, EV_FPS, replay_fps);
			fstate->last_replay_fps = replay_fps;
		}
	}
}

static void stage_give_clear_bonus(const StageInfo *stage, StageClearBonus *bonus) {
	memset(bonus, 0, sizeof(*bonus));

	// FIXME: this is clunky...
	if(!global.is_practice_mode && stage->type == STAGE_STORY) {
		StageInfo *next = stage_get(stage->id + 1);

		if(next == NULL || next->type != STAGE_STORY) {
			bonus->all_clear = true;
		}
	}

	if(stage->type == STAGE_STORY) {
		bonus->base = stage->id * 1000000;
	}

	if(bonus->all_clear) {
		bonus->base += global.plr.point_item_value * 100;
		bonus->graze = global.plr.graze * (global.plr.point_item_value / 10);
	}

	bonus->voltage = imax(0, (int)global.plr.voltage - (int)global.voltage_threshold) * (global.plr.point_item_value / 25);
	bonus->lives = global.plr.lives * global.plr.point_item_value * 5;

	// TODO: maybe a difficulty multiplier?

	bonus->total = bonus->base + bonus->voltage + bonus->lives;
	player_add_points(&global.plr, bonus->total, global.plr.pos);
}

INLINE bool stage_should_yield(void) {
	return (global.boss && !boss_is_fleeing(global.boss)) || dialog_is_active(global.dialog);
}

static LogicFrameAction stage_logic_frame(void *arg) {
	StageFrameState *fstate = arg;
	StageInfo *stage = fstate->stage;

	++fstate->logic_calls;

	stage_update_fps(fstate);

	if(_stage_should_skip()) {
		global.plr.iddqd = true;
	}

	if(global.shake_view > 30) {
		global.shake_view = 30;
	}

	if(global.shake_view_fade) {
		global.shake_view -= global.shake_view_fade;

		if(global.shake_view <= 0) {
			global.shake_view = global.shake_view_fade = 0;
		}
	}

	((global.replaymode == REPLAY_PLAY) ? replay_input : stage_input)();

	if(global.gameover != GAMEOVER_TRANSITIONING) {
		cosched_run_tasks(&fstate->sched);

		if(!stage_should_yield()) {
			stage->procs->event();
		}

		if(global.gameover == GAMEOVER_SCORESCREEN && global.frames - global.gameover_time == GAMEOVER_SCORE_DELAY) {
			StageClearBonus b;
			stage_give_clear_bonus(stage, &b);
			stage_display_clear_screen(&b);
		}

		if(stage->type == STAGE_SPELL && !global.boss && !fstate->transition_delay) {
			fstate->transition_delay = 120;
		}

		stage->procs->update();
	}

	replay_stage_check_desync(global.replay_stage, global.frames, (rng_u64() ^ global.plr.points) & 0xFFFF, global.replaymode);
	stage_logic();

	if(fstate->transition_delay) {
		if(!--fstate->transition_delay) {
			stage_finish(GAMEOVER_WIN);
		}
	} else {
		update_transition();
	}

	if(global.replaymode == REPLAY_RECORD && global.plr.points > progress.hiscore) {
		progress.hiscore = global.plr.points;
	}

	if(global.gameover > 0) {
		return LFRAME_STOP;
	}

#ifdef DEBUG
	if(_skip_to_dialog && dialog_is_active(global.dialog)) {
		_skip_to_dialog = false;
		global.plr.iddqd = false;
	}

	taisei_set_skip_mode(_stage_should_skip());

	if(taisei_is_skip_mode_enabled() || gamekeypressed(KEY_SKIP)) {
		return LFRAME_SKIP;
	}
#endif

	if(global.frameskip || (global.replaymode == REPLAY_PLAY && gamekeypressed(KEY_SKIP))) {
		return LFRAME_SKIP;
	}

	return LFRAME_WAIT;
}

static RenderFrameAction stage_render_frame(void *arg) {
	StageFrameState *fstate = arg;
	StageInfo *stage = fstate->stage;

	if(_stage_should_skip()) {
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

void stage_enter(StageInfo *stage, CallChain next) {
	assert(stage);
	assert(stage->procs);

	#define STUB_PROC(proc, stub) do {\
		if(!stage->procs->proc) { \
			stage->procs->proc = stub; \
			log_debug(#proc " proc is missing"); \
		} \
	} while(0)

	static const ShaderRule shader_rules_stub[1] = { NULL };

	STUB_PROC(preload, stage_stub_proc);
	STUB_PROC(begin, stage_stub_proc);
	STUB_PROC(end, stage_stub_proc);
	STUB_PROC(draw, stage_stub_proc);
	STUB_PROC(event, stage_stub_proc);
	STUB_PROC(update, stage_stub_proc);
	STUB_PROC(shader_rules, (ShaderRule*)shader_rules_stub);

	if(global.gameover == GAMEOVER_WIN) {
		global.gameover = 0;
	} else if(global.gameover) {
		run_call_chain(&next, NULL);
		return;
	}

	// I really want to separate all of the game state from the global struct sometime
	global.stage = stage;

	ent_init();
	stage_objpools_alloc();
	stage_draw_pre_init();
	stage_preload();
	stage_draw_init();

	rng_make_active(&global.rand_game);
	stage_start(stage);

	if(global.replaymode == REPLAY_RECORD) {
		uint64_t start_time = (uint64_t)time(0);
		uint64_t seed = makeseed();
		rng_seed(&global.rand_game, seed);

		global.replay_stage = replay_create_stage(&global.replay, stage, start_time, seed, global.diff, &global.plr);

		// make sure our player state is consistent with what goes into the replay
		player_init(&global.plr);
		replay_stage_sync_player_state(global.replay_stage, &global.plr);

		log_debug("Start time: %"PRIu64, start_time);
		log_debug("Random seed: 0x%"PRIx64, seed);

		StageProgress *p = stage_get_progress_from_info(stage, global.diff, true);

		if(p) {
			log_debug("You played this stage %u times", p->num_played);
			log_debug("You cleared this stage %u times", p->num_cleared);

			++p->num_played;
			p->unlocked = true;
		}
	} else {
		ReplayStage *stg = global.replay_stage;

		assert(stg != NULL);
		assert(stage_get(stg->stage) == stage);

		rng_seed(&global.rand_game, stg->rng_seed);

		log_debug("REPLAY_PLAY mode: %d events, stage: \"%s\"", stg->numevents, stage->title);
		log_debug("Start time: %"PRIu64, stg->start_time);
		log_debug("Random seed: 0x%"PRIx64, stg->rng_seed);

		global.diff = stg->diff;
		player_init(&global.plr);
		replay_stage_sync_player_state(stg, &global.plr);
		stg->playpos = 0;
	}

	StageFrameState *fstate = calloc(1 , sizeof(*fstate));
	cosched_init(&fstate->sched);
	cosched_set_invoke_target(&fstate->sched);
	fstate->stage = stage;
	fstate->cc = next;

	#ifdef DEBUG
	_skip_to_dialog = env_get_int("TAISEI_SKIP_TO_DIALOG", 0);
	_skip_to_bookmark = env_get_string_nonempty("TAISEI_SKIP_TO_BOOKMARK", NULL);
	taisei_set_skip_mode(_stage_should_skip());
	#endif

	stage->procs->begin();
	player_stage_post_init(&global.plr);

	if(global.stage->type != STAGE_SPELL) {
		display_stage_title(stage);
	}

	eventloop_enter(fstate, stage_logic_frame, stage_render_frame, stage_end_loop, FPS);
}

void stage_end_loop(void* ctx) {
	StageFrameState *s = ctx;

	if(global.replaymode == REPLAY_RECORD) {
		replay_stage_event(global.replay_stage, global.frames, EV_OVER, 0);
		global.replay_stage->plr_points_final = global.plr.points;

		if(global.gameover == GAMEOVER_WIN) {
			global.replay_stage->flags |= REPLAY_SFLAG_CLEAR;
		}
	}

	s->stage->procs->end();
	stage_draw_shutdown();
	stage_free();
	player_free(&global.plr);
	cosched_finish(&s->sched);
	rng_make_active(&global.rand_visual);
	free_all_refs();
	ent_shutdown();
	stage_objpools_free();
	stop_sounds();

	taisei_commit_persistent_data();
	taisei_set_skip_mode(false);

	if(taisei_quit_requested()) {
		global.gameover = GAMEOVER_ABORT;
	}

	run_call_chain(&s->cc, NULL);
	free(s);
}

void stage_unlock_bgm(const char *bgm) {
	if(global.replaymode != REPLAY_PLAY && !global.plr.continues_used) {
		progress_unlock_bgm(bgm);
	}
}
