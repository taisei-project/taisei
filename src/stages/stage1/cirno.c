/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "cirno.h"

#include "common_tasks.h"

void stage1_draw_cirno_spellbg(Boss *c, int time) {
	r_color4(0.5, 0.5, 0.5, 1.0);
	fill_viewport(time/700.0, time/700.0, 1, "stage1/cirnobg");
	r_blend(BLEND_MOD);
	r_color4(0.7, 0.7, 0.7, 0.5);
	fill_viewport(-time/700.0 + 0.5, time/700.0+0.5, 0.4, "stage1/cirnobg");
	r_blend(BLEND_PREMUL_ALPHA);
	r_color4(0.35, 0.35, 0.35, 0.0);
	fill_viewport(0, -time/100.0, 0, "stage1/snowlayer");
	r_color4(1.0, 1.0, 1.0, 1.0);
}

Boss *stage1_spawn_cirno(cmplx pos) {
	Boss *cirno = create_boss(_("Cirno"), "cirno", pos);
	boss_set_portrait(cirno, "cirno", NULL, "normal");
	cirno->shadowcolor = *RGBA_MUL_ALPHA(0.6, 0.7, 1.0, 0.25);
	cirno->glowcolor = *RGB(0.2, 0.35, 0.5);
	return cirno;
}

void stage1_cirno_wander(Boss *boss, real dist, real lower_bound) {
	Rect bounds = viewport_bounds(64);
	bounds.bottom = lower_bound;
	boss->move.attraction_point = common_wander(boss->pos, dist, bounds);
}
