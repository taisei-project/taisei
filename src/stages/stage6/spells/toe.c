/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"
#include "stagetext.h"
#include "common_tasks.h"

#define LASER_EXTENT (4+1.5*global.diff-D_Normal)
#define LASER_LENGTH 60

void elly_spellbg_toe(Boss *b, int t) {
	r_shader("sprite_default");

	r_draw_sprite(&(SpriteParams) {
		.pos = { VIEWPORT_W/2, VIEWPORT_H/2 },
		.scale.both = 0.75 + 0.0005 * t,
		.rotation.angle = t * 0.1 * DEG2RAD,
		.sprite = "stage6/spellbg_toe",
		.color = RGB(0.6, 0.6, 0.6),
	});

	float positions[][2] = {
		{-160,0},
		{0,0},
		{0,100},
		{0,200},
		{0,300},
	};

	int delays[] = {
		-20,
		0,
		FERMIONTIME,
		HIGGSTIME,
		YUKAWATIME,
	};

	int count = sizeof(delays)/sizeof(int);
	for(int i = 0; i < count; i++) {
		if(t<delays[i])
			break;

		r_color(RGBA_MUL_ALPHA(1, 1, 1, 0.5*clamp((t-delays[i])*0.1,0,1)));
		char texname[33];
		snprintf(texname, sizeof(texname), "stage6/toelagrangian/%d", i);
		float wobble = fmax(0,t-BREAKTIME)*0.03;

		r_draw_sprite(&(SpriteParams) {
			.sprite = texname,
			.pos = {
				VIEWPORT_W/2+positions[i][0]+cos(wobble+i)*wobble,
				VIEWPORT_H/2-150+positions[i][1]+sin(i+wobble)*wobble,
			},
		});
	}

	r_color4(1, 1, 1, 1);
	r_shader_standard();
}

static cmplx wrap_around(cmplx *pos) {
	// This function only works approximately. If more than one of these conditions are true,
	// dir has to correspond to the wall that was passed first for the
	// current preview display to work.
	//
	// with some clever geometry this could be either fixed here or in the
	// preview calculation, but the spell as it is currently seems to work
	// perfectly with this simplified version.

	cmplx dir = 0;
	if(creal(*pos) < -10) {
		*pos += VIEWPORT_W;
		dir += -1;
	}
	if(creal(*pos) > VIEWPORT_W+10) {
		*pos -= VIEWPORT_W;
		dir += 1;
	}
	if(cimag(*pos) < -10) {
		*pos += I*VIEWPORT_H;
		dir += -I;
	}
	if(cimag(*pos) > VIEWPORT_H+10) {
		*pos -= I*VIEWPORT_H;
		dir += I;
	}

	return dir;
}

static int elly_toe_boson_effect(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	p->angle = creal(p->args[3]) * fmax(0, t) / (double)p->timeout;
	return ACTION_NONE;
}

static Color* boson_color(Color *out_clr, int pos, int warps) {
	float f = pos / 3.0;
	*out_clr = *HSL((warps-0.3*(1-f))/3.0, 1+f, 0.5);
	return out_clr;
}

