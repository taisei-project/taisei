/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "framerate.h"
#include "global.h"
#include "video.h"

void fpscounter_reset(FPSCounter *fps) {
	hrtime_t frametime = 1.0 / FPS;
	const int log_size = sizeof(fps->frametimes)/sizeof(hrtime_t);

	for(int i = 0; i < log_size; ++i) {
		fps->frametimes[i] = frametime;
	}

	fps->fps = 1.0 / frametime;
	fps->last_update_time = time_get();
}

void fpscounter_update(FPSCounter *fps) {
	const int log_size = sizeof(fps->frametimes)/sizeof(hrtime_t);
	double frametime = time_get() - fps->last_update_time;

	memmove(fps->frametimes, fps->frametimes + 1, (log_size - 1) * sizeof(hrtime_t));
	fps->frametimes[log_size - 1] = frametime;

	hrtime_t avg = 0.0;

	for(int i = 0; i < log_size; ++i) {
		avg += fps->frametimes[i];
	}

	fps->fps = 1.0 / (avg / log_size);
	fps->last_update_time = time_get();
}

uint32_t get_effective_frameskip(void) {
	uint32_t frameskip;

	if(global.frameskip > 0) {
		frameskip = global.frameskip;
	} else {
		frameskip = config_get_int(CONFIG_VID_FRAMESKIP);
	}

	return frameskip;
}

void loop_at_fps(LogicFrameFunc logic_frame, RenderFrameFunc render_frame, void *arg, uint32_t fps) {
	assert(logic_frame != NULL);
	assert(render_frame != NULL);
	assert(fps > 0);

	hrtime_t frame_start_time = time_get();
	hrtime_t next_frame_time = frame_start_time;
	hrtime_t target_frame_time = ((hrtime_t)1.0) / fps;

	FrameAction rframe_action = RFRAME_SWAP;
	FrameAction lframe_action = LFRAME_WAIT;

	int32_t delay = getenvint("TAISEI_FRAMELIMITER_SLEEP", 0);
	bool exact_delay = getenvint("TAISEI_FRAMELIMITER_SLEEP_EXACT", 1);
	bool compensate = getenvint("TAISEI_FRAMELIMITER_COMPENSATE", 1);
	bool uncapped_rendering_env = getenvint("TAISEI_FRAMELIMITER_LOGIC_ONLY", 0);
	bool late_swap = config_get_int(CONFIG_VID_LATE_SWAP);

	uint32_t frame_num = 0;

	// don't care about thread safety, we can render only on the main thread anyway
	static uint8_t recursion_detector;
	++recursion_detector;

#ifdef SPAM_FPS
	hrtime_t frametimes[4096];
	int frametimes_idx = 0;
#endif

	while(true) {
		bool uncapped_rendering = uncapped_rendering_env;
		frame_start_time = time_get();

begin_frame:

#ifdef DEBUG
		if(gamekeypressed(KEY_FPSLIMIT_OFF)) {
			uncapped_rendering = false;
		} else {
			uncapped_rendering = uncapped_rendering_env;
		}
#endif

		if(late_swap && rframe_action == RFRAME_SWAP) {
			video_swap_buffers();
		}

		global.fps.busy.last_update_time = time_get();

		++frame_num;

		if(uncapped_rendering) {
			uint32_t logic_frames = 0;

			while(lframe_action != LFRAME_STOP && next_frame_time < frame_start_time) {
				uint8_t rval = recursion_detector;

				lframe_action = logic_frame(arg);
				fpscounter_update(&global.fps.logic);
				++logic_frames;

				if(rval != recursion_detector) {
					log_debug(
						"Recursive call detected (%u != %u), resetting next frame time to avoid a large skip",
						rval,
						recursion_detector
					);
					next_frame_time = time_get() + target_frame_time;
					break;
				}

				hrtime_t frametime = target_frame_time;

				if(lframe_action == LFRAME_SKIP) {
					frametime *= 0.1;
				}

				next_frame_time += frametime;

				hrtime_t total = time_get() - frame_start_time;

				if(total > target_frame_time) {
					next_frame_time = frame_start_time;
					log_debug("Executing logic took too long (%f), giving up", (double)total);
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
			lframe_action = logic_frame(arg);
			fpscounter_update(&global.fps.logic);
		}

		if(!uncapped_rendering && frame_num % get_effective_frameskip()) {
			rframe_action = RFRAME_DROP;
		} else {
			rframe_action = render_frame(arg);
			fpscounter_update(&global.fps.render);

#ifdef SPAM_FPS
			frametimes[frametimes_idx++] = *global.fps.render.frametimes;
			size_t s = sizeof(frametimes)/sizeof(*frametimes);

			if(frametimes_idx == s) {
				hrtime_t total = 0;

				for(int i = 0; i < s; ++i) {
					total += frametimes[i];
				}

				frametimes_idx = 0;
				log_info("%zi frames in %.2fs = %.2f FPS", s, (double)total, (double)(1 / (total / s)));
			}
#endif
		}

		if(lframe_action == LFRAME_STOP) {
			break;
		}

		if(!late_swap && rframe_action == RFRAME_SWAP) {
			video_swap_buffers();
		}

		fpscounter_update(&global.fps.busy);

		if(lframe_action == LFRAME_SKIP || uncapped_rendering) {
			continue;
		}

#ifdef DEBUG
		if(gamekeypressed(KEY_FPSLIMIT_OFF)) {
			continue;
		}
#endif

		next_frame_time = frame_start_time + target_frame_time;

		if(compensate) {
			hrtime_t rt = time_get();
			hrtime_t diff = rt - next_frame_time;

			if(diff >= 0) {
				// frame took too long...
				// try to compensate in the next frame to avoid slowdown
				frame_start_time = rt - min(diff, target_frame_time);
				goto begin_frame;
			}
		}

		if(delay > 0) {
			int32_t realdelay = delay;
			int32_t maxdelay = (int32_t)(1000 * (next_frame_time - time_get()));

			if(realdelay > maxdelay) {
				if(exact_delay) {
					log_debug("Delay of %i ignored. Maximum is %i, TAISEI_FRAMELIMITER_SLEEP_EXACT is active", realdelay, maxdelay);
					realdelay = 0;
				} else {
					log_debug("Delay reduced from %i to %i", realdelay, maxdelay);
					realdelay = maxdelay;
				}
			}

			if(realdelay > 0) {
				SDL_Delay(realdelay);
			}
		}

		while(time_get() < next_frame_time) {
			continue;
		}
	}
}
