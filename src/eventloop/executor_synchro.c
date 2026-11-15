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
#include "video.h"

typedef enum PacingMode {
	PMODE_UNDEFINED = -1,
	PMODE_NONE,      // no limiter: run as fast as possible
	PMODE_TIMER,     // sync to wall clock time, VRR recommended
	PMODE_DISPLAY,   // sync to display refresh rate

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
		case PMODE_DISPLAY:   return "display";
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

#define UNLAGGER_WINDOW				 			20
#define UNLAGGER_HOLD_FRAMES		 			120
#define UNLAGGER_DELAY_BACKOFF		 			SDL_US_TO_NS(400)
#define UNLAGGER_DELAY_GAIN			 			0.1
#define UNLAGGER_DELAY_TARGET_SLACK_MIN			SDL_US_TO_NS(500)
#define UNLAGGER_DELAY_TARGET_SLACK_MAX			SDL_US_TO_NS(3000)
#define UNLAGGER_DELAY_TARGET_SLACK_BACKOFF		SDL_US_TO_NS(100)
#define UNLAGGER_DELAY_TARGET_SLACK_RECOVER		0.0005

typedef struct Unlagger {
	hrtime_t delay;
	hrtime_t min_block_time;
	hrtime_t max_block_time;
	hrtime_t target_slack;
	uint hold_frames;
	uint window_frames;
	uint misses;
} Unlagger;

// SDL's reported refresh rate can fucking lie apparently, e.g. gamescope on steam deck always reports something north
// of ~90 Hz even if the real rate is different (changed by the menu slider). Which means we have to do this annoying
// shit to estimate the real rate, thank Valve love you too.

#define RATE_ESTIMATOR_MAX_SPREAD	0.015
#define RATE_ESTIMATOR_MIN_BLOCK	SDL_US_TO_NS(100)

typedef struct RateEstimator {
	hrtime_t samples[67];  //  SIX SEVEN!!!! SIX SEVEEEEN!!!!!!!!!! XDDDDDDDDDDDDDDDDDDDDDDDDXDDDXDDDDDDDDDD!!!!!!!!
	hrtime_t threshold;
	uint num_samples;
	uint next_sample;
} RateEstimator;

typedef struct DisplayPacer {
	hrtime_t last_sync_time;
	hrtime_t vblank_period;
	IntRatio display_rate;
	IntRatio paced_rate;
	uint64_t accum;
	uint64_t raw_accum;
	uint logic_rate;
	uint nonblocking_vsync_streak;
	RateEstimator rate_estimator;
	Unlagger unlagger;
} DisplayPacer;

static void rate_estimator_add_sample(RateEstimator *e, hrtime_t sample) {
	e->samples[e->next_sample] = sample;
	e->next_sample = (e->next_sample + 1) % countof(e->samples);
	e->num_samples = min(e->num_samples + 1, countof(e->samples));
}

static int hrtime_cmp(const void *pa, const void *pb) {
	hrtime_t a = *(const hrtime_t*)pa;
	hrtime_t b = *(const hrtime_t*)pb;
	return (a > b) - (a < b);
}

static hrtime_t rate_estimator_median_period(const RateEstimator *e) {
	if(e->num_samples < countof(e->samples)) {
		return 0;
	}

	hrtime_t sorted[countof(e->samples)];
	memcpy(sorted, e->samples, sizeof(sorted));
	qsort(sorted, countof(sorted), sizeof(*sorted), hrtime_cmp);

	hrtime_t q1 = sorted[countof(e->samples) / 4];
	hrtime_t q2 = sorted[countof(e->samples) / 2];
	hrtime_t q3 = sorted[countof(e->samples) * 3 / 4];

	attr_unused auto spread = (double)(q3 - q1)/q2;
	bool ok = q3 - q1 <= q2 * RATE_ESTIMATOR_MAX_SPREAD;

	debug_text("Median: %0.2f Hz  q3~q1: %.02f Hz ~ %.02f Hz  spread: %.05f %s",
		HRTIME_RESOLUTION / (double)q2,
		HRTIME_RESOLUTION / (double)q3,
		HRTIME_RESOLUTION / (double)q1,
		spread, ok ? "(good)" : "(bad)"
	);

	if(!ok) {
		debug_text("%zu > %g (spread: %0.8f)", q3 - q1, q2 * RATE_ESTIMATOR_MAX_SPREAD, spread);
		// too much noise; rate possibly just changed
		return 0;
	}

	return q2;
}

static IntRatio snap_refresh_rate(IntRatio refresh_rate, uint logic_rate) {
	if(UNLIKELY(refresh_rate.num <= 0)) {
		return refresh_rate;
	}

	// Pick a rate to count vblanks at for display pacing.
	// We take the exact display refresh rate and, if it's close enough, snap it to the closest
	// multiple of the logic rate. This is necessary to run smoothly on odd refresh rates like
	// ~59.94 Hz, which are unfortunately quite common. The tradeoff is that the game speed will
	// be off by a tiny amount. This is basically imperceptible, but music sync will drift
	// slightly over long stages. That could be dealt with by resampling the audio, but that's
	// probably overkill. Buy a VRR monitor if you want the best of all worlds.

	// Maximum game speed deviation fraction.
	// The weirdest mode on my monitor refreshes at ~60.32 Hz, which is ~0.5% too fast.
	// This is roughly ~double that amount.
	const double tolerance = 0.01;

	double hz = int_ratio_to_double(refresh_rate);
	int vblanks_per_tick = lround(hz / logic_rate);

	if(vblanks_per_tick >= 1 && fabs(hz / (vblanks_per_tick * logic_rate) - 1) <= tolerance) {
		return (IntRatio) { vblanks_per_tick * logic_rate, 1 };
	}

	int ticks_per_vblank = lround(logic_rate / hz);

	if(ticks_per_vblank >= 2 && fabs(logic_rate / (ticks_per_vblank * hz) - 1) <= tolerance) {
		return (IntRatio) { logic_rate, ticks_per_vblank };
	}

	// enjoy your stutterfest
	return refresh_rate;
}

static hrtime_t rate_to_period(IntRatio rate) {
	return HRTIME_RESOLUTION * rate.den / rate.num;
}

static double periods_mismatch(hrtime_t a, hrtime_t b) {
	return fabs((double)a / (double)b - 1.0);
}

static DisplayPacer display_pacer_init(uint logic_rate) {
	// We'll be optimistic and trust the reported rate initially, if it's known.
	// We'll still measure it in the background and adjust it if it differs significantly.
	// The measurement is not always reliable, sometimes the iqr spread is too high and it just never converges.
	// This can happen in windowed mode on my Linux Wayland system (niri), although fullscreen converges fine (probably
	// when the game gets unredirected by the compositor, which is what you want).
	// On most setups the reported rate should, hopefully, be correct, so a failed measurement won't be a problem.
	IntRatio display_rate = video_get_refresh_rate();

	return (DisplayPacer) {
		.last_sync_time = time_get(),
		.logic_rate = logic_rate,
		.unlagger.min_block_time = INT64_MAX,
		.rate_estimator.threshold = RATE_ESTIMATOR_MIN_BLOCK,
		.display_rate = display_rate,
		.paced_rate = snap_refresh_rate(display_rate, logic_rate),
		.vblank_period = display_rate.den > 0 ? rate_to_period(display_rate) : 0,
	};
}

static void display_pacer_reset(DisplayPacer *p, uint logic_rate) {
	p->last_sync_time = time_get();

	if(p->logic_rate != logic_rate && p->vblank_period) {
		p->paced_rate = snap_refresh_rate(p->display_rate, logic_rate);
	}

	p->logic_rate = logic_rate;
	p->accum = 0;
	p->raw_accum = 0;
	p->unlagger.delay = 0;
	p->unlagger.hold_frames = 0;
	p->unlagger.window_frames = 0;
}

static bool display_pacer_update_rate(DisplayPacer *p) {
	hrtime_t measured_period = rate_estimator_median_period(&p->rate_estimator);

	if(!measured_period) {
		// not enough data, keep the last known rate for now
		return p->vblank_period > 0;
	}

	double snap_tolerance = 0.01;
	double drift_tolerance = 0.015;

	if(p->vblank_period > 0) {
		auto mismatch = periods_mismatch(measured_period, p->vblank_period);
		debug_text("Rate mismatch: %0.04f  /  %0.04f", mismatch, drift_tolerance);
		if(mismatch <= drift_tolerance) {
			return true;
		}
	}

	IntRatio reported_rate = video_get_refresh_rate();
	IntRatio rate;

	if(reported_rate.num > 0 && periods_mismatch(measured_period, rate_to_period(reported_rate)) <= snap_tolerance) {
		// close enough, trust the reported rate
		rate = reported_rate;
	} else {
		// fucking liar
		rate = (IntRatio) { lround(1e12 / measured_period), 1000 };
	}

	if(int_ratio_equals(p->display_rate, rate)) {
		return true;
	}

	p->display_rate = rate;
	p->paced_rate = snap_refresh_rate(rate, p->logic_rate);
	p->vblank_period = rate_to_period(rate);
	p->unlagger = (Unlagger) {
		.min_block_time = INT64_MAX,
		.target_slack = p->unlagger.target_slack,
	};
	p->accum = 0;

	log_info("Display refresh rate: reported %g Hz, measured %g Hz; pacing as if %g Hz",
		int_ratio_to_double(reported_rate),
		(double)HRTIME_RESOLUTION / measured_period,
		int_ratio_to_double(p->paced_rate)
	);

	return true;
}

static void display_pacer_unlagger_update(
	DisplayPacer *p, WaitState *wait,
	uint vblanks, hrtime_t blocked_time, hrtime_t prev_work_time, bool late
) {
	// Here we play chicken with the vblank interval.
	// Try to delay doing any work as much as possible so we can land the frame just in
	// time for the refresh. This reduces input latency, because the input gets sampled
	// much closer to the actual time the frame gets displayed on the screen. Otherwise
	// backpressure builds up and we get at least one full frame of latency. See also the
	// comment in eventloop_run() below.

	auto u = &p->unlagger;
	u->min_block_time = min(u->min_block_time, blocked_time);
	u->max_block_time = max(u->max_block_time, blocked_time);
	u->target_slack = max(u->target_slack, UNLAGGER_DELAY_TARGET_SLACK_MIN);

	const hrtime_t min_work_time = SDL_US_TO_NS(500);

	if(late) {
		u->delay = max(0, (shrtime_t)u->delay - (shrtime_t)UNLAGGER_DELAY_BACKOFF);
		u->target_slack = min(u->target_slack + UNLAGGER_DELAY_TARGET_SLACK_BACKOFF, UNLAGGER_DELAY_TARGET_SLACK_MAX);
		u->hold_frames = UNLAGGER_HOLD_FRAMES;
		++u->misses;
	} else if(++u->window_frames == UNLAGGER_WINDOW) {
		if(!u->hold_frames) {
			// shrtime_t t = lerp(u->min_block_time, u->max_block_time, 0.05);
			shrtime_t t = u->min_block_time;

			auto gain = t - (shrtime_t)u->target_slack;
			gain *= UNLAGGER_DELAY_GAIN;

			if(LIKELY(min_work_time < p->vblank_period)) {
				u->delay = clamp((shrtime_t)u->delay + gain, 0, (shrtime_t)(p->vblank_period - min_work_time));
			} else {
				u->delay = 0;
			}
		}

		u->min_block_time = INT64_MAX;
		u->max_block_time = 0;
		u->window_frames = 0;
	}

	if(u->hold_frames) {
		--u->hold_frames;
	} else {
		u->target_slack = lerp(u->target_slack, UNLAGGER_DELAY_TARGET_SLACK_MIN, UNLAGGER_DELAY_TARGET_SLACK_RECOVER);
	}

	debug_text("Unlagger delay: %.2fms  %zuns", SDL_NS_TO_MS((double)u->delay), u->delay);
	debug_text("Unlagger min block: %.2fms", SDL_NS_TO_MS((double)u->min_block_time));
	debug_text("Unlagger slack: %.2fms", SDL_NS_TO_MS((double)u->target_slack));
	debug_text("Unlagger window: %i / %i", u->window_frames, UNLAGGER_WINDOW);
	debug_text("Unlagger hold: %i / %i", u->hold_frames, UNLAGGER_HOLD_FRAMES);
	debug_text("Unlagger total misses: %i", u->misses);
	debug_text("Prev work time: %.2fms  (avg: %.2fms)",
		SDL_NS_TO_MS((double)prev_work_time), SDL_NS_TO_MS((double)global.fps.busy.frametime));

	if(u->delay) {
		wait_until(wait, p->last_sync_time + u->delay);
	}
}

static void display_pacer_lag_report(const DisplayPacer *p, hrtime_t elapsed) {
	attr_unused double lag_frames = (elapsed * p->logic_rate) / (double)HRTIME_RESOLUTION;
	log_debug("Lagged by %g frames (limit %d), not catching up", lag_frames, MAX_LAG_FRAMES);
}

static uint display_pacer_update(
	DisplayPacer *p, WaitState *wait, hrtime_t blocked_time, hrtime_t prev_work_time
) {
	hrtime_t now = time_get();
	hrtime_t elapsed = now - p->last_sync_time;
	p->last_sync_time = now;

	bool blocked = blocked_time >= p->rate_estimator.threshold;

	if(blocked && !p->nonblocking_vsync_streak) {
		rate_estimator_add_sample(&p->rate_estimator, elapsed);
		debug_text("Vsync blocked: YES");
	} else {
		debug_text("Vsync blocked: NO");
	}

	if(blocked) {
		p->nonblocking_vsync_streak = 0;
	} else {
		++p->nonblocking_vsync_streak;
	}

	debug_text("Non-blocking vsyncs: %u", p->nonblocking_vsync_streak);

	// TODO: detect non-blocking presentation and sleep
	// Can happen when e.g. the window is minimized or the driver is dogshit

	if(!display_pacer_update_rate(p)) {
		debug_text("Display: measuring refresh rate (%u/%zu)",
			p->rate_estimator.num_samples, countof(p->rate_estimator.samples));

		p->raw_accum += elapsed * p->logic_rate;
		auto ticks = p->raw_accum / HRTIME_RESOLUTION;

		if(ticks > MAX_LAG_FRAMES) {
			display_pacer_lag_report(p, elapsed);
			p->raw_accum = 0;
			return 1;
		}

		p->raw_accum -= ticks * HRTIME_RESOLUTION;
		return ticks;
	}

	debug_text("Display: %g Hz, pacing: %g Hz",
		int_ratio_to_double(p->display_rate), int_ratio_to_double(p->paced_rate));
	debug_text("elapsed/vblank: %0.2f", (double)elapsed / p->vblank_period);

	auto vblanks = (elapsed + p->vblank_period / 2) / p->vblank_period;

	p->accum += vblanks * p->logic_rate * p->paced_rate.den;
	auto ticks = p->accum / p->paced_rate.num;

	if(ticks > MAX_LAG_FRAMES) {
		display_pacer_lag_report(p, elapsed);
		p->accum = 0;
		ticks = 1;
	} else {
		p->accum -= ticks * p->paced_rate.num;
	}

	const hrtime_t jitter = SDL_US_TO_NS(1000);
	bool late = (elapsed > p->vblank_period + jitter || vblanks > 1) && prev_work_time < p->vblank_period;
	display_pacer_unlagger_update(p, wait, vblanks, blocked_time, prev_work_time, late);
	p->rate_estimator.threshold = p->unlagger.target_slack;// - (p->unlagger.target_slack >> 2);

	return ticks;
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

	if(r_vsync_current() != VSYNC_NORMAL) {
		// we live in magical la-la-land where the driver surely wouldn't lie to us
		return PMODE_TIMER;
	}

	IntRatio refresh_rate = video_get_refresh_rate();

	if(refresh_rate.num <= 0) {
		return PMODE_TIMER;
	}

	return PMODE_DISPLAY;
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
	DisplayPacer display_pacer;

	uint frame_num = 0;

	hrtime_t prev_work_time = 0;	// times only what happens in the logic and render callbacks
	hrtime_t prev_present_start_time = time_get();

	hrtime_t scene_start_time = 0;
	uint scene_logic_ticks = 0;

	display_pacer = display_pacer_init(evloop.frame_times.target_fps);
	bool want_low_latency = true;

	while(frame != NULL) {
		// In VSYNC mode, this call may block to wait for a swapchain.
		// Alternatively, r_present_frame (called at the very end, after rendering) may block instead.
		// We don't care which one of them blocks, because they are right next to each other in the cycle.
		// What's important is that all waits happen before the logic, immediately followed by rendering.
		r_await_frame(want_low_latency);

		// Measure how much time we just spent blocked on vsync, for the display pacer.
		// With vsync, every presented frame goes into a FIFO queue of N images. N can be as low as 2 if we're lucky,
		// but usually it's 3 or higher. We only get blocked when the queue is full, and the time we spent blocked
		// is essentially a measure of input lag, which we want to minimize.
		auto blocked_time = time_get() - prev_present_start_time;

		debug_text_clear();

		// debug_text("  r_await_frame time: %.02fms", SDL_NS_TO_MS((double)await_time));
		// debug_text("r_present_frame time: %.02fms", SDL_NS_TO_MS((double)prev_present_time));
		debug_text("Await+present block time: %.02fms", SDL_NS_TO_MS((double)blocked_time));

		pace_mode = decide_pacing_mode(forced_pace_mode);

		debug_text("Vsync: %s (want: %s)", vsync_names[r_vsync_current()], vsync_names[config_get_int(CONFIG_VSYNC)]);
		debug_text("Pacing mode: %s", pacing_mode_name(pace_mode));
		debug_text("FPS:  L:%.2f  R:%.2f", global.fps.logic.fps, global.fps.render.fps);

		hrtime_t next_deadline;
		uint logic_ticks = 1;

		if(pace_mode != prev_pace_mode || evloop.frame_times.target_fps != frame->target_fps) {
			evloop.frame_times.target_fps = frame->target_fps;

			timer_pacer = timer_pacer_init(evloop.frame_times.target_fps);
			display_pacer_reset(&display_pacer, evloop.frame_times.target_fps);

			next_deadline = timer_pacer_deadline(&timer_pacer, 1);
			prev_pace_mode = pace_mode;
		} else if(pace_mode == PMODE_TIMER) {
			next_deadline = timer_pacer_update(&timer_pacer, &wait_state);
		} else if(pace_mode == PMODE_DISPLAY) {
			logic_ticks = display_pacer_update(&display_pacer, &wait_state, blocked_time, prev_work_time);
			next_deadline = display_pacer.last_sync_time + HRTIME_RESOLUTION / evloop.frame_times.target_fps;
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

		if(logic_ticks == 0) {
			assert(pace_mode == PMODE_DISPLAY);

			// We're running at an integer multiple of the logic FPS.
			// We don't have interpolation yet, so we have nothing new to draw,
			// but we have to present the previous frame again to maintain vsync.

			// TODO: this may fail, handle it somehow.
			//
			// NOTE: rendering the same frame again would have minor side-effects, because a few effects use feedback
			// buffers and expect a constant draw rate. This is not *super* important, and we already ignore this
			// in frameskip mode. So we have 3 options, from most to least desirable:
			//
			// 	1. Split feedback operations into a separate pass that runs at logic rate.
			//  2. Just render the same frame again if repeat fails; accept the overhead and slight correctness issues.
			//  3. Ignore the failure let them fuck up the timing.
			//
			// Coincidentally, this is also in descending order of implementation effort, so we do 3 for now.
			//
			// TODO: Also make CONFIG_VID_FRAMESKIP take this path. It's not the same as global.frameskip, we still
			// want the FPS limiter and frame pacing there.
			prev_present_start_time = time_get();
			r_present_frame();
			continue;
		}

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
			prev_present_start_time = time_get();
			prev_work_time = prev_present_start_time - work_begin_time;
			continue;
		}

		if(should_render_frame(frame_num++)) {
			run_render_frame(frame);

			hrtime_t work_end_time = time_get();
			fpscounter_update(&global.fps.busy);

			prev_work_time = work_end_time - work_begin_time;
			prev_present_start_time = work_end_time;

			r_present_frame();
			fpscounter_update(&global.fps.render);
		} else {
			prev_present_start_time = time_get();
			prev_work_time = prev_present_start_time - work_begin_time;
			fpscounter_update(&global.fps.busy);
		}
	}
}

// get me the fuck out of this file holy shit what a rabbit hole
