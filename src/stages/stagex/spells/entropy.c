/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "spells.h"
#include "bitarray.h"

#define FULL_PERIOD 1200
#define PARTS 128

const static struct {
	struct {
		Color main;
		Color fringe;
		Color trail;
	} colors;
} cbullet_types[] = {
	{
		.colors.main   = {  0.0,  0.5,  1.0,  1.0 },
		.colors.fringe = {  0.0,  1.0,  0.0,  1.0 },
		.colors.trail  = { -1.0,  1.0,  1.0,  0.0 },
	},
	{
		.colors.main   = {  0.5,  0.0,  1.0,  1.0 },
		.colors.fringe = {  1.0,  1.0,  0.0,  1.0 },
		.colors.trail  = {  1.0, -1.0,  0.5,  0.0 },
	},
};

typedef struct CompressedBulletData {
	BoxedProjectile main;
	BoxedProjectile loose_parts[PARTS];
	BIT_ARRAY(PARTS) contained_parts_bitmatp;
	uint num_contained_parts;
	cmplx full_collision;
	cmplx full_size;
	int type;
} CompressedBulletData;

static void update_main_bullet(CompressedBulletData *data) {
	auto main = NOT_NULL(ENT_UNBOX(data->main));

	real f = (real)data->num_contained_parts / PARTS;
	real scale = lerp(0.25, 1.0, f);

	main->collision_size = data->full_collision * scale;
	main->size = data->full_size * scale;
	main->scale = (1 + I) * scale;

	ProjFlags hidden_flags = PFLAG_NOCOLLISION | PFLAG_NOGRAZE;

	if(f == 0) {
		main->flags |= hidden_flags;
		main->ent.draw_layer = LAYER_NODRAW;
	} else {
		main->flags &= ~hidden_flags;
		main->ent.draw_layer = LAYER_BULLET;
	}
}

static BoxedProjectile extract_part(CompressedBulletData *data, uint idx) {
	if(!bitarray_get(&data->contained_parts_bitmatp, idx)) {
		return (BoxedProjectile) {};
	}

	assert(!ENT_UNBOX(data->loose_parts[idx]));
	auto main = NOT_NULL(ENT_UNBOX(data->main));

	auto part = PROJECTILE(
		.proto = pp_diamond,
		.pos = main->pos,
		.color = main->color,
	);

	data->num_contained_parts--;
	bitarray_set(&data->contained_parts_bitmatp, idx, 0);
	update_main_bullet(data);

	return data->loose_parts[idx] = ENT_BOX(part);
}

TASK(follow_ref, { BoxedProjectile p; BoxedProjectile ref; }) {
	auto p = TASK_BIND(ARGS.p);

	for(Projectile *ref; (ref = ENT_UNBOX((ARGS.ref))); YIELD) {
		p->pos = ref->pos;
	}
}

static void return_part(CompressedBulletData *data, int idx) {
	assert(!bitarray_get(&data->contained_parts_bitmatp, idx));
	auto part = NOT_NULL(ENT_UNBOX(data->loose_parts[idx]));

	part->flags |= PFLAG_NOCLEARBONUS | PFLAG_NOCLEAREFFECT;
	auto trail = spawn_projectile_highlight_effect(part);
	INVOKE_TASK(follow_ref, ENT_BOX(trail), data->main);

	data->loose_parts[idx].ent = NULL;
	data->num_contained_parts++;
	bitarray_set(&data->contained_parts_bitmatp, idx, 1);
	update_main_bullet(data);

	kill_projectile(part);
}

TASK(part_cleanup, { BoxedProjectile part; }) {
	auto p = TASK_BIND(ARGS.part);
	clear_projectile(p, CLEAR_HAZARDS_BULLETS);
}

TASK(part, { CompressedBulletData *data; int idx; cmplx dir; int layer; }) {
	auto p = TASK_BIND(extract_part(ARGS.data, ARGS.idx));
	p->flags |= PFLAG_NOMOVE | PFLAG_NOAUTOREMOVE | PFLAG_MANUALANGLE;

	INVOKE_TASK_AFTER(&TASK_EVENTS(THIS_TASK)->finished, part_cleanup, ENT_BOX(p));

	play_sfx("warp");

	int lifetime = 120;
	cmplx ofs = 0;
	cmplx true_ofs = 0;
	cmplx prev_true_ofs = 0;

	cmplx r = (ARGS.layer & 1) ? I : -I;
	MoveParams move = move_asymptotic_halflife(ARGS.dir * 3 * r, 2 * ARGS.dir, lifetime/3.0);

	Color c0 = p->color;
	Color c1 = cbullet_types[ARGS.data->type].colors.fringe;

	int type = ARGS.data->type;

	for(int t = 0; t < lifetime; ++t, YIELD) {
		auto main = ENT_UNBOX(ARGS.data->main);

		if(!main) {
			return;
		}
		move_update(&ofs, &move);

		real f = (t / (lifetime - 1.0));
		real cf = glm_ease_cubic_inout(f);

		if(type == 1) {
			f = glm_ease_cubic_in(f);
		} else {
			f = glm_ease_quart_in(f);
		}

		if(cf < 0.5) {
			p->color = color_lerp(c0, c1, cf * 2);
		} else {
			p->color = color_lerp(c1, c0, (cf - 0.5) * 2);
		}

		true_ofs = clerp(ofs, 0, f);

		if(type == 1) {
			true_ofs *= cdir(((ARGS.layer & 1) ? -1 : 1) * (M_TAU * t * t) / 30000.0);
		}

		if(true_ofs != prev_true_ofs) {
			p->angle = carg(true_ofs - prev_true_ofs);
		}

		prev_true_ofs = true_ofs;
		p->pos = main->pos + true_ofs;
	}

	play_sfx("shot3");
	return_part(ARGS.data, ARGS.idx);
}

