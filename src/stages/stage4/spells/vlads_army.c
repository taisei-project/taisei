/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"
#include "../kurumi.h"

#include "global.h"


static void kurumi_extra_shield_pos(Enemy *e, int time) {
	double dst = 75 + 100 * fmax((60 - time) / 60.0, 0);
	double spd = cimag(e->args[0]) * fmin(time / 120.0, 1);
	e->args[0] += spd;
	e->pos = global.boss->pos + dst * cexp(I*creal(e->args[0]));
}

static bool kurumi_extra_shield_expire(Enemy *e, int time) {
	if(time > creal(e->args[1])) {
		e->hp = 0;
		return true;
	}

	return false;
}

static int kurumi_extra_dead_shield_proj(Projectile *p, int time) {
	if(time < 0) {
		return ACTION_ACK;
	}

	p->color = *color_lerp(
		RGBA(2.0, 0.0, 0.0, 0.0),
		RGBA(0.2, 0.1, 0.5, 0.0),
	fmin(time / 60.0f, 1.0f));

	return asymptotic(p, time);
}

static int kurumi_extra_dead_shield(Enemy *e, int time) {
	if(time < 0) {
		return 1;
	}

	if(!(time % 6)) {
		// complex dir = cexp(I*(M_PI * 0.5 * nfrand() + carg(global.plr.pos - e->pos)));
		// complex dir = cexp(I*(carg(global.plr.pos - e->pos)));
		cmplx dir = cexp(I*creal(e->args[0]));
		PROJECTILE(
			.proto = pp_rice,
			.pos = e->pos,
			.rule = kurumi_extra_dead_shield_proj,
			.args = { 2*dir, 10 }
		);
		play_sound("shot1");
	}

	time += cimag(e->args[1]);

	kurumi_extra_shield_pos(e, time);

	if(kurumi_extra_shield_expire(e, time)) {
		int cnt = 10;
		for(int i = 0; i < cnt; ++i) {
			cmplx dir = cexp(I*M_PI*2*i/(double)cnt);
			tsrand_fill(2);
			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos,
				.rule = kurumi_extra_dead_shield_proj,
				.args = { 1.5 * (1 + afrand(0)) * dir, 4 + anfrand(1) },
			);
		}

		play_sound("boom");
	}

	return 1;
}

static int kurumi_extra_shield(Enemy *e, int time) {
	if(time == EVENT_DEATH) {
		if(global.boss && !(global.gameover > 0) && !boss_is_dying(global.boss) && e->args[2] == 0) {
			create_enemy2c(e->pos, ENEMY_IMMUNE, kurumi_slave_visual, kurumi_extra_dead_shield, e->args[0], e->args[1]);
		}
		return 1;
	}

	if(time < 0) {
		return 1;
	}

	e->args[1] = creal(e->args[1]) + time*I;

	kurumi_extra_shield_pos(e, time);
	kurumi_extra_shield_expire(e, time);

	return 1;
}

static int kurumi_extra_bigfairy1(Enemy *e, int time) {
	if(time < 0) {
		return 1;
	}
	TIMER(&time);

	int escapetime = 400+4000*(global.diff == D_Lunatic);
	if(time < escapetime) {
		GO_TO(e, e->args[0], 0.02);
	} else  {
		GO_TO(e, e->args[0]-I*VIEWPORT_H,0.01)
	}

	FROM_TO(50,escapetime,60) {
		int count = 5;
		cmplx phase = cexp(I*2*M_PI*frand());
		for(int i = 0; i < count; i++) {
			cmplx arg = cexp(I*2*M_PI*i/count);
			if(global.diff == D_Lunatic)
				arg *= phase;
			create_lasercurve2c(e->pos, 20, 200, RGBA(1.0, 0.3, 0.7, 0.0), las_accel, arg, 0.1*arg);
			PROJECTILE(
				.proto = pp_bullet,
				.pos = e->pos,
				.color = RGB(1.0, 0.3, 0.7),
				.rule = accelerated,
				.args = { arg, 0.1*arg },
			);
		}

		play_sound("laser1");
	}

	return 1;
}

DEPRECATED_DRAW_RULE
static void kurumi_extra_drainer_draw(Projectile *p, int time, ProjDrawRuleArgs args) {
	cmplx org = p->pos;
	cmplx targ = p->args[1];
	double a = 0.5 * creal(p->args[2]);
	Texture *tex = r_texture_get("part/sinewave");

	r_shader_standard();

	r_color4(1.0 * a, 0.5 * a, 0.5 * a, 0);
	loop_tex_line_p(org, targ, 16, time * 0.01, tex);

	r_color4(0.4 * a, 0.2 * a, 0.2 * a, 0);
	loop_tex_line_p(org, targ, 18, time * 0.0043, tex);

	r_color4(0.5 * a, 0.2 * a, 0.5 * a, 0);
	loop_tex_line_p(org, targ, 24, time * 0.003, tex);
}

