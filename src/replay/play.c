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
} ReplayContext;

static void replay_do_cleanup(CallChainResult ccr);
static void replay_do_play(CallChainResult ccr);
static void replay_do_post_play(CallChainResult ccr);

void replay_play(Replay *rpy, int firstidx, CallChain next) {
	if(firstidx >= rpy->numstages || firstidx < 0) {
		log_error("No stage #%i in the replay", firstidx);
		run_call_chain(&next, NULL);
		return;
	}

	ReplayContext *ctx = calloc(1, sizeof(*ctx));
	ctx->cc = next;
	ctx->rpy = rpy;
	ctx->stage_idx = firstidx;

	replay_do_play(CALLCHAIN_RESULT(ctx, NULL));
}

static void replay_do_play(CallChainResult ccr) {
	ReplayContext *ctx = ccr.ctx;
	ReplayStage *rstg = NULL;
	StageInfo *stginfo = NULL;
	Replay *rpy = ctx->rpy;

	while(ctx->stage_idx < rpy->numstages) {
		rstg = rpy->stages + ctx->stage_idx++;
		stginfo = stageinfo_get_by_id(rstg->stage);

		if(!stginfo) {
			log_warn("Invalid stage %X in replay at %i skipped.", rstg->stage, ctx->stage_idx);
			continue;
		}

		break;
	}

	if(stginfo == NULL) {
		replay_do_cleanup(ccr);
	} else {
		replay_state_init_play(&global.replay.input, rpy, rstg);
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
	free_resources(false);

	CallChain cc = ctx->cc;
	free(ctx);
	run_call_chain(&cc, NULL);
}
