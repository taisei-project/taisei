/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "spells.h"
#include "../corruption.h"

#define MARKED_PFLAGS (PFLAG_NOMOVE | PFLAG_MANUALANGLE)

#define PROTO_MARKABLE pp_ball
#define PROTO_MARKED   pp_bigball

static bool is_markable(Projectile *p) {
	return p->proto == PROTO_MARKABLE;
}

static bool is_marked(Projectile *p) {
	return p->proto == PROTO_MARKED;
}

TASK(trash, { cmplx origin; }) {
	DECLARE_ENT_ARRAY(Projectile, projs, 5);
	auto o = ARGS.origin;

	play_sfx("shot1");

	ARC_LOOP(l, projs.capacity, -I, M_PI/4) {
		cmplx v = 300 * l.dir;
		im(v) = max(-rng_range(80, 120), im(v));
		ENT_ARRAY_ADD(&projs, PROJECTILE(
			.proto = PROTO_MARKABLE,
			.color = RGB(1, 0, 0),
			.pos = o,
			.move = move_from_towards(o, o + v, 0.1),
		));
	}

	WAIT(60);
	play_sfx("redirect");

	ENT_ARRAY_FOREACH(&projs, Projectile *p, {
		cmplx d = cnormalize(global.plr.pos - p->pos); // I;
		cmplx r = cdir(M_PI/32*rng_sreal());
		p->move = move_asymptotic_halflife(p->move.velocity, d*r, 60);
	});

	LineSegment wall_left   = { 0,            VIEWPORT_H*I };
	LineSegment wall_right  = { VIEWPORT_W,   VIEWPORT_W+VIEWPORT_H*I };
	LineSegment wall_top    = { 0,            VIEWPORT_W };
	LineSegment wall_bottom = { VIEWPORT_H*I, VIEWPORT_W+VIEWPORT_H*I };

	for(;;) {
		YIELD;
		ENT_ARRAY_FOREACH(&projs, Projectile *p, {
			LineSegment m = { p->pos - p->move.velocity, p->pos };

			if(re(p->move.velocity) < 0 && lineseg_lineseg_intersection(m, wall_left, &p->pos)) {
				p->move.velocity = creflect(p->move.velocity, 1);
				p->move.acceleration = creflect(p->move.acceleration, 1);
			} else if(re(p->move.velocity) > 0 && lineseg_lineseg_intersection(m, wall_right, &p->pos)) {
				p->move.velocity = creflect(p->move.velocity, -1);
				p->move.acceleration = creflect(p->move.acceleration, -1);
			} else if(im(p->move.velocity) < 0 && lineseg_lineseg_intersection(m, wall_top, &p->pos)) {
				p->move.velocity = creflect(p->move.velocity, I);
				p->move.acceleration = creflect(p->move.acceleration, I);
			} else if(im(p->move.velocity) > 0 && lineseg_lineseg_intersection(m, wall_bottom, &p->pos)) {
				p->move.velocity = creflect(p->move.velocity, -I);
				p->move.acceleration = creflect(p->move.acceleration, -I);
			}

			p->prevpos = p->pos;
		});
	}
}

TASK(slave_move, { BoxedYumemiSlave slave; cmplx p0; cmplx p1; }) {
	auto slave = TASK_BIND(ARGS.slave);

	for(int t = 0;; ++t, YIELD) {
		real f = psin(0.005 * t - M_PI);
		slave->pos = clerp(ARGS.p0, ARGS.p1, f);
	}
}

static Projectile *find_target(cmplx a, cmplx b) {
	real min_dist = DBL_MAX;
	Projectile *target = NULL;
	cmplx o = a;

	LineSegment s = { a, b };

	for(Projectile *p = global.projs.first; p; p = p->next) {
		if(p->type == PROJ_ENEMY && is_markable(p)) {
			real dist2 = cabs2(p->pos - o);
			if(dist2 < min_dist) {
				Ellipse e = {
					.origin = p->pos,
					.angle = p->angle,
					.axes = p->collision_size,
				};

				if(lineseg_ellipse_intersect(s, e)) {
					min_dist = dist2;
					target = p;
				}
			}
		}
	}

	return target;
}