static int kurumi_extra_drainer(Projectile *p, int time) {
	if(time == EVENT_DEATH) {
		free_ref(p->args[0]);
		return ACTION_ACK;
	}

	if(time < 0) {
		return ACTION_ACK;
	}

	Enemy *e = REF(p->args[0]);
	p->pos = global.boss->pos;

	if(e) {
		p->args[1] = e->pos;
		p->args[2] = approach(p->args[2], 1, 0.5);

		if(time > 40 && e->hp > 0) {
			// TODO: maybe add a special sound for this?

			float drain = fmin(4, e->hp);
			ent_damage(&e->ent, &(DamageInfo) { .amount = drain });
			global.boss->current->hp = fmin(global.boss->current->maxhp, global.boss->current->hp + drain * 2);
		}
	} else {
		p->args[2] = approach(p->args[2], 0, 0.5);
		if(!creal(p->args[2])) {
			return ACTION_DESTROY;
		}
	}

	return ACTION_NONE;
}

static void kurumi_extra_create_drainer(Enemy *e) {
	PROJECTILE(
		.size = 1+I,
		.pos = e->pos,
		.rule = kurumi_extra_drainer,
		.draw_rule = kurumi_extra_drainer_draw,
		.args = { add_ref(e) },
		.shader = "sprite_default",
		.flags = PFLAG_NOCLEAR | PFLAG_NOCOLLISION,
	);
}

static void kurumi_extra_swirl_visual(Enemy *e, int time, bool render) {
	if(!render) {
		// why the hell was this here?
		// Swirl(e, time, render);
		return;
	}

	// FIXME: blend
	r_blend(BLEND_SUB);
	Swirl(e, time, render);
	r_blend(BLEND_PREMUL_ALPHA);
}

static void kurumi_extra_fairy_visual(Enemy *e, int time, bool render) {
	if(!render) {
		Fairy(e, time, render);
		return;
	}

	// r_blend(BLEND_ADD);
	r_shader("sprite_negative");
	Fairy(e, time, render);
	r_shader("sprite_default");
	// r_blend(BLEND_ALPHA);
}

static void kurumi_extra_bigfairy_visual(Enemy *e, int time, bool render) {
	if(!render) {
		BigFairy(e, time, render);
		return;
	}

	// r_blend(BLEND_ADD);
	r_shader("sprite_negative");
	BigFairy(e, time, render);
	r_shader("sprite_default");
	// r_blend(BLEND_ALPHA);
}

static int kurumi_extra_fairy(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, ITEM_POINTS, 1);
		return 1;
	}

	if(e->flags & EFLAG_NO_AUTOKILL && t > 50)
		e->flags &= ~EFLAG_NO_AUTOKILL;

	if(creal(e->args[0]-e->pos) != 0)
		e->moving = true;
	e->dir = creal(e->args[0]-e->pos) < 0;

	FROM_TO(0,90,1) {
		GO_TO(e,e->args[0],0.1)
	}

	/*FROM_TO(100, 200, 22-global.diff*3) {
		PROJECTILE("ball", e->pos, RGB(1, 0.3, 0.5), asymptotic, 2*cexp(I*M_PI*2*frand()), 3);
	}*/

	int attacktime = creal(e->args[1]);
	int flytime = cimag(e->args[1]);
	FROM_TO_SND("shot1_loop", attacktime-20,attacktime+20,20) {
		cmplx vel = cexp(I*frand()*2*M_PI)*(2+0.1*(global.diff-D_Easy));
		if(e->args[2] == 0) { // attack type
			int corners = 5;
			double len = 50;
			int count = 5;
			for(int i = 0; i < corners; i++) {
				for(int j = 0; j < count; j++) {
					cmplx pos = len/2/tan(2*M_PI/corners)*I+(j/(double)count-0.5)*len;
					pos *= cexp(I*2*M_PI/corners*i);
					PROJECTILE(
						.proto = pp_flea,
						.pos = e->pos+pos,
						.color = RGB(1, 0.3, 0.5),
						.rule = linear,
						.args = { vel+0.1*I*pos/cabs(pos) }
					);
				}
			}
		} else {
			int count = 30;
			double rad = 20;
			for(int j = 0; j < count; j++) {
				double x = (j/(double)count-0.5)*2*M_PI;
				cmplx pos = 0.5*cos(x)+sin(2*x) + (0.5*sin(x)+cos(2*x))*I;
				pos*=vel/cabs(vel);
				PROJECTILE(
					.proto = pp_flea,
					.pos = e->pos+rad*pos,
					.color = RGB(0.5, 0.3, 1),
					.rule = linear,
					.args = { vel+0.1*pos },
				);
			}
		}
	}
	AT(attacktime) {
		e->args[0] = global.plr.pos-e->pos;
		kurumi_extra_create_drainer(e);
		play_sound("redirect");
	}
	FROM_TO(attacktime,attacktime+flytime,1) {
		e->pos += e->args[0]/flytime;
	}

	FROM_TO(attacktime,attacktime+flytime,20-global.diff*3) {
		if(global.diff>D_Easy) {
			Color *clr = RGBA_MUL_ALPHA(0.1 + 0.07 * _i, 0.3, 1 - 0.05 * _i, 0.8);
			clr->a = 0;

			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos,
				.color = clr,
				.timeout = 50,
			);
		}
	}
	if(t > attacktime + flytime + 20 && global.boss) {
		GO_TO(e,global.boss->pos,0.04)
	}

	return 1;
}