static int elly_toe_boson(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	p->angle = carg(p->args[0]);

	int activate_time = creal(p->args[2]);
	int num_in_trail = cimag(p->args[2]);
	assert(activate_time >= 0);

	Color thiscolor_additive = p->color;
	thiscolor_additive.a = 0;

	if(t < activate_time) {
		float fract = clamp(2 * (t+1) / (float)activate_time, 0, 1);

		float a = 0.1;
		float x = fract;
		float modulate = sin(x * M_PI) * (0.5 + cos(x * M_PI));
		fract = (pow(a, x) - 1) / (a - 1);
		fract *= (1 + (1 - x) * modulate);

		p->pos = p->args[3] * fract + p->pos0 * (1 - fract);

		if(t % 3 == 0) {
			PARTICLE(
				.sprite = "smoothdot",
				.pos = p->pos,
				.color = &thiscolor_additive,
				.rule = linear,
				.timeout = 10,
				.draw_rule = Fade,
				.args = { 0, p->args[0] },
			);
		}

		return ACTION_NONE;
	}

	if(t == activate_time && num_in_trail == 2) {
		play_sfx("shot_special1");
		play_sfx("redirect");

			PARTICLE(
				.sprite = "blast",
				.pos = p->pos,
				.color = HSLA(carg(p->args[0]),0.5,0.5,0),
				.rule = elly_toe_boson_effect,
				.draw_rule = ScaleFade,
				.timeout = 60,
				.args = {
					0,
					p->args[0] * 1.0,
					0. + 1.0 * I,
					M_PI*2*frand(),
				},
				.angle = 0,
				.flags = PFLAG_REQUIREDPARTICLE,
			);
	}

	p->pos += p->args[0];
	cmplx prev_pos = p->pos;
	int warps_left = creal(p->args[1]);
	int warps_initial = cimag(p->args[1]);

	if(wrap_around(&p->pos) != 0) {
		p->prevpos = p->pos; // don't lerp

		if(warps_left-- < 1) {
			return ACTION_DESTROY;
		}

		p->args[1] = warps_left + warps_initial * I;

		if(num_in_trail == 3) {
			play_sfx_ex("warp", 0, false);
			// play_sound("redirect");
		}

		PARTICLE(
			.sprite = "myon",
			.pos = prev_pos,
			.color = &thiscolor_additive,
			.timeout = 50,
			.draw_rule = ScaleFade,
			.args = { 0, 0, 5.00 },
			.angle = M_PI*2*frand(),
		);

		PARTICLE(
			.sprite = "stardust",
			.pos = prev_pos,
			.color = &thiscolor_additive,
			.timeout = 60,
			.draw_rule = ScaleFade,
			.args = { 0, 0, 2 * I },
			.angle = M_PI*2*frand(),
			.flags = PFLAG_REQUIREDPARTICLE,
		);

		boson_color(&p->color, num_in_trail, warps_initial - warps_left);
		thiscolor_additive = p->color;
		thiscolor_additive.a = 0;

		PARTICLE(
			.sprite = "stardust",
			.pos = p->pos,
			.color = &thiscolor_additive,
			.rule = elly_toe_boson_effect,
			.draw_rule = ScaleFade,
			.timeout = 30,
			.args = { 0, p->args[0] * 2, 2 * I, M_PI*2*frand() },
			.angle = 0,
			.flags = PFLAG_REQUIREDPARTICLE,
		);
	}

	float tLookahead = 40;
	cmplx posLookahead = p->pos+p->args[0]*tLookahead;
	cmplx dir = wrap_around(&posLookahead);
	if(dir != 0 && t%3 == 0 && warps_left > 0) {
		cmplx pos0 = posLookahead - VIEWPORT_W/2*(1-creal(dir))-I*VIEWPORT_H/2*(1-cimag(dir));

		// Re [a b^*] behaves like the 2D vector scalar product
		float tOvershoot = creal(pos0*conj(dir))/creal(p->args[0]*conj(dir));
		posLookahead -= p->args[0]*tOvershoot;

		Color *clr = boson_color(&(Color){0}, num_in_trail, warps_initial - warps_left + 1);
		clr->a = 0;

		PARTICLE(
			.sprite = "smoothdot",
			.pos = posLookahead,
			.color = clr,
			.timeout = 10,
			.rule = linear,
			.draw_rule = Fade,
			.args = { p->args[0] },
			.flags = PFLAG_REQUIREDPARTICLE,
		);
	}

	return ACTION_NONE;
}

static cmplx elly_toe_laser_pos(Laser *l, float t) { // a[0]: direction, a[1]: type, a[2]: width
	int type = creal(l->args[1]+0.5);

	if(t == EVENT_BIRTH) {
		switch(type) {
		case 0:
			l->color = *RGBA(0.4, 0.4, 1.0, 0.0);
			break;
		case 1:
			l->color = *RGBA(1.0, 0.4, 0.4, 0.0);
			break;
		case 2:
			l->color = *RGBA(0.4, 1.0, 0.4, 0.0);
			break;
		case 3:
			l->color = *RGBA(1.0, 0.4, 1.0, 0.0);
			break;
		default:
			log_fatal("Unknown Elly laser type.");
		}
		return 0;
	}

	double width = creal(l->args[2]);
	switch(type) {
	case 0:
		return l->pos+l->args[0]*t;
	case 1:
		return l->pos+l->args[0]*(t+width*I*sin(t/width));
	case 2:
		return l->pos+l->args[0]*(t+width*(0.6*(cos(3*t/width)-1)+I*sin(3*t/width)));
	case 3:
		return l->pos+l->args[0]*(t+floor(t/width)*width);
	}

	return 0;
}

static int elly_toe_laser_particle_rule(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		free_ref(p->args[3]);
	}

	if(t < 0) {
		return ACTION_ACK;
	}

	Laser *l = REF(p->args[3]);

	if(!l) {
		return ACTION_DESTROY;
	}

	p->pos = l->prule(l, 0);

	return ACTION_NONE;
}

