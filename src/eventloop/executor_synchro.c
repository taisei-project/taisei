/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "eventloop_private.h"
#include "util.h"
#include "framerate.h"
#include "global.h"

void eventloop_run(void) {
	assert(is_main_thread());

	if(evloop.stack_ptr == NULL) {
		return;
	}

	LoopFrame *frame = evloop.stack_ptr;
	FrameTimes frame_times;
	frame_times.target = frame->frametime;
	frame_times.start = time_get();
	frame_times.next = frame_times.start + frame_times.target;
	int32_t sleep = env_get("TAISEI_FRAMELIMITER_SLEEP", 3);
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

		frame_times.start = time_get();

begin_frame:
		global.fps.busy.last_update_time = time_get();
		frame_times.target = frame->frametime;
		++frame_num;

		LogicFrameAction lframe_action = LFRAME_WAIT;

		if(uncapped_rendering) {
			attr_unused uint32_t logic_frames = 0;

			while(lframe_action != LFRAME_STOP && frame_times.next < frame_times.start) {
				lframe_action = handle_logic(&frame, &frame_times);

				if(!frame || lframe_action == LFRAME_STOP) {
					goto begin_main_loop;
				}

				++logic_frames;
				hrtime_t total = time_get() - frame_times.start;

				if(total > frame_times.target) {
					frame_times.next = frame_times.start;
					log_debug("Executing logic took too long (%"PRIuTIME"), giving up", total);
				} else {
					frame_times.next += frame_times.target;
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
			lframe_action = handle_logic(&frame, &frame_times);

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

		frame_times.next = frame_times.start + frame_times.target;

		if(compensate) {
			hrtime_t rt = time_get();

			if(rt > frame_times.next) {
				// frame took too long...
				// try to compensate in the next frame to avoid slowdown
				frame_times.start = rt - imin(rt - frame_times.next, frame_times.target);
				goto begin_frame;
			}
		}

		if(sleep > 0) {
			// CAUTION: All of these casts are important!
			while((shrtime_t)frame_times.next - (shrtime_t)time_get() > (shrtime_t)frame_times.target / sleep) {
				uint32_t nap_multiplier = 1;
				uint32_t nap_divisor = 3;
				hrtime_t nap_raw = imax(0, (shrtime_t)frame_times.next - (shrtime_t)time_get());
				uint32_t nap_sdl = (nap_multiplier * nap_raw * 1000) / (HRTIME_RESOLUTION * nap_divisor);
				nap_sdl = imax(nap_sdl, 1);
				SDL_Delay(nap_sdl);
			}
		}

		while(time_get() < frame_times.next);
	}
}
