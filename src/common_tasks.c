/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "common_tasks.h"
#include "random.h"
#include "util/glm.h"

DEFINE_EXTERN_TASK(common_drop_items) {
	cmplx p = *ARGS.pos;

	for(int i = 0; i < ITEM_LAST - ITEM_FIRST; ++i) {
		for(int j = ARGS.items.as_array[i]; j; --j) {
			spawn_item(p, i + ITEM_FIRST);
			WAIT(2);
		}
	}
}

void common_move_loop(cmplx *restrict pos, MoveParams *restrict mp) {
	for(;;) {
		move_update(pos, mp);
		YIELD;
	}
}

DEFINE_EXTERN_TASK(common_move) {
	if(ARGS.ent.ent) {
		TASK_BIND(ARGS.ent);
	}

	MoveParams p = ARGS.move_params;
	common_move_loop(ARGS.pos, &p);
}

DEFINE_EXTERN_TASK(common_move_ext) {
	if(ARGS.ent.ent) {
		TASK_BIND(ARGS.ent);
	}

	common_move_loop(ARGS.pos, ARGS.move_params);
}

DEFINE_EXTERN_TASK(common_call_func) {
	ARGS.func();
}

DEFINE_EXTERN_TASK(common_start_bgm) {
	stage_start_bgm(ARGS.bgm);
}

static Projectile *spawn_charge_particle(cmplx target, real dist, const Color *clr) {
	cmplx pos = target + rng_dir() * dist;

	return PARTICLE(
		.sprite = "graze",
		.color = clr,
		.pos = pos,
		.draw_rule = pdraw_timeout_scalefade(2, 0.1*(1+I), 0, 1),
		.move = move_towards(target, rng_range(0.12, 0.16)),
		.timeout = 30,
		// .scale = 0.5+1.5*I,
		.flags = PFLAG_NOREFLECT,
		.layer = LAYER_PARTICLE_HIGH,
	);
}

static void randomize_hue(Color *clr, float r) {
	float h, s, l, a = clr->a;
	color_get_hsl(clr, &h, &s, &l);
	h += rng_sreal() * r;
	*clr = *HSLA(h, s, l, a);
}

DEFINE_EXTERN_TASK(common_charge) {
	real dist = 256;
	int delay = 3;
	int maxtime = ARGS.time;
	real rayfactor = 1.0 / (real)maxtime;
	float hue_rand = 0.02;

	const char *snd_charge = ARGS.sound.charge;
	const char *snd_discharge = ARGS.sound.discharge;
	SFXPlayID charge_snd_id = 0;

	if(snd_charge) {
		charge_snd_id = play_sfx(snd_charge);
	}

	Color local_color;

	cmplx anchor_null = 0;
	cmplx pos_offset = ARGS.pos;
	const cmplx *pos_anchor = ARGS.anchor;
	if(!pos_anchor) {
		pos_anchor = &anchor_null;
	}

	const Color *p_color = ARGS.color_ref;
	if(!p_color) {
		assert(ARGS.color != NULL);
		local_color = *ARGS.color;
		p_color = &local_color;
	}

	if(ARGS.bind_to_entity.ent) {
		TASK_BIND(ARGS.bind_to_entity);
	}

	// This is just about below LAYER_PARTICLE_HIGH
	// We want it on a separate layer for better sprite batching efficiency,
	// because these sprites are on a different texture than most.
	drawlayer_t blast_layer = LAYER_PARTICLE_PETAL | 0x80;

	for(int i = 0; i < maxtime;) {
		cmplx pos = *pos_anchor + pos_offset;
		int stage = (5 * i) / maxtime;
		real sdist = dist * glm_ease_quad_out((stage + 1) / 6.0);

		for(int j = 0; j < stage + 1; ++j) {
			Color color = *p_color;
			randomize_hue(&color, hue_rand);
			color_lerp(&color, RGBA(1, 1, 1, 0), rng_real() * 0.2);
			spawn_charge_particle(pos, sdist * (1 + 0.1 * rng_sreal()), &color);
			color_mul_scalar(&color, 0.2);
		}

		Color color = *p_color;
		randomize_hue(&color, hue_rand);
		color_lerp(&color, RGBA(1, 1, 1, 0), rng_real() * 0.2);
		color_mul_scalar(&color, 0.5);

		PARTICLE(
			.sprite = "blast_huge_rays",
			.color = &color,
			.pos = pos,
			.draw_rule = pdraw_timeout_scalefade(0, 1, 1, 0),
			.timeout = 30,
			.flags = PFLAG_NOREFLECT,
			.scale = glm_ease_bounce_out(rayfactor * (i + 1)),
			.angle = rng_angle(),
			.layer = blast_layer,
		);

		i += WAIT(delay);
	}

	if(snd_discharge) {
		replace_sfx(charge_snd_id, snd_discharge);
	} else {
		stop_sound(charge_snd_id);
	}

	local_color = *p_color;
	randomize_hue(&local_color, hue_rand);
	color_mul_scalar(&local_color, 2.0);

	PARTICLE(
		.sprite = "blast_huge_halo",
		.color = &local_color,
		.pos = *pos_anchor + pos_offset,
		.draw_rule = pdraw_timeout_scalefade(0, 2, 1, 0),
		.timeout = 30,
		.flags = PFLAG_NOREFLECT,
		.angle = rng_angle(),
		.layer = blast_layer,
	);
}

DEFINE_EXTERN_TASK(common_set_bitflags) {
	assume(ARGS.pflags != NULL);
	*ARGS.pflags = ((*ARGS.pflags & ARGS.mask) | ARGS.set);
}

cmplx common_wander(cmplx origin, double dist, Rect bounds) {
	int attempts = 32;
	double angle;
	cmplx dest;
	cmplx dir;

	// assert(point_in_rect(origin, bounds));

	while(attempts--) {
		angle = rng_angle();
		dir = cdir(angle);
		dest = origin + dist * dir;

		if(point_in_rect(dest, bounds)) {
			return dest;
		}
	}

	log_warn("Clipping fallback  origin = %f%+fi  dist = %f  bounds.top_left = %f%+fi  bounds.bottom_right = %f%+fi",
		creal(origin), cimag(origin),
		dist,
		creal(bounds.top_left), cimag(bounds.top_left),
		creal(bounds.bottom_right), cimag(bounds.bottom_right)
	);

	// TODO: implement proper line-clipping here?

	double step = cabs(bounds.bottom_right - bounds.top_left) / 16;
	dir *= step;
	dest = origin;

	while(point_in_rect(dest + dir, bounds)) {
		dest += dir;
	}

	return dest;
}
