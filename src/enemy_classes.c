/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "enemy_classes.h"
#include "common_tasks.h"
#include "global.h"
#include "coroutine.h"

#define ECLASS_HP_SWIRL         100
#define ECLASS_HP_FAIRY         400
#define ECLASS_HP_BIG_FAIRY     2000
#define ECLASS_HP_HUGE_FAIRY    8000
#define ECLASS_HP_SUPER_FAIRY   28000

typedef struct BaseEnemyVisualParams {
	union {
		Sprite *spr;
		Animation *ani;
	};
} BaseEnemyVisualParams;

typedef struct FairyVisualParams {
	BaseEnemyVisualParams base;
} FairyVisualParams;

SpriteParams ecls_anyfairy_sprite_params(
	Enemy *fairy,
	EnemyDrawParams draw_params,
	SpriteParamsBuffer *out_spbuf
) {
	FairyVisualParams *vp = fairy->visual.drawdata;
	auto ani = vp->base.ani;
	const char *seqname = !fairy->moving ? "main" : (fairy->dir ? "left" : "right");
	Sprite *spr = animation_get_frame(ani, get_ani_sequence(ani, seqname), draw_params.time);

	out_spbuf->color = *RGB(1, 1, 1);

	return (SpriteParams) {
		.color = &out_spbuf->color,
		.sprite_ptr = spr,
		.pos.as_cmplx = draw_params.pos,
	};
}

static void anyfairy_draw(Enemy *e, EnemyDrawParams p) {
	SpriteParamsBuffer spbuf;
	SpriteParams sp = ecls_anyfairy_sprite_params(e, p, &spbuf);
	r_draw_sprite(&sp);
}

TASK(fairy_circle, {
	BoxedEnemy e;
	Sprite *sprite;
	Color color;
	float spin_rate;
	float scale_base;
	float scale_osc_ampl;
	float scale_osc_freq;
}) {
	Projectile *circle = TASK_BIND(PARTICLE(
		.sprite_ptr = ARGS.sprite,
		.color = &ARGS.color,
		.flags = PFLAG_NOMOVE | PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE | PFLAG_NOAUTOREMOVE,
		.layer = LAYER_PARTICLE_LOW,
	));

	float scale_osc_phase = 0.0f;

	for(Enemy *e; (e = ENT_UNBOX(ARGS.e)); YIELD) {
		circle->pos = enemy_visual_pos(e);
		circle->angle += ARGS.spin_rate;
		float s = ARGS.scale_base + ARGS.scale_osc_ampl * sinf(scale_osc_phase);
		circle->scale = CMPLXF(s, s);
		scale_osc_phase += ARGS.scale_osc_freq;
	}

	kill_projectile(circle);
}

TASK(fairy_flame_emitter, {
	BoxedEnemy e;
	int period;
	Color color;
}) {
	Enemy *e = TASK_BIND(ARGS.e);
	int period = ARGS.period;
	Sprite *spr = res_sprite("part/smoothdot");
	WAIT(rng_irange(0, period) + 1);

	DECLARE_ENT_ARRAY(Projectile, parts, 16);
	cmplx old_pos = e->pos;
	YIELD;

	for(int t = 0;; ++t, YIELD) {
		ENT_ARRAY_FOREACH(&parts, Projectile *p, {
			p->move.attraction_point += e->pos - old_pos - I + rng_sreal() * 0.5;
		});

		ENT_ARRAY_COMPACT(&parts);

		old_pos = e->pos;

		if(!(t % period)) {
			cmplx offset = rng_sreal() * 8;
			offset += rng_sreal() * 10 * I;
			cmplx spawn_pos = e->pos + offset;

			ENT_ARRAY_ADD(&parts, PARTICLE(
				.sprite_ptr = spr,
				.pos = spawn_pos,
				.color = &ARGS.color,
				.draw_rule = pdraw_timeout_scalefade(2+2*I, 0.5+2*I, 1, 0),
				.angle = M_PI/2 + rng_sreal() * M_PI/16,
				.timeout = 50,
				.move = move_towards(0, spawn_pos, 0.3),
				.flags = PFLAG_MANUALANGLE,
				.layer = LAYER_PARTICLE_MID,
			));
		}
	}
}

