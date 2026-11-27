/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "replay.h"

#include "gamesession.h"

typedef struct ReplayContext {
	CallChain next;
	GameSession game;
} ReplayContext;

static void replay_play_finished(CallChainResult ccr) {
	ReplayContext *ctx = ccr.ctx;
	CallChain next = ctx->next;
	game_session_deinit(&ctx->game);
	mem_free(ctx);
	run_call_chain(&next, NULL);
}

void replay_play(Replay *rpy, int firstidx, ReplayPlayFlags flags, Replay *rerecord, CallChain next) {
	auto ctx = ALLOC(ReplayContext, { .next = next });

	bool ok = game_session_init_replay(&ctx->game, &(ReplayPlaybackSettings) {
		.replay = rpy,
		.first_stage_idx = firstidx,
		.flags = flags,
		.replay_out = rerecord,
	});

	if(!ok) {
		mem_free(ctx);
		run_call_chain(&next, NULL);
		return;
	}

	game_session_enter(&ctx->game, CALLCHAIN(replay_play_finished, ctx));
}
