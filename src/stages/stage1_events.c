/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stage1_events.h"
#include "global.h"
#include "stagetext.h"
#include "common_tasks.h"

static Dialog *stage1_dialog_pre_boss(void) {
	PlayerMode *pm = global.plr.mode;
	Dialog *d = dialog_create();
	dialog_set_char(d, DIALOG_LEFT, pm->character->lower_name, "normal", NULL);
	dialog_set_char(d, DIALOG_RIGHT, "cirno", "normal", NULL);
	pm->dialog->stage1_pre_boss(d);
	dialog_add_action(d, &(DialogAction) { .type = DIALOG_SET_BGM, .data = "stage1boss"});
	return d;
}

static Dialog *stage1_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	Dialog *d = dialog_create();
	dialog_set_char(d, DIALOG_LEFT, pm->character->lower_name, "normal", NULL);
	dialog_set_char(d, DIALOG_RIGHT, "cirno", "defeated", "defeated");
	pm->dialog->stage1_post_boss(d);
	return d;
}

static Projectile* spawn_stain(cmplx pos, float angle, int to) {
	return PARTICLE(
		.sprite = "stain",
		.pos = pos,
		.draw_rule = ScaleFade,
		.timeout = to,
		.angle = angle,
		.color = RGBA(0.4, 0.4, 0.4, 0),
		.args = {0, 0, 0.8*I}
	);
}