void kurumi_extra(Boss *b, int time) {
	int length = 400;
	int t = time % length;
	int direction = (time/length)%2;

	int castlimit = b->current->maxhp * 0.05;
	int shieldlimit = b->current->maxhp * 0.1;

	TIMER(&t);

	if(time == EVENT_DEATH) {
		enemy_kill_all(&global.enemies);
		return;
	}

	if(time < 120)
		GO_TO(b, VIEWPORT_W * 0.5 + VIEWPORT_H * 0.28 * I, 0.01)

	if(t == 0 && b->current->hp >= shieldlimit) {
		int cnt = 12;
		for(int i = 0; i < cnt; ++i) {
			double a = M_PI * 2 * i / (double)cnt;
			int hp = 500;
			create_enemy2c(b->pos, hp, kurumi_extra_swirl_visual, kurumi_extra_shield, a + 0.05*I, 800);
			create_enemy2c(b->pos, hp, kurumi_extra_swirl_visual, kurumi_extra_shield, a - 0.05*I, 800);
		}
	}

	AT(90) {
		int cnt = 20;
		for(int i = 0; i < cnt; i++) {
			b->current->hp -= 600;
			if(b->current->hp < castlimit)
				b->current->hp = castlimit;
			tsrand_fill(2);
			cmplx pos = VIEWPORT_W/2*afrand(0)+I*afrand(1)*VIEWPORT_H*2/3;
			if(direction)
				pos = VIEWPORT_W-creal(pos)+I*cimag(pos);
			// immune so they don’t get killed while they are still offscreen.
			Enemy *fairy = create_enemy3c(pos-300*(1-2*direction),500,kurumi_extra_fairy_visual,kurumi_extra_fairy,pos,100+20*i+100*(1.1-0.05*global.diff)*I,direction);
			fairy->flags |= EFLAG_NO_AUTOKILL;
		}

		// XXX: maybe add a special sound for this?
		play_sound("shot_special1");
	}

	cmplx sidepos = VIEWPORT_W * (0.5+0.3*(1-2*direction)) + VIEWPORT_H * 0.28 * I;
	FROM_TO(90,120,1) {
		GO_TO(b, sidepos,0.1)
	}

	FROM_TO(190,200,1) {
		GO_TO(b, sidepos+30*I,0.1)
	}

	AT(190){
		aniplayer_queue_frames(&b->ani, "muda", 110);
		aniplayer_queue(&b->ani, "main", 0);
	}

	FROM_TO(300,400,1) {
		GO_TO(b,VIEWPORT_W * 0.5 + VIEWPORT_H * 0.28 * I,0.1)
	}

	if(global.diff >= D_Hard) {
		AT(300) {
			double ofs = VIEWPORT_W * 0.5;
			cmplx pos = 0.5 * VIEWPORT_W + I * (VIEWPORT_H - 100);
			cmplx targ = 0.5 *VIEWPORT_W + VIEWPORT_H * 0.3 * I;
			create_enemy1c(pos + ofs, 3300, kurumi_extra_bigfairy_visual, kurumi_extra_bigfairy1, targ + 0.8*ofs);
			create_enemy1c(pos - ofs, 3300, kurumi_extra_bigfairy_visual, kurumi_extra_bigfairy1, targ - 0.8*ofs);
		}
	}
	if((t == length-20 && global.diff == D_Easy)|| b->current->hp < shieldlimit) {
		for(Enemy *e = global.enemies.first; e; e = e->next) {
			if(e->logic_rule == kurumi_extra_shield) {
				e->args[2] = 1; // discharge extra shield
				e->hp = 0;
				continue;
			}
		}
	}
}
