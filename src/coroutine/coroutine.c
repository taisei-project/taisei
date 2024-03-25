/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "internal.h"
#include "util.h"

void coroutines_init(void) {
	cotask_global_init();
}

void coroutines_shutdown(void) {
	cotask_global_shutdown();
}

#ifdef CO_TASK_STATS
#include "video.h"
#include "resource/font.h"
#endif

void coroutines_draw_stats(void) {
#ifdef CO_TASK_STATS
	if(STAT_VAL(num_tasks_in_use) == 0 && STAT_VAL(num_switches_this_frame) == 0) {
		return;
	}

	static char buf[128];

	TextParams tp = {
		.pos = { SCREEN_W },
		.color = RGB(1, 1, 1),
		.shader_ptr = res_shader("text_default"),
		.font_ptr = res_font("monotiny"),
		.align = ALIGN_RIGHT,
	};

	float ls = font_get_lineskip(tp.font_ptr);

	tp.pos.y += ls;

#ifdef CO_TASK_STATS_STACK
	snprintf(buf, sizeof(buf), "Peak stack: %zukb    Tasks: %4zu / %4zu ",
		STAT_VAL(peak_stack_usage) / 1024,
		STAT_VAL(num_tasks_in_use),
		STAT_VAL(num_tasks_allocated)
	);
#else
	snprintf(buf, sizeof(buf), "Tasks: %4zu / %4zu ",
		STAT_VAL(num_tasks_in_use),
		STAT_VAL(num_tasks_allocated)
	);
#endif

	text_draw(buf, &tp);

	tp.pos.y += ls;
	snprintf(buf, sizeof(buf), "Switches/frame: %4zu ", STAT_VAL(num_switches_this_frame));
	text_draw(buf, &tp);

	STAT_VAL_SET(num_switches_this_frame, 0);
#endif
}
