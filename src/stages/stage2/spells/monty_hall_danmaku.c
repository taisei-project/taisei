/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

#include "global.h"

static int timeout_deadproj_linear(Projectile *p, int time) {
	// TODO: maybe add a "clear on timeout" flag?

	if(time < 0) {
		return ACTION_ACK;
	}

	if(time > creal(p->args[0])) {
		p->type = PROJ_DEAD;
	}

	p->pos += p->args[1];
	p->angle = carg(p->args[1]);
	return ACTION_NONE;
}

static int hina_monty_slave(Enemy *s, int time) {
	if(time < 0) {
		return 1;
	}

	if(time > 60 && time < 720-140 + 20*(global.diff-D_Lunatic) && !(time % (int)(fmax(2 + (global.diff < D_Normal), (120 - 0.5 * time))))) {
		play_sfx_loop("shot1_loop");

		PROJECTILE(
			.proto = pp_crystal,
			.pos = s->pos,
			.color = RGB(0.5 + 0.5 * psin(time*0.2), 0.3, 1.0 - 0.5 * psin(time*0.2)),
			.rule = asymptotic,
			.args = {
				5*I + 1 * (sin(time) + I * cos(time)),
				4
			}
		);

		if(global.diff > D_Easy) {
			PROJECTILE(
				.proto = pp_crystal,
				.pos = s->pos,
				.color = RGB(0.5 + 0.5 * psin(time*0.2), 0.3, 1.0 - 0.5 * psin(time*0.2)),
				.rule = timeout_deadproj_linear,
				.args = {
					500,
					-0.5*I + 1 * (sin(time) + I * cos(time))
				}
			);
		}
	}

	return 1;
}

static void hina_monty_slave_visual(Enemy *s, int time, bool render) {
	Swirl(s, time, render);

	if(!render) {
		return;
	}

	Sprite *soul = get_sprite("proj/soul");
	double scale = fabs(swing(clamp(time / 60.0, 0, 1), 3)) * 1.25;

	Color *clr1 = RGBA(1.0, 0.0, 0.0, 0.0);
	Color *clr2 = RGBA(0.0, 0.0, 1.0, 0.0);
	Color *clr3 = RGBA(psin(time*0.05), 0.0, 1.0 - psin(time*0.05), 0.0);

	r_mat_mv_push();
	r_mat_mv_translate(creal(s->pos), cimag(s->pos), 0);
	r_shader("sprite_bullet");

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = soul,
		.rotation.angle = time * DEG2RAD,
		.scale.x = scale * (0.6 + 0.6 * psin(time*0.1)),
		.scale.y = scale * (0.7 + 0.5 * psin(time*0.1 + M_PI)),
		.color = clr1,
	});

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = soul,
		.rotation.angle = time * DEG2RAD,
		.scale.x = scale * (0.7 + 0.5 * psin(time*0.1 + M_PI)),
		.scale.y = scale * (0.6 + 0.6 * psin(time*0.1)),
		.color = clr2,
	});

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = soul,
		.rotation.angle = -time * DEG2RAD,
		.scale.both = scale,
		.color = clr3,
	});

	r_mat_mv_pop();
}