static void elly_toe_laser_particle(Laser *l, cmplx origin) {
	Color *c = color_mul(COLOR_COPY(&l->color), &l->color);
	c->a = 0;

	PARTICLE(
		.sprite = "stardust",
		.pos = origin,
		.color = c,
		.rule = elly_toe_laser_particle_rule,
		.draw_rule = ScaleFade,
		.timeout = 30,
		.args = { 0, 0, 1.5 * I, add_ref(l) },
		.angle = M_PI*2*frand(),
	);

	PARTICLE(
		.sprite = "stain",
		.pos = origin,
		.color = color_mul_scalar(COLOR_COPY(c),0.5),
		.rule = elly_toe_laser_particle_rule,
		.draw_rule = ScaleFade,
		.timeout = 20,
		.args = { 0, 0, 1 * I, add_ref(l) },
		.angle = M_PI*2*frand(),
		.flags = PFLAG_REQUIREDPARTICLE,
	);

	PARTICLE(
		.sprite = "smoothdot",
		.pos = origin,
		.color = c,
		.rule = elly_toe_laser_particle_rule,
		.draw_rule = ScaleFade,
		.timeout = 40,
		.args = { 0, 0, 1, add_ref(l) },
		.angle = M_PI*2*frand(),
		.flags = PFLAG_REQUIREDPARTICLE,
	);
}

static void elly_toe_laser_logic(Laser *l, int t) {
	if(t == l->deathtime) {
		static const int transfer[][4] = {
			{1, 0, 0, 0},
			{0,-1,-1, 3},
			{0,-1, 2, 3},
			{0, 3, 3, 1}
		};

		cmplx newpos = l->prule(l,t);
		if(creal(newpos) < 0 || cimag(newpos) < 0 || creal(newpos) > VIEWPORT_W || cimag(newpos) > VIEWPORT_H)
			return;

		int newtype, newtype2;
		do {
			newtype = 3.999999*frand();
			newtype2 = transfer[(int)(creal(l->args[1])+0.5)][newtype];
			// actually in this case, gluons are also always possible
			if(newtype2 == 1 && frand() > 0.5) {
				newtype = 2;
			}

			// I removed type 3 because i can’t draw dotted lines and it would be too difficult either way
		} while(newtype2 == -1 || newtype2 == 3 || newtype == 3);

		cmplx origin = l->prule(l,t);
		cmplx newdir = cexp(0.3*I);

		Laser *l1 = create_laser(origin,LASER_LENGTH,LASER_LENGTH,RGBA(1, 1, 1, 0),
			elly_toe_laser_pos,elly_toe_laser_logic,
			l->args[0]*newdir,
			newtype,
			LASER_EXTENT,
			0
		);

		Laser *l2 = create_laser(origin,LASER_LENGTH,LASER_LENGTH,RGBA(1, 1, 1, 0),
			elly_toe_laser_pos,elly_toe_laser_logic,
			l->args[0]/newdir,
			newtype2,
			LASER_EXTENT,
			0
		);

		elly_toe_laser_particle(l1, origin);
		elly_toe_laser_particle(l2, origin);
	}
	l->pos+=I*0.2;
}

static bool elly_toe_its_yukawatime(cmplx pos) {
	int t = global.frames-global.boss->current->starttime;

	if(pos == 0)
		return t>YUKAWATIME;

	int sectors = 4;

	pos -= global.boss->pos;
	if(((int)(carg(pos)/2/M_PI*sectors+sectors+0.005*t))%2)
		return t > YUKAWATIME+0.2*cabs(pos);

	return false;
}

static int elly_toe_fermion_yukawa_effect(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		free_ref(p->args[0]);
	}

	if(t < 0) {
		return ACTION_ACK;
	}

	Projectile *master = REF(p->args[0]);

	if(master) {
		p->pos = master->pos;
	}

	return ACTION_NONE;
}