void cirno_pfreeze_bg(Boss *c, int time) {
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

Boss* stage1_spawn_cirno(cmplx pos) {
	Boss *cirno = create_boss("Cirno", "cirno", pos);
	boss_set_portrait(cirno, get_sprite("dialog/cirno"), get_sprite("dialog/cirno_face_normal"));
	cirno->shadowcolor = *RGBA_MUL_ALPHA(0.6, 0.7, 1.0, 0.25);
	cirno->glowcolor = *RGB(0.2, 0.35, 0.5);
	return cirno;
}

static void cirno_intro_boss(Boss *c, int time) {
	if(time < 0)
		return;
	TIMER(&time);
	GO_TO(c, VIEWPORT_W/2.0 + 100.0*I, 0.05);

	AT(120)
		global.dialog = stage1_dialog_pre_boss();
}

static void cirno_iceplosion0(Boss *c, int time) {
	int t = time % 300;
	TIMER(&t);

	if(time < 0)
		return;

	AT(20) {
		aniplayer_queue(&c->ani,"(9)",1);
		aniplayer_queue(&c->ani,"main",0);
		play_sound("shot_special1");
	}

	FROM_TO(20,30,2) {
		int i;
		int n = 8+global.diff;
		for(i = 0; i < n; i++) {
			PROJECTILE(
				.proto = pp_plainball,
				.pos = c->pos,
				.color = RGB(0,0,0.5),
				.rule = asymptotic,
				.args = { (3+_i/3.0)*cexp(I*(2*M_PI/n*i + carg(global.plr.pos-c->pos))), _i*0.7 }
			);
		}
	}

	FROM_TO_SND("shot1_loop",40,100,1+2*(global.diff<D_Hard)) {
		PROJECTILE(
			.proto = pp_crystal,
			.pos = c->pos,
			.color = RGB(0.3,0.3,0.8),
			.rule = accelerated,
			.args = { global.diff/4.0*rng_dir() + 2.0*I, 0.002*cdir(M_PI/10.0*(_i%20)) }
		);
	}

	FROM_TO(150, 300, 30-5*global.diff) {
		float dif = rng_angle();
		int i;
		play_sound("shot1");
		for(i = 0; i < 20; i++) {
			PROJECTILE(
				.proto = pp_plainball,
				.pos = c->pos,
				.color = RGB(0.04*_i,0.04*_i,0.4+0.04*_i),
				.rule = asymptotic,
				.args = { (3+_i/4.0)*cexp(I*(2*M_PI/8.0*i + dif)), 2.5 }
			);
		}
	}
}

void cirno_crystal_rain(Boss *c, int time) {
	int t = time % 500;
	TIMER(&t);

	if(time < 0)
		return;

	// PLAY_FOR("shot1_loop",0,499);

	if(!(time % 10)) {
		play_sound("shot2");
	}

	int hdiff = max(0, (int)global.diff - D_Normal);

	if(rng_chance(0.05 + 0.1 * global.diff)) {
		RNG_ARRAY(rng, 2);
		PROJECTILE(
			.proto = pp_crystal,
			.pos = vrng_range(rng[0], 0, VIEWPORT_W),
			.color = RGB(0.2,0.2,0.4),
			.rule = accelerated,
			.args = { 1.0*I, 0.01*I + (-0.005+0.005*global.diff) * vrng_sreal(rng[1]) }
		);
	}

	AT(100)
		aniplayer_queue(&c->ani,"(9)",0);
	AT(400)
		aniplayer_queue(&c->ani,"main",0);
	FROM_TO(100, 400, 120-20*global.diff - 10 * hdiff) {
		float i;
		bool odd = (hdiff? (_i&1) : 0);
		float n = (global.diff-1+hdiff*4 + odd)/2.0;

		play_sound("shot_special1");
		for(i = -n; i <= n; i++) {
			PROJECTILE(
				.proto = odd? pp_plainball : pp_bigball,
				.pos = c->pos,
				.color = RGB(0.2,0.2,0.9),
				.rule = asymptotic,
				.args = { 2*cexp(I*carg(global.plr.pos-c->pos)+0.3*I*i), 2.3 }
			);
		}
	}

	GO_AT(c, 20, 70, 1+0.6*I);
	GO_AT(c, 120, 170, -1+0.2*I);
	GO_AT(c, 230, 300, -1+0.6*I);

	FROM_TO(400, 500, 1)
		GO_TO(c, VIEWPORT_W/2.0 + 100.0*I, 0.01);
}

static void cirno_iceplosion1(Boss *c, int time) {
	int t = time % 300;
	TIMER(&t);

	if(time < 0)
		GO_TO(c, VIEWPORT_W/2.0 + 100.0*I, 0.02);

	AT(20) {
		play_sound("shot_special1");
	}
	AT(20)
		aniplayer_queue(&c->ani,"(9)",0);
	AT(30)
		aniplayer_queue(&c->ani,"main",0);

	FROM_TO(20,30,2) {
		int i;
		for(i = 0; i < 15+global.diff; i++) {
			PROJECTILE(
				.proto = pp_plainball,
				.pos = c->pos,
				.color = RGB(0,0,0.5),
				.rule = asymptotic,
				.args = { (3+_i/3.0)*cexp(I*((2)*M_PI/8.0*i + (0.1+0.03*global.diff) * rng_sreal())), _i*0.7 }
			);
		}
	}

	FROM_TO_SND("shot1_loop",40,100,2+2*(global.diff<D_Hard)) {
		for(int i = 1; i >= -1; i -= 2) {
			PROJECTILE(
				.proto = pp_crystal,
				.pos = c->pos + i * 100,
				.color = RGB(0.3,0.3,0.8),
				.rule = accelerated,
				.args = {
					1.5*rng_dir() - i * 0.4 + 2.0*I*global.diff/4.0,
					0.002*cexp(I*(M_PI/10.0*(_i%20)))
				}
			);
		}
	}

	FROM_TO(150, 300, 30 - 6 * global.diff) {
		float dif = rng_angle();
		int i;

		if(_i > 15) {
			_i = 15;
		}

		play_sound("shot1");
		for(i = 0; i < 20; i++) {
			PROJECTILE(
				.proto = pp_plainball,
				.pos = c->pos,
				.color = RGB(0.04*_i,0.04*_i,0.4+0.04*_i),
				.rule = asymptotic,
				.args = { (3+_i/3.0)*cdir(2*M_PI/8.0*i + dif), 2.5 }
			);
		}
	}
}

static Color* halation_color(Color *out_clr, float phase) {
	if(phase < 0.5) {
		*out_clr = *color_lerp(
			RGB(0.4, 0.4, 0.75),
			RGB(0.4, 0.4, 0.3),
			phase * phase
		);
	} else {
		*out_clr = *color_lerp(
			RGB(0.4, 0.4, 0.3),
			RGB(1.0, 0.3, 0.2),
			(phase - 0.5) * 2
		);
	}

	return out_clr;
}

static void halation_laser(Laser *l, int time) {
	static_laser(l, time);

	if(time >= 0) {
		halation_color(&l->color, l->width / cimag(l->args[1]));
		l->color.a = 0;
	}
}

static cmplx halation_calc_orb_pos(cmplx center, float rotation, int proj, int projs) {
	double f = (double)((proj % projs)+0.5)/projs;
	return 200 * cexp(I*(rotation + f * 2 * M_PI)) + center;
}

static int halation_orb(Projectile *p, int time) {
	if(time < 0) {
		return ACTION_ACK;
	}

	if(!(time % 4)) {
		spawn_stain(p->pos, global.frames * 15, 20);
	}

	cmplx center = p->args[0];
	double rotation = p->args[1];
	int id = creal(p->args[2]);
	int count = cimag(p->args[2]);
	int halate_time = creal(p->args[3]);
	int phase_time = 60;

	cmplx pos0 = halation_calc_orb_pos(center, rotation, id, count);
	cmplx pos1 = halation_calc_orb_pos(center, rotation, id + count/2, count);
	cmplx pos2 = halation_calc_orb_pos(center, rotation, id + count/2 - 1, count);
	cmplx pos3 = halation_calc_orb_pos(center, rotation, id + count/2 - 2, count);

	GO_TO(p, pos2, 0.1);

	if(p->type == PROJ_DEAD) {
		return 1;
	}

	if(time == halate_time) {
		create_laserline_ab(pos2, pos3, 15, phase_time * 0.5, phase_time * 2.0, &p->color);
		create_laserline_ab(pos0, pos2, 15, phase_time, phase_time * 1.5, &p->color)->lrule = halation_laser;
	} if(time == halate_time + phase_time * 0.5) {
		play_sound("laser1");
	} else if(time == halate_time + phase_time) {
		play_sound("shot1");
		create_laserline_ab(pos0, pos1, 12, phase_time, phase_time * 1.5, &p->color)->lrule = halation_laser;
	} else if(time == halate_time + phase_time * 2) {
		play_sound("shot1");
		create_laserline_ab(pos0, pos3, 15, phase_time, phase_time * 1.5, &p->color)->lrule = halation_laser;
		create_laserline_ab(pos1, pos3, 15, phase_time, phase_time * 1.5, &p->color)->lrule = halation_laser;
	} else if(time == halate_time + phase_time * 3) {
		play_sound("shot1");
		create_laserline_ab(pos0, pos1, 12, phase_time, phase_time * 1.5, &p->color)->lrule = halation_laser;
		create_laserline_ab(pos0, pos2, 15, phase_time, phase_time * 1.5, &p->color)->lrule = halation_laser;
	} else if(time == halate_time + phase_time * 4) {
		play_sound("shot1");
		play_sound("shot_special1");

		Color colors[] = {
			// i *will* revert your commit if you change this, no questions asked.
			{ 226/255.0, 115/255.0,  45/255.0, 1 },
			{  54/255.0, 179/255.0, 221/255.0, 1 },
			{ 140/255.0, 147/255.0, 149/255.0, 1 },
			{  22/255.0,  96/255.0, 165/255.0, 1 },
			{ 241/255.0, 197/255.0,  31/255.0, 1 },
			{ 204/255.0,  53/255.0,  84/255.0, 1 },
			{ 116/255.0,  71/255.0, 145/255.0, 1 },
			{  84/255.0, 171/255.0,  72/255.0, 1 },
			{ 213/255.0,  78/255.0, 141/255.0, 1 },
		};

		int pcount = sizeof(colors)/sizeof(Color);
		float rot = rng_angle();

		for(int i = 0; i < pcount; ++i) {
			PROJECTILE(
				.proto = pp_crystal,
				.pos = p->pos,
				.color = colors+i,
				.rule = asymptotic,
				.args = { cdir(rot + M_PI * 2 * (float)(i+1)/pcount), 3 }
			);
		}

		return ACTION_DESTROY;
	}

	return 1;
}

void cirno_snow_halation(Boss *c, int time) {
	int t = time % 300;
	TIMER(&t);

	// TODO: get rid of the "static" nonsense already! #ArgsForBossAttacks2017
	// tfw it's 2018 and still no args
	// tfw when you then add another static
	static cmplx center;
	static float rotation;
	static int cheater;

	if(time == EVENT_BIRTH)
		cheater = 0;

	if(time < 0) {
		return;
	}

	if(cheater >= 8) {
		GO_TO(c, global.plr.pos,0.05);
		aniplayer_queue(&c->ani,"(9)",0);
	} else {
		GO_TO(c, VIEWPORT_W/2.0+100.0*I, 0.05);
	}

	AT(60) {
		center = global.plr.pos;
		rotation = (M_PI/2.0) * (1 + time / 300);
		aniplayer_queue(&c->ani,"(9)",0);
	}

	const int interval = 3;
	const int projs = 10 + 4 * (global.diff - D_Hard);

	FROM_TO_SND("shot1_loop", 60, 60 + interval * (projs/2 - 1), interval) {
		int halate_time = 35 - _i * interval;

		for(int p = _i*2; p <= _i*2 + 1; ++p) {
			Color clr;
			halation_color(&clr, 0);
			clr.a = 0;

			PROJECTILE(
				.proto = pp_plainball,
				.pos = halation_calc_orb_pos(center, rotation, p, projs),
				.color = &clr,
				.rule = halation_orb,
				.args = {
					center, rotation, p + I * projs, halate_time
				},
				.max_viewport_dist = 200,
				.flags = PFLAG_NOCLEAR | PFLAG_NOCOLLISION,
			);
		}
	}

	AT(100 + interval * projs/2) {
		aniplayer_queue(&c->ani,"main",0);

		if(cabs(global.plr.pos-center)>cabs(halation_calc_orb_pos(0,0,0,projs))) {
			char *text[] = {
				"",
				"What are you doing??",
				"Dodge it properly!",
				"I bet you can’t do it! Idiot!",
				"I spent so much time on this attack!",
				"Maybe it is too smart for secondaries!",
				"I think you don’t even understand the timer!",
				"You- You Idiootttt!",
			};

			if(cheater < sizeof(text)/sizeof(text[0])) {
				stagetext_add(text[cheater], global.boss->pos+100*I, ALIGN_CENTER, get_font("standard"), RGB(1,1,1), 0, 100, 10, 20);
				cheater++;
			}
		}
	}
}

static int cirno_icicles(Projectile *p, int t) {
	int turn = 60;

	if(t < 0) {
		if(t == EVENT_BIRTH) {
			p->angle = carg(p->args[0]);
		}

		return ACTION_ACK;
	}

	if(t < turn) {
		p->pos += p->args[0]*pow(0.9,t);
	} else if(t == turn) {
		p->args[0] = 2.5*cexp(I*(carg(p->args[0])-M_PI/2.0+M_PI*(creal(p->args[0]) > 0)));
		if(global.diff > D_Normal)
			p->args[0] += rng_range(0, 0.05);
		play_sound("redirect");
		spawn_projectile_highlight_effect(p);
	} else if(t > turn) {
		p->pos += p->args[0];
	}

	p->angle = carg(p->args[0]);

	return ACTION_NONE;
}

void cirno_icicle_fall(Boss *c, int time) {
	int t = time % 400;
	TIMER(&t);

	if(time < 0)
		return;

	GO_TO(c, VIEWPORT_W/2.0+120.0*I, 0.01);

	AT(20)
		aniplayer_queue(&c->ani,"(9)",0);
	AT(200)
		aniplayer_queue(&c->ani,"main",0);

	FROM_TO(20,200,30-3*global.diff) {
		play_sound("shot1");
		for(float i = 2-0.2*global.diff; i < 5; i+=1./(1+global.diff)) {
			PROJECTILE(.proto = pp_crystal, .pos = c->pos, .color = RGB(0.3,0.3,0.9), .rule = cirno_icicles, .args = { 6*i*cexp(I*(-0.1+0.1*_i)) });
			PROJECTILE(.proto = pp_crystal, .pos = c->pos, .color = RGB(0.3,0.3,0.9), .rule = cirno_icicles, .args = { 6*i*cexp(I*(M_PI+0.1-0.1*_i)) });
		}
	}

	if(global.diff > D_Easy) {
		FROM_TO_SND("shot1_loop",120,200,3) {
			float f = rng_range(0, _i);

			PROJECTILE(.proto = pp_ball, .pos = c->pos, .color = RGB(0.,0.,0.3), .rule = accelerated, .args = { 0.2*(-2*I-1.5+f),-0.02*I });
			PROJECTILE(.proto = pp_ball, .pos = c->pos, .color = RGB(0.,0.,0.3), .rule = accelerated, .args = { 0.2*(-2*I+1.5+f),-0.02*I });
		}
	}

	if(global.diff > D_Normal) {
		FROM_TO(300,400,10) {
			play_sound("shot1");
			float x = VIEWPORT_W/2+VIEWPORT_W/2*(0.3+_i/10.);
			float angle1 = rng_range(0, M_PI/10);
			float angle2 = rng_range(0, M_PI/10);
			for(float i = 1; i < 5; i++) {
				PROJECTILE(
					.proto = pp_ball,
					.pos = x,
					.color = RGB(0.,0.,0.3),
					.rule = accelerated,
					.args = {
						i*I*0.5*cdir(angle1),
						0.001*I-(global.diff == D_Lunatic)*0.001*rng_real()
					}
				);

				PROJECTILE(
					.proto = pp_ball,
					.pos = VIEWPORT_W-x,
					.color = RGB(0.,0.,0.3),
					.rule = accelerated,
					.args = {
						i*I*0.5*cdir(-angle2),
						0.001*I+(global.diff == D_Lunatic)*0.001*rng_real()
					}
				);
			}
		}
	}
}

static int cirno_crystal_blizzard_proj(Projectile *p, int time) {
	if(time < 0) {
		return ACTION_ACK;
	}

	if(!(time % 12)) {
		spawn_stain(p->pos, global.frames * 15, 20);
	}

	if(time > 100 + global.diff * 100) {
		p->args[0] *= 1.03;
	}

	return asymptotic(p, time);
}

void cirno_crystal_blizzard(Boss *c, int time) {
	int t = time % 700;
	TIMER(&t);

	if(time < 0) {
		GO_TO(c, VIEWPORT_W/2.0+300*I, 0.1);
		return;
	}

	FROM_TO(60, 360, 10) {
		play_sound("shot1");
		int i, cnt = 14 + global.diff * 3;
		for(i = 0; i < cnt; ++i) {
			PROJECTILE(
				.proto = pp_crystal,
				.pos = i*VIEWPORT_W/cnt,
				.color = i % 2? RGB(0.2,0.2,0.4) : RGB(0.5,0.5,0.5),
				.rule = accelerated,
				.args = {
					0, 0.02*I + 0.01*I * (i % 2? 1 : -1) * sin((i*3+global.frames)/30.0)
				},
			);
		}
	}

	AT(330)
		aniplayer_queue(&c->ani,"(9)",0);
	AT(699)
		aniplayer_queue(&c->ani,"main",0);
	FROM_TO_SND("shot1_loop",330, 700, 1) {
		GO_TO(c, global.plr.pos, 0.01);

		if(!(time % (1 + D_Lunatic - global.diff))) {
			RNG_ARRAY(rng, 2);
			PROJECTILE(
				.proto = pp_wave,
				.pos = c->pos,
				.color = RGBA(0.2, 0.2, 0.4, 0.0),
				.rule = cirno_crystal_blizzard_proj,
				.args = {
					20 * (0.1 + 0.1 * vrng_sreal(rng[0])) * cexp(I*(carg(global.plr.pos - c->pos) + vrng_sreal(rng[1]) * 0.2)),
					5
				},
			);
		}

		if(!(time % 7)) {
			play_sound("shot1");
			int i, cnt = global.diff - 1;
			for(i = 0; i < cnt; ++i) {
				PROJECTILE(
					.proto = pp_ball,
					.pos = c->pos,
					.color = RGBA(0.1, 0.1, 0.5, 0.0),
					.rule = accelerated,
					.args = { 0, 0.01 * cexp(I*(global.frames/20.0 + 2*i*M_PI/cnt)) },
				);
			}
		}
	}
}

void cirno_benchmark(Boss* b, int t) {
	if(t < 0) {
		return;
	}

	int N = 5000; // number of particles on the screen

	if(t == 0) {
		aniplayer_queue(&b->ani, "(9)", 0);
	}
	double speed = 10;
	int c = N*speed/VIEWPORT_H;
	for(int i = 0; i < c; i++) {
		double x = rng_range(0, VIEWPORT_W);
		double plrx = creal(global.plr.pos);
		x = plrx + sqrt((x-plrx)*(x-plrx)+100)*(1-2*(x<plrx));

		Projectile *p = PROJECTILE(
			.proto = pp_ball,
			.pos = x,
			.color = RGB(0.1, 0.1, 0.5),
			.rule = linear,
			.args = { speed*I },
			.flags = PFLAG_NOGRAZE,
		);

		if(rng_chance(0.1)) {
			p->flags &= ~PFLAG_NOGRAZE;
		}

		if(t > 700 && rng_chance(0.5))
			projectile_set_prototype(p, pp_plainball);

		if(t > 1200 && rng_chance(0.5))
			p->color = *RGB(1.0, 0.2, 0.8);

		if(t > 350 && rng_chance(0.5))
			p->color.a = 0;
	}
}

TASK(burst_fairy, { cmplx pos; cmplx dir; }) {
	Enemy *e = TASK_BIND_UNBOXED(create_enemy1c(ARGS.pos, 700, Fairy, NULL, ARGS.dir));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 1,
		.power = 1,
	});

	e->move.attraction_point = ARGS.pos + 120*I;
	e->move.attraction = 0.03;

	WAIT(60);

	play_sound("shot1");
	int n = 1.5 * global.diff - 1;

	for(int i = -n; i <= n; i++) {
		cmplx aim = cdir(carg(global.plr.pos - e->pos) + 0.2 * i);

		PROJECTILE(
			.proto = pp_crystal,
			.pos = e->pos,
			.color = RGB(0.2, 0.3, 0.5),
			.move = move_asymptotic_simple(aim * (2 + 0.5 * global.diff), 5),
		);
	}

	WAIT(1);

	e->move.attraction = 0;
	e->move.acceleration = 0.04 * ARGS.dir;
	e->move.retention = 1;

	for(;;) {
		YIELD;
	}
}

