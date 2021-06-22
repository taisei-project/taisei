/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"
#include "../wriggle.h"

#include "common_tasks.h"
#include "global.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

DEPRECATED_DRAW_RULE
static void wriggle_fstorm_proj_draw(Projectile *p, int time, ProjDrawRuleArgs args) {
	float f = 1-fmin(time/60.0,1);
	r_mat_mv_push();
	r_mat_mv_translate(creal(p->pos), cimag(p->pos), 0);
	r_mat_mv_rotate(p->angle + M_PI/2, 0, 0, 1);
	ProjDrawCore(p, &p->color);

	if(f > 0) {
		// TODO: Maybe convert this into a particle effect?
		Sprite *s = p->sprite;
		Color c = p->color;
		c.a = 0;
		p->sprite = get_sprite("proj/ball");
		r_mat_mv_scale(f,f,f);
		ProjDrawCore(p, &c);
		p->sprite = s;
	}

	r_mat_mv_pop();
}

static int wriggle_fstorm_proj(Projectile *p, int time) {
	if(time < 0) {
		return ACTION_ACK;
	}

	if(cabs(global.plr.pos-p->pos) > 100) {
		p->args[2]+=1;
	} else {
		p->args[2]-=1;
		if(creal(p->args[2]) < 0)
			p->args[2] = 0;
	}

	int turntime = rint(creal(p->args[0]));
	int t = rint(creal(p->args[2]));
	if(t < turntime) {
		float f = t/(float)turntime;
		p->color = *RGB(0.3+0.7*(1 - pow(1 - f, 4)), 0.3+0.3*f*f, 0.7-0.7*f);
	}

	if(t == turntime && global.boss) {
		p->args[1] = global.boss->pos-p->pos;
		p->args[1] *= 2/cabs(p->args[1]);
		p->angle = carg(p->args[1]);
		p->birthtime = global.frames;
		p->draw_rule = (ProjDrawRule) { wriggle_fstorm_proj_draw };
		p->sprite = NULL;
		projectile_set_prototype(p, pp_rice);
		spawn_projectile_highlight_effect(p);

		for(int i = 0; i < 3; ++i) {
			tsrand_fill(2);
			PARTICLE(
				.sprite = "flare",
				.pos = p->pos,
				.rule = linear,
				.timeout = 60,
				.args = { (1+afrand(0))*cexp(I*tsrand_a(1)) },
				.draw_rule = Shrink,
			);
		}

		play_sound_ex("redirect", 3, false);
	}

	p->pos += p->args[1];
	return ACTION_NONE;
}

void wriggle_firefly_storm(Boss *boss, int time) {
	TIMER(&time)

	if(time < 0) {
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/2, 0.05)
		return;
	}

	bool lun = global.diff == D_Lunatic;

	AT(0) {
		aniplayer_queue(&boss->ani,"fly",0);
	}

	FROM_TO_SND("shot1_loop", 30, 9000, 2) {
		int i, cnt = 2;
		for(i = 0; i < cnt; ++i) {
			float r = tanh(sin(_i/200.));
			float v = lun ? cos(_i/150.)/pow(cosh(atanh(r)),2) : 0.5;
			cmplx pos = 230*cexp(I*(_i*0.301+2*M_PI/cnt*i))*r;

			PROJECTILE(
				.proto = (global.diff >= D_Hard) && !(i%10) ? pp_bigball : pp_ball,
				.pos = boss->pos+pos,
				.color = RGB(0.2,0.2,0.6),
				.rule = wriggle_fstorm_proj,
				.args = {
					(global.diff == D_Easy) ? 40 : 100-25*(!lun)-20*(global.diff == D_Normal),
					cexp(I*(!lun)*0.6)*pos/cabs(pos)*(1+v)
				},
			);
		}
	}
}
