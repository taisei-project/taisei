/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "eventloop_private.h"

#include "global.h"
#include "util/env.h"

#define SLEEP_DEBUG 0

#if SLEEP_DEBUG
	#undef SLEEP_DEBUG
	#define SLEEP_DEBUG(...) log_debug(__VA_ARGS__)
#else
	#undef SLEEP_DEBUG
	#define SLEEP_DEBUG(...) (void)0
#endif

void eventloop_run(void) {
	assert(thread_current_is_main());

	if(evloop.stack_ptr == NULL) {
		return;
	}

	LoopFrame *frame = evloop.stack_ptr;
	evloop.frame_times.target = frame->frametime;
	evloop.frame_times.start = time_get();
	evloop.frame_times.next = evloop.frame_times.start + evloop.frame_times.target;

	bool sleep_enabled = env_get("TAISEI_FRAMELIMITER_SLEEP", true);
	bool compensate = env_get("TAISEI_FRAMELIMITER_COMPENSATE", true);
	bool uncapped_rendering_env, uncapped_rendering;
	shrtime_t sleep_margin = 0;

	if(global.is_replay_verification) {
		uncapped_rendering_env = false;
	} else {
		uncapped_rendering_env = env_get("TAISEI_FRAMELIMITER_LOGIC_ONLY", false);
	}

	uncapped_rendering = uncapped_rendering_env;
	uint32_t frame_num = 0;

begin_main_loop:
	while(frame != NULL) {

#ifdef DEBUG
		if(uncapped_rendering_env) {
			uncapped_rendering = !gamekeypressed(KEY_FPSLIMIT_OFF);
		}
#endif

		evloop.frame_times.start = time_get();

		attr_unused shrtime_t error = (shrtime_t)evloop.frame_times.start - (shrtime_t)evloop.frame_times.next;
		SLEEP_DEBUG("Error: %lli", (long long)error);

begin_frame:
		global.fps.busy.last_update_time = time_get();
		evloop.frame_times.target = frame->frametime;
		++frame_num;

		LogicFrameAction lframe_action = LFRAME_WAIT;

		if(uncapped_rendering) {
			attr_unused uint32_t logic_frames = 0;

			while(lframe_action != LFRAME_STOP && evloop.frame_times.next < evloop.frame_times.start) {
				lframe_action = handle_logic(&frame, &evloop.frame_times);

				if(!frame || lframe_action == LFRAME_STOP) {
					goto begin_main_loop;
				}

				++logic_frames;
				hrtime_t total = time_get() - evloop.frame_times.start;

				if(total > evloop.frame_times.target) {
					evloop.frame_times.next = evloop.frame_times.start;
					log_debug("Executing logic took too long (%"PRIuTIME"), giving up", total);
				} else {
					evloop.frame_times.next += evloop.frame_times.target;
				}
			}

			if(logic_frames > 1) {
				log_debug(
					"Dropped %u logic frame%s in superframe #%u",
					logic_frames - 1,
					logic_frames > 2 ? "s" : "",
					frame_num
				);
			}
		} else {
			lframe_action = handle_logic(&frame, &evloop.frame_times);

			if(!frame || lframe_action == LFRAME_STOP) {
				goto begin_main_loop;
			}
		}

		if((uncapped_rendering || !(frame_num % get_effective_frameskip())) && !global.is_replay_verification) {
			run_render_frame(frame);
		}

		fpscounter_update(&global.fps.busy);

		if(uncapped_rendering || global.frameskip > 0 || global.is_replay_verification) {
			continue;
		}

#ifdef DEBUG
		if(gamekeypressed(KEY_FPSLIMIT_OFF)) {
			continue;
		}
#endif

		evloop.frame_times.next = evloop.frame_times.start + evloop.frame_times.target;

		if(compensate) {
			hrtime_t rt = time_get();

			if(rt > evloop.frame_times.next) {
				// frame took too long...
				// try to compensate in the next frame to avoid slowdown
				hrtime_t adjustment = min(rt - evloop.frame_times.next, evloop.frame_times.target);
				log_debug("Frame took too long, next target adjusted by %lluns (~%llums)",
					(unsigned long long)adjustment, (unsigned long long)SDL_NS_TO_MS(adjustment));
				evloop.frame_times.start = rt - adjustment;
				goto begin_frame;
			}
		}

		if(sleep_enabled) {
			shrtime_t remaining_time = (shrtime_t)evloop.frame_times.next - (shrtime_t)time_get();
			shrtime_t sleep = max(remaining_time - sleep_margin, 0);

			if(sleep > 0) {
				SDL_DelayNS(sleep);

				shrtime_t old_remaining_time = remaining_time;
				remaining_time = (shrtime_t)evloop.frame_times.next - (shrtime_t)time_get();
				shrtime_t slept = old_remaining_time - remaining_time;
				shrtime_t overshoot = slept - sleep;
				shrtime_t min_margin = overshoot * 2;

				SLEEP_DEBUG("Sleep: %lli intended  /  %lli actual", (long long)sleep, (long long)slept);

				if(min_margin > sleep_margin) {
					SLEEP_DEBUG("Adjusting minimum sleep margin: %lli --> %lli",
						(long long)sleep_margin, (long long)min_margin
					);
					sleep_margin = min_margin;
				}

				SLEEP_DEBUG("margin: %ld", sleep_margin);
				SLEEP_DEBUG("overshoot: %ld", overshoot);
			}

			sleep_margin -= (sleep_margin >> 10);
		}

		while(time_get() < evloop.frame_times.next);
	}
}
