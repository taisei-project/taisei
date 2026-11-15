/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "eventloop_private.h"

#include "debug_text.h"
#include "global.h"
#include "util/env.h"

typedef enum PacingMode {
	PMODE_UNDEFINED = -1,
	PMODE_NONE,      // no limiter: run as fast as possible
	PMODE_TIMER,     // sync to wall clock time, VRR recommended

	NUM_PMODES,
} PacingMode;

attr_unused static const char *vsync_names[] = {
	[VSYNC_NONE]     = "VSYNC_NONE",
	[VSYNC_NORMAL]   = "VSYNC_NORMAL",
	[VSYNC_ADAPTIVE] = "VSYNC_ADAPTIVE",
};

attr_unused static const char *pacing_mode_name(PacingMode mode) {
	switch(mode) {
		case PMODE_NONE:      return "none";
		case PMODE_TIMER:     return "timer";
		default:              UNREACHABLE;
	}
}

typedef struct WaitState  {
	shrtime_t sleep_margin;
	bool sleep_enabled;
} WaitState;

typedef struct TimerPacer {
	hrtime_t base;
	uint fps;
	uint frames;
} TimerPacer;

#define MAX_LAG_FRAMES 3

static void wait_until(WaitState *s, hrtime_t deadline) {
	if(s->sleep_enabled) {
		shrtime_t remaining = (shrtime_t)deadline - (shrtime_t)time_get();
		shrtime_t sleep = remaining - s->sleep_margin;

		if(sleep > 0) {
			SDL_DelayNS(sleep);

			shrtime_t slept = remaining - ((shrtime_t)deadline - (shrtime_t)time_get());
			shrtime_t min_margin = (slept - sleep) * 2;

			if(min_margin > s->sleep_margin) {
				s->sleep_margin = min_margin;
			}
		}

		s->sleep_margin -= s->sleep_margin >> 10;
	}

	while(time_get() < deadline);
}

static TimerPacer timer_pacer_init(uint fps) {
	return (TimerPacer) {
		.base = time_get(),
		.fps = fps,
		.frames = 0,
	};
}

static hrtime_t timer_pacer_deadline(const TimerPacer *p, uint frames) {
	// accumulating truncated HRTIME_RESOLUTION/p->fps would drift very slightly over time
	return p->base + (frames * HRTIME_RESOLUTION) / p->fps;
}

static hrtime_t timer_pacer_update(TimerPacer *p, WaitState *wait) {
	hrtime_t deadline = timer_pacer_deadline(p, p->frames + 1);
	hrtime_t now = time_get();

	if(now < deadline) {
		wait_until(wait, deadline);
		++p->frames;
	} else if(now - deadline > MAX_LAG_FRAMES * HRTIME_RESOLUTION / p->fps) {
		attr_unused double lag_frames = ((now - deadline) * p->fps) / (double)HRTIME_RESOLUTION;
		log_debug("Lagged by %g frames (limit %d), not catching up", lag_frames, MAX_LAG_FRAMES);
		*p = timer_pacer_init(p->fps);
	} else {
		++p->frames;
	}

	return timer_pacer_deadline(p, p->frames + 1);
}

static bool should_render_frame(uint frame_num) {
	return !global.is_replay_verification && (frame_num % get_effective_frameskip() == 0);
}

static LogicFrameAction run_logic(LoopFrame **frame, uint logic_ticks) {
	LogicFrameAction lframe_action = LFRAME_WAIT;
	LoopFrame *original_frame = *frame;

	for(uint i = 0; i < logic_ticks && *frame == original_frame; ++i) {
		// This handles LFRAME_SKIP and LFRAME_SKIP_ALWAYS internally
		lframe_action = handle_logic(frame, &evloop.frame_times);
		assert(*frame != NULL || lframe_action == LFRAME_STOP);

		if(lframe_action == LFRAME_STOP) {
			break;
		}
	}

	debug_text_commit();
	return lframe_action;
}

static PacingMode decide_pacing_mode(PacingMode forced_mode) {
#ifdef DEBUG
	if(gamekeypressed(KEY_FPSLIMIT_OFF)) {
		return PMODE_NONE;
	}
#endif

	if(global.frameskip > 0 || global.is_replay_verification) {
		return PMODE_NONE;
	}

	if(forced_mode != PMODE_UNDEFINED) {
		return forced_mode;
	}

	return PMODE_TIMER;
}

