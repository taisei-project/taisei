/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "spells.h"
#include "../wriggle.h"

#include "common_tasks.h"
#include "global.h"

typedef struct SunContext {
	int bullets_absorbed;
} SunContext;

typedef struct SunArgs {
	BoxedProjectile ent;
	SunContext *context;  // Valid if UNBOX(ent) != NULL
} SunArgs;

TASK(colorshift, { BoxedProjectile p; Color target_color; }) {
	auto p = TASK_BIND(ARGS.p);

	for(;;YIELD) {
		color_approach(&p->color, &ARGS.target_color, 0.01);
	}
}

TASK(attraction, { SunArgs sun; BoxedProjectile p; cmplx dir; real twist; }) {
	auto p = TASK_BIND(ARGS.p);
	p->flags = PFLAG_NOAUTOREMOVE;

	p->move = move_asymptotic(p->move.velocity, p->move.velocity * 0.01 + ARGS.dir, 0.8);
	p->move.retention *= cdir(ARGS.twist);
	p->move.attraction_point = NOT_NULL(ENT_UNBOX(ARGS.sun.ent))->pos;

	WAIT(60);
	play_sfx("redirect");

	p->color.r = 1;
	spawn_projectile_highlight_effect(p);

	int rampuptime = 60;
	float cf = 0.998;

	for(int i = 0; i < rampuptime; ++i, YIELD) {
		p->color.g *= cf;
		p->move.attraction = 0.25 * i / (rampuptime - 1.0);
	}

	for(;;YIELD) {
		auto sun = ENT_UNBOX(ARGS.sun.ent);

		if(sun) {
			p->move.attraction_point = sun->pos;
		}

		p->color.g *= cf;
		p->color.a *= cf * cf;
		p->move.attraction += 0.0001;

		real mindist = 12;
		if(cabs2(p->pos - p->move.attraction_point) < mindist * mindist) {
			play_sfx("shot1");

			int cnt = 2;
			for(int i = 0; i < cnt; ++i) {
				cmplx r = cdir(M_TAU/4 * lerp(-1, 1, i / (cnt - 1.0)));
				real speed = 0.5;
				auto f = PROJECTILE(
					.proto = pp_flea,
					.pos = p->pos,
					// .move = move_asymptotic_simple(0.5 * rng_dir(), 10),
					.move = move_asymptotic_simple(speed * cnormalize(p->move.velocity) * r, 10),
					.flags = PFLAG_NOSPAWNFADE,
					.color = RGBA(3, 1.5, 1, 0),
				);

				INVOKE_TASK(colorshift, ENT_BOX(f), *RGB(0.5, 0.2, 0.2));
			}

			if(sun) {
				if(!ARGS.sun.context->bullets_absorbed++) {
					play_sfx("boom");

					PARTICLE(
						.pos = sun->pos,
						.sprite = "blast_huge_halo",
						.color = &sun->color,
						.flags = PFLAG_MANUALANGLE,
						.angle = rng_angle(),
						.timeout = 120,
						.draw_rule = pdraw_timeout_scalefade(0, 2, 2, 0),
					);
				}
			}

			kill_projectile(p);
		}
	}
}

TASK(spawner, { SunArgs sun; cmplx dir; int num_bullets; }) {
	auto sun = NOT_NULL(ENT_UNBOX(ARGS.sun.ent));

	real t = 120;
	real d = hypot(VIEWPORT_W, VIEWPORT_H);
	real m = 120;
	cmplx o = sun->pos + d * ARGS.dir;

	auto p = TASK_BIND(PROJECTILE(
		.proto = pp_ball,
		.pos = o,
		.color = RGBA(0.5, 1.0, 0.5, 0),
		.move = move_accelerated(0, -ARGS.dir * (d - m) / (0.5 * t * t)),
		.flags = PFLAG_NOAUTOREMOVE,
	));

	real lspeed = difficulty_value(2, 2, 3, 3);

	auto l = create_laser(
		sun->pos, t/8, t, RGBA(0, 0.5, 1, 0),
		laser_rule_accelerated(0, -lspeed*p->move.acceleration));
	l->width = 20;
	create_laserline_ab(o, sun->pos, 10, t, t, RGBA(0, 0.5, 1, 0));
	play_sfx("laser1");
	play_sfx("boon");

	int t0 = t * 0.92;
	int t1 = t - t0;

	WAIT(t0);
	p->move = move_dampen(p->move.velocity, 0.9);
	WAIT(t1);
	play_sfx("shot_special1");

	RADIAL_LOOP(l, ARGS.num_bullets, rng_dir()) {
		auto c = PROJECTILE(
			.proto = pp_rice,
			.pos = p->pos,
			.color = RGBA(0, 0.7, 0.25, 1),
			.flags = PFLAG_NOSPAWNFLARE,
			.move = move_linear(p->move.velocity),
		);

		INVOKE_TASK(attraction, ARGS.sun, ENT_BOX(c), l.dir, 0.1 * (1 - 2 * (l.i & 1)));
	}

	PROJECTILE(
		.color = RGBA(1.5, 1.0, 0.5, 0),
		.pos = p->pos,
		.proto = pp_soul,
		.timeout = 1,
		.move = move_linear(p->move.velocity),
	);

	kill_projectile(p);
}

