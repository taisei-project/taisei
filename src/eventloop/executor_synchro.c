/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "eventloop_private.h"

#include "global.h"
#include "util/env.h"

void eventloop_run(void) {
	assert(thread_current_is_main());

	if(evloop.stack_ptr == NULL) {
		return;
	}

	LoopFrame *frame = evloop.stack_ptr;
	evloop.frame_times.target = frame->frametime;
	evloop.frame_times.start = time_get();
	evloop.frame_times.next = evloop.frame_times.start + evloop.frame_times.target;
	shrtime_t sleep_divisor = env_get("TAISEI_FRAMELIMITER_SLEEP", 3);
	shrtime_t max_sleep = sleep_divisor > 0 ? (shrtime_t)evloop.frame_times.target / sleep_divisor : 0;
	bool compensate = env_get("TAISEI_FRAMELIMITER_COMPENSATE", 1);
	bool uncapped_rendering_env, uncapped_rendering;

	if(global.is_replay_verification) {
		uncapped_rendering_env = false;
	} else {
		uncapped_rendering_env = env_get("TAISEI_FRAMELIMITER_LOGIC_ONLY", 0);
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
				evloop.frame_times.start = rt - min(rt - evloop.frame_times.next, evloop.frame_times.target);
				goto begin_frame;
			}
		}

		SDL_DelayPrecise(clamp((shrtime_t)time_get() - (shrtime_t)evloop.frame_times.next, 0, max_sleep));
		while(time_get() < evloop.frame_times.next);
	}
}
