/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "scuttle.h"

#include "common_tasks.h"
#include "i18n/i18n.h"

void stage3_draw_scuttle_spellbg(Boss *h, int time) {
	float a = 1.0;

	if(time < 0)
		a += (time / (float)ATTACK_START_DELAY);
	float s = 0.3 + 0.7 * a;

	r_color4(0.1*a, 0.1*a, 0.1*a, a);
	draw_sprite(VIEWPORT_W/2, VIEWPORT_H/2, "stage3/spellbg2");
	fill_viewport(-time/200.0 + 0.5, time/400.0+0.5, s, "stage3/spellbg1");
	r_color4(0.1, 0.1, 0.1, 0);
	fill_viewport(time/300.0 + 0.5, -time/340.0+0.5, s*0.5, "stage3/spellbg1");
	r_shader("maristar_bombbg");
	r_uniform_float("t", time/400.);
	r_uniform_float("decay", 0.);
	r_uniform_vec2("plrpos", 0.5,0.5);
	fill_viewport(0.0, 0.0, 1, "stage3/spellbg1");

	r_shader_standard();
	r_color4(1, 1, 1, 1);
}

Boss *stage3_spawn_scuttle(cmplx pos) {
	Boss *scuttle = create_boss(_("Scuttle"), "scuttle", pos);
	boss_set_portrait(scuttle, "scuttle", NULL, "normal");
	scuttle->glowcolor = *RGB(0.5, 0.6, 0.3);
	scuttle->shadowcolor = *RGBA_MUL_ALPHA(0.7, 0.3, 0.1, 0.5);
	scuttle->zoomcolor = *RGB(0.4, 0.1, 0.4);
	return scuttle;
}
