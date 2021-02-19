/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "kurumi.h"

#include "global.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

static void kurumi_global_rule(Boss *b, int time) {
	// FIXME: avoid running this every frame!

	if(b->current && ATTACK_IS_SPELL(b->current->type)) {
		b->shadowcolor = *RGBA_MUL_ALPHA(0.0, 0.4, 0.5, 0.5);
	} else {
		b->shadowcolor = *RGBA_MUL_ALPHA(1.0, 0.1, 0.0, 0.5);
	}
}

Boss *stage4_spawn_kurumi(cmplx pos) {
	Boss* b = create_boss("Kurumi", "kurumi", pos);
	boss_set_portrait(b, "kurumi", NULL, "normal");
	b->glowcolor = *RGB(0.5, 0.1, 0.0);
	b->global_rule = kurumi_global_rule;
	return b;
}

void kurumi_slave_visual(Enemy *e, int t, bool render) {
	if(render) {
		return;
	}

	if(!(t%2)) {
		cmplx offset = (frand()-0.5)*30;
		offset += (frand()-0.5)*20.0*I;
		PARTICLE(
			.sprite = "smoothdot",
			.pos = offset,
			.color = RGBA(0.3, 0.0, 0.0, 0.0),
			.draw_rule = Shrink,
			.rule = enemy_flare,
			.timeout = 50,
			.args = { (-50.0*I-offset)/50.0, add_ref(e) },
			.flags = PFLAG_REQUIREDPARTICLE,
		);
	}
}

void kurumi_slave_static_visual(Enemy *e, int t, bool render) {
	if(render) {
		return;
	}

	if(e->args[1]) {
		PARTICLE(
			.sprite = "smoothdot",
			.pos = e->pos,
			.color = RGBA(1, 1, 1, 0),
			.draw_rule = Fade,
			.timeout = 30,
		);
	}
}

void kurumi_spell_bg(Boss *b, int time) {
	float f = 0.5+0.5*sin(time/80.0);

	r_mat_mv_push();
	r_mat_mv_translate(VIEWPORT_W/2, VIEWPORT_H/2,0);
	r_mat_mv_scale(0.6, 0.6, 1);
	r_color3(f, 1 - f, 1 - f);
	draw_sprite(0, 0, "stage4/kurumibg1");
	r_mat_mv_pop();
	r_color4(1, 1, 1, 0);
	fill_viewport(time/300.0, time/300.0, 0.5, "stage4/kurumibg2");
	r_color4(1, 1, 1, 1);
}

int kurumi_splitcard(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	if(t == creal(p->args[2])) {
		p->args[0] += p->args[3];
		p->color.b *= -1;
		play_sound_ex("redirect", 10, false);
		spawn_projectile_highlight_effect(p);
	}

	return asymptotic(p, t);
}
