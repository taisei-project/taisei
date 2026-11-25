/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "framerate.h"
#include "global.h"
#include "video.h"

void fpscounter_reset(FPSCounter *fps, hrtime_t init_frametime, hrtime_t now) {
	const int log_size = ARRAY_SIZE(fps->frametimes);

	for(int i = 0; i < log_size; ++i) {
		fps->frametimes[i] = init_frametime;
	}

	fps->fps = HRTIME_RESOLUTION / (long double)init_frametime;
	fps->frametime = init_frametime;
	fps->last_update_time = now;
}

void fpscounter_update(FPSCounter *fps, hrtime_t when) {
	const int log_size = ARRAY_SIZE(fps->frametimes);
	hrtime_t update_time = when;
	hrtime_t frametime = update_time - fps->last_update_time;

	memmove(fps->frametimes, fps->frametimes + 1, (log_size - 1) * sizeof(hrtime_t));
	fps->frametimes[log_size - 1] = frametime;

	hrtime_t avg = 0;

	for(int i = 0; i < log_size; ++i) {
		avg += fps->frametimes[i];
	}

	fps->fps = HRTIME_RESOLUTION / (avg / (double)log_size);
	fps->frametime = avg / log_size;
	fps->last_update_time = update_time;
}
