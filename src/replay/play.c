/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "replay.h"
#include "struct.h"
#include "state.h"

#include "stageinfo.h"
#include "../stage.h"
#include "global.h"

typedef struct ReplayContext {
	CallChain cc;
	Replay *rpy;
	int stage_idx;
	bool demo_mode;
} ReplayContext;

static void replay_do_cleanup(CallChainResult ccr);
static void replay_do_play(CallChainResult ccr);
static void replay_do_post_play(CallChainResult ccr);

void replay_play(Replay *rpy, int firstidx, bool demo_mode, CallChain next) {
	if(firstidx >= rpy->stages.num_elements || firstidx < 0) {
		log_error("No stage #%i in the replay", firstidx);
		run_call_chain(&next, NULL);
		return;
	}

	replay_do_play(CALLCHAIN_RESULT(ALLOC(ReplayContext, {
		.cc = next,
		.rpy = rpy,
		.stage_idx = firstidx,
		.demo_mode = demo_mode,
	}), NULL));
}

static void replay_do_play(CallChainResult ccr) {
	ReplayContext *ctx = ccr.ctx;
	ReplayStage *rstg = NULL;
	StageInfo *stginfo = NULL;
	Replay *rpy = ctx->rpy;

	while(ctx->stage_idx < rpy->stages.num_elements) {
		rstg = dynarray_get_ptr(&rpy->stages, ctx->stage_idx++);
		stginfo = stageinfo_get_by_id(rstg->stage);

		if(!stginfo) {
			log_warn("Invalid stage %X in replay at %i skipped.", rstg->stage, ctx->stage_idx);
			continue;
		}

		break;
	};

	if(stginfo == NULL) {
		replay_do_cleanup(ccr);
	} else {
		assume(rstg != NULL);
		replay_state_init_play(&global.replay.input, rpy, rstg);
		global.replay.input.play.demo_mode = ctx->demo_mode;
		replay_state_deinit(&global.replay.output);
		global.plr.mode = plrmode_find(rstg->plr_char, rstg->plr_shot);
		stage_enter(stginfo, CALLCHAIN(replay_do_post_play, ctx));
	}
}

static void replay_do_post_play(CallChainResult ccr) {
	ReplayContext *ctx = ccr.ctx;

	if(global.gameover == GAMEOVER_ABORT) {
		replay_do_cleanup(ccr);
		return;
	}

	if(global.gameover == GAMEOVER_RESTART) {
		--ctx->stage_idx;
	}

	global.gameover = 0;
	replay_do_play(ccr);
}

static void replay_do_cleanup(CallChainResult ccr) {
	ReplayContext *ctx = ccr.ctx;

	global.gameover = 0;
	replay_state_deinit(&global.replay.input);
	res_unload_all(false);

	CallChain cc = ctx->cc;
	mem_free(ctx);
	run_call_chain(&cc, NULL);
}
