/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

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

static void add_spellpractice_stages(int *spellnum, bool (*filter)(AttackInfo*), uint16_t spellbits) {
	for(int i = 0 ;; ++i) {
		StageInfo *s = stages + i;

		if(s->type == STAGE_SPELL) {
			break;
		}

		for(AttackInfo *a = s->spell; a->rule; ++a) {
			if(!filter(a)) {
				continue;
			}

			for(Difficulty diff = D_Easy; diff < D_Easy + NUM_SELECTABLE_DIFFICULTIES; ++diff) {
				if(a->idmap[diff - D_Easy] >= 0) {
					uint16_t id = spellbits | a->idmap[diff - D_Easy] | (s->id << 8);

					char *title = strfmt("Spell %d", ++(*spellnum));
					char *subtitle = strjoin(a->name, " ~ ", difficulty_name(diff), NULL);

					add_stage(id, s->procs->spellpractice_procs, STAGE_SPELL, title, subtitle, a, diff);
					s = stages + i; // stages just got realloc'd, so we must update the pointer

					free(title);
					free(subtitle);
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

//           id  procs          type         title      subtitle                       spells                       diff
	add_stage(1, &stage1_procs, STAGE_STORY, "Stage 1", "Misty Lake",                  (AttackInfo*)&stage1_spells, D_Any);
	add_stage(2, &stage2_procs, STAGE_STORY, "Stage 2", "Walk Along the Border",       (AttackInfo*)&stage2_spells, D_Any);
	add_stage(3, &stage3_procs, STAGE_STORY, "Stage 3", "Through the Tunnel of Light", (AttackInfo*)&stage3_spells, D_Any);
	add_stage(4, &stage4_procs, STAGE_STORY, "Stage 4", "Forgotten Mansion",           (AttackInfo*)&stage4_spells, D_Any);
	add_stage(5, &stage5_procs, STAGE_STORY, "Stage 5", "Climbing the Tower of Babel", (AttackInfo*)&stage5_spells, D_Any);
	add_stage(6, &stage6_procs, STAGE_STORY, "Stage 6", "Roof of the World",           (AttackInfo*)&stage6_spells, D_Any);

	// generate spellpractice stages
	add_spellpractice_stages(&spellnum, spellfilter_normal, STAGE_SPELL_BIT);
	add_spellpractice_stages(&spellnum, spellfilter_extra, STAGE_SPELL_BIT | STAGE_EXTRASPELL_BIT);

	end_stages();

#ifdef DEBUG
	for(int i = 0; stages[i].procs; ++i) {
		if(stages[i].type == STAGE_SPELL && !(stages[i].id & STAGE_SPELL_BIT)) {
			log_fatal("Spell stage has an ID without the spell bit set: 0x%04x", stages[i].id);
		}

		for(int j = 0; stages[j].procs; ++j) {
			if(i != j && stages[i].id == stages[j].id) {
				log_fatal("Duplicate ID 0x%04x in stages array, indices: %i, %i", stages[i].id, i, j);
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
		global.plr.lives = 0;
		global.plr.bombs = 0;
		global.plr.power = PLR_SPELLPRACTICE_POWER;
	} else if(global.is_practice_mode) {
		global.plr.lives = PLR_STGPRACTICE_LIVES;
		global.plr.bombs = PLR_STGPRACTICE_BOMBS;
		global.plr.power = PLR_STGPRACTICE_POWER;
	}

	reset_sounds();
}

static bool ingame_menu_interrupts_bgm(void) {
	return global.stage->type != STAGE_SPELL;
}

void stage_pause(void) {
	MenuData menu;

	if(global.replaymode == REPLAY_PLAY) {
		create_ingame_menu_replay(&menu);
	} else {
		create_ingame_menu(&menu);
	}

	if(ingame_menu_interrupts_bgm()) {
		stop_bgm(false);
	}

	pause_sounds();
	menu_loop(&menu);

	if(global.game_over) {
		stop_sounds();

		if(ingame_menu_interrupts_bgm() || global.game_over != GAMEOVER_RESTART) {
			fade_bgm();
		}
	} else {
		resume_sounds();
		resume_bgm();
	}
}

void stage_gameover(void) {
	if(global.stage->type == STAGE_SPELL && config_get_int(CONFIG_SPELLSTAGE_AUTORESTART)) {
		global.game_over = GAMEOVER_RESTART;
		return;
	}

	MenuData menu;
	create_gameover_menu(&menu);

	// XXX: The save_bgm and restore_bgm code is broken.
	// Disabled it for now, as we don't have a gameover BGM yet
	// When we do, it needs to be fixed.
	// It should be probably implemented by the backend, it has a better idea
	// about how to manage its own state than the audio frontend code.

	/*
	bool interrupt_bgm = (global.stage->type != STAGE_SPELL);

	if(interrupt_bgm) {
		save_bgm();
		start_bgm("gameover");
	}
	*/

	pause_sounds();

	if(ingame_menu_interrupts_bgm()) {
		stop_bgm(false);
	}

	menu_loop(&menu);
	resume_sounds();

	/*
	if(interrupt_bgm) {
		restore_bgm();
	}
	*/

	resume_bgm();
}

bool stage_input_handler_gameplay(SDL_Event *event, void *arg) {
	TaiseiEvent type = TAISEI_EVENT(event->type);
	int32_t code = event->user.code;

	switch(type) {
		case TE_GAME_KEY_DOWN:
			if(code == KEY_HAHAIWIN) {
#ifdef DEBUG
				stage_finish(GAMEOVER_WIN);
#endif
				break;
			}

#ifndef DEBUG // no cheating for peasants
			if(	code == KEY_IDDQD ||
				code == KEY_POWERUP ||
				code == KEY_POWERDOWN)
				break;
#endif

			player_event_with_replay(&global.plr, EV_PRESS, code);
			break;

		case TE_GAME_KEY_UP:
			player_event_with_replay(&global.plr, EV_RELEASE, code);
			break;

		case TE_GAME_PAUSE:
			stage_pause();
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
	if(event->type == MAKE_TAISEI_EVENT(TE_GAME_PAUSE)) {
		stage_pause();
	}

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

	process_enemies(&global.enemies);
	process_projectiles(&global.projs, true);
	process_items();
	process_lasers();
	process_projectiles(&global.particles, false);

	update_sounds();

	if(global.dialog) {
		int to = global.dialog->messages[global.dialog->pos].timeout;

		if(
			(to && to >= global.frames) ||
			((global.plr.inputflags & INFLAG_SKIP) && global.frames - global.dialog->page_time > 3)
		) {
			page_dialog(&global.dialog);
		}
	}

	if(global.boss) {
		process_boss(&global.boss);
	}

	global.frames++;

	if(!global.dialog && (!global.boss || boss_is_fleeing(global.boss)))
		global.timer++;

	if(global.replaymode == REPLAY_PLAY &&
		global.frames == global.replay_stage->events[global.replay_stage->numevents-1].frame - FADE_TIME &&
		global.game_over != GAMEOVER_TRANSITIONING) {
		stage_finish(GAMEOVER_DEFEAT);
	}

	// BGM handling
	if(global.dialog && global.dialog->messages[global.dialog->pos].side == BGM) {
		stage_start_bgm(global.dialog->messages[global.dialog->pos].msg);
		page_dialog(&global.dialog);
	}
}

void stage_clear_hazards(bool force) {
	for(Projectile *p = global.projs; p; p = p->next) {
		if(p->type == EnemyProj || p->type == FakeProj)
			p->type = DeadProj;
	}

	for(Laser *l = global.lasers; l; l = l->next) {
		if(!l->unclearable || force)
			l->dead = true;
	}
}

void stage_clear_hazards_instantly(bool force) {
	for(Projectile *p = global.projs, *next; p; p = next) {
		next = p->next;

		if(p->type == EnemyProj || p->type == FakeProj || p->type == DeadProj) {
			create_bpoint(p->pos);
			delete_projectile(&global.projs, p);
		}
	}

	// TODO: clear these instantly as well
	for(Laser *l = global.lasers; l; l = l->next) {
		if(!l->unclearable || force)
			l->dead = true;
	}
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

	stagetext_free();
}

static void stage_finalize(void *arg) {
	global.game_over = (intptr_t)arg;
}

void stage_finish(int gameover) {
	assert(global.game_over != GAMEOVER_TRANSITIONING);
	global.game_over = GAMEOVER_TRANSITIONING;
	set_transition_callback(TransFadeBlack, FADE_TIME, FADE_TIME*2, stage_finalize, (void*)(intptr_t)gameover);
	fade_bgm();

	if(global.replaymode == REPLAY_PLAY) {
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

	if(global.stage->type != STAGE_SPELL)
		enemies_preload();

	global.stage->procs->preload();
}

static void display_stage_title(StageInfo *info) {
	stagetext_add(info->title,    VIEWPORT_W/2 + I * (VIEWPORT_H/2-40), AL_Center, _fonts.mainmenu, rgb(1, 1, 1), 50, 85, 35, 35);
	stagetext_add(info->subtitle, VIEWPORT_W/2 + I * (VIEWPORT_H/2),    AL_Center, _fonts.standard, rgb(1, 1, 1), 60, 85, 35, 35);

}

void stage_start_bgm(const char *bgm) {
	start_bgm(bgm);

	if(current_bgm.title && current_bgm.started_at >= 0) {
		char txt[strlen(current_bgm.title) + 6];
		snprintf(txt, sizeof(txt), "BGM: %s", current_bgm.title);
		stagetext_add(txt, VIEWPORT_W-15 + I * (VIEWPORT_H-20), AL_Right, _fonts.standard, rgb(1, 1, 1), 30, 85, 35, 35);
	}
}

typedef struct StageFrameState {
	StageInfo *stage;
	int transition_delay;
	uint16_t last_replay_fps;
} StageFrameState;

static bool stage_fpslimit_condition(void *arg) {
	return (global.replaymode != REPLAY_PLAY || !gamekeypressed(KEY_SKIP)) && !global.frameskip;
}

static bool stage_frame(void *arg) {
	StageFrameState *fstate = arg;
	StageInfo *stage = fstate->stage;

	((global.replaymode == REPLAY_PLAY) ? replay_input : stage_input)();

	if(global.game_over != GAMEOVER_TRANSITIONING) {
		if((!global.boss || boss_is_fleeing(global.boss)) && !global.dialog) {
			stage->procs->event();
		}

		if(stage->type == STAGE_SPELL && !global.boss && global.game_over != GAMEOVER_RESTART) {
			stage_finish(GAMEOVER_WIN);
			fstate->transition_delay = 60;
		}

		stage->procs->update();
	}

	if(!global.timer && stage->type != STAGE_SPELL) {
		// must be done here to let the event function start a BGM first
		display_stage_title(stage);
	}

	replay_stage_check_desync(global.replay_stage, global.frames, (tsrand() ^ global.plr.points) & 0xFFFF, global.replaymode);
	stage_logic();

	if(global.replaymode == REPLAY_RECORD && global.plr.points > progress.hiscore) {
		progress.hiscore = global.plr.points;
	}

	if(fstate->transition_delay) {
		--fstate->transition_delay;
	}

	if(global.frameskip && global.frames % global.frameskip) {
		if(!fstate->transition_delay) {
			update_transition();
		}

		return true;
	}

	tsrand_lock(&global.rand_game);
	tsrand_switch(&global.rand_visual);
	BEGIN_DRAW_CODE();
	stage_draw_scene(stage);
	END_DRAW_CODE();
	tsrand_unlock(&global.rand_game);
	tsrand_switch(&global.rand_game);

	draw_transition();

	if(!fstate->transition_delay) {
		update_transition();
	}

	SDL_GL_SwapWindow(video.window);

	fpscounter_update(&global.fps);
	fpscounter_update(&global.fps_busy);

	if(global.replaymode == REPLAY_RECORD) {
		uint16_t replay_fps = (uint16_t)rint(global.fps.fps);

		if(replay_fps != fstate->last_replay_fps) {
			replay_stage_event(global.replay_stage, global.frames, EV_FPS, replay_fps);
			fstate->last_replay_fps = replay_fps;
		}
	}

	return global.game_over <= 0;
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

	stage_objpools_alloc();
	stage_preload();
	stage_draw_preload();

	uint32_t seed = (uint32_t)time(0);
	tsrand_switch(&global.rand_game);
	tsrand_seed_p(&global.rand_game, seed);
	stage_start(stage);

	if(global.replaymode == REPLAY_RECORD) {
		if(config_get_int(CONFIG_SAVE_RPY)) {
			global.replay_stage = replay_create_stage(&global.replay, stage, seed, global.diff, &global.plr);

			// make sure our player state is consistent with what goes into the replay
			player_init(&global.plr);
			replay_stage_sync_player_state(global.replay_stage, &global.plr);
		} else {
			global.replay_stage = NULL;
		}

		log_debug("Random seed: %u", seed);

		StageProgress *p = stage_get_progress_from_info(stage, global.diff, true);

		if(p) {
			log_debug("You played this stage %u times", p->num_played);
			log_debug("You cleared this stage %u times", p->num_cleared);

			++p->num_played;

			if(!global.plr.continues_used) {
				p->unlocked = true;
			}
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

	player_stage_post_init(&global.plr);
	stage->procs->begin();

	StageFrameState fstate = { .stage = stage };
	fpscounter_reset(&global.fps);
	loop_at_fps(stage_frame, stage_fpslimit_condition, &fstate, FPS);

	if(global.replaymode == REPLAY_RECORD) {
		replay_stage_event(global.replay_stage, global.frames, EV_OVER, 0);

		if(global.game_over == GAMEOVER_WIN) {
			global.replay_stage->flags |= REPLAY_SFLAG_CLEAR;
		}
	}

	stage->procs->end();
	stage_free();
	player_free(&global.plr);
	tsrand_switch(&global.rand_visual);
	free_all_refs();
	stage_objpools_free();
	stop_sounds();
}