TASK(circletoss_shoot_circle, { BoxedEnemy e; int duration; int interval; }) {
	Enemy *e = TASK_BIND(ARGS.e);

	int cnt = ARGS.duration / ARGS.interval;
	double angle_step = M_TAU / cnt;

	for(int i = 0; i < cnt; ++i) {
		play_loop("shot1_loop");
		e->move.velocity *= 0.8;

		cmplx aim = cdir(angle_step * i);

		PROJECTILE(
			.proto = pp_rice,
			.pos = e->pos,
			.color = RGB(0.6, 0.2, 0.7),
			.move = move_asymptotic_simple(2 * aim, i * 0.5),
		);

		WAIT(ARGS.interval);
	}
}

TASK(circletoss_shoot_toss, { BoxedEnemy e; int times; int duration; int period; }) {
	Enemy *e = TASK_BIND(ARGS.e);

	while(ARGS.times--) {
		for(int i = ARGS.duration; i--;) {
			play_loop("shot1_loop");

			double aim_angle = carg(global.plr.pos - e->pos);
			aim_angle += 0.05 * global.diff * rng_real();

			cmplx aim = cdir(aim_angle);
			aim *= rng_range(1, 3);

			PROJECTILE(
				.proto = pp_thickrice,
				.pos = e->pos,
				.color = RGB(0.2, 0.4, 0.8),
				.move = move_asymptotic_simple(aim, 3),
			);

			WAIT(1);
		}

		WAIT(ARGS.period - ARGS.duration);
	}
}

