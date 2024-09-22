/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "spells.h"

#include "../draw.h"

#define LASER_LENGTH 60

void elly_spellbg_toe(Boss *b, int t) {
	r_shader("sprite_default");

	r_draw_sprite(&(SpriteParams) {
		.pos = { VIEWPORT_W / 2.0, VIEWPORT_H / 2.0 },
		.scale.both = 0.75 + 0.0005 * t,
		.rotation.angle = t * 0.1 * DEG2RAD,
		.sprite_ptr = res_sprite("stage6/spellbg_toe"),
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

	int count = sizeof(delays) / sizeof(int);
	for(int i = 0; i < count; i++) {
		float a = clamp((t - delays[i]) / 240.0, 0, 1);

		if(a <= 0) {
			break;
		}

		char texname[33];
		snprintf(texname, sizeof(texname), "stage6/toelagrangian/%d", i);
		float wobble = max(0, t - BREAKTIME) * 0.03f;

		a = glm_ease_quad_inout(a) * 0.4;
		Color color;

		SpriteParams sp = {
			.color = &color,
			.sprite_ptr = res_sprite(texname),
			.pos = {
				VIEWPORT_W / 2.0 + positions[i][0] + cos(wobble + i) * wobble,
				VIEWPORT_H / 2.0 - 150 + positions[i][1] + sin(i + wobble) * wobble,
			},
			.blend = BLEND_PREMUL_ALPHA,
		};

		cmplxf basepos = sp.pos.as_cmplx;

		float f = a * 0.2;
		color.a = f;

		for(int j = 0; j < 3; ++j) {
			sp.pos.as_cmplx = basepos + 2 * cdir(0.1 * t + j*M_TAU/3);
			color.r = -f * (j == 0);
			color.g = -f * (j == 1);
			color.b = -f * (j == 2);
			r_draw_sprite(&sp);
		}

		color = *RGBA(a, a, a, a);
		sp.pos.as_cmplx = basepos;
		r_draw_sprite(&sp);
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
	if(re(*pos) < -10) {
		*pos += VIEWPORT_W;
		dir += -1;
	}
	if(re(*pos) > VIEWPORT_W+10) {
		*pos -= VIEWPORT_W;
		dir += 1;
	}
	if(im(*pos) < -10) {
		*pos += I*VIEWPORT_H;
		dir += -I;
	}
	if(im(*pos) > VIEWPORT_H+10) {
		*pos -= I*VIEWPORT_H;
		dir += I;
	}

	return dir;
}

// XXX: should this not be a draw rule?
TASK(toe_boson_effect_spin, { BoxedProjectile p; }) {
	Projectile *p = TASK_BIND(ARGS.p);
	float target_angle = rng_angle();
	for(int t = 0; t < p->timeout; t++) {
		p->angle = target_angle * t / (float)p->timeout;
		YIELD;
	}
}

static Color* boson_color(Color *out_clr, int pos, int warps) {
	float f = pos / 3.0;
	*out_clr = *HSL(( warps - 0.3 * (1 - f)) / 3.0, 1 + f, 0.5);
	return out_clr;
}

TASK(toe_boson, { cmplx pos; cmplx wait_pos; cmplx vel; int num_warps; int activation_delay; int trail_idx; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_rice,
		.pos = ARGS.pos,
		.color = boson_color(&(Color){0}, ARGS.trail_idx, 0),
		.max_viewport_dist = 20,
	));

	int warps_left = ARGS.num_warps;

	p->angle = carg(ARGS.vel);

	assert(ARGS.activation_delay >= 0);

	Color thiscolor_additive = p->color;
	thiscolor_additive.a = 0;

	int activation_move_time = ARGS.activation_delay / 2;
	for(int t = 0; t < activation_move_time; t++) {
		real f = glm_ease_bounce_out(t/(real)activation_move_time);
		p->pos = clerp(ARGS.pos, ARGS.wait_pos, f);

		if(t % 3 == 0) {
			PARTICLE(
				.sprite = "smoothdot",
				.pos = p->pos,
				.color = &thiscolor_additive,
				.draw_rule = pdraw_timeout_fade(1, 0),
				.timeout = 10,
			);
		}

		YIELD;
	}
	WAIT(ARGS.activation_delay - activation_move_time);

	if(ARGS.trail_idx == 2) {
		play_sfx("shot_special1");
		play_sfx("redirect");

		Projectile *part = PARTICLE(
			.sprite = "blast",
			.pos = p->pos,
			.color = HSLA(carg(ARGS.vel), 0.5, 0.5, 0),
			.draw_rule = pdraw_timeout_scalefade(0, 1, 1, 0),
			.timeout = 60,
			.angle = 0,
			.flags = PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE,
		);

		INVOKE_TASK(toe_boson_effect_spin, ENT_BOX(part));

	}

	p->move = move_linear(ARGS.vel);
	cmplx prev_pos = p->pos;
	for(int t = 0;; t++) {
		if(wrap_around(&p->pos) != 0) {
			p->prevpos = p->pos; // don't lerp
			warps_left--;

			if(warps_left < 0) {
				kill_projectile(p);
				return;
			}

			if(ARGS.trail_idx == 3) {
				play_sfx_ex("warp", 0, false);
			}

			PARTICLE(
				.sprite = "myon",
				.pos = prev_pos,
				.color = &thiscolor_additive,
				.timeout = 50,
				.draw_rule = pdraw_timeout_scalefade(5, 0, 1, 0),
				.angle = rng_angle(),
				.flags = PFLAG_MANUALANGLE,
			);

			PARTICLE(
				.sprite = "stardust",
				.pos = prev_pos,
				.color = &thiscolor_additive,
				.timeout = 60,
				.draw_rule = pdraw_timeout_scalefade(0, 2, 1, 0),
				.angle = rng_angle(),
				.flags = PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE,
			);

			boson_color(&p->color, ARGS.trail_idx, ARGS.num_warps - warps_left);
			thiscolor_additive = p->color;
			thiscolor_additive.a = 0;

			Projectile *part = PARTICLE(
				.sprite = "stardust",
				.pos = p->pos,
				.color = &thiscolor_additive,
				.draw_rule = pdraw_timeout_scalefade(0, 2, 1, 0),
				.timeout = 30,
				.flags = PFLAG_REQUIREDPARTICLE,
			);

			INVOKE_TASK(toe_boson_effect_spin, ENT_BOX(part));
		}

		float lookahead_time = 40;
		cmplx pos_lookahead = p->pos + ARGS.vel * lookahead_time;
		cmplx dir = wrap_around(&pos_lookahead);

		if(dir != 0 && t % 3 == 0 && warps_left > 0) {
			cmplx pos0 = pos_lookahead - VIEWPORT_W / 2.0 * (1 - re(dir)) - I * VIEWPORT_H / 2 * (1 - im(dir));

			// Re [a b^*] behaves like the 2D vector scalar product
			float overshoot_time = re(pos0 * conj(dir)) / re(ARGS.vel * conj(dir));
			pos_lookahead -= ARGS.vel * overshoot_time;

			Color *clr = boson_color(&(Color){0}, ARGS.trail_idx, ARGS.num_warps - warps_left + 1);
			clr->a = 0;

			PARTICLE(
				.sprite = "smoothdot",
				.pos = pos_lookahead,
				.color = clr,
				.timeout = 10,
				.move = move_linear(ARGS.vel),
				.draw_rule = pdraw_timeout_fade(1, 0),
				.flags = PFLAG_REQUIREDPARTICLE,
			);
		}

		prev_pos = p->pos;

		YIELD;
	}
}

typedef struct LaserRuleTOEData {
	cmplx velocity;
	real width;
	int type;
} LaserRuleTOEData;

static cmplx toe_laser_rule_impl(Laser *l, real t, void *ruledata) {
	LaserRuleTOEData *rd = ruledata;

	switch(rd->type) {
		case 0: return l->pos + rd->velocity * t;
		case 1: return l->pos + rd->velocity * (t + rd->width * I * sin(t / rd->width));
		case 2: return l->pos + rd->velocity * (t + rd->width * (0.6 * (cos(3 * t / rd->width) - 1) + I * sin(3 * t /
rd->width)));
		case 3: return l->pos + rd->velocity * (t + floor(t / rd->width) * rd->width);
	}

	UNREACHABLE;
}

static LaserRule toe_laser_rule(cmplx velocity, int type) {
	LaserRuleTOEData rd = {
		.velocity = velocity,
		.type = type,
		.width = difficulty_value(3.5, 5, 6.5, 8),
	};
	return MAKE_LASER_RULE(toe_laser_rule_impl, rd);
}

static void toe_laser_particle(Laser *l, cmplx origin) {
	Color *c = color_mul(COLOR_COPY(&l->color), &l->color);
	c->a = 0;

	PARTICLE(
		.sprite = "stardust",
		.pos = origin,
		.color = c,
		.draw_rule = pdraw_timeout_scalefade(0, 1.5, 1, 0),
		.timeout = 30,
		.angle = rng_angle(),
		.flags = PFLAG_MANUALANGLE,
	);

	PARTICLE(
		.sprite = "stain",
		.pos = origin,
		.color = color_mul_scalar(COLOR_COPY(c), 0.5),
		.draw_rule = pdraw_timeout_scalefade(0, 1, 1, 0),
		.timeout = 20,
		.angle = rng_angle(),
		.flags = PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE,
	);

	PARTICLE(
		.sprite = "smoothdot",
		.pos = origin,
		.color = c,
		.draw_rule = pdraw_timeout_scalefade(0, 1, 1, 0),
		.timeout = 40,
		.angle = rng_angle(),
		.flags = PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE,
	);
}


DECLARE_TASK(toe_laser, { cmplx pos; cmplx vel; int type; int deathtime; });

TASK(toe_laser_respawn, { cmplx pos; cmplx vel; int type; }) {
	static const int transfer[][4] = {
		{1, 0, 0, 0},
		{0,-1,-1, 3},
		{0,-1, 2, 3},
		{0, 3, 3, 1}
	};

	if(re(ARGS.pos) < 0 || im(ARGS.pos) < 0 || re(ARGS.pos) > VIEWPORT_W || im(ARGS.pos) > VIEWPORT_H)
		return;

	int newtype, newtype2;
	do {
		newtype = 3.999999*rng_real();
		newtype2 = transfer[ARGS.type][newtype];
		// actually in this case, gluons are also always possible
		if(newtype2 == 1 && rng_chance(0.5)) {
			newtype = 2;
		}

		// I removed type 3 because i can’t draw dotted lines and it would be too difficult either way
	} while(newtype2 == -1 || newtype2 == 3 || newtype == 3);

	cmplx newdir = cdir(0.3);

	INVOKE_TASK(toe_laser, ARGS.pos, ARGS.vel * newdir, newtype, LASER_LENGTH);
	INVOKE_TASK(toe_laser, ARGS.pos, ARGS.vel / newdir, newtype2, LASER_LENGTH);
}

DEFINE_TASK(toe_laser) {
	static const Color type_to_color[] = {
		{0.4, 0.4, 1.0, 0.0},
		{1.0, 0.4, 0.4, 0.0},
		{0.4, 1.0, 0.4, 0.0},
		{1.0, 0.4, 1.0, 0.0},
	};

	Laser *l = TASK_BIND(
		create_laser(ARGS.pos, LASER_LENGTH, ARGS.deathtime, &type_to_color[ARGS.type],
			toe_laser_rule(ARGS.vel, ARGS.type)));
	toe_laser_particle(l, ARGS.pos);

	cmplx drift = 0.2 * I;

	for(int t = 0;; t++) {
		if(t == l->deathtime - 1) {
			cmplx newpos = laser_pos_at(l, l->deathtime);
			INVOKE_TASK(toe_laser_respawn, newpos, ARGS.vel, ARGS.type);
		}

		l->pos += drift;
		YIELD;
	}
}

static bool toe_its_yukawatime(cmplx pos) {
	int t = global.frames-global.boss->current->starttime;

	if(pos == 0)
		return t>YUKAWATIME;

	int sectors = 4;

	pos -= global.boss->pos;
	if(((int)(carg(pos)/2/M_PI*sectors+sectors+0.005*t))%2)
		return t > YUKAWATIME+0.2*cabs(pos);

	return false;
}

TASK(toe_fermion_yukawa_effect, { BoxedProjectile parent; }) {
	Color thiscolor_additive = NOT_NULL(ENT_UNBOX(ARGS.parent))->color;
	thiscolor_additive.a = 0;

	Projectile *p = TASK_BIND(PARTICLE(
		.sprite = "blast",
		.color = &thiscolor_additive,
		.timeout = 60,
		.draw_rule = pdraw_timeout_scalefade(0, 1, 1, 0),
		.angle = rng_angle(),
		.flags = PFLAG_MANUALANGLE,
	));

	for(;;) {
		p->pos = NOT_NULL(ENT_UNBOX(ARGS.parent))->pos;
		YIELD;
	}
}

TASK(toe_fermion_effects, { BoxedProjectile p; }) {
	Projectile *p = TASK_BIND(ARGS.p);

	Color thiscolor_additive = p->color;
	thiscolor_additive.a = 0;

	for(int t = 0;; t += WAIT(5)) {
		float particle_scale = min(1.0f, 0.5f * p->sprite->w / 28.0f);

		PARTICLE(
			.sprite = "stardust",
			.pos = p->pos,
			.color = &thiscolor_additive,
			.timeout = min(t / 6.0f, 10),
			.draw_rule = pdraw_timeout_scalefade(particle_scale * 0.5, particle_scale * 2, 1, 0),
			.angle = rng_angle(),
			.flags = PFLAG_MANUALANGLE,
		);
	}
}

TASK(toe_fermion, { ProjPrototype *proto; cmplx pos; cmplx dest; int color_idx; bool disable_yukawa; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = ARGS.proto,
		.pos = ARGS.pos,
		.color = RGBA(ARGS.color_idx == 0, ARGS.color_idx == 1, ARGS.color_idx == 2, 0),
		.max_viewport_dist = 50,
		.flags = PFLAG_NOSPAWNFLARE,
	));

	INVOKE_SUBTASK(toe_fermion_effects, ENT_BOX(p));

	int speeduptime = 40;
	real phase = ARGS.color_idx * M_TAU / 3;

	cmplx center_pos = 0;

	bool yukawaed = false;

	for(int t = 0;; t++, YIELD) {
		if(t <= speeduptime) {
			center_pos += (ARGS.dest - center_pos) * 0.01;
		} else {
			center_pos += cnormalize(center_pos) * (t - speeduptime) * 0.01;
		}

		p->pos = ARGS.pos + center_pos * cdir(0.01 * t) + 5 * cdir(t * 0.05 + phase);

		if(ARGS.disable_yukawa) {
			continue;
		}

		if(toe_its_yukawatime(p->pos)) {
			if(!yukawaed) {
				projectile_set_prototype(p, pp_bigball);
				yukawaed = true;
				play_sfx_ex("shot_special1", 5, false);

				INVOKE_SUBTASK(toe_fermion_yukawa_effect, ENT_BOX(p));
			}

			center_pos *= 1.01;
		} else if(yukawaed) {
			projectile_set_prototype(p, pp_ball);
			yukawaed = 0;
		}
	}
}

TASK(toe_higgs, { cmplx pos; cmplx vel; Color color; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_flea,
		.pos = ARGS.pos,
		.color = &ARGS.color,
		.flags = PFLAG_NOSPAWNFLARE,
	));

	bool yukawaed = false;

	for(int t = 0;; t++) {
		if(toe_its_yukawatime(p->pos)) {
			if(!yukawaed) {
				projectile_set_prototype(p, pp_rice);
				yukawaed = true;
			}
			ARGS.vel *= 1.01;
		} else if(yukawaed) {
			projectile_set_prototype(p, pp_flea);
			yukawaed = false;
		}

		int global_time = global.frames - global.boss->current->starttime - HIGGSTIME;
		int max_time = SYMMETRYTIME - HIGGSTIME;
		real angular_velocity = 0.5 * M_PI * difficulty_value(0, 0, 1, 2);

		if(global_time > max_time) {
			global_time = max_time;
		}

		cmplx vel = ARGS.vel * cdir(angular_velocity * global_time / (real)max_time);
		p->pos = ARGS.pos + t * vel;

		YIELD;
	}
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
	int count = 0;

	for(int cycle = 0; cycle < end_time/cycle_step; cycle++) {
		count = 30 - (count == 0); // get rid of the spawn safe spot on first cycle
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
				dir *= cdir(0.15 * sign(re(dir)) * sin(cycle));

				cmplx wait_pos = boss->pos + 18 * dir * i;

				INVOKE_TASK(toe_boson,
					    .pos = boss->pos,
					    .wait_pos = wait_pos,
					    .vel = 2.5 * dir,
					    .num_warps = num_warps,
					    .activation_delay = 42*2 - step * t,
					    .trail_idx = i
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
			INVOKE_TASK(toe_fermion, pp_ball, boss->pos, dest, clr);
		}

		WAIT(step);
	}
}