static int elly_toe_fermion(Projectile *p, int t) {
	if(t == EVENT_BIRTH) {
		p->pos0 = 0;
	}

	if(t < 0) {
		return ACTION_ACK;
	}

	int speeduptime = creal(p->args[2]);
	if(t > speeduptime)
		p->pos0 += p->pos0/cabs(p->pos0)*(t-speeduptime)*0.01;
	else
		p->pos0 += (p->args[0]-p->pos0)*0.01;
	p->pos = global.boss->pos+p->pos0*cexp(I*0.01*t)+5*cexp(I*t*0.05+I*p->args[1]);

	Color thiscolor_additive = p->color;
	thiscolor_additive.a = 0;

	if(t > 0 && t % 5 == 0) {
		double particle_scale = fmin(1.0, 0.5 * p->sprite->w / 28.0);

		PARTICLE(
			.sprite = "stardust",
			.pos = p->pos,
			.color = &thiscolor_additive,
			.timeout = fmin(t / 6.0, 10),
			.draw_rule = ScaleFade,
			.args = { 0, 0, particle_scale * (0.5 + 2 * I) },
			.angle = M_PI*2*frand(),
		);
	}

	if(creal(p->args[3]) < 0) // add a way to disable the yukawa phase
		return ACTION_NONE;

	if(elly_toe_its_yukawatime(p->pos)) {
		if(!p->args[3]) {
			projectile_set_prototype(p, pp_bigball);
			p->args[3]=1;
			play_sfx_ex("shot_special1", 5, false);

			PARTICLE(
				.sprite = "blast",
				.pos = p->pos,
				.color = &thiscolor_additive,
				.rule = elly_toe_fermion_yukawa_effect,
				.timeout = 60,
				.draw_rule = ScaleFade,
				.args = { add_ref(p), 0, 0 + 1 * I },
				.angle = M_PI*2*frand(),
			);

		}

		p->pos0*=1.01;
	} else if(p->args[3]) {
		projectile_set_prototype(p, pp_ball);
		p->args[3]=0;
	}

	return ACTION_NONE;
}

static int elly_toe_higgs(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	if(elly_toe_its_yukawatime(p->pos)) {
		if(!p->args[3]) {
			projectile_set_prototype(p, pp_rice);
			p->args[3]=1;
		}
		p->args[0]*=1.01;
	} else if(p->args[3]) {
		projectile_set_prototype(p, pp_flea);
		p->args[3]=0;
	}

	int global_time = global.frames - global.boss->current->starttime - HIGGSTIME;
	int max_time = SYMMETRYTIME - HIGGSTIME;
	double rotation = 0.5 * M_PI * fmax(0, (int)global.diff - D_Normal);

	if(global_time > max_time) {
		global_time = max_time;
	}

	cmplx vel = p->args[0] * cexp(I*rotation*global_time/(float)max_time);
	p->pos = p->pos0 + t * vel;
	p->angle = carg(vel);

	return ACTION_NONE;
}

// This spell contains some obscure (and some less obscure) physics references.
// However the first priority was good looks and playability, so most of the
// references are pretty bad.
//
// I am waiting for the day somebody notices this and writes a 10-page comment
// about it on reddit. ;_; So I add a disclaimer beforehand (2018). Maybe it can also
// serve as entertainment for the interested hacker stumbling across these
// lines.
//
// Disclaimer:
//
// 1. This spell is based on the Standard Model of Particle physics, which is
//	  very complicated, so the picture here is always very very very dumbed down.
//
// 2. The first two phases stand for the bosons and fermions in the model. My
//	  way of representing them was to choose a heavily overlapping pattern for
//	  the bosons and a non-overlapping one for the fermions (nod to the Pauli
//	  principle). The rest of them is not a good reference at all.
//
// 3. Especially this rgb rotating thing the fermions do has not much to do
//	  with physics. Maybe it represents color charge? And the rotation
//	  represents the way fermions transform strangely under rotations?
//
// 4. The symmetry thing is one fm closer to the truth. In reality the symmetry
//	  that is broken in the phase we live in is continuous, but that is not easy
//	  to do in danmaku, so I took a mirror symmetry and broke it.
//	  Then in reality, the higgs gets a funny property that makes the other
//	  things which interact with it get a mass.
//	  In the spell, I represented that by adding those sectors where the fermions
//	  become big (massive ;)).
//
// 5. The last part is a nod to Feynman diagrams and perturbation theory. Have
//	  been planning to add those to the game for quite some time. They are one
//	  of the iconic sights in modern physics and they can be used as laser danmaku.
//	  The rules here are again simplified:
//
//	  I only use QED and QCD (not the whole SM) and leave out ghosts and the 4
//	  gluon vertex (because I can’t draw dashed lasers and the 4-vertex doesn’t
//	  fit into the pattern).
//
//	  Of course, drawing laser danmaku is easy. The actual calculations behind
//	  the diagrams are one of the most complicated fields in Physics (fun fact:
//	  they are a driving force for CAS software and high precision integration
//	  methods). This whole thing is called perturbation theory and one of its
//	  problems is that it doesn’t really converge, so that’s the reason behind
//	  “Perturbation theory breaking down”
//
// (9). I’m only human though and this is not exactly the stuff I do, so I’m
//		not an expert and more things might be off.
//