TASK(fairy_stardust_emitter, {
	BoxedEnemy e;
	int period;
	Color color;
}) {
	Enemy *e = TASK_BIND(ARGS.e);
	int period = 9;
	Sprite *spr = res_sprite("part/stardust");
	WAIT(rng_irange(0, period) + 1);

	DECLARE_ENT_ARRAY(Projectile, parts, 32);
	cmplx old_pos = e->pos;
	YIELD;

	for(int t = 0;; ++t, YIELD) {
		ENT_ARRAY_FOREACH(&parts, Projectile *p, {
			p->move.attraction_point += e->pos - old_pos;
		});

		ENT_ARRAY_COMPACT(&parts);

		old_pos = e->pos;

		if(!(t % period)) {
			RNG_ARRAY(rng, 4);
			cmplx pos = e->pos + vrng_sreal(rng[0]) * 12 + vrng_sreal(rng[1]) * 10 * I;

			ENT_ARRAY_ADD(&parts, PARTICLE(
				.sprite_ptr = spr,
				.pos = pos,
				.color = &ARGS.color,
				.draw_rule = pdraw_timeout_scalefade_exp(0.1 * (1+I), 2 * (1+I), 1, 0, 2),
				.angle = vrng_angle(rng[0]),
				.timeout = 180,
				.move = move_towards(0, pos, 0.18 + 0.01 * vrng_sreal(rng[1])),
				.flags = PFLAG_MANUALANGLE,
				.layer = LAYER_PARTICLE_MID,
			));
		}
	}
}

TASK(enemy_drop_items, { BoxedEnemy e; ItemCounts items; }) {
	// NOTE: Don't bind here! We need this task to outlive the enemy.
	Enemy *e = NOT_NULL(ENT_UNBOX(ARGS.e));

	if(e->damage_info && DAMAGETYPE_IS_PLAYER(e->damage_info->type)) {
		common_drop_items(e->pos, &ARGS.items);
	}

	// NOTE: Needed to keep drawdata alive for this frame (see _spawn below)
	YIELD;
}

static Enemy *_spawn(
	cmplx pos, const ItemCounts *item_drops, real hp, EnemyDrawFunc draw, size_t drawdata_alloc
) {
	assert(drawdata_alloc > 0);
	Enemy *e = create_enemy(pos, hp, (EnemyVisual) { draw });

	// NOTE: almost all enemies drop items, so we can abuse the itemdrop task's stack to hold onto
	// the draw data buffer for us.

	auto t = INVOKE_TASK_WHEN(&e->events.killed, enemy_drop_items, {
		.e = ENT_BOX(e),
		.items = LIKELY(item_drops) ? *item_drops : (ItemCounts) { }
	});
	e->visual.drawdata = cotask_malloc(t, drawdata_alloc);

	return e;
}

// Spawn enemy; allocate and initialize its drawdata buffer
#define spawn(_pos, _items, _hp, _draw, ...) ({ \
	auto _e = _spawn(_pos, _items, _hp, _draw, sizeof(__VA_ARGS__)); \
	*(typeof(__VA_ARGS__)*)(_e->visual.drawdata) = __VA_ARGS__; \
	_e; \
});

static void swirl_draw(Enemy *e, EnemyDrawParams p) {
	BaseEnemyVisualParams *vp = e->visual.drawdata;
	r_draw_sprite(&(SpriteParams) {
		.color = RGB(1, 1 ,1),
		.sprite_ptr = vp->spr,
		.pos.as_cmplx = p.pos,
		.rotation.angle = p.time * 10 * DEG2RAD,
	});
}

Enemy *(espawn_swirl)(cmplx pos, const ItemCounts *item_drops) {
	return spawn(pos, item_drops, ECLASS_HP_SWIRL, swirl_draw, (BaseEnemyVisualParams) {
		.spr = res_sprite("enemy/swirl"),
	});
}

static Enemy *spawn_fairy(cmplx pos, const ItemCounts *item_drops, real hp, Animation *ani) {
	return spawn(pos, item_drops, hp, anyfairy_draw, (FairyVisualParams) {
		.base.ani = ani,
	});
}