TASK(circletoss_fairy, { cmplx pos; cmplx velocity; cmplx exit_accel; int exit_time; }) {
	Enemy *e = TASK_BIND_UNBOXED(create_enemy1c(ARGS.pos, 1500, BigFairy, NULL, 0));

	e->move = move_linear(ARGS.velocity);

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 2,
		.power = 1,
	});

	INVOKE_SUBTASK_DELAYED(60, circletoss_shoot_circle, ENT_BOX(e),
		.duration = 40,
		.interval = 2 + (global.diff < D_Hard)
	);

	if(global.diff > D_Easy) {
		INVOKE_SUBTASK_DELAYED(90, circletoss_shoot_toss, ENT_BOX(e),
			.times = 4,
			.period = 150,
			.duration = 5 + 7 * global.diff
		);
	}

	WAIT(ARGS.exit_time);
	e->move.acceleration += ARGS.exit_accel;
	STALL;
}

TASK(sinepass_swirl_move, { BoxedEnemy e; cmplx v; cmplx sv; }) {
	Enemy *e = TASK_BIND(ARGS.e);
	cmplx sv = ARGS.sv;
	cmplx v = ARGS.v;

	for(;;) {
		sv -= cimag(e->pos - e->pos0) * 0.03 * I;
		e->pos += sv * 0.4 + v;
		YIELD;
	}
}

TASK(sinepass_swirl, { cmplx pos; cmplx vel; cmplx svel; }) {
	Enemy *e = TASK_BIND_UNBOXED(create_enemy1c(ARGS.pos, 100, Swirl, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 1,
	});

	INVOKE_TASK(sinepass_swirl_move, ENT_BOX(e), ARGS.vel, ARGS.svel);

	WAIT(difficulty_value(25, 20, 15, 10));

	int shot_interval = difficulty_value(120, 40, 30, 20);

	for(;;) {
		play_sound("shot1");

		cmplx aim = cnormalize(global.plr.pos - e->pos);
		aim *= difficulty_value(2, 2, 2.5, 3);

		PROJECTILE(
			.proto = pp_ball,
			.pos = e->pos,
			.color = RGB(0.8, 0.8, 0.4),
			.move = move_asymptotic_simple(aim, 5),
		);

		WAIT(shot_interval);
	}
}

TASK(circle_fairy, { cmplx pos; cmplx target_pos; }) {
	Enemy *e = TASK_BIND_UNBOXED(create_enemy1c(ARGS.pos, 1400, BigFairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 3,
		.power = 2,
	});

	e->move.attraction = 0.005;
	e->move.retention = 0.8;
	e->move.attraction_point = ARGS.target_pos;

	WAIT(120);

	int shot_interval = 2;
	int shot_count = difficulty_value(10, 10, 20, 25);
	int round_interval = 120 - shot_interval * shot_count;

	for(int round = 0; round < 2; ++round) {
		double a_ofs = rng_angle();

		for(int i = 0; i < shot_count; ++i) {
			cmplx aim;

			aim = circle_dir_ofs((round & 1) ? i : shot_count - i, shot_count, a_ofs);
			aim *= difficulty_value(1.7, 2.0, 2.5, 2.5);

			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos,
				.color = RGB(0.6, 0.2, 0.7),
				.move = move_asymptotic_simple(aim, i * 0.5),
			);

			play_loop("shot1_loop");
			WAIT(shot_interval);
		}

		e->move.attraction_point += 30 * rng_dir();
		WAIT(round_interval);
	}

	WAIT(10);
	e->move.attraction = 0;
	e->move.retention = 1;
	e->move.acceleration = -0.04 * I * cdir(rng_range(0, M_TAU / 12));
	STALL;
}

TASK(drop_swirl, { cmplx pos; cmplx vel; cmplx accel; }) {
	Enemy *e = TASK_BIND_UNBOXED(create_enemy1c(ARGS.pos, 100, Swirl, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 2,
	});

	e->move = move_accelerated(ARGS.vel, ARGS.accel);

	int shot_interval = difficulty_value(120, 40, 30, 20);

	WAIT(20);

	while(true) {
		cmplx aim = cnormalize(global.plr.pos - e->pos);
		aim *= 1 + 0.3 * global.diff + rng_real();

		play_sound("shot1");
		PROJECTILE(
			.proto = pp_ball,
			.pos = e->pos,
			.color = RGB(0.8, 0.8, 0.4),
			.move = move_linear(aim),
		);

		WAIT(shot_interval);
	}
}

