/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "util.h"
#include "stage.h"

#include <time.h>
#include "global.h"
#include "video.h"
#include "resource/bgm.h"
#include "replay.h"
#include "config.h"
#include "player.h"
#include "menu/ingamemenu.h"
#include "menu/gameovermenu.h"
#include "audio.h"
#include "log.h"
#include "stagetext.h"
#include "stagedraw.h"
#include "stageobjects.h"

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

		for(AttackInfo *a = s->spell; a->rule; ++a) {
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
	global.game_over = 0;
	global.shake_view = 0;

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

static void stage_ingame_menu_loop(MenuData *menu) {
	if(ingame_menu_interrupts_bgm()) {
		stop_bgm(false);
	}

	pause_sounds();
	menu_loop(menu);

	if(global.game_over) {
		stop_sounds();

		if(ingame_menu_interrupts_bgm() || global.game_over != GAMEOVER_RESTART) {
			stage_fade_bgm();
		}
	} else {
		resume_sounds();
		resume_bgm();
	}
}

void stage_pause(void) {
	if(global.game_over == GAMEOVER_TRANSITIONING) {
		return;
	}

	MenuData menu;

	if(global.replaymode == REPLAY_PLAY) {
		create_ingame_menu_replay(&menu);
	} else {
		create_ingame_menu(&menu);
	}

	stage_ingame_menu_loop(&menu);
}

void stage_gameover(void) {
	if(global.stage->type == STAGE_SPELL && config_get_int(CONFIG_SPELLSTAGE_AUTORESTART)) {
		global.game_over = GAMEOVER_RESTART;
		return;
	}

	MenuData menu;
	create_gameover_menu(&menu);
	stage_ingame_menu_loop(&menu);
}

static bool stage_input_common(SDL_Event *event, void *arg) {
	TaiseiEvent type = TAISEI_EVENT(event->type);
	int32_t code = event->user.code;

	switch(type) {
		case TE_GAME_KEY_DOWN:
			switch(code) {
				case KEY_STOP:
					// global.game_over = GAMEOVER_DEFEAT;
					stage_finish(GAMEOVER_DEFEAT);
					return true;

				case KEY_RESTART:
					// global.game_over = GAMEOVER_RESTART;
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

bool stage_input_handler_gameplay(SDL_Event *event, void *arg) {
	TaiseiEvent type = TAISEI_EVENT(event->type);
	int32_t code = event->user.code;

	if(stage_input_common(event, arg)) {
		return false;
	}

	switch(type) {
		case TE_GAME_KEY_DOWN:
			if(code == KEY_HAHAIWIN) {
#ifdef DEBUG
				stage_finish(GAMEOVER_WIN);
#endif
				break;
			}

#ifndef DEBUG // no cheating for peasants
			if( code == KEY_IDDQD ||
				code == KEY_POWERUP ||
				code == KEY_POWERDOWN)
				break;
#endif

			player_event_with_replay(&global.plr, EV_PRESS, code);
			break;

		case TE_GAME_KEY_UP:
			player_event_with_replay(&global.plr, EV_RELEASE, code);
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

bool stage_input_handler_replay(SDL_Event *event, void *arg) {
	stage_input_common(event, arg);
	return false;
}

void replay_input(void) {
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
				global.game_over = GAMEOVER_DEFEAT;
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

void stage_input(void) {
	events_poll((EventHandler[]){
		{ .proc = stage_input_handler_gameplay },
		{NULL}
	}, EFLAG_GAME);
	player_fix_input(&global.plr);
	player_applymovement(&global.plr);
}

static void stage_logic(void) {
	player_logic(&global.plr);

	process_boss(&global.boss);
	process_enemies(&global.enemies);
	process_projectiles(&global.projs, true);
	process_items();
	process_lasers();
	process_projectiles(&global.particles, false);
	process_dialog(&global.dialog);

	update_sounds();

	global.frames++;

	if(!global.dialog && (!global.boss || boss_is_fleeing(global.boss))) {
		global.timer++;
	}

	if(global.replaymode == REPLAY_PLAY &&
		global.frames == global.replay_stage->events[global.replay_stage->numevents-1].frame - FADE_TIME &&
		global.game_over != GAMEOVER_TRANSITIONING) {
		stage_finish(GAMEOVER_DEFEAT);
	}
}

void stage_clear_hazards_predicate(bool (*predicate)(EntityInterface *ent, void *arg), void *arg, ClearHazardsFlags flags) {
	if(flags & CLEAR_HAZARDS_BULLETS) {
		ProjectileListInterface list_ptrs;

		for(Projectile *p = global.projs.first; p; p = list_ptrs.next) {
			if(!predicate || predicate(&p->ent, arg)) {
				clear_projectile(&global.projs, p, flags & CLEAR_HAZARDS_FORCE, flags & CLEAR_HAZARDS_NOW, &list_ptrs);
			} else {
				*&list_ptrs.list_interface = p->list_interface;
			}
		}
	}

	if(flags & CLEAR_HAZARDS_LASERS) {
		for(Laser *l = global.lasers.first, *next; l; l = next) {
			next = l->next;

			if(!predicate || predicate(&l->ent, arg)) {
				clear_laser(&global.lasers, l, flags & CLEAR_HAZARDS_FORCE, flags & CLEAR_HAZARDS_NOW);
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

void stage_clear_hazards_at(complex origin, double radius, ClearHazardsFlags flags) {
	Circle area = { origin, radius };
	stage_clear_hazards_predicate(proximity_predicate, &area, flags);
}

static void stage_free(void) {
	delete_enemies(&global.enemies);
	delete_enemies(&global.plr.slaves);
	delete_items();
	delete_lasers();

	delete_projectiles(&global.projs);
	delete_projectiles(&global.particles);

	if(global.dialog) {
		delete_dialog(global.dialog);
		global.dialog = NULL;
	}

	if(global.boss) {
		free_boss(global.boss);
		global.boss = NULL;
	}

	lasers_free();
	stagetext_free();
}

static void stage_finalize(void *arg) {
	global.game_over = (intptr_t)arg;
}

void stage_finish(int gameover) {
	// assert(global.game_over != GAMEOVER_TRANSITIONING);

	if(global.game_over == GAMEOVER_TRANSITIONING) {
		log_debug("Requested gameover %i, but already transitioning", gameover);
		return;
	}

	global.game_over = GAMEOVER_TRANSITIONING;
	set_transition_callback(TransFadeBlack, FADE_TIME, FADE_TIME*2, stage_finalize, (void*)(intptr_t)gameover);
	stage_fade_bgm();

	if(global.replaymode == REPLAY_PLAY || gameover != GAMEOVER_WIN) {
		return;
	}

	StageProgress *p = stage_get_progress_from_info(global.stage, global.diff, true);

	if(p) {
		++p->num_cleared;
	}
}

static void stage_preload(void) {
	difficulty_preload();
	projectiles_preload();
	player_preload();
	items_preload();
	boss_preload();
	lasers_preload();

	if(global.stage->type != STAGE_SPELL) {
		enemies_preload();
	}

	global.stage->procs->preload();
}

static void display_stage_title(StageInfo *info) {
	stagetext_add(info->title,    VIEWPORT_W/2 + I * (VIEWPORT_H/2-40), ALIGN_CENTER, get_font("big"), RGB(1, 1, 1), 50, 85, 35, 35);
	stagetext_add(info->subtitle, VIEWPORT_W/2 + I * (VIEWPORT_H/2),    ALIGN_CENTER, get_font("standard"), RGB(1, 1, 1), 60, 85, 35, 35);
}

void stage_start_bgm(const char *bgm) {
	char *old_title = NULL;

	if(current_bgm.title && global.stage->type == STAGE_SPELL) {
		old_title = strdup(current_bgm.title);
	}

	start_bgm(bgm);

	if(current_bgm.title && current_bgm.started_at >= 0 && (!old_title || strcmp(current_bgm.title, old_title))) {
		char txt[strlen(current_bgm.title) + 6];
		snprintf(txt, sizeof(txt), "BGM: %s", current_bgm.title);
		stagetext_add(txt, VIEWPORT_W-15 + I * (VIEWPORT_H-20), ALIGN_RIGHT, get_font("standard"), RGB(1, 1, 1), 30, 180, 35, 35);
	}

	free(old_title);
}

typedef struct StageFrameState {
	StageInfo *stage;
	int transition_delay;
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

static FrameAction stage_logic_frame(void *arg) {
	StageFrameState *fstate = arg;
	StageInfo *stage = fstate->stage;

	stage_update_fps(fstate);

	if(global.shake_view_fade) {
		global.shake_view -= global.shake_view_fade;

		if(global.shake_view <= 0) {
			global.shake_view = global.shake_view_fade = 0;
		}
	}

	((global.replaymode == REPLAY_PLAY) ? replay_input : stage_input)();

	if(global.game_over != GAMEOVER_TRANSITIONING) {
		if((!global.boss || boss_is_fleeing(global.boss)) && !global.dialog) {
			stage->procs->event();
		}

		if(stage->type == STAGE_SPELL && !global.boss && !fstate->transition_delay) {
			fstate->transition_delay = 120;
		}

		stage->procs->update();
	}

	replay_stage_check_desync(global.replay_stage, global.frames, (tsrand() ^ global.plr.points) & 0xFFFF, global.replaymode);
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

	if(global.game_over > 0) {
		return LFRAME_STOP;
	}

	if(global.frameskip || (global.replaymode == REPLAY_PLAY && gamekeypressed(KEY_SKIP))) {
		return LFRAME_SKIP;
	}

	return LFRAME_WAIT;
}

static FrameAction stage_render_frame(void *arg) {
	StageFrameState *fstate = arg;
	StageInfo *stage = fstate->stage;

	tsrand_lock(&global.rand_game);
	tsrand_switch(&global.rand_visual);
	BEGIN_DRAW_CODE();
	stage_draw_scene(stage);
	END_DRAW_CODE();
	tsrand_unlock(&global.rand_game);
	tsrand_switch(&global.rand_game);
	draw_transition();

	return RFRAME_SWAP;
}

void stage_loop(StageInfo *stage) {
	assert(stage);
	assert(stage->procs);
	assert(stage->procs->preload);
	assert(stage->procs->begin);
	assert(stage->procs->end);
	assert(stage->procs->draw);
	assert(stage->procs->event);
	assert(stage->procs->update);
	assert(stage->procs->shader_rules);

	if(global.game_over == GAMEOVER_WIN) {
		global.game_over = 0;
	} else if(global.game_over) {
		return;
	}

	// I really want to separate all of the game state from the global struct sometime
	global.stage = stage;

	ent_init();
	stage_objpools_alloc();
	stage_preload();
	stage_draw_init();

	uint32_t seed = (uint32_t)time(0);
	tsrand_switch(&global.rand_game);
	tsrand_seed_p(&global.rand_game, seed);
	stage_start(stage);

	if(global.replaymode == REPLAY_RECORD) {
		global.replay_stage = replay_create_stage(&global.replay, stage, seed, global.diff, &global.plr);

		// make sure our player state is consistent with what goes into the replay
		player_init(&global.plr);
		replay_stage_sync_player_state(global.replay_stage, &global.plr);

		log_debug("Random seed: %u", seed);

		StageProgress *p = stage_get_progress_from_info(stage, global.diff, true);

		if(p) {
			log_debug("You played this stage %u times", p->num_played);
			log_debug("You cleared this stage %u times", p->num_cleared);

			++p->num_played;
			p->unlocked = true;
		}
	} else {
		if(!global.replay_stage) {
			log_fatal("Attemped to replay a NULL stage");
			return;
		}

		ReplayStage *stg = global.replay_stage;
		log_debug("REPLAY_PLAY mode: %d events, stage: \"%s\"", stg->numevents, stage_get(stg->stage)->title);

		tsrand_seed_p(&global.rand_game, stg->seed);
		log_debug("Random seed: %u", stg->seed);

		global.diff = stg->diff;
		player_init(&global.plr);
		replay_stage_sync_player_state(stg, &global.plr);
		stg->playpos = 0;
	}

	stage->procs->begin();
	player_stage_post_init(&global.plr);

	if(global.stage->type != STAGE_SPELL) {
		display_stage_title(stage);
	}

	StageFrameState fstate = { .stage = stage };
	loop_at_fps(stage_logic_frame, stage_render_frame, &fstate, FPS);

	if(global.replaymode == REPLAY_RECORD) {
		replay_stage_event(global.replay_stage, global.frames, EV_OVER, 0);

		if(global.game_over == GAMEOVER_WIN) {
			global.replay_stage->flags |= REPLAY_SFLAG_CLEAR;
		}
	}

	stage->procs->end();
	stage_draw_shutdown();
	stage_free();
	player_free(&global.plr);
	tsrand_switch(&global.rand_visual);
	free_all_refs();
	ent_shutdown();
	stage_objpools_free();
	stop_sounds();

	if(taisei_quit_requested()) {
		global.game_over = GAMEOVER_ABORT;
	}
}