static Enemy *espawn_fairy_weak(
	cmplx pos, const ItemCounts *item_drops, Animation *fairy_ani, Sprite *circle_spr
) {
	auto e = spawn_fairy(pos, item_drops, ECLASS_HP_FAIRY, fairy_ani);
	INVOKE_TASK(fairy_circle, ENT_BOX(e),
		.sprite = circle_spr,
		.color = *RGB(1, 1, 1),
		.spin_rate = 10 * DEG2RAD,
		.scale_base = 0.8f,
		.scale_osc_ampl = 1.0f / 6.0f,
		.scale_osc_freq = 0.1f,
	);
	return e;
}

Enemy *(espawn_fairy_blue)(cmplx pos, const ItemCounts *item_drops) {
	return espawn_fairy_weak(
		pos, item_drops,
		res_anim("enemy/fairy_blue"),
		res_sprite("fairy_circle")
	);
}

Enemy *(espawn_fairy_red)(cmplx pos, const ItemCounts *item_drops) {
	return espawn_fairy_weak(
		pos, item_drops,
		res_anim("enemy/fairy_red"),
		res_sprite("fairy_circle_red")
	);
}

Enemy *(espawn_big_fairy)(cmplx pos, const ItemCounts *item_drops) {
	auto e = spawn_fairy(pos, item_drops, ECLASS_HP_BIG_FAIRY, res_anim("enemy/bigfairy"));
	INVOKE_TASK(fairy_circle, ENT_BOX(e),
		.sprite = res_sprite("fairy_circle_big"),
		.color = *RGB(1, 1, 1),
		.spin_rate = 10 * DEG2RAD,
		.scale_base = 0.8f,
		.scale_osc_ampl = 1.0f / 6.0f,
		.scale_osc_freq = 0.1f,
	);
	INVOKE_TASK(fairy_flame_emitter, ENT_BOX(e),
		.period = 5,
		.color = *RGBA(0.0, 0.2, 0.3, 0.0)
	);
	return e;
}

Enemy *(espawn_huge_fairy)(cmplx pos, const ItemCounts *item_drops) {
	auto e = spawn_fairy(pos, item_drops, ECLASS_HP_HUGE_FAIRY, res_anim("enemy/hugefairy"));
	INVOKE_TASK(fairy_circle, ENT_BOX(e),
		.sprite = res_sprite("fairy_circle_big"),
		.color = *RGBA(1, 1, 1, 0.95),
		.spin_rate = 5 * DEG2RAD,
		.scale_base = 0.85f,
		.scale_osc_ampl = 0.1f,
		.scale_osc_freq = 1.0f / 15.0f,
	);
	INVOKE_TASK(fairy_flame_emitter, ENT_BOX(e),
		.period = 6,
		.color = *RGBA(0.0, 0.2, 0.3, 0.0)
	);
	INVOKE_TASK(fairy_flame_emitter, ENT_BOX(e),
		.period = 6,
		.color = *RGBA(0.3, 0.0, 0.2, 0.0)
	);
	return e;
}

Enemy *(espawn_super_fairy)(cmplx pos, const ItemCounts *item_drops) {
	auto e = spawn_fairy(pos, item_drops, ECLASS_HP_SUPER_FAIRY, res_anim("enemy/superfairy"));
	INVOKE_TASK(fairy_circle, ENT_BOX(e),
		.sprite = res_sprite("fairy_circle_big_and_mean"),
		.color = *RGBA(1, 1, 1, 0.6),
		.spin_rate = 5 * DEG2RAD,
		.scale_base = 0.9f,
		.scale_osc_ampl = 0.1f,
		.scale_osc_freq = 1.0f / 15.0f,
	);
	INVOKE_TASK(fairy_flame_emitter, ENT_BOX(e),
		.period = 5,
		.color = *RGBA(0.2, 0.0, 0.3, 0.0)
	);
	INVOKE_TASK(fairy_stardust_emitter, ENT_BOX(e),
		.period = 15,
		.color = *RGBA(0.0, 0.0, 0.0, 0.8)
	);
	return e;
}
