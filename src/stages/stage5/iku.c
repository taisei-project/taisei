/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "iku.h"

#include "stage5.h"

#include "common_tasks.h"

void lightning_particle(cmplx pos, int t) {
	if(!(t % 5)) {
		char *part = frand() > 0.5 ? "lightning0" : "lightning1";
		PARTICLE(
			.sprite = part,
			.pos = pos,
			.color = RGBA(1.0, 1.0, 1.0, 0.0),
			.timeout = 20,
			.draw_rule = Fade,
			.flags = PFLAG_REQUIREDPARTICLE,
			.angle = frand()*2*M_PI,
		);
	}
}

void iku_slave_visual(Enemy *e, int t, bool render) {
	if(render) {
		return;
	}

	cmplx offset = (frand()-0.5)*10 + (frand()-0.5)*10.0*I;

	if(e->args[2] && !(t % 5)) {
		lightning_particle(e->pos + 3*offset, t);
	}

	if(!(t % 3)) {
		float alpha = 1;

		if(!e->args[2]) {
			alpha *= 0.03;
		}

		Color *clr = RGBA_MUL_ALPHA(0.1*alpha, 0.1*alpha, 0.6*alpha, 0.5*alpha);
		clr->a = 0;

		PARTICLE(
			.sprite = "lightningball",
			.pos = 0,
			.color = clr,
			.draw_rule = Fade,
			.rule = enemy_flare,
			.timeout = 50,
			.args = { offset*0.1, add_ref(e) },
			.flags = PFLAG_REQUIREDPARTICLE,
		);
	}
}

void cloud_common(void) {
	tsrand_fill(4);
	float v = (afrand(2)+afrand(3))*0.5+1.0;

	PROJECTILE(
		// FIXME: add prototype, or shove it into the basic ones somehow,
		// or just replace this with some thing else
		.sprite_ptr = get_sprite("part/lightningball"),
		.size = 48 * (1+I),
		.collision_size = 21.6 * (1+I),

		.pos = VIEWPORT_W*afrand(0)-15.0*I,
		.color = RGBA_MUL_ALPHA(0.2, 0.0, 0.4, 0.6),
		.rule = accelerated,
		.args = {
			1-2*afrand(1)+v*I,
			-0.01*I
		},
		.shader = "sprite_default",
	);
}

int lightning_slave(Enemy *e, int t) {
	if(t < 0)
		return 1;
	if(t > 200)
		return ACTION_DESTROY;

	TIMER(&t);

	e->pos += e->args[0];

	FROM_TO(0,200,20)
		e->args[0] *= cexp(I * (0.25 + 0.25 * frand() * M_PI));

	FROM_TO(0, 200, 3)
		if(cabs(e->pos-global.plr.pos) > 60) {
			Color *clr = RGBA(1-1/(1+0.01*_i), 0.5-0.01*_i, 1, 0);

			Projectile *p = PROJECTILE(
				.proto = pp_wave,
				.pos = e->pos,
				.color = clr,
				.rule = asymptotic,
				.args = {
					0.75*e->args[0]/cabs(e->args[0])*I,
					10
				},
			);

			if(projectile_in_viewport(p)) {
				for(int i = 0; i < 3; ++i) {
					tsrand_fill(2);
					lightning_particle(p->pos + 5 * afrand(0) * cexp(I*M_PI*2*afrand(1)), 0);
				}

				play_sfx_ex("shot3", 0, false);
				// play_sound_ex("redirect", 0, true);
			}
		}

	return 1;
}

int zigzag_bullet(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	int l = 50;
	p->pos = p->pos0+(abs(((2*t)%l)-l/2)*I+t)*2*p->args[0];

	if(t%2 == 0) {
		PARTICLE(
			.sprite = "lightningball",
			.pos = p->pos,
			.color = RGBA(0.1, 0.1, 0.6, 0.0),
			.timeout = 15,
			.draw_rule = Fade,
		);
	}

	return ACTION_NONE;
}

static cmplx induction_bullet_traj(Projectile *p, float t) {
	return p->pos0 + p->args[0]*t*cexp(p->args[1]*t);
}

int induction_bullet(Projectile *p, int time) {
	if(time < 0) {
		return ACTION_ACK;
	}

	float t = time*sqrt(global.diff);

	if(global.diff > D_Normal && !p->args[2]) {
		t = time*0.6;
		t = 230-t;
		if(t < 0)
			return ACTION_DESTROY;
	}

	p->pos = induction_bullet_traj(p,t);

	if(time == 0) {
		// don't lerp; the spawn position is very different on hard/lunatic and would cause false hits
		p->prevpos = p->pos;
	}

	p->angle = carg(p->args[0]*cexp(p->args[1]*t)*(1+p->args[1]*t));
	return 1;
}

cmplx cathode_laser(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->shader = r_shader_get_optional("lasers/iku_cathode");
		return 0;
	}

	l->args[1] = I*cimag(l->args[1]);

	return l->pos + l->args[0]*t*cexp(l->args[1]*t);
}

