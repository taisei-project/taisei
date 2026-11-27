/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "gamesession.h"

#include "credits.h"
#include "log.h"
#include "menu/savereplay.h"
#include "replay/struct.h"
#include "stage.h"

static void game_session_common_init(GameSession *game) {
	*game = (GameSession) {};
	res_group_init(&game->rg);
}

bool game_session_init(GameSession *game, const GameSettings *settings) {
	PlayerMode *plrmode = plrmode_find(settings->player_char, settings->player_shot);

	if(!plrmode) {
		log_error("Invalid player mode %i/%i", settings->player_char, settings->player_shot);
		return false;
	}

	game_session_common_init(game);
	game->plr.mode = plrmode;
	game->diff = settings->difficulty;
	game->is_practice_mode = settings->practice_mode;
	game->init_stage = NOT_NULL(settings->init_stage);

	if(settings->replay_out) {
		replay_state_init_record(&game->replay.output, settings->replay_out);
	}

	return true;
}

bool game_session_init_replay(GameSession *game, const ReplayPlaybackSettings *settings) {
	Replay *rpy = NOT_NULL(settings->replay);
	int first = settings->first_stage_idx;

	if(first < 0 || first >= rpy->stages.num_elements) {
		log_error("No stage #%i in the replay", first);
		return false;
	}

	game_session_common_init(game);

	// initialized properly later
	game->replay.input.replay = rpy;
	game->replay.input.stage = NOT_NULL(dynarray_get_ptr(&game->replay.input.replay->stages, first));

	game->replay.first_stage_idx = first;
	game->replay.play_flags = settings->flags;

	if(settings->replay_out) {
		replay_state_init_record(&game->replay.output, settings->replay_out);
	}

	return true;
}

void game_session_deinit(GameSession *game) {
	replay_state_deinit(&game->replay.input);
	replay_state_deinit(&game->replay.output);
	res_group_release(&game->rg);
	*game = (GameSession) {};
}

static void game_session_over(CallChainResult ccr) {
	GameSession *game = ccr.ctx;
	run_call_chain(&game->next, game);
}

static void game_session_show_credits(CallChainResult ccr) {
	credits_enter(CALLCHAIN(game_session_over, ccr.ctx));
}

static void game_session_show_ending(CallChainResult ccr) {
	GameSession *game = ccr.ctx;
	Player *plr = &game->plr;
	PlayerCharacter *character = plr->mode->character;
	PlrCharEndingCutscene *ec_id;

	if(plr->stats.total.continues_used) {
		ec_id = &character->ending.bad;
	} else {
		ec_id = &character->ending.good;
	}

	progress_track_ending(ec_id->ending_id);
	cutscene_enter(CALLCHAIN(game_session_show_credits, game), ec_id->cutscene_id);
}

static void game_session_finish(GameSession *game) {
	CallChain cc = CALLCHAIN(game_session_over, game);

	if(game->gameover == GAMEOVER_WIN && game_is_story_mode(game) && !game_is_replay_playback(game)) {
		res_group_release(&game->rg);
		res_purge();
		credits_preload(&game->rg);
		cc = CALLCHAIN(game_session_show_ending, game);
	}

	if(game->replay.input.replay || !game->replay.output.replay) {
		run_call_chain(&cc, NULL);
	} else {
		ask_save_replay(game->replay.output.replay, cc);
	}
}

static void game_session_enter_stage(GameSession *game, StageInfo *stage, CallChain next) {
	game->gameover = GAMEOVER_NONE;
	res_group_release(&game->rg);
	stage_enter(game, stage, &game->rg, next);
}

static void game_session_live_reset(GameSession *game) {
	if(game->replay.output.replay) {
		replay_reset(game->replay.output.replay);
		replay_state_init_record(&game->replay.output, game->replay.output.replay);
	}

	PlayerMode *mode = NOT_NULL(game->plr.mode);
	player_init(&game->plr);
	stats_init(&game->plr.stats);
	game->plr.mode = mode;
	game->stage.info = NOT_NULL(game->init_stage);
}

static void game_session_post_stage(CallChainResult ccr);

static void game_session_live_next_stage(GameSession *game) {
	game_session_enter_stage(game, NOT_NULL(game->stage.info), CALLCHAIN(game_session_post_stage, game));
}

static void game_session_live_post_stage(GameSession *game) {
	if(game->gameover == GAMEOVER_RESTART_STAGE) {
		game_session_live_next_stage(game);
		return;
	}

	if(game->gameover == GAMEOVER_RESTART_GAME) {
		game_session_live_reset(game);
		game_session_live_next_stage(game);
		return;
	}

	if(game->gameover == GAMEOVER_WIN && game_is_story_mode(game)) {
		if(game->stage.info[1].type == STAGE_STORY) {
			// It's important to only increment this conditionally, hence the funny info[1] check
			// otherwise game_is_story_mode() becomes false after we've ran out of stages
			++game->stage.info;
			game_session_live_next_stage(game);
			return;
		}
	}

	game_session_finish(game);
}

static void game_session_replay_next_stage(GameSession *game) {
	Replay *rpy = NOT_NULL(game->replay.input.replay);
	ReplayStage *rstg = NOT_NULL(game->replay.input.stage);
	StageInfo *stage = NULL;

	assert(rstg >= rpy->stages.data && rstg <= rpy->stages.data + rpy->stages.num_elements);

	while(rstg < rpy->stages.data + rpy->stages.num_elements) {
		if((stage = stageinfo_get_by_id(rstg->stage))) {
			break;
		}

		log_warn("Invalid stage %X in replay at %zi skipped", rstg->stage, rstg - rpy->stages.data);
		rstg = ++game->replay.input.stage;
	}

	if(!stage) {
		game_session_finish(game);
		return;
	}

	if(!(game->plr.mode = plrmode_find(rstg->plr_char, rstg->plr_shot))) {
		log_warn("Stage stage %X in replay at %zi skipped: invalid player mode %i/%i",
			rstg->stage, rstg - rpy->stages.data, rstg->plr_char, rstg->plr_shot);

		++game->replay.input.stage;
		game_session_replay_next_stage(game);
		return;
	}

	replay_state_init_play(&game->replay.input, rpy, rstg);
	game->replay.input.play.demo_mode = game->replay.play_flags & REPLAY_PLAY_DEMO;
	game->is_practice_mode = (rstg->flags & REPLAY_SFLAG_PRACTICE) || stage->type == STAGE_SPELL;
	game_session_enter_stage(game, stage, CALLCHAIN(game_session_post_stage, game));
}

static void game_session_replay_post_stage(GameSession *game) {
	if(game->gameover == GAMEOVER_ABORT) {
		game_session_finish(game);
		return;
	}

	if(game->gameover == GAMEOVER_RESTART_GAME) {
		int idx = game->replay.first_stage_idx;
		game->replay.input.stage = NOT_NULL(dynarray_get_ptr(&game->replay.input.replay->stages, idx));
	} else if(game->gameover != GAMEOVER_RESTART_STAGE) {
		++game->replay.input.stage;
	}

	game_session_replay_next_stage(game);
}

static void game_session_post_stage(CallChainResult ccr) {
	GameSession *game = ccr.ctx;

	if(game_is_replay_playback(game)) {
		game_session_replay_post_stage(game);
	} else {
		game_session_live_post_stage(game);
	}
}

void game_session_enter(GameSession *game, CallChain next) {
	game->next = next;

	if(game_is_replay_playback(game)) {
		game_session_replay_next_stage(game);
	} else {
		game_session_live_reset(game);
		game_session_live_next_stage(game);
	}
}
