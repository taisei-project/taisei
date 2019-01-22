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
	hrtime_t frametime = HRTIME_RESOLUTION / FPS;
	const int log_size = sizeof(fps->frametimes)/sizeof(hrtime_t);

	for(int i = 0; i < log_size; ++i) {
		fps->frametimes[i] = frametime;
	}

	fps->fps = HRTIME_RESOLUTION / (long double)frametime;
	fps->frametime = frametime;
	fps->last_update_time = time_get();
}

void fpscounter_update(FPSCounter *fps) {
	const int log_size = sizeof(fps->frametimes)/sizeof(hrtime_t);
	double frametime = time_get() - fps->last_update_time;

	memmove(fps->frametimes, fps->frametimes + 1, (log_size - 1) * sizeof(hrtime_t));
	fps->frametimes[log_size - 1] = frametime;

	hrtime_t avg = 0;

	for(int i = 0; i < log_size; ++i) {
		avg += fps->frametimes[i];
	}

	fps->fps = HRTIME_RESOLUTION / (avg / (long double)log_size);
	fps->frametime = avg / log_size;
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
	hrtime_t target_frame_time = HRTIME_RESOLUTION / fps;

	FrameAction rframe_action = RFRAME_SWAP;
	FrameAction lframe_action = LFRAME_WAIT;

	int32_t sleep = env_get("TAISEI_FRAMELIMITER_SLEEP", 3);
	bool compensate = env_get("TAISEI_FRAMELIMITER_COMPENSATE", 1);
	bool uncapped_rendering_env = env_get("TAISEI_FRAMELIMITER_LOGIC_ONLY", 0);

	if(global.is_replay_verification) {
		uncapped_rendering_env = false;
	}

	uint32_t frame_num = 0;

	// don't care about thread safety, we can render only on the main thread anyway
	static uint8_t recursion_detector;
	++recursion_detector;

	while(true) {
		bool uncapped_rendering = uncapped_rendering_env;
		frame_start_time = time_get();

begin_frame:

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
					frametime /= imax(1, config_get_int(CONFIG_SKIP_SPEED));
				}

				next_frame_time += frametime;

				hrtime_t total = time_get() - frame_start_time;

				if(total > target_frame_time) {
					next_frame_time = frame_start_time;
					log_debug("Executing logic took too long (%"PRIuTIME"), giving up", total);
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
			uint cnt = 0;

			do {
				lframe_action = logic_frame(arg);
				fpscounter_update(&global.fps.logic);
			} while(lframe_action == LFRAME_SKIP && ++cnt < config_get_int(CONFIG_SKIP_SPEED));
		}

		if(taisei_quit_requested()) {
			break;
		}

		if((!uncapped_rendering && frame_num % get_effective_frameskip()) || global.is_replay_verification) {
			rframe_action = RFRAME_DROP;
		} else {
			r_framebuffer_clear(NULL, CLEAR_ALL, RGBA(0, 0, 0, 1), 1);
			rframe_action = render_frame(arg);
			fpscounter_update(&global.fps.render);
		}

		if(rframe_action == RFRAME_SWAP) {
			video_swap_buffers();
		}

		if(lframe_action == LFRAME_STOP) {
			break;
		}

		fpscounter_update(&global.fps.busy);

		if(uncapped_rendering || global.frameskip > 0) {
			continue;
		}

#ifdef DEBUG
		if(gamekeypressed(KEY_FPSLIMIT_OFF)) {
			continue;
		}
#endif

		next_frame_time = frame_start_time + target_frame_time;
		// next_frame_time = frame_start_time + 2 * target_frame_time - global.fps.logic.frametime;

		if(compensate) {
			hrtime_t rt = time_get();

			if(rt > next_frame_time) {
				// frame took too long...
				// try to compensate in the next frame to avoid slowdown
				frame_start_time = rt - imin(rt - next_frame_time, target_frame_time);
				goto begin_frame;
			}
		}

		if(sleep > 0) {
			// CAUTION: All of these casts are important!
			while((shrtime_t)next_frame_time - (shrtime_t)time_get() > (shrtime_t)target_frame_time / sleep) {
				uint32_t nap_multiplier = 1;
				uint32_t nap_divisor = 3;
				hrtime_t nap_raw = imax(0, (shrtime_t)next_frame_time - (shrtime_t)time_get());
				uint32_t nap_sdl = (nap_multiplier * nap_raw * 1000) / (HRTIME_RESOLUTION * nap_divisor);
				nap_sdl = imax(nap_sdl, 1);
				SDL_Delay(nap_sdl);
			}
		}

		while(time_get() < next_frame_time);
	}
}