void hina_monty(Boss *h, int time) {
	int t = time % 720;
	TIMER(&t);

	static short slave_pos, bad_pos, good_pos, plr_pos;
	static int cwidth = VIEWPORT_W / 3.0;
	static cmplx targetpos;

	if(time == EVENT_DEATH) {
		enemy_kill_all(&global.enemies);
		return;
	}

	if(time < 0) {
		targetpos = VIEWPORT_W/2.0 + VIEWPORT_H/2.0 * I;
		return;
	}

	AT(0) {
		plr_pos = creal(global.plr.pos) / cwidth;
		bad_pos = tsrand() % 3;
		do good_pos = tsrand() % 3; while(good_pos == bad_pos);

		play_sound("laser1");

		for(int i = 0; i < 2; ++i) {
			int x = cwidth * (1 + i);
			create_laserline_ab(x, x + VIEWPORT_H*I, 15, 30, 60, RGBA(0.3, 1.0, 1.0, 0.0))->unclearable = true;
			create_laserline_ab(x, x + VIEWPORT_H*I, 20, 240, 600, RGBA(1.0, 0.3, 1.0, 0.0))->unclearable = true;
		}

		enemy_kill_all(&global.enemies);
	}

	AT(120) {
		do slave_pos = tsrand() % 3; while(slave_pos == plr_pos || slave_pos == good_pos);
		while(bad_pos == slave_pos || bad_pos == good_pos) bad_pos = tsrand() % 3;

		cmplx o = cwidth * (0.5 + slave_pos) + VIEWPORT_H/2.0*I - 200.0*I;

		play_sound("laser1");
		create_laserline_ab(h->pos, o, 15, 30, 60, RGBA(1.0, 0.3, 0.3, 0.0));
		aniplayer_queue(&h->ani,"guruguru",1);
		aniplayer_queue(&h->ani,"main",0);
	}

	AT(140) {
		play_sound("shot_special1");
		create_enemy4c(cwidth * (0.5 + slave_pos) + VIEWPORT_H/2.0*I - 200.0*I, ENEMY_IMMUNE, hina_monty_slave_visual, hina_monty_slave, 0, 0, 0, 1);
	}

	AT(190) {
		targetpos = cwidth * (0.5 + good_pos) + VIEWPORT_H/2.0*I - 200.0*I;
	}

	AT(240) {
		// main laser barrier activation
		play_sound("laser1");
	}

	FROM_TO(220, 360 + 60 * imax(0, global.diff - D_Easy), 60) {
		play_sound("shot_special1");

		float cnt = (2.0+global.diff) * 5;
		for(int i = 0; i < cnt; i++) {
			bool top = ((global.diff > D_Hard) && (_i % 2));
			cmplx o = !top*VIEWPORT_H*I + cwidth*(bad_pos + i/(double)(cnt - 1));

			PROJECTILE(
				.proto = pp_ball,
				.pos = o,
				.color = top ? RGBA(0, 0, 0.7, 0) : RGBA(0.7, 0, 0, 0),
				.rule = accelerated,
				.args = {
					0,
					(top ? -0.5 : 1) * 0.004 * (sin((M_PI * 4 * i / (cnt - 1)))*0.1*global.diff - I*(1 + psin(i + global.frames)))
				},
			);
		}
	}

	{
		const int step = 2;
		const int cnt = 10;
		const int burst_dur = (cnt - 1) * step;
		const int cycle_dur = burst_dur+10*(D_Hard-global.diff);
		const int start = 210;
		const int end = 540;
		const int ncycles = (end - start) / cycle_dur;

		AT(start) {
			aniplayer_queue_frames(&h->ani,"guruguru",end-start);
			aniplayer_queue(&h->ani,"main",0);
		}

		FROM_TO_INT(start, start + cycle_dur * ncycles - 1, cycle_dur, burst_dur, step) {
			play_sound("shot1");

			double p = _ni / (double)(cnt-1);
			double c = p;
			double m = 0.60 + 0.01 * global.diff;

			p *= m;
			if(_i % 2) {
				p = 1.0 - p;
			}

			cmplx o = cwidth * (p + 0.5/(cnt-1) - 0.5) + h->pos;

			if(global.diff > D_Normal) {
				PROJECTILE(
					.proto = pp_card,
					.pos = o,
					.color = RGB(c * 0.8, 0, (1 - c) * 0.8),
					.rule = accelerated,
					.args = { -2.5*I, 0.05*I }
				);
			} else {
				PROJECTILE(
					.proto = pp_card,
					.pos = o,
					.color = RGB(c * 0.8, 0, (1 - c) * 0.8),
					.rule = linear,
					.args = { 2.5*I }
				);
			}
		}
	}

	FROM_TO(240, 390, 5) {
		create_item(VIEWPORT_H*I + cwidth*(good_pos + frand()), -50.0*I, _i % 2 ? ITEM_POINTS : ITEM_POWER);
	}

	AT(600) {
		targetpos = cwidth * (0.5 + slave_pos) + VIEWPORT_H/2.0*I;
	}

	GO_TO(h, targetpos, 0.06);
}
