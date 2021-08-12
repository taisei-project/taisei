/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "hirestime.h"

typedef enum RenderFrameAction {
	RFRAME_SWAP,
	RFRAME_DROP,
} RenderFrameAction;

typedef enum LogicFrameAction {
	LFRAME_WAIT,
	LFRAME_SKIP,
	LFRAME_SKIP_ALWAYS,
	LFRAME_STOP,
} LogicFrameAction;

typedef LogicFrameAction (*LogicFrameFunc)(void *context);
typedef RenderFrameAction (*RenderFrameFunc)(void *context);
typedef void (*PostLoopFunc)(void *context);

typedef struct FrameTimes {
	hrtime_t target;
	hrtime_t start;
	hrtime_t next;
} FrameTimes;

#ifdef DEBUG_CALLCHAIN
	#include "util/debug.h"
	#include "log.h"
#endif

typedef struct CallChainResult {
	void *ctx;
	void *result;
} CallChainResult;

typedef struct CallChain {
	void (*callback)(CallChainResult result);
	void *ctx;
#ifdef DEBUG_CALLCHAIN
	DebugInfo _debug_;
#endif
} CallChain;

#ifdef DEBUG_CALLCHAIN
	#define CALLCHAIN(callback, ctx) ((CallChain) { (callback), (ctx), _DEBUG_INFO_INITIALIZER_ })
#else
	#define CALLCHAIN(callback, ctx) ((CallChain) { (callback), (ctx) })
#endif

#define NO_CALLCHAIN CALLCHAIN(NULL, NULL)

#define CALLCHAIN_RESULT(ctx, result) ((CallChainResult) { (ctx), (result) })

void eventloop_enter(
	void *context,
	LogicFrameFunc frame_logic,
	RenderFrameFunc frame_render,
	PostLoopFunc on_leave,
	uint target_fps
) attr_nonnull(1, 2, 3);

void eventloop_run(void);

FrameTimes eventloop_get_frame_times(void);

#ifdef DEBUG_CALLCHAIN
INLINE attr_nonnull(1) void run_call_chain(CallChain *cc, void *result, DebugInfo caller_dbg) {
	if(cc->callback != NULL) {
		log_debug("Calling CC set in %s (%s:%u)",
			cc->_debug_.func, cc->_debug_.file, cc->_debug_.line);
		log_debug("             from %s (%s:%u)",
			caller_dbg.func, caller_dbg.file, caller_dbg.line);
		cc->callback(CALLCHAIN_RESULT(cc->ctx, result));
	} else {
		log_debug("Dead end at %s (%s:%u)",
			caller_dbg.func, caller_dbg.file, caller_dbg.line
		);
	}
}

#define run_call_chain(cc, result) run_call_chain(cc, result, _DEBUG_INFO_)
#else
INLINE attr_nonnull(1) void run_call_chain(CallChain *cc, void *result) {
	if(cc->callback != NULL) {
		cc->callback(CALLCHAIN_RESULT(cc->ctx, result));
	}
}
#endif