TASK(multiburst_fairy, { cmplx pos; cmplx target_pos; cmplx exit_accel; }) {
	Enemy *e = TASK_BIND_UNBOXED(create_enemy1c(ARGS.pos, 1000, Fairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 3,
		.power = 2,
	});

	e->move.attraction = 0.05;
	// e->move.retention = 0.8;
	e->move.attraction_point = ARGS.target_pos;

	WAIT(60);

	int burst_interval = difficulty_value(22, 20, 18, 16);
	int bursts = 4;

	for(int i = 0; i < bursts; ++i) {
		play_sound("shot1");
		int n = global.diff - 1;

		for(int j = -n; j <= n; j++) {
			cmplx aim = cdir(carg(global.plr.pos - e->pos) + j / 5.0);
			aim *= 2.5;

			PROJECTILE(
				.proto = pp_crystal,
				.pos = e->pos,
				.color = RGB(0.2, 0.3, 0.5),
				.move = move_linear(aim),
			);
		}

		WAIT(burst_interval);
	}

	WAIT(10);
	e->move.attraction = 0;
	e->move.retention = 1;
	e->move.acceleration = ARGS.exit_accel;
}

TASK(instantcircle_fairy_shoot, { BoxedEnemy e; int cnt; double speed; double boost; }) {
	Enemy *e = TASK_BIND(ARGS.e);
	play_sound("shot_special1");

	for(int i = 0; i < ARGS.cnt; ++i) {
		cmplx vel = ARGS.speed * circle_dir(i, ARGS.cnt);

		PROJECTILE(
			.proto = pp_rice,
			.pos = e->pos,
			.color = RGB(0.6, 0.2, 0.7),
			.move = move_asymptotic_simple(vel, ARGS.boost),
		);
	}
}

TASK(instantcircle_fairy, { cmplx pos; cmplx target_pos; cmplx exit_accel; }) {
	Enemy *e = TASK_BIND_UNBOXED(create_enemy1c(ARGS.pos, 1200, Fairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 2,
		.power = 4,
	});

	e->move = move_towards(ARGS.target_pos, 0.04);
	BoxedEnemy be = ENT_BOX(e);

	INVOKE_TASK_DELAYED(75, instantcircle_fairy_shoot, be,
		.cnt = difficulty_value(22, 24, 26, 28),
		.speed = 1.5,
		.boost = 2.0
	);

	if(global.diff > D_Easy) {
		INVOKE_TASK_DELAYED(95, instantcircle_fairy_shoot, be,
			.cnt = difficulty_value(0, 26, 29, 32),
			.speed = 3,
			.boost = 3.0
		);
	}

	WAIT(200);
	e->move.attraction = 0;
	e->move.retention = 1;
	e->move.acceleration = ARGS.exit_accel;
}

TASK(waveshot, { cmplx pos; real angle; real spread; real freq; int shots; int interval; } ) {
	for(int i = 0; i < ARGS.shots; ++i) {
		cmplx v = 4 * cdir(ARGS.angle + ARGS.spread * triangle(ARGS.freq * i));

		play_loop("shot1_loop");
		PROJECTILE(
			.proto = pp_thickrice,
			.pos = ARGS.pos,
			.color = RGBA(0.0, 0.5 * (1.0 - i / (ARGS.shots - 1.0)), 1.0, 1),
			// .move = move_asymptotic(-(8-0.1*i) * v, v, 0.8),
			.move = move_accelerated(-v, 0.02 * v),
		);

		WAIT(ARGS.interval);
	}
}

TASK(waveshot_fairy, { cmplx pos; cmplx target_pos; cmplx exit_accel; }) {
	Enemy *e = TASK_BIND_UNBOXED(create_enemy1c(ARGS.pos, 4200, BigFairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 4,
		.power = 2,
	});

	e->move = move_towards(ARGS.target_pos, 0.03);

	WAIT(120);

	cmplx orig_pos = e->pos;
	real angle = carg(global.plr.pos - orig_pos);
	cmplx pos = orig_pos - 24 * cdir(angle);
	INVOKE_SUBTASK(waveshot, pos, angle, rng_sign() * M_PI/14, 1.0/12.0, 61, 1);

	WAIT(120);

	e->move.attraction = 0;
	e->move.retention = 0.8;
	e->move.acceleration = ARGS.exit_accel;
}

TASK(explosion_fairy, { cmplx pos; cmplx target_pos; cmplx exit_accel; }) {
	Enemy *e = TASK_BIND_UNBOXED(create_enemy1c(ARGS.pos, 6000, BigFairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 8,
	});

	e->move = move_towards(ARGS.target_pos, 0.04);

	WAIT(60);

	int cnt = 60;
	real ofs = rng_angle();

	play_sound("shot_special1");

	for(int i = 0; i < cnt; ++i) {
		cmplx aim = 4 * circle_dir_ofs(i, cnt, ofs);
		real s = 0.5 + 0.5 * triangle(6.0 * i / (real)cnt);

		Color clr;

		if(s == 1) {
			clr = *RGB(1, 0, 0);
		} else {
			clr = *color_lerp(
				RGB(0.1, 0.6, 1.0),
				RGB(1.0, 0.0, 0.3),
				s * s
			);
			color_mul(&clr, &clr);
		}

		PROJECTILE(
			.proto = s == 1 ? pp_bigball : pp_ball,
			.pos = e->pos,
			.color = &clr,
			.move = move_asymptotic_simple(aim, 1 + 8 * s),
		);

		for(int j = 0; j < 4; ++j) {
			aim *= 0.8;
			PROJECTILE(
				.proto = pp_rice,
				.pos = e->pos,
				.color = &clr,
				// .move = move_asymptotic_simple(aim * (0.8 - 0.2 * j), 1 + 4 * s),
				.move = move_asymptotic_simple(aim, 1 + 8 * s),
			);
		}
	}

	WAIT(10);

	e->move.attraction = 0;
	e->move.retention = 0.8;
	e->move.acceleration = ARGS.exit_accel;
}

// opening. projectile bursts
TASK(burst_fairies_1, NO_ARGS) {
	for(int i = 3; i--;) {
		INVOKE_TASK(burst_fairy, VIEWPORT_W/2 + 70,  1 + 0.6*I);
		INVOKE_TASK(burst_fairy, VIEWPORT_W/2 - 70, -1 + 0.6*I);
		stage_wait(25);
	}
}

// more bursts. fairies move / \ like
TASK(burst_fairies_2, NO_ARGS) {
	for(int i = 3; i--;) {
		double ofs = 70 + i * 40;
		INVOKE_TASK(burst_fairy, ofs,               1 + 0.6*I);
		stage_wait(15);
		INVOKE_TASK(burst_fairy, VIEWPORT_W - ofs, -1 + 0.6*I);
		stage_wait(15);
	}
}

TASK(burst_fairies_3, NO_ARGS) {
	for(int i = 10; i--;) {
		cmplx pos = VIEWPORT_W/2 - 200 * sin(1.17 * global.frames);
		INVOKE_TASK(burst_fairy, pos, rng_sign());
		stage_wait(60);
	}
}

// swirl, sine pass
TASK(sinepass_swirls, { int duration; double level; double dir; }) {
	int duration = ARGS.duration;
	double dir = ARGS.dir;
	cmplx pos = CMPLX(ARGS.dir < 0 ? VIEWPORT_W : 0, ARGS.level);
	int delay = difficulty_value(30, 20, 15, 10);

	for(int t = 0; t < duration; t += delay) {
		INVOKE_TASK(sinepass_swirl, pos, 3.5 * dir, 7.0 * I);
		stage_wait(delay);
	}
}