TASK(toe_part_symmetry, { BoxedBoss boss; }) {
	Boss *b = TASK_BIND(ARGS.boss);

	play_sfx("charge_generic");
	play_sfx("boom");
	stagetext_add("Symmetry broken!", VIEWPORT_W / 2.0 + I * VIEWPORT_H / 4.0, ALIGN_CENTER, res_font("big"), RGB(1, 1, 1), 0, 100, 10, 20);
	stage_shake_view(10);

	PARTICLE(
		.sprite = "blast",
		.pos = b->pos,
		.color = RGBA(1.0, 0.3, 0.3, 0.0),
		.timeout = 60,
		.draw_rule = pdraw_timeout_scalefade(0, 5, 1, 0),
		.flags = PFLAG_REQUIREDPARTICLE,
	);

	for(int i = 0; i < 5; ++i) {
		RNG_ARRAY(R, 2);
		PARTICLE(
			.sprite = "stain",
			.pos = b->pos,
			.color = RGBA(0.3, 0.3, 1.0, 0.0),
			.timeout = vrng_range(R[0], 80.0, 120.0),
			.draw_rule = pdraw_timeout_scalefade(0.8, 4, 1, 0),
			.angle = vrng_angle(R[1]),
			.flags = PFLAG_MANUALANGLE,
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

				INVOKE_TASK(toe_higgs,
					.pos = boss->pos,
					.vel = vel,
					.color = *RGB(dir * (t > fast_steps), 0, 1)
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
			INVOKE_TASK(toe_fermion,
				.proto = pp_soul,
				.pos = boss->pos,
				.dest = 50 * cdir(1.3 * t),
				.color_idx = clr,
				.disable_yukawa = true,
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
	boss->move = move_from_towards(boss->pos, ELLY_TOE_TARGET_POS, 0.1);
	stage_shake_view(50);
	boss_set_portrait(boss, "elly", "beaten", "shouting");
	BEGIN_BOSS_ATTACK(&ARGS);

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
	stagetext_add("Coupling the Higgs!", VIEWPORT_W / 2.0 + I * VIEWPORT_H / 4.0, ALIGN_CENTER, res_font("big"), RGB(1, 1, 1), 0, 100, 10, 20);
	stage_shake_view(10);

	WAIT(BREAKTIME - YUKAWATIME);

	play_sfx("charge_generic");
	stagetext_add("Perturbation theory", VIEWPORT_W / 2.0 + I * VIEWPORT_H / 4.0, ALIGN_CENTER, res_font("big"), RGB(1, 1, 1), 0, 100, 10, 20);
	stagetext_add("breaking down!", VIEWPORT_W / 2.0+ I * VIEWPORT_H / 4.0 + 30 * I, ALIGN_CENTER, res_font("big"), RGB(1, 1, 1), 0, 100, 10, 20);
	stage_shake_view(10);

	INVOKE_SUBTASK_DELAYED(35, toe_part_break_fermions, ENT_BOX(boss));

	for(;;) {
		play_sfx_ex("laser1", 0, true);

		cmplx dir = rng_dir();
		int count = 8;
		for(int i = 0; i < count; i++) {
			INVOKE_TASK(toe_laser,
				.pos = boss->pos,
				.vel = 2 * cdir(M_TAU / count * i) * dir,
				.deathtime = LASER_LENGTH / 2.0
			);
		}

		WAIT(100);
	}
}