TASK(toe_part_bosons, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	int step = 1;
	int start_time = 0;
	int end_time = FERMIONTIME + 1000;
	int cycle_step = 200;
	int solo_cycles = (FERMIONTIME - start_time) / cycle_step - global.diff + 1;
	int warp_cycles = (HIGGSTIME - start_time) / cycle_step - 1;


	for(int cycle = 0; cycle < end_time/cycle_step; cycle++) {
		int count = 30 - (count == 0); // get rid of the spawn safe spot on first cycle
		int cycle_time = count * step;
		
		for(int t = 0; t < cycle_time; t += WAIT(step)) {
			play_sfx("shot2");

			int pnum = t;
			int num_warps = 0;

			if(cycle < solo_cycles) {
				num_warps = global.diff;
			} else if(cycle < warp_cycles && global.diff > D_Normal) {
				num_warps = 1;
			}

			// The fact that there are 4 bullets per trail is actually one of the physics references.
			for(int i = 0; i < 4; i++) {
				pnum = (count - pnum - 1);

				cmplx dir = I*cdir(M_TAU / count * (pnum + 0.5));
				dir *= cdir(0.15 * sign(creal(dir)) * sin(cycle));

				cmplx bpos = boss->pos + 18 * dir * i;

				PROJECTILE(
					.proto = pp_rice,
					.pos = boss->pos,
					.color = boson_color(&(Color){0}, i, 0),
					.rule = elly_toe_boson,
					.args = {
						2.5*dir,
						num_warps * (1 + I),
						42*2 - step * t + i*I, // real: activation delay, imag: pos in trail (0 is furtherst from head)
						bpos,
					},
					.max_viewport_dist = 20,
				);
			}
		}

		WAIT(cycle_step - cycle_time);
	}
}

TASK(toe_part_fermions, { BoxedBoss boss; int duration; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	play_sfx("charge_generic");
	stage_shake_view(10);
	
	int step = 2;

	for(int t = 0; t < ARGS.duration/step; t++) {
		play_sfx_ex("shot1", 5, false);

		cmplx dest = 100 * cdir(t);
		for(int clr = 0; clr < 3; clr++) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = boss->pos,
				.color = RGBA(clr==0, clr==1, clr==2, 0),
				.rule = elly_toe_fermion,
				.args = {
					dest,
					clr*2*M_PI/3,
					 40,
				},
				.max_viewport_dist = 50,
				.flags = PFLAG_NOSPAWNFLARE,
			);
		}

		WAIT(step);
	}
}

TASK(toe_part_symmetry, { BoxedBoss boss; }) {
	Boss *b = TASK_BIND(ARGS.boss);

	play_sfx("charge_generic");
	play_sfx("boom");
	stagetext_add("Symmetry broken!", VIEWPORT_W / 2 + I * VIEWPORT_H / 4, ALIGN_CENTER, get_font("big"), RGB(1, 1, 1), 0, 100, 10, 20);
	stage_shake_view(10);

	PARTICLE(
		.sprite = "blast",
		.pos = b->pos,
		.color = RGBA(1.0, 0.3, 0.3, 0.0),
		.timeout = 60,
		.draw_rule = ScaleFade,
		.args = { 0, 0, 5*I },
		.flags = PFLAG_REQUIREDPARTICLE,
	);

	for(int i = 0; i < 5; ++i) {
		RNG_ARRAY(R, 2);
		PARTICLE(
			.sprite = "stain",
			.pos = b->pos,
			.color = RGBA(0.3, 0.3, 1.0, 0.0),
			.timeout = vrng_range(R[0], 80.0, 120.0),
			.draw_rule = ScaleFade,
			.args = { 0, 0, 2 * (0.4 + 2 * I) },
			.angle = vrng_angle(R[1])
		);
	}
}