void eventloop_run(void) {
	assert(thread_current_is_main());

	if(evloop.stack_ptr == NULL) {
		return;
	}

	// TODO: rename LoopFrame, it's a horrible misnomer. It actually represents a continuous "scene"
	LoopFrame *frame = evloop.stack_ptr;

	evloop.frame_times.target_fps = frame->target_fps;
	evloop.frame_times.start = time_get();

	WaitState wait_state = {
		.sleep_enabled = env_get("TAISEI_FRAMELIMITER_SLEEP", true),
		.sleep_margin = HRTIME_RESOLUTION / (3 * evloop.frame_times.target_fps),
	};

	PacingMode forced_pace_mode = env_get("TAISEI_FRAMELIMITER_PACE_MODE", PMODE_UNDEFINED);

	if(forced_pace_mode < PMODE_UNDEFINED || forced_pace_mode >= NUM_PMODES) {
		forced_pace_mode = PMODE_UNDEFINED;
	}

	PacingMode pace_mode = forced_pace_mode;
	PacingMode prev_pace_mode = PMODE_UNDEFINED;
	TimerPacer timer_pacer;

	uint frame_num = 0;

	hrtime_t scene_start_time = 0;
	uint scene_logic_ticks = 0;

	bool want_low_latency = true;

	while(frame != NULL) {
		// In VSYNC mode, this call may block to wait for a swapchain.
		// Alternatively, r_present_frame (called at the very end, after rendering) may block instead.
		// We don't care which one of them blocks, because they are right next to each other in the cycle.
		// What's important is that all waits happen before the logic, immediately followed by rendering.
		r_await_frame(want_low_latency);

		debug_text_clear();

		pace_mode = decide_pacing_mode(forced_pace_mode);

		debug_text("Vsync: %s (want: %s)", vsync_names[r_vsync_current()], vsync_names[config_get_int(CONFIG_VSYNC)]);
		debug_text("Pacing mode: %s", pacing_mode_name(pace_mode));
		debug_text("FPS:  L:%.2f  R:%.2f", global.fps.logic.fps, global.fps.render.fps);

		hrtime_t next_deadline;
		uint logic_ticks = 1;

		if(pace_mode != prev_pace_mode || evloop.frame_times.target_fps != frame->target_fps) {
			evloop.frame_times.target_fps = frame->target_fps;

			timer_pacer = timer_pacer_init(evloop.frame_times.target_fps);

			next_deadline = timer_pacer_deadline(&timer_pacer, 1);
			prev_pace_mode = pace_mode;
		} else if(pace_mode == PMODE_TIMER) {
			next_deadline = timer_pacer_update(&timer_pacer, &wait_state);
		} else if(pace_mode == PMODE_NONE) {
			next_deadline = time_get() + HRTIME_RESOLUTION / evloop.frame_times.target_fps;
		} else {
			UNREACHABLE;
		}

		attr_unused auto prev_start = evloop.frame_times.start;
		evloop.frame_times.start = time_get();
		evloop.frame_times.next = next_deadline;

		assert(evloop.frame_times.start > prev_start);
		attr_unused auto elapsed = evloop.frame_times.start - prev_start;
		debug_text("Elapsed: %.02fms ~= %.2f FPS", SDL_NS_TO_MS((double)elapsed), (double)HRTIME_RESOLUTION / elapsed);

		hrtime_t work_begin_time = time_get();
		global.fps.busy.last_update_time = work_begin_time;

		if(!scene_start_time) {
			scene_start_time = work_begin_time;
		}

		debug_text_commit();

		auto prev_frame = frame;
		scene_logic_ticks += logic_ticks;
		LogicFrameAction laction = run_logic(&frame, logic_ticks);

		if(frame != prev_frame) {
			scene_start_time = work_begin_time;
			scene_logic_ticks = 1;
		}

		shrtime_t logic_time = scene_logic_ticks * HRTIME_RESOLUTION / evloop.frame_times.target_fps;
		shrtime_t actual_time = work_begin_time - scene_start_time;
		debug_text("Logic time: %.02fms", SDL_NS_TO_MS((double)(logic_time)));
		debug_text(" Real time: %.02fms", SDL_NS_TO_MS((double)(actual_time)));
		debug_text("Time drift: %.02fms", SDL_NS_TO_MS((double)(logic_time - actual_time)));

		if(laction == LFRAME_STOP) {
			continue;
		}

		if(should_render_frame(frame_num++)) {
			run_render_frame(frame);

			fpscounter_update(&global.fps.busy);
			r_present_frame();
			fpscounter_update(&global.fps.render);
		} else {
			fpscounter_update(&global.fps.busy);
		}
	}
}
