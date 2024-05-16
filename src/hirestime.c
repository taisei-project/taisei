/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "util.h"
#include "hirestime.h"
#include "thread.h"

static bool use_hires;
static hrtime_t time_current;
static hrtime_t time_offset;
static uint64_t prev_hires_time;
static uint64_t prev_hires_freq;
static uint64_t fast_path_mul;

INLINE void set_freq(uint64_t freq) {
	prev_hires_freq = freq;
	lldiv_t d = lldiv(HRTIME_RESOLUTION, freq);
	fast_path_mul = d.quot * (d.rem == 0);
}

static void time_update(void) {
	bool retry;

	do {
		retry = false;

		uint64_t freq = SDL_GetPerformanceFrequency();
		uint64_t cntr = SDL_GetPerformanceCounter();

		if(freq != prev_hires_freq) {
			log_debug("High resolution timer frequency changed: was %"PRIu64", now %"PRIu64". Saved time offset: %"PRIuTIME"", prev_hires_freq, freq, time_offset);
			time_offset = time_current;
			set_freq(freq);
			prev_hires_time = SDL_GetPerformanceCounter();
			retry = true;
			continue;
		}

		hrtime_t time_new;

		if(fast_path_mul) {
			time_new = time_offset + (cntr - prev_hires_time) * fast_path_mul;
		} else {
			time_new = time_offset + umuldiv64(cntr - prev_hires_time, HRTIME_RESOLUTION, freq);
		}

		if(time_new < time_current) {
			log_warn("BUG: time went backwards. Was %"PRIuTIME", now %"PRIuTIME". Possible cause: your OS sucks spherical objects. Attempting to correct this...", time_current, time_new);
			time_offset = time_current;
			time_current = 0;
			prev_hires_time = SDL_GetPerformanceCounter();
			retry = true;
		} else {
			time_current = time_new;
		}
	} while(retry);
}

void time_init(void) {
	use_hires = env_get("TAISEI_HIRES_TIMER", 1);

	if(use_hires) {
		log_info("Using the system high resolution timer");
		prev_hires_time = SDL_GetPerformanceCounter();
		set_freq(SDL_GetPerformanceFrequency());
	} else {
		log_info("Not using the system high resolution timer: disabled by environment");
		return;
	}
}

void time_shutdown(void) {

}

hrtime_t time_get(void) {
	if(use_hires) {
		assert(thread_current_is_main());
		time_update();
		return time_current;
	}

	return SDL_GetTicks() * (HRTIME_RESOLUTION / 1000);
}