// big fairies, circle + projectile toss
TASK(circletoss_fairies_1, NO_ARGS) {
	for(int i = 0; i < 2; ++i) {
		INVOKE_TASK(circletoss_fairy,
			.pos = VIEWPORT_W * i + VIEWPORT_H / 3 * I,
			.velocity = 2 - 4 * i - 0.3 * I,
			.exit_accel = 0.03 * (1 - 2 * i) - 0.04 * I ,
			.exit_time = (global.diff > D_Easy) ? 500 : 240
		);

		stage_wait(50);
	}
}

TASK(drop_swirls, { int cnt; cmplx pos; cmplx vel; cmplx accel; }) {
	for(int i = 0; i < ARGS.cnt; ++i) {
		INVOKE_TASK(drop_swirl, ARGS.pos, ARGS.vel, ARGS.accel);
		stage_wait(20);
	}
}

TASK(schedule_swirls, NO_ARGS) {
	INVOKE_TASK(drop_swirls, 25, VIEWPORT_W/3, 2*I, 0.06);
	stage_wait(400);
	INVOKE_TASK(drop_swirls, 25, 200*I, 4, -0.06*I);
}

TASK(circle_fairies_1, NO_ARGS) {
	for(int i = 0; i < 3; ++i) {
		for(int j = 0; j < 3; ++j) {
			INVOKE_TASK(circle_fairy, VIEWPORT_W - 64, VIEWPORT_W/2 - 100 + 200 * I + 128 * j);
			stage_wait(60);
		}

		stage_wait(90);

		for(int j = 0; j < 3; ++j) {
			INVOKE_TASK(circle_fairy, 64, VIEWPORT_W/2 + 100 + 200 * I - 128 * j);
			stage_wait(60);
		}

		stage_wait(240);
	}
}

TASK(multiburst_fairies_1, NO_ARGS) {
	for(int row = 0; row < 3; ++row) {
		for(int col = 0; col < 5; ++col) {
			log_debug("WTF %i %i", row, col);
			cmplx pos = rng_range(0, VIEWPORT_W);
			cmplx target_pos = 64 + 64 * col + I * (64 * row + 100);
			cmplx exit_accel = 0.02 * I + 0.03;
			INVOKE_TASK(multiburst_fairy, pos, target_pos, exit_accel);

			WAIT(10);
		}

		WAIT(120);
	}
}

TASK(instantcircle_fairies, { int duration; }) {
	int interval = difficulty_value(160, 130, 100, 70);

	for(int t = ARGS.duration; t > 0; t -= interval) {
		double x = VIEWPORT_W/2 + 205 * sin(2.13*global.frames);
		double y = VIEWPORT_H/2 + 120 * cos(1.91*global.frames);
		INVOKE_TASK(instantcircle_fairy, x, x+y*I, 0.2 * I);
		WAIT(interval);
	}
}

TASK(waveshot_fairies, { int duration; }) {
	int interval = 200;

	for(int t = ARGS.duration; t > 0; t -= interval) {
		double x = VIEWPORT_W/2 + round(rng_sreal() * 69);
		double y = rng_range(200, 240);
		INVOKE_TASK(waveshot_fairy, x, x+y*I, 0.15 * I);
		WAIT(interval);
	}
}

TASK_WITH_INTERFACE(midboss_intro, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK();
	BEGIN_BOSS_ATTACK();
	boss->move = move_towards(VIEWPORT_W/2.0 + 200.0*I, 0.035);
}

#define SNOWFLAKE_ARMS 6

static int snowflake_bullet_limit(int size) {
	// >= number of bullets spawned per snowflake of this size
	return SNOWFLAKE_ARMS * 4 * size;
}

TASK(make_snowflake, { cmplx pos; MoveParams move; int size; double rot_angle; BoxedProjectileArray *array; }) {
	const double spacing = 12;
	const int split = 3;
	int t = 0;

	for(int j = 0; j < ARGS.size; j++) {
		play_loop("shot1_loop");

		for(int i = 0; i < SNOWFLAKE_ARMS; i++) {
			double ang = M_TAU / SNOWFLAKE_ARMS * i + ARGS.rot_angle;
			cmplx phase = cdir(ang);
			cmplx pos0 = ARGS.pos + spacing * j * phase;

			Projectile *p;

			for(int side = -1; side <= 1; side += 2) {
				p = PROJECTILE(
					.proto = pp_crystal,
					.pos = pos0 + side * 5 * I * phase,
					.color = RGB(0.0 + 0.05 * j, 0.1 + 0.1 * j, 0.9),
					.move = ARGS.move,
					.angle = ang + side * M_PI / 4,
					.max_viewport_dist = 128,
					.flags = PFLAG_MANUALANGLE,
				);
				move_update_multiple(t, &p->pos, &p->move);
				p->pos0 = p->prevpos = p->pos;
				ENT_ARRAY_ADD(ARGS.array, p);
			}

			WAIT(1);
			++t;

			if(j > split) {
				cmplx pos1 = ARGS.pos + spacing * split * phase;

				for(int side = -1; side <= 1; side += 2) {
					cmplx phase2 = cdir(M_PI / 4 * side) * phase;
					cmplx pos2 = pos1 + (spacing * (j - split)) * phase2;

					p = PROJECTILE(
						.proto = pp_crystal,
						.pos = pos2,
						.color = RGB(0.0, 0.3 * ARGS.size / 5, 1),
						.move = ARGS.move,
						.angle = ang + side * M_PI / 4,
						.max_viewport_dist = 128,
						.flags = PFLAG_MANUALANGLE,
					);
					move_update_multiple(t, &p->pos, &p->move);
					p->pos0 = p->prevpos = p->pos;
					ENT_ARRAY_ADD(ARGS.array, p);
				}

				WAIT(1);
				++t;
			}
		}
	}
}

TASK_WITH_INTERFACE(icy_storm, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK();
	boss->move = move_towards(CMPLX(VIEWPORT_W/2, 200), 0.02);
	BEGIN_BOSS_ATTACK();
	boss->move = move_stop(0.8);

	int flake_spawn_interval = difficulty_value(11, 10, 9, 8);
	int flakes_per_burst = difficulty_value(3, 5, 7, 9);
	double launch_speed = difficulty_value(5, 6.25, 6.875, 8.75);
	int size_base = 5;
	int size_oscillation = 3;
	int size_max = size_base + size_oscillation;
	int burst_interval = difficulty_value(120, 80, 80, 80);

	int flakes_limit = flakes_per_burst * snowflake_bullet_limit(size_max);
	DECLARE_ENT_ARRAY(Projectile, snowflake_projs, flakes_limit);

	for(int burst = 0;; ++burst) {
		double angle_ofs = carg(global.plr.pos - boss->pos);
		int size = size_base + size_oscillation * sin(burst * 2.21);

		for(int flake = 0; flake < flakes_per_burst; ++flake) {
			double angle = circle_angle(flake + flakes_per_burst / 2, flakes_per_burst) + angle_ofs;
			MoveParams move = move_asymptotic(launch_speed * cdir(angle), 0, 0.95);
			INVOKE_SUBTASK(make_snowflake, boss->pos, move, size, angle, &snowflake_projs);
			WAIT(flake_spawn_interval);
		}

		WAIT(65 - 4 * (size_base + size_oscillation - size));

		play_sound("redirect");
		// play_sound("shot_special1");

		ENT_ARRAY_FOREACH(&snowflake_projs, Projectile *p, {
			spawn_projectile_highlight_effect(p)->opacity = 0.25;
			color_lerp(&p->color, RGB(0.5, 0.5, 0.5), 0.5);
			p->move.velocity = 2 * cdir(p->angle);
			p->move.acceleration = -cdir(p->angle) * difficulty_value(0.1, 0.15, 0.2, 0.2);
		});

		ENT_ARRAY_CLEAR(&snowflake_projs);
		WAIT(burst_interval);
	}
}