TASK(mark, { StageXCorruption *corruption; BoxedProjectile p; }) {
	auto p = TASK_BIND(ARGS.p);
	int mtime = 20;
	Color mcolor = RGB(0.75, 0.75, 0.75);

	play_sfx("boon");

	for(int t = 0; t < mtime; ++t) {
		YIELD;
		p->color = color_lerp(p->color, mcolor, 0.1);
	}

	p->color = mcolor;
	p->flags |= MARKED_PFLAGS;
	projectile_set_prototype(p, PROTO_MARKED);
	spawn_projectile_highlight_effect(p);

	WAIT_EVENT(&p->events.cleared);

	if(p->clear_flags & CLEAR_HAZARDS_FORCE) {
		return;
	}

	play_sfx("shot_special1");
	play_sfx("warp");

	real corruption_radius = 120;
	auto spr = res_sprite("part/blast_huge_halo");
	real s = (4 * corruption_radius) / spr->w;

	PARTICLE(
		.sprite_ptr = spr,
		.pos = p->pos,
		.angle = rng_angle(),
		.flags = PFLAG_MANUALANGLE | PFLAG_NOMOVE | PFLAG_REQUIREDPARTICLE,
		.timeout = 30,
		.color = color_mul_scalar(RGBA(1, 0.1, 0, 0.5), 2),
		.draw_rule = pdraw_timeout_scalefade(s, 0, 1, 0),
	);

	stagex_corruption_spawn_zone(ARGS.corruption, p->pos, corruption_radius, 60 * 15);
}

TASK(laser_effects, { BoxedLaser l; }) {
	auto l = TASK_BIND(ARGS.l);

	auto spr_stain = res_sprite("part/stain");
	auto spr_rays = res_sprite("part/blast_huge_rays");

	for(;;YIELD) {
		if(!l->collision_active) {
			continue;
		}

		auto pcolor = l->color;
		pcolor.a = 0;
		auto rd = NOT_NULL(laser_get_ruledata_linear(l));
		auto a = l->pos;
		auto b = a + rd->velocity * l->timespan;

		auto lv = cnormalize(rd->velocity);
		auto v = 4 * lv * cdir(M_PI/18 * rng_sreal());

		for(int i = 0; i < 2; ++i) {
			PARTICLE(
				.sprite_ptr = spr_stain,
				.color = pcolor,
				.angle = rng_angle(),
				.flags = PFLAG_MANUALANGLE,
				.move = move_linear((cmplx[]) { v*I, v/I } [i]),
				.pos = a - lv * 12,
				.timeout = 15,
				.draw_rule = pdraw_timeout_scalefade(0.1, 1, 1, 0),
			);
		}

		PARTICLE(
			.sprite_ptr = spr_rays,
			.color = pcolor,
			.angle = rng_angle(),
			.flags = PFLAG_MANUALANGLE,
			.pos = b,
			.timeout = 5,
			.draw_rule = pdraw_timeout_scalefade(0.1, 1, 1, 0),
		);
	}
}

