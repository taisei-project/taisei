/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "scuttle.h"

#include "common_tasks.h"
#include "i18n/i18n.h"

void stagex_draw_scuttle_spellbg(Boss *h, int time) {
	// TODO: this is a quick placeholder based on her stage 3 BG; it looks okay but something more elaborate might be warranted

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
	r_uniform_float("t", 100-time/400.);
	r_uniform_float("decay", 0.);
	r_uniform_vec2("plrpos", re(h->pos) / VIEWPORT_W, im(h->pos) / VIEWPORT_H);
	fill_viewport(0.0, 0.0, 1, "stagex/bg");

	r_shader_standard();
	r_color4(1, 1, 1, 1);
}

Boss *stagex_spawn_scuttle(cmplx pos0) {
	Boss *scuttle = create_boss(N_("Scutƫle"), "scuttle", pos0);
	boss_set_portrait(scuttle, "scuttle", NULL, "normal");
	scuttle->shadowcolor = RGBA(0.5, 0.0, 0.22, 1);
	scuttle->glowcolor = RGBA(0.30, 0.0, 0.12, 0);
	scuttle->zoomcolor = RGB(0.4, 0.1, 0.4);
	return scuttle;
}