DEFINE_EXTERN_TASK(stage1_spell_perfect_freeze) {
	Boss *boss = INIT_BOSS_ATTACK();
	BEGIN_BOSS_ATTACK();

	for(int run = 1;;run++) {
		boss->move = move_towards(VIEWPORT_W/2.0 + 100.0*I, 0.04);

		int n = global.diff;
		int nfrog = n*60;

		DECLARE_ENT_ARRAY(Projectile, projs, nfrog);

		WAIT(20);
		for(int i = 0; i < nfrog/n; i++) {
			play_loop("shot1_loop");

			float r = rng_f32();
			float g = rng_f32();
			float b = rng_f32();

			for(int j = 0; j < n; j++) {
				float speed = rng_range(1.0f, 5.0f + 0.5f * global.diff);

				ENT_ARRAY_ADD(&projs, PROJECTILE(
					.proto = pp_ball,
					.pos = boss->pos,
					.color = RGB(r, g, b),
					.move = move_linear(speed * rng_dir()),
				));
			}
			YIELD;
		}
		WAIT(20);

		ENT_ARRAY_FOREACH(&projs, Projectile *p, {
			spawn_stain(p->pos, p->angle, 30);
			spawn_stain(p->pos, p->angle, 30);
			spawn_projectile_highlight_effect(p);
			play_sound("shot_special1");

			p->color = *RGB(0.9, 0.9, 0.9);
			p->move.retention = 0.8 * rng_dir();
			if(rng_chance(0.2)) {
				YIELD;
			}
		});

		WAIT(60);
		double dir = rng_sign();
		boss->move = (MoveParams){ .velocity = dir*2.7+I, .retention = 0.99, .acceleration = -dir*0.017 };

		aniplayer_queue(&boss->ani,"(9)",0);
		int d = max(0, global.diff - D_Normal);
		WAIT(60-5*global.diff);

		ENT_ARRAY_FOREACH(&projs, Projectile *p, {
			p->color = *RGB(0.9, 0.9, 0.9);
			p->move.retention = 1 + 0.002 * global.diff * rng_f64();
			p->move.velocity = 2 * rng_dir();
			spawn_stain(p->pos, p->angle, 30);
			spawn_projectile_highlight_effect(p);
			play_sound_ex("shot2", 0, false);

			if(rng_chance(0.4)) {
				YIELD;
			}
		});

		for(int i = 0; i < 30+10*d; i++) {
			play_loop("shot1_loop");
			float r1, r2;

			if(global.diff > D_Normal) {
				r1 = sin(i/M_PI*5.3) * cos(2*i/M_PI*5.3);
				r2 = cos(i/M_PI*5.3) * sin(2*i/M_PI*5.3);
			} else {
				r1 = rng_f32();
				r2 = rng_f32();
			}

			cmplx aim = cnormalize(global.plr.pos - boss->pos);
			float speed = 2+0.2*global.diff;

			for(int sign = -1; sign <= 1; sign += 2) {
				PROJECTILE(
					.proto = pp_rice,
					.pos = boss->pos + sign*60,
					.color = RGB(0.3, 0.4, 0.9),
					.move = move_asymptotic_simple(speed*aim*cdir(0.5*(sign > 0 ? r1 : r2)), 2.5+(global.diff>D_Normal)*0.1*sign*I),
				);
			}
			WAIT(6-global.diff/2);
		}
		aniplayer_queue(&boss->ani,"main",0);

		WAIT(40-5*global.diff);
	}
}

TASK_WITH_INTERFACE(midboss_flee, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK();
	BEGIN_BOSS_ATTACK();
	boss->move = move_towards(-250 + 30 * I, 0.02);
}

TASK(spawn_midboss, NO_ARGS) {
	STAGE_BOOKMARK_DELAYED(120, midboss);

	Boss *boss = global.boss = stage1_spawn_cirno(VIEWPORT_W + 220 + 30.0*I);

	boss_add_attack_task(boss, AT_Move, "Introduction", 2, 0, TASK_INDIRECT(BossAttack, midboss_intro), NULL);
	boss_add_attack_task(boss, AT_Normal, "Icy Storm", 20, 24000, TASK_INDIRECT(BossAttack, icy_storm), NULL);
	boss_add_attack_from_info(boss, &stage1_spells.mid.perfect_freeze, false);
	boss_add_attack_task(boss, AT_Move, "Introduction", 2, 0, TASK_INDIRECT(BossAttack, midboss_flee), NULL);

	boss_start_attack(boss, boss->attacks);

	WAIT(60);
	stage1_bg_enable_snow();
}

TASK(tritoss_fairy, { cmplx pos; cmplx velocity; cmplx end_velocity; }) {
	Enemy *e = TASK_BIND_UNBOXED(create_enemy1c(ARGS.pos, 16000, BigFairy, NULL, 0));

	INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, {
		.points = 5,
		.power = 5,
	});

	e->move = move_linear(ARGS.velocity);
	WAIT(60);
	e->move.retention = 0.9;
	WAIT(20);

	int interval = difficulty_value(7,6,5,3);
	int rounds = 680/interval;
	for(int k = 0; k < rounds; k++) {
		play_sound("shot1");

		float a = M_PI / 30.0 * ((k/7) % 30) + 0.1 * rng_f32();
		int n = difficulty_value(3,4,4,5);

		for(int i = 0; i < n; i++) {
			PROJECTILE(
				.proto = pp_thickrice,
				.pos = e->pos,
				.color = RGB(0.2, 0.4, 0.8),
				.move = move_asymptotic_simple(2*cdir(a+2.0*M_PI/n*i), 3),
			);
		}

		if(k == rounds/2 || k == rounds-1) {
			play_sound("shot_special1");
			int n2 = difficulty_value(20, 23, 26, 30);
			for(int i = 0; i < n2; i++) {
				PROJECTILE(
					.proto = pp_rice,
					.pos = e->pos,
					.color = RGB(0.6, 0.2, 0.7),
					.move = move_asymptotic_simple(1.5*cexp(I*2*M_PI/n2*i),2),
				);

				if(global.diff > D_Easy) {
					PROJECTILE(
						.proto = pp_rice,
						.pos = e->pos,
						.color = RGB(0.6, 0.2, 0.7),
						.move = move_asymptotic_simple(3*cexp(I*2*M_PI/n2*i), 3.0),
					);
				}
			}
		}
		WAIT(interval);
	}

	WAIT(20);
	e->move = move_asymptotic_simple(ARGS.end_velocity, -1);
}