static void sun_move(Projectile *p) {
	if(p->move.acceleration) {
		return;
	}

	real a = difficulty_value(0.001, 0.001, 0.002, 0.002);
	p->move = move_accelerated(p->move.velocity, a * cnormalize(global.plr.pos - p->pos));

	auto halo = res_sprite("part/blast_huge_halo");

	PARTICLE(
		.pos = p->pos,
		.sprite_ptr = halo,
		.color = color_add(COLOR_COPY(&p->color), RGBA(0, 0, 1, 0)),
		.scale = re(p->scale) * p->sprite->w / halo->w,
		.flags = PFLAG_MANUALANGLE | PFLAG_REQUIREDPARTICLE,
		.angle = rng_angle(),
		.timeout = 20,
		.draw_rule = pdraw_timeout_scalefade(2, 1, 5, 0),
	);

	play_sfx("warp");
}

TASK(sun_move, { BoxedProjectile sun; }) {
	sun_move(TASK_BIND(ARGS.sun));
}

TASK(sun_flare, { BoxedProjectile sun; }) {
	auto p = TASK_BIND(ARGS.sun);
	auto halo = res_sprite("part/blast_huge_rays");

	int d = 2;
	for(;;WAIT(d)) {
		float smear = 2;
		PARTICLE(
			.sprite_ptr = halo,
			.scale = re(p->scale) * p->sprite->w / halo->w,
			.pos = p->pos,
			.angle = rng_angle(), // p->angle,
			.flags = PFLAG_NOMOVE | PFLAG_MANUALANGLE,
			.timeout = 10 * smear,
			.draw_rule = pdraw_timeout_scalefade(1, 1.25, 1, 0),
			.color = &p->color,
			.shader_ptr = p->shader,
			.layer = p->ent.draw_layer,
			.opacity = d/smear,
		);
	}
}

TASK(sun, { cmplx pos; SunArgs *out_sunargs; int max_absorb; }) {
	auto p = TASK_BIND(PROJECTILE(
		.proto = pp_bigball,
		.pos = ARGS.pos,
		.color = RGBA(1.5, 1.0, 0.2, 0),
		.flags = PFLAG_INDESTRUCTIBLE | PFLAG_NOCLEAR | PFLAG_MANUALANGLE,
		.angle_delta = 1.2,
	));

	INVOKE_TASK(sun_flare, ENT_BOX(p));

	SunContext sctx = {};
	*ARGS.out_sunargs = (SunArgs) { ENT_BOX(p), &sctx };

	cmplx size0 = p->size;
	cmplx csize0 = p->collision_size * 0.8 + p->size * 0.2;
	real max_size = difficulty_value(9, 10, 12, 14);
	real absorb_scale = 1.0 / ARGS.max_absorb;

	auto timed_move = cotask_box(INVOKE_SUBTASK_DELAYED(600, sun_move, ENT_BOX(p)));

	for(;;YIELD) {
		real f = sctx.bullets_absorbed * absorb_scale;
		real s = lerp(1, max_size, f);

		p->size = size0 * s;
		p->collision_size = csize0 * s;
		p->scale = CMPLX(s, s);
		p->color.g = lerpf(1.0f, 0.25f, f);

		if(sctx.bullets_absorbed >= ARGS.max_absorb) {
			break;
		}
	}

	CANCEL_TASK(timed_move);
	sun_move(p);
}

TASK(slave, { BoxedBoss boss; cmplx target_pos; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	WriggleSlave *slave = stage3_host_wriggle_slave(boss->pos);

	MoveParams m = move_from_towards(slave->pos, ARGS.target_pos, 0.05);
	INVOKE_SUBTASK(common_move, &slave->pos, m);

	PROJECTILE(
		.color = RGBA(0.1, 1.0, 0.25, 0),
		.pos = slave->pos,
		.proto = pp_soul,
		.timeout = 1,
		.move = m,
	);

	WAIT(80);

	int num_spawners = difficulty_value(8, 8, 12, 12);
	int bullets_per_spawner = difficulty_value(20, 30, 30, 40);
	int max_bullets = num_spawners * bullets_per_spawner;

	SunArgs sun_args;
	INVOKE_TASK(sun, slave->pos, &sun_args, max_bullets);

	cmplx dir = I; // rng_dir();
	cmplx r = cdir(M_TAU/num_spawners);

	for(int i = 0; i < num_spawners; ++i) {
		INVOKE_TASK(spawner, sun_args, dir, bullets_per_spawner);
		dir *= r;
	}

	play_sfx("shot_special1");
}

DEFINE_EXTERN_TASK(stage3_spell_moths_to_a_flame) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2 + VIEWPORT_H*I/3, 0.05);
	BEGIN_BOSS_ATTACK(&ARGS);
	int cycle = 500;
	int charge_time = 120;

	Rect bounds = viewport_bounds(64);
	bounds.bottom = VIEWPORT_H/3;

	for(;;) {
		boss->move.attraction_point = common_wander(boss->pos, 200, bounds);
		aniplayer_soft_switch(&boss->ani, "specialshot_charge", 1);
		aniplayer_queue(&boss->ani, "specialshot_hold", 0);
		common_charge(charge_time, &boss->pos, 0, RGBA(0, 1, 0.2, 0));
		aniplayer_queue(&boss->ani, "specialshot_release", 1);
		aniplayer_queue(&boss->ani, "main", 0);
		boss->move.attraction_point = common_wander(boss->pos, 92, bounds);
		cmplx aim = 0.5 * (global.plr.pos - boss->pos);
		INVOKE_SUBTASK(slave, ENT_BOX(boss), boss->pos + aim);
		WAIT(cycle - charge_time);
	}
}