TASK(marker, { StageXCorruption *corruption; }) {
	cmplx p0 = VIEWPORT_W;
	cmplx p1 = p0 + VIEWPORT_H*I;
	cmplx laser_dir = -1;
	real ltime = 60 * 60 * 60;
	real lwidth = 30;

	auto slave = stagex_host_yumemi_slave(p0, 0);
	INVOKE_SUBTASK(slave_move, ENT_BOX(slave), p0, p1);

	BoxedProjectile target = {};
	BoxedLaser laser = {};

	cmplx laser_target = slave->pos;

	bool laser_sfx_played = false;

	for(;;YIELD) {
		Laser *l = ENT_UNBOX(laser);
		if(!l) {
			l = create_laserline_ab(
				slave->pos, slave->pos + laser_dir * VIEWPORT_W, lwidth, 120, ltime, RGB(0, 0.5, 1));
			l->width_exponent = 0;
			laser_sfx_played = false;
			laser = ENT_BOX(l);
			INVOKE_TASK(laser_effects, laser);
		}

		if(!laser_sfx_played && l->width > lwidth*0.25) {
			laser_sfx_played = true;
			play_sfx("laser1");
		}

		cmplx a = slave->pos;
		cmplx b = a + laser_dir * VIEWPORT_W;

		Projectile *p = ENT_UNBOX(target);

		if(p && is_marked(p)) {
			p = NULL;
		}

		if(!p) {
			if((p = find_target(a, b))) {
				target = ENT_BOX(p);
				INVOKE_TASK(mark, ARGS.corruption, target);
			}
		}

		if(p) {
			p->move.velocity = 0;
			b = p->pos;
		}

		capproach_asymptotic_p(&laser_target, b, 0.2, 1e-5);
		laserline_set_ab(l, a, laser_target);
	}
}

TASK(sweeper, { StageXCorruption *corruption; cmplx sweep_dir; }) {
	cmplx margin = 60 * ARGS.sweep_dir;
	cmplx p0 = VIEWPORT_H*0.5 * (I - ARGS.sweep_dir) - margin;
	cmplx p1 = p0 + VIEWPORT_H * ARGS.sweep_dir + margin * 2;
	cmplx laser_dir = 1;

	auto slave = stagex_host_yumemi_slave(p0, 1);
	int sweep_time = 120;

	BoxedLaser laser = {};

	for(int t = 0; t < sweep_time; ++t, YIELD) {
		real f = t / (sweep_time - 1.0);
		f = glm_ease_quad_in(f);
		slave->pos = clerp(p0, p1, f);

		Laser *l = ENT_UNBOX(laser);
		if(!l) {
			l = create_laser(0, 4, sweep_time - t, RGB(1, 0.5, 0.1), laser_rule_linear(0));
			l->width_exponent = 0;
			laser_make_static(l);
			laser = ENT_BOX(l);
		}

		LineSegment s = { slave->pos, slave->pos + laser_dir * VIEWPORT_W };
		laserline_set_ab(l, s.a, s.b);
		laser_charge(l, global.frames - l->birthtime, 30, 5);
		l->collision_active = false;

		for(Projectile *p = global.projs.first; p; p = p->next) {
			if(p->type == PROJ_ENEMY && is_marked(p)) {
				Ellipse e = {
					.origin = p->pos,
					.angle = p->angle,
					.axes = p->collision_size,
				};

				if(lineseg_ellipse_intersect(s, e)) {
					p->flags |= PFLAG_NOCLEARBONUS;
					clear_projectile(p, CLEAR_HAZARDS_BULLETS);
				}
			}
		}
	}
}

DEFINE_EXTERN_TASK(stagex_spell_garbage) {
	Boss *boss = stagex_init_boss_attack(&ARGS);
	boss->move = move_towards(boss->move.velocity, BOSS_DEFAULT_GO_POS, 0.02);
	BEGIN_BOSS_ATTACK(&ARGS.base);

	auto corruption = ARGS.corruption;

	INVOKE_SUBTASK(marker, corruption);
	cmplx sweep_dir = I;

	for(;;) {
		common_charge(120, &boss->pos, 0, RGBA(1, 0.2, 0.2, 0));

		for(int i = 0; i < 3; ++i) {
			INVOKE_TASK_DELAYED(10 * i, trash, boss->pos);
		}

		WAIT(360);

		INVOKE_SUBTASK(sweeper, corruption, sweep_dir);
		sweep_dir = -sweep_dir;
	}
}