TASK(toe_part_higgs, { BoxedBoss boss; int fast_duration; int full_duration; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	int fast_step = 4;
	int slow_step = 8; 

	int fast_steps = ARGS.fast_duration/fast_step;
	int full_steps = fast_steps + (ARGS.full_duration - ARGS.fast_duration)/slow_step;

	int time = 0; 

	for(int t = 0; t < full_steps; t++) {
		play_sfx_loop("shot1_loop");
		int arms = difficulty_value(4, 4, 5, 7);

		for(int dir = 0; dir < 2; dir++) {
			for(int arm = 0; arm < arms; arm++) {
				cmplx vel = -2 * I * cdir(M_PI / (arms+1) * (arm+1) + 0.1 * sin(time * 0.1 + arm));
				
				if(dir) {
					vel = -conj(vel);
				}
				
				if(t > fast_steps) {
					int tr = t - fast_steps;
					vel *= cexp(-I * 0.064 * tr * tr + 0.01 * rng_real() * dir);
				}

				PROJECTILE(
					.proto = pp_flea,
					.pos = boss->pos,
					.color = RGB(dir * (t > fast_steps),0,1),
					.rule = elly_toe_higgs,
					.args = { vel },
					.flags = PFLAG_NOSPAWNFLARE,
				);
			}
		}
		
		time += WAIT(t < fast_steps ? 4 : 8);
	}
}

TASK(toe_part_break_fermions, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	play_sfx("redirect");

	for(int t = 0;; t++) {
		for(int clr = 0; clr < 3; clr++) {
			PROJECTILE(
				.proto = pp_soul,
				.pos = boss->pos,
				.color = RGBA(clr==0, clr==1, clr==2, 0),
				.rule = elly_toe_fermion,
				.args = {
					50*cdir(1.3 * t),
					clr*2*M_PI/3,
					40,
					-1,
				},
				.max_viewport_dist = 50,
				.flags = PFLAG_NOSPAWNFLARE,
			);
		}
		WAIT(14);
	}
}

TASK(toe_noise_sfx) {
	for(;;) {
		play_sfx_loop("noise1");
		YIELD;
	}
}

DEFINE_EXTERN_TASK(stage6_spell_toe) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);
	
	stage_shake_view(50);
	boss_set_portrait(boss, "elly", "beaten", "shouting");
	boss->move = move_towards(ELLY_TOE_TARGET_POS, 0.1);

	assert(cabs(boss->pos - ELLY_TOE_TARGET_POS) < 1);
	elly_clap(boss,50000);

	INVOKE_SUBTASK(toe_part_bosons, ENT_BOX(boss));
	INVOKE_SUBTASK_DELAYED(FERMIONTIME, toe_part_fermions, ENT_BOX(boss), YUKAWATIME - FERMIONTIME + 240);
	INVOKE_SUBTASK_DELAYED(SYMMETRYTIME, toe_part_symmetry, ENT_BOX(boss));
	INVOKE_SUBTASK_DELAYED(HIGGSTIME, toe_part_higgs, ENT_BOX(boss),
		.fast_duration = SYMMETRYTIME - HIGGSTIME,
		.full_duration = YUKAWATIME - HIGGSTIME + 100
	);
	INVOKE_SUBTASK_DELAYED(YUKAWATIME - 60, toe_noise_sfx);

	WAIT(YUKAWATIME);

	play_sfx("charge_generic");
	stagetext_add("Coupling the Higgs!", VIEWPORT_W / 2.0 + I * VIEWPORT_H / 4.0, ALIGN_CENTER, get_font("big"), RGB(1, 1, 1), 0, 100, 10, 20);
	stage_shake_view(10);

	WAIT(YUKAWATIME - BREAKTIME);

	play_sfx("charge_generic");
	stagetext_add("Perturbation theory", VIEWPORT_W / 2.0 +I*VIEWPORT_H / 4.0, ALIGN_CENTER, get_font("big"), RGB(1, 1, 1), 0, 100, 10, 20);
	stagetext_add("breaking down!", VIEWPORT_W / 2.0+ I * VIEWPORT_H / 4.0 + 30 * I, ALIGN_CENTER, get_font("big"), RGB(1, 1, 1), 0, 100, 10, 20);
	stage_shake_view(10);

	INVOKE_SUBTASK_DELAYED(35, toe_part_break_fermions, ENT_BOX(boss));

	for(;;) {
		play_sfx_ex("laser1", 0, true);

		cmplx dir = rng_dir();
		int count = 8;
		for(int i = 0; i < count; i++) {
			create_laser(boss->pos, LASER_LENGTH, LASER_LENGTH / 2.0, RGBA(1, 1, 1, 0),
				elly_toe_laser_pos, elly_toe_laser_logic,
				2 * cdir(M_TAU / count * i) * dir,
				0,
				LASER_EXTENT,
				0
			);
		}
		
		WAIT(100);
	}
}