TASK(decompress, { CompressedBulletData *data; }) {
	int type = ARGS.data->type;

	uint parts_per_layer = type ? 4 : 8;
	uint layers = PARTS / parts_per_layer;
	assert(parts_per_layer * layers == PARTS);

	auto main = TASK_BIND(ARGS.data->main);
	common_charge(60, &main->pos, 0, color_mul(main->color, RGBA(0.5, 0.5, 0.5, 0.25)));

	auto init_dir = rng_dir();

	for(uint i = 0; i < layers; ++i) {
		RADIAL_LOOP(l, parts_per_layer, init_dir) {
			int idx = i * parts_per_layer + l.i;
			INVOKE_SUBTASK(part, ARGS.data, idx, l.dir, i);
		}

		WAIT(type ? 1 : 2);
	}

	AWAIT_SUBTASKS;
}

TASK(compressed_bullet_trail, { CompressedBulletData *data; }) {
	auto data = NOT_NULL(ARGS.data);
	auto p = TASK_BIND(data->main);

	auto trail_spr = res_sprite("part/blast_huge_halo");
	auto trail_scale = p->sprite->w / trail_spr->w;

	for(;;YIELD) {
		if(p->ent.draw_layer != LAYER_NODRAW) {
			PARTICLE(
				.pos = p->pos,
				.timeout = 30,
				.sprite_ptr = trail_spr,
				.draw_rule = pdraw_timeout_scalefade_exp(0, 4, 1, 0, 2),
				.color = cbullet_types[data->type].colors.trail,
				.scale = p->scale * trail_scale,
				.flags = PFLAG_NOMOVE | PFLAG_MANUALANGLE,
				.angle = rng_angle(),
			);
		}
	}
}

TASK(compressed_bullet, { BoxedBoss boss; cmplx offset; cmplx dir; int wave; int num; }) {
	auto boss = NOT_NULL(ENT_UNBOX(ARGS.boss));

	int type = ARGS.wave % 2;
	assert((unsigned)type < countof(cbullet_types));

	auto p = TASK_BIND(PROJECTILE(
		.proto = pp_bigball,
		.pos = boss->pos + ARGS.offset,
		.color = cbullet_types[type].colors.main,
		.flags =
			PFLAG_INDESTRUCTIBLE |
			PFLAG_NOCLEAR |
			PFLAG_NOAUTOREMOVE |
			PFLAG_NOCOLLISION |
			PFLAG_NOSPAWNFLARE |
			PFLAG_NOMOVE,
	));

	play_sfx("boon");
	int spawn_time = 120;

	for(int i = 0; i < spawn_time; ++i) {
		real f = i / (spawn_time - 1.0);
		p->opacity = f * f;
		f = glm_ease_quart_in(f);
		p->scale = (1 + I) * lerp(4, 1, f);
		p->color.a = f;
		p->pos = boss->pos + ARGS.offset + ARGS.dir * 128 * (1 - f);
		YIELD;
		boss = NOT_NULL(ENT_UNBOX(ARGS.boss));
	}

	play_sfx("shot_special1");
	p->flags &= ~(PFLAG_NOCOLLISION | PFLAG_NOMOVE);
	spawn_projectile_highlight_effect(p);

	CompressedBulletData data = {
		.main = ENT_BOX(p),
		.num_contained_parts = PARTS,
		.full_collision = p->collision_size,
		.full_size = p->size,
		.type = type,
	};

	memset(&data.contained_parts_bitmatp, 0xff, sizeof(data.contained_parts_bitmatp));

	p->move = (MoveParams) {
		.velocity = I * ARGS.dir,
		.retention = cdir((M_TAU / FULL_PERIOD) * ((ARGS.wave & 1) ? 1 : -1)),
	};

	INVOKE_SUBTASK(compressed_bullet_trail, &data);

	int period = FULL_PERIOD/2;
	WAIT(period/2 * (ARGS.num % 2));

	BoxedTask decompress_task = {};

	for(;;) {
		if(!cotask_unbox(decompress_task) && p->ent.draw_layer == LAYER_NODRAW) {
			kill_projectile(p);
			return;
		}

		decompress_task = cotask_box(INVOKE_SUBTASK(decompress, &data));
		WAIT(period);
	}
}

TASK(spawn_wave, { BoxedBoss boss; int wave; }) {
	// cmplx init_dir = (ARGS.wave % 2 ? (cdir(M_TAU/16)) : (1)) * I;
	cmplx init_dir = I;

	RADIAL_LOOP(l, 8, init_dir) {
		INVOKE_TASK(compressed_bullet, ARGS.boss, l.dir * 64, l.dir, ARGS.wave, l.i);
	}
}

DEFINE_EXTERN_TASK(stagex_spell_entropy) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(boss->move.velocity, VIEWPORT_W/2 + VIEWPORT_H/2*I, 0.04);
	BEGIN_BOSS_ATTACK(&ARGS);

	const int num_waves = 8;
	const int delay = FULL_PERIOD / num_waves;
	const int omit_waves = 3;
	static_assert(num_waves * delay == FULL_PERIOD);

	for(int wave = 0; wave < num_waves - omit_waves; wave++) {
		INVOKE_TASK_DELAYED(wave * delay, spawn_wave, ENT_BOX(boss), wave);
	}
}