TASK(spawn_boss, NO_ARGS) {
	STAGE_BOOKMARK_DELAYED(120, boss);

	Boss *boss = global.boss = stage1_spawn_cirno(-230 + 100.0*I);

	boss_add_attack(boss, AT_Move, "Introduction", 2, 0, cirno_intro_boss, NULL);
	boss_add_attack(boss, AT_Normal, "Iceplosion 0", 20, 23000, cirno_iceplosion0, NULL);
	boss_add_attack_from_info(boss, &stage1_spells.boss.crystal_rain, false);
	boss_add_attack(boss, AT_Normal, "Iceplosion 1", 20, 24000, cirno_iceplosion1, NULL);

	if(global.diff > D_Normal) {
		boss_add_attack_from_info(boss, &stage1_spells.boss.snow_halation, false);
	}

	boss_add_attack_from_info(boss, &stage1_spells.boss.icicle_fall, false);
	boss_add_attack_from_info(boss, &stage1_spells.extra.crystal_blizzard, false);

	boss_start_attack(boss, boss->attacks);
}

TASK(stage_timeline, NO_ARGS) {
	stage_start_bgm("stage1");
	stage_set_voltage_thresholds(50, 125, 300, 600);

	// INVOKE_TASK(waveshot_fairy, VIEWPORT_W/2, (VIEWPORT_W+VIEWPORT_H*I)*0.5, 0.5*I);
	// return;

	INVOKE_TASK_DELAYED(100, burst_fairies_1);
	INVOKE_TASK_DELAYED(240, burst_fairies_2);
	INVOKE_TASK_DELAYED(440, sinepass_swirls, 180, 100, 1);
	INVOKE_TASK_DELAYED(480, circletoss_fairies_1);
	INVOKE_TASK_DELAYED(660, circle_fairies_1);
	INVOKE_TASK_DELAYED(900, schedule_swirls);
	INVOKE_TASK_DELAYED(1500, burst_fairies_3);
	INVOKE_TASK_DELAYED(2200, multiburst_fairies_1);

	INVOKE_TASK_DELAYED(2200, common_call_func, stage1_bg_raise_camera);
	STAGE_BOOKMARK_DELAYED(2500, pre-midboss);
	INVOKE_TASK_DELAYED(2700, spawn_midboss);

	while(!global.boss) YIELD;
	int midboss_time = WAIT_EVENT(&global.boss->events.defeated).frames;
	int filler_time = 2180;
	int time_ofs = 500 - midboss_time;

	log_debug("midboss_time = %i; filler_time = %i; time_ofs = %i", midboss_time, filler_time, time_ofs);

	STAGE_BOOKMARK(post-midboss);

	int swirl_spam_time = 760;

	for(int i = 0; i < swirl_spam_time; i += 30) {
		int o = ((int[]) { 0, 1, 0, -1 })[(i / 60) % 4];
		INVOKE_TASK_DELAYED(i + time_ofs, sinepass_swirls, 40, 132 + 32 * o, 1 - 2 * ((i / 60) & 1));
	}

	time_ofs += swirl_spam_time;

	INVOKE_TASK_DELAYED(40 + time_ofs, burst_fairies_1);

	int instacircle_time = filler_time - swirl_spam_time - 600;

	for(int i = 0; i < instacircle_time; i += 180) {
		INVOKE_TASK_DELAYED(i + time_ofs, sinepass_swirls, 80, 132, 1);
		INVOKE_TASK_DELAYED(120 + i + time_ofs, instantcircle_fairies, 120);
	}

	WAIT(filler_time - midboss_time);
	STAGE_BOOKMARK(post-midboss-filler);

	INVOKE_TASK_DELAYED(100, circletoss_fairy,           -25 + VIEWPORT_H/3*I,  1 - 0.5*I, 0.01 * ( 1 - I), 200);
	INVOKE_TASK_DELAYED(125, circletoss_fairy, VIEWPORT_W+25 + VIEWPORT_H/3*I, -1 - 0.5*I, 0.01 * (-1 - I), 200);

	if(global.diff > D_Normal) {
		INVOKE_TASK_DELAYED(100, circletoss_fairy,           -25 + 2*VIEWPORT_H/3*I,  1 - 0.5*I, 0.01 * ( 1 - I), 200);
		INVOKE_TASK_DELAYED(125, circletoss_fairy, VIEWPORT_W+25 + 2*VIEWPORT_H/3*I, -1 - 0.5*I, 0.01 * (-1 - I), 200);
	}

	INVOKE_TASK_DELAYED(240, waveshot_fairies, 600);
	INVOKE_TASK_DELAYED(400, burst_fairies_3);

	STAGE_BOOKMARK_DELAYED(1000, post-midboss-filler-2);

	INVOKE_TASK_DELAYED(1000, burst_fairies_1);
	INVOKE_TASK_DELAYED(1120, explosion_fairy,              120*I, VIEWPORT_W-80 + 120*I, -0.2+0.1*I);
	INVOKE_TASK_DELAYED(1280, explosion_fairy, VIEWPORT_W + 220*I,            80 + 220*I,  0.2+0.1*I);

	STAGE_BOOKMARK_DELAYED(1400, post-midboss-filler-3);

	INVOKE_TASK_DELAYED(1400, drop_swirls, 25, 2*VIEWPORT_W/3, 2*I, -0.06);
	INVOKE_TASK_DELAYED(1600, drop_swirls, 25,   VIEWPORT_W/3, 2*I,  0.06);

	INVOKE_TASK_DELAYED(1520, tritoss_fairy, VIEWPORT_W / 2 - 30*I, 3 * I, -2.6 * I);

	INVOKE_TASK_DELAYED(1820, circle_fairy, VIEWPORT_W + 42 + 300*I, VIEWPORT_W - 130 + 240*I);
	INVOKE_TASK_DELAYED(1820, circle_fairy,            - 42 + 300*I,              130 + 240*I);

	INVOKE_TASK_DELAYED(1880, instantcircle_fairy, VIEWPORT_W + 42 + 300*I, VIEWPORT_W -  84 + 260*I, 0.2 * (-2 - I));
	INVOKE_TASK_DELAYED(1880, instantcircle_fairy,            - 42 + 300*I,               84 + 260*I, 0.2 * ( 2 - I));

	INVOKE_TASK_DELAYED(2120, waveshot_fairy, VIEWPORT_W + 42 + 300*I,               130 + 140*I, 0.2 * (-2 - I));
	INVOKE_TASK_DELAYED(2120, waveshot_fairy,            - 42 + 300*I, VIEWPORT_W -  130 + 140*I, 0.2 * ( 2 - I));

	STAGE_BOOKMARK_DELAYED(2300, pre-boss);

	WAIT(2560);
	INVOKE_TASK(spawn_boss);
	while(!global.boss) YIELD;
	WAIT_EVENT(&global.boss->events.defeated);

	stage_unlock_bgm("stage1boss");

	WAIT(120);
	stage1_bg_disable_snow();
	WAIT(120);

	global.dialog = stage1_dialog_post_boss();
	while(dialog_is_active(global.dialog)) {
		YIELD;
	}

	WAIT(5);
	stage_finish(GAMEOVER_SCORESCREEN);
}

void stage1_events(void) {
	TIMER(&global.timer);

	AT(0) {
		INVOKE_TASK(stage_timeline);
	}

	return;
}
