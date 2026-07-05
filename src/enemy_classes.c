/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "enemy_classes.h"

#include "common_tasks.h"
#include "global.h"

#define ECLASS_HP_SWIRL         100
#define ECLASS_HP_FAIRY         400
#define ECLASS_HP_BIG_FAIRY     2000
#define ECLASS_HP_HUGE_FAIRY    8000
#define ECLASS_HP_SUPER_FAIRY   28000

#define LAYER_ENEMY_BACKGROUND           (LAYER_BACKGROUND | 0x12)
#define LAYER_ENEMY_PARTICLE_BACKGROUND  (LAYER_BACKGROUND | 0x11)
#define LAYER_ENEMY_CIRCLE_BACKGROUND    (LAYER_BACKGROUND | 0x10)

typedef enum EnemyDrawOrder {
	ECLASS_ORDER_SWIRL = 1,
	ECLASS_ORDER_FAIRY,
	ECLASS_ORDER_BIG_FAIRY,
	ECLASS_ORDER_HUGE_FAIRY,
	ECLASS_ORDER_SUPER_FAIRY,
} EnemyDrawOrder;

typedef struct EnemyCommonParams {
	const ItemCounts *item_drops;
	cmplx pos;
	real hp;
	EnemyDrawOrder draw_order;
} EnemyCommonParams;

/*
 * Core spawner
 */

TASK(enemy_drop_items, { BoxedEnemy e; ItemCounts items; }) {
	// NOTE: DO NOT USE TAKS_BIND() HERE
	// common_drop_items() yields internally, and we need it to keep going even after the enemy is gone.
	Enemy *e = NOT_NULL(ENT_UNBOX(ARGS.e));

	if(e->damage_info && DAMAGETYPE_IS_PLAYER(e->damage_info->type)) {
		common_drop_items(e->pos, &ARGS.items);
	}
}

static Enemy *enemy_common_spawn(EnemyCommonParams *p) {
	Enemy *e = create_enemy(p->pos, p->hp);
	e->ent.draw_layer = LAYER_ENEMY | ((drawlayer_low_t)p->draw_order << 8);

	INVOKE_TASK_WHEN(&e->events.killed, enemy_drop_items, {
		.e = ENT_BOX(e),
		.items = LIKELY(p->item_drops) ? *p->item_drops : (ItemCounts) { }
	});

	return e;
}

/*
 * Utils
 */

static cmplxf visual_pos(cmplx base_pos, const BaseEnemyVisual *visual) {
	return clerpf(base_pos, visual->fakepos.pos, visual->fakepos.blendfactor);
}

static void kill_projectile_after_task(Projectile *p) {
	INVOKE_TASK_AFTER(&TASK_EVENTS(THIS_TASK)->finished, common_kill_projectile, ENT_BOX(p));
}

/*
 * Fairies
 */

typedef struct FairyParams {
	Animation *main_anim;
	Sprite *circle_sprite;
	FairyHandle *out;
} FairyParams;

static cmplx fairy_visual_pos(const Enemy *e, const FairyVisual *visual) {
	return visual_pos(enemy_visual_pos(e), &visual->base);
}

static SpriteParams fairy_make_sprite_params(
	const Enemy *fairy,
	const FairyVisual *visual,
	int time,
	SpriteParamsBuffer *out_spbuf
) {
	auto ani = visual->base.ani;
	const char *seqname = !fairy->moving ? "main" : (fairy->dir ? "left" : "right");
	Sprite *spr = animation_get_frame(ani, get_ani_sequence(ani, seqname), time);

	float o = visual->base.opacity;
	float b = 1.0f - visual->base.fakepos.blendfactor;
	out_spbuf->color = *RGBA(o*b, o*b, o*b, o);
	out_spbuf->shader_params.vector[0] = visual->summon.progress;
	out_spbuf->shader_params.vector[1] = visual->summon.cloak;
	out_spbuf->shader_params.vector[2] = visual->summon.mask_ofs_bits.val;
	out_spbuf->shader_params.vector[3] = global.frames / 60.0f;

	return (SpriteParams) {
		.color = &out_spbuf->color,
		.sprite_ptr = spr,
		.pos.as_cmplx = fairy_visual_pos(fairy, visual),
		.scale = { visual->base.scale, visual->base.scale },
		.shader_ptr = visual->shader,
		.aux_textures = { visual->noise_texture },
		.shader_params = &out_spbuf->shader_params,
	};
}

static void fairy_draw(Enemy *fairy, const FairyVisual *visual, int time) {
	SpriteParamsBuffer spbuf;
	SpriteParams sp = fairy_make_sprite_params(fairy, visual, time, &spbuf);
	r_draw_sprite(&sp);
}

static void fairy_draw_loop(Enemy *e, const FairyVisual *visual) {
	for(int t = 0;; ++t) {
		WAIT_EVENT_OR_DIE(&e->events.draw);
		fairy_draw(e, visual, t);
	}
}

static Enemy *make_fairy(FairyVisual *visual, EnemyCommonParams *pcommon, FairyParams *pfairy) {
	auto e = enemy_common_spawn(pcommon);

	*visual = (typeof(*visual)) {
		.base = {
			.ani = pfairy->main_anim,
			.scale = 1,
			.opacity = 1,
		},
		.shader = res_shader("sprite_fairy"),
		.noise_texture = res_texture("fractal_noise"),
		.summon.progress = 1,
	};

	*pfairy->out = (FairyHandle) { ENT_BOX(e), visual };
	return e;
}

TASK(fairy_circle, {
	BoxedEnemy e;
	FairyVisual *visual;
	Sprite *sprite;
	Color color;
	float spin_rate;
	float scale_base;
	float scale_osc_ampl;
	float scale_osc_freq;
}) {
	Enemy *e = NOT_NULL(ENT_UNBOX(ARGS.e));
	auto visual = NOT_NULL(ARGS.visual);

	Projectile *circle = TASK_BIND(PARTICLE(
		.sprite_ptr = ARGS.sprite,
		.color = &ARGS.color,
		.flags = PFLAG_NOMOVE | PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE | PFLAG_NOAUTOREMOVE,
		.layer = LAYER_NODRAW,
	));

	kill_projectile_after_task(circle);

	visual->circle = ENT_BOX(circle);
	YIELD;
	circle->ent.draw_layer = LAYER_PARTICLE_LOW;

	float scale_osc_phase = 0.0f;

	for(;(e = ENT_UNBOX(ARGS.e)); YIELD) {
		if(visual->summon.progress >= 1 && visual->summon.cloak > 0) {
			fapproach_asymptotic_p(&visual->summon.cloak, 0, 0.1, 1e-2);
		}

		circle->pos = fairy_visual_pos(e, visual);
		circle->angle += ARGS.spin_rate;
		float s = ARGS.scale_base + ARGS.scale_osc_ampl * sinf(scale_osc_phase);
		s *= visual->base.scale;
		circle->scale = CMPLXF(s, s);
		circle->opacity = visual->base.opacity * (1 - visual->base.fakepos.blendfactor);
		circle->ent.draw_layer = visual->base.fakepos.blendfactor
			? LAYER_ENEMY_CIRCLE_BACKGROUND
			: LAYER_PARTICLE_LOW,
		scale_osc_phase += ARGS.scale_osc_freq;
	}
}

TASK(fairy_flame_emitter, {
	BoxedEnemy e;
	FairyVisual *visual;
	int period;
	Color color;
}) {
	Enemy *e = TASK_BIND(ARGS.e);
	auto visual = NOT_NULL(ARGS.visual);

	int period = ARGS.period;
	Sprite *spr = res_sprite("part/smoothdot");
	WAIT(rng_irange(0, period) + 1);

	DECLARE_ENT_ARRAY(Projectile, parts, 16);

	cmplx old_pos = fairy_visual_pos(e, ARGS.visual);

	for(int t = 0;; ++t, YIELD) {
		cmplx epos = fairy_visual_pos(e, ARGS.visual);

		ENT_ARRAY_FOREACH(&parts, Projectile *p, {
			p->move.attraction_point += epos - old_pos - I + rng_sreal() * 0.5;
		});

		ENT_ARRAY_COMPACT(&parts);

		old_pos = epos;

		if(!(t % period)) {
			cmplx offset = rng_sreal() * 8;
			offset += rng_sreal() * 10 * I;
			cmplx spawn_pos = epos + visual->base.scale * offset;

			ENT_ARRAY_ADD(&parts, PARTICLE(
				.sprite_ptr = spr,
				.pos = spawn_pos,
				.color = &ARGS.color,
				.draw_rule = pdraw_timeout_scalefade(2+2*I, 0.5+2*I, 1, 0),
				.angle = M_PI/2 + rng_sreal() * M_PI/16,
				.timeout = 50,
				.move = move_towards(0, spawn_pos, 0.3),
				.flags = PFLAG_MANUALANGLE,
				.layer = visual->base.fakepos.blendfactor
					? LAYER_ENEMY_PARTICLE_BACKGROUND
					: LAYER_PARTICLE_MID,
				.scale = visual->base.scale,
				.opacity = visual->base.opacity,
			));
		}
	}
}

TASK(fairy_stardust_emitter, {
	BoxedEnemy e;
	FairyVisual *visual;
	int period;
	Color color;
}) {
	Enemy *e = TASK_BIND(ARGS.e);
	FairyVisual *visual = NOT_NULL(ARGS.visual);

	int period = 9;
	Sprite *spr = res_sprite("part/stardust");
	WAIT(rng_irange(0, period) + 1);

	DECLARE_ENT_ARRAY(Projectile, parts, 32);

	cmplx old_pos = fairy_visual_pos(e, ARGS.visual);

	for(int t = 0;; ++t, YIELD) {
		cmplx epos = fairy_visual_pos(e, ARGS.visual);

		ENT_ARRAY_FOREACH(&parts, Projectile *p, {
			p->move.attraction_point += epos - old_pos;
			p->scale = (1 + I) * visual->base.scale;
			p->opacity = visual->base.opacity;
		});

		ENT_ARRAY_COMPACT(&parts);

		old_pos = epos;

		if(!(t % period)) {
			RNG_ARRAY(rng, 4);
			cmplx ofs = vrng_sreal(rng[0]) * 12 + vrng_sreal(rng[1]) * 10 * I;
			cmplx pos = epos + visual->base.scale * ofs;

			ENT_ARRAY_ADD(&parts, PARTICLE(
				.sprite_ptr = spr,
				.pos = pos,
				.color = &ARGS.color,
				.draw_rule = pdraw_timeout_scalefade_exp(0.1 * (1+I), 2 * (1+I), 1, 0, 2),
				.angle = vrng_angle(rng[0]),
				.timeout = 180,
				.move = move_towards(0, pos, 0.18 + 0.01 * vrng_sreal(rng[1])),
				.flags = PFLAG_MANUALANGLE,
				.layer = visual->base.fakepos.blendfactor
					? LAYER_ENEMY_PARTICLE_BACKGROUND
					: LAYER_PARTICLE_MID,
				.scale = visual->base.scale,
				.opacity = visual->base.opacity,
			));
		}
	}
}

TASK(fairy_weak, {
	EnemyCommonParams common;
	FairyParams fairy;
}) {
	FairyVisual visual;
	auto e = TASK_BIND(make_fairy(&visual, &ARGS.common, &ARGS.fairy));

	INVOKE_SUBTASK(fairy_circle, ENT_BOX(e), &visual,
		.sprite = ARGS.fairy.circle_sprite,
		.color = *RGB(1, 1, 1),
		.spin_rate = 10 * DEG2RAD,
		.scale_base = 0.8f,
		.scale_osc_ampl = 1.0f / 6.0f,
		.scale_osc_freq = 0.1f,
	);

	fairy_draw_loop(e, &visual);
}

FairyHandle ecls_spawn_fairy_blue(cmplx pos, const ItemCounts *item_drops) {
	FairyHandle h = {};
	INVOKE_TASK(fairy_weak, {
		.common = {
			.pos = pos,
			.item_drops = item_drops,
			.hp = ECLASS_HP_FAIRY,
			.draw_order = ECLASS_ORDER_FAIRY,
		},
		.fairy = {
			.main_anim = res_anim("enemy/fairy_blue"),
			.circle_sprite = res_sprite("fairy_circle"),
			.out = &h,
		},
	});
	return h;
}

FairyHandle ecls_spawn_fairy_red(cmplx pos, const ItemCounts *item_drops) {
	FairyHandle h = {};
	INVOKE_TASK(fairy_weak, {
		.common = {
			.pos = pos,
			.item_drops = item_drops,
			.hp = ECLASS_HP_FAIRY,
			.draw_order = ECLASS_ORDER_FAIRY,
		},
		.fairy = {
			.main_anim = res_anim("enemy/fairy_red"),
			.circle_sprite = res_sprite("fairy_circle_red"),
			.out = &h,
		},
	});
	return h;
}

TASK(fairy_big, {
	EnemyCommonParams common;
	FairyParams fairy;
}) {
	FairyVisual visual;
	auto e = TASK_BIND(make_fairy(&visual, &ARGS.common, &ARGS.fairy));

	INVOKE_SUBTASK(fairy_circle, ENT_BOX(e), &visual,
		.sprite = ARGS.fairy.circle_sprite,
		.color = *RGB(1, 1, 1),
		.spin_rate = 10 * DEG2RAD,
		.scale_base = 0.8f,
		.scale_osc_ampl = 1.0f / 6.0f,
		.scale_osc_freq = 0.1f,
	);

	INVOKE_SUBTASK(fairy_flame_emitter, ENT_BOX(e), &visual,
		.period = 5,
		.color = *RGBA(0.0, 0.2, 0.3, 0.0)
	);

	fairy_draw_loop(e, &visual);
}

FairyHandle ecls_spawn_big_fairy(cmplx pos, const ItemCounts *item_drops) {
	FairyHandle h = {};
	INVOKE_TASK(fairy_big, {
		.common = {
			.pos = pos,
			.item_drops = item_drops,
			.hp = ECLASS_HP_BIG_FAIRY,
			.draw_order = ECLASS_ORDER_BIG_FAIRY,
		},
		.fairy = {
			.main_anim = res_anim("enemy/bigfairy"),
			.circle_sprite = res_sprite("fairy_circle_big"),
			.out = &h,
		},
	});
	return h;
}

TASK(fairy_huge, {
	EnemyCommonParams common;
	FairyParams fairy;
}) {
	FairyVisual visual;
	auto e = TASK_BIND(make_fairy(&visual, &ARGS.common, &ARGS.fairy));

	INVOKE_SUBTASK(fairy_circle, ENT_BOX(e), &visual,
		.sprite = ARGS.fairy.circle_sprite,
		.color = *RGBA(1, 1, 1, 0.95),
		.spin_rate = 5 * DEG2RAD,
		.scale_base = 0.85f,
		.scale_osc_ampl = 0.1f,
		.scale_osc_freq = 1.0f / 15.0f,
	);

	INVOKE_SUBTASK(fairy_flame_emitter, ENT_BOX(e), &visual,
		.period = 6,
		.color = *RGBA(0.0, 0.2, 0.3, 0.0)
	);

	INVOKE_SUBTASK(fairy_flame_emitter, ENT_BOX(e), &visual,
		.period = 6,
		.color = *RGBA(0.3, 0.0, 0.2, 0.0)
	);

	fairy_draw_loop(e, &visual);
}

FairyHandle ecls_spawn_huge_fairy(cmplx pos, const ItemCounts *item_drops) {
	FairyHandle h = {};
	INVOKE_TASK(fairy_huge, {
		.common = {
			.pos = pos,
			.item_drops = item_drops,
			.hp = ECLASS_HP_HUGE_FAIRY,
			.draw_order = ECLASS_ORDER_HUGE_FAIRY,
		},
		.fairy = {
			.main_anim = res_anim("enemy/hugefairy"),
			.circle_sprite = res_sprite("fairy_circle_big"),
			.out = &h,
		},
	});
	return h;
}

TASK(fairy_super, {
	EnemyCommonParams common;
	FairyParams fairy;
}) {
	FairyVisual visual;
	auto e = TASK_BIND(make_fairy(&visual, &ARGS.common, &ARGS.fairy));

	INVOKE_SUBTASK(fairy_circle, ENT_BOX(e), &visual,
		.sprite = ARGS.fairy.circle_sprite,
		.color = *RGBA(1, 1, 1, 0.6),
		.spin_rate = 5 * DEG2RAD,
		.scale_base = 0.9f,
		.scale_osc_ampl = 0.1f,
		.scale_osc_freq = 1.0f / 15.0f,
	);

	INVOKE_SUBTASK(fairy_flame_emitter, ENT_BOX(e), &visual,
		.period = 5,
		.color = *RGBA(0.2, 0.0, 0.3, 0.0)
	);

	INVOKE_SUBTASK(fairy_stardust_emitter, ENT_BOX(e), &visual,
		.period = 15,
		.color = *RGBA(0.0, 0.0, 0.0, 0.8)
	);

	fairy_draw_loop(e, &visual);
}

FairyHandle ecls_spawn_super_fairy(cmplx pos, const ItemCounts *item_drops) {
	FairyHandle h = {};
	INVOKE_TASK(fairy_super, {
		.common = {
			.pos = pos,
			.item_drops = item_drops,
			.hp = ECLASS_HP_SUPER_FAIRY,
			.draw_order = ECLASS_ORDER_SUPER_FAIRY,
		},
		.fairy = {
			.main_anim = res_anim("enemy/superfairy"),
			.circle_sprite = res_sprite("fairy_circle_big_and_mean"),
			.out = &h,
		},
	});
	return h;
}

/*
 * Swirls
 */

typedef struct SwirlParams {
	SwirlHandle *out;
} SwirlParams;

static SpriteParams swirl_make_sprite_prams(
	const Enemy *swirl,
	const SwirlVisual *visual,
	int time,
	SpriteParamsBuffer *spbuf
) {
	float o = visual->base.opacity;
	float b = 1.0f - visual->base.fakepos.blendfactor;

	spbuf->color = *RGBA(o*b*b, o*b*b, o*b*b, o);
	spbuf->shader_params = (ShaderCustomParams) { 1.0f };

	return (SpriteParams) {
		.color = &spbuf->color,
		.sprite_ptr = visual->base.spr,
		.shader_ptr = visual->shader,
		.shader_params = &spbuf->shader_params,
		.pos.as_cmplx = visual_pos(enemy_visual_pos(swirl), &visual->base),
		.rotation.angle = time * 10 * DEG2RAD,
		.scale = { visual->base.scale, visual->base.scale },
	};
}

static void swirl_draw(Enemy *swirl, const SwirlVisual *visual, int time) {
	SpriteParamsBuffer spbuf;
	SpriteParams sp = swirl_make_sprite_prams(swirl, visual, time, &spbuf);
	r_draw_sprite(&sp);
}

static void swirl_draw_loop(Enemy *e, const SwirlVisual *visual) {
	for(int t = 0;; ++t) {
		WAIT_EVENT_OR_DIE(&e->events.draw);
		swirl_draw(e, visual, t);
	}
}

static Enemy *make_swirl(SwirlVisual *visual, EnemyCommonParams *pcommon, SwirlParams *pswirl) {
	auto e = enemy_common_spawn(pcommon);

	*visual = (typeof(*visual)) {
		.base = {
			.spr = res_sprite("enemy/swirl"),
			.scale = 1,
			.opacity = 1,
		},
		.shader = res_shader("sprite_particle"),
	};

	*pswirl->out = (SwirlHandle) { ENT_BOX(e), visual };
	return e;
}

TASK(swirl, {
	EnemyCommonParams common;
	SwirlParams swirl;
}) {
	SwirlVisual visual;
	auto e = TASK_BIND(make_swirl(&visual, &ARGS.common, &ARGS.swirl));
	swirl_draw_loop(e, &visual);
}

SwirlHandle ecls_spawn_swirl(cmplx pos, const ItemCounts *item_drops) {
	SwirlHandle h = {};
	INVOKE_TASK(swirl, {
		.common = {
			.pos = pos,
			.item_drops = item_drops,
			.hp = ECLASS_HP_SWIRL,
			.draw_order = ECLASS_ORDER_SWIRL,
		},
		.swirl = {
			.out = &h,
		}
	});
	return h;
}

/*
 * Entrances
 */

static inline void entrance_util_set_flags(Enemy *e, EnemyFlag *tempflags) {
	EnemyFlag flags = EFLAG_NO_HIT | EFLAG_NO_HURT | EFLAG_INVULNERABLE;
	*tempflags = flags & ~e->flags;
	e->flags |= flags;
}

static inline void entrance_util_restore_flags(Enemy *e, EnemyFlag *tempflags) {
	e->flags &= ~*tempflags;
}

static void ecls_base_3d_move_in(
	BoxedEnemy ent, BaseEnemyVisual *visual, Camera3D *cam, vec3 init_pos_3d, int duration
) {
	auto e = NOT_NULL(ENT_UNBOX(ent));
	assert(duration > 0);

	EnemyFlag tempflags;
	entrance_util_set_flags(e, &tempflags);

	DrawLayer layer = e->ent.draw_layer;
	e->ent.draw_layer = LAYER_ENEMY_BACKGROUND;

	for(int i = 1;;) {
		vec3 initpos_projected;
		camera3d_project(cam, init_pos_3d, initpos_projected);
		visual->fakepos.pos = CMPLXF(initpos_projected[0], initpos_projected[1]);

		float f = i / (float)duration;
		visual->fakepos.blendfactor = 1.0f - glm_ease_cubic_in(f);
		visual->scale = glm_ease_cubic_in(f);
		visual->opacity = glm_ease_sine_out(f);

		if(i == duration) {
			break;
		}

		drawlayer_low_t lowmask = (LAYER_LOW_MASK & (drawlayer_low_t)(f * ((1 << LAYER_LOW_BITS) - 1)));
		e->ent.draw_layer = LAYER_ENEMY_BACKGROUND | lowmask;

		++i;
		YIELD;

		if(UNLIKELY((e = ENT_UNBOX(ent)) == NULL)) {
			return;
		}
	}

	entrance_util_restore_flags(e, &tempflags);
	e->ent.draw_layer = layer;
}

FairyHandle ecls_fairy_3d_move_in(FairyHandle fairy, Camera3D *cam, vec3 init_pos_3d, int duration) {
	ecls_base_3d_move_in(fairy.entity, &NOT_NULL(fairy.visual)->base, cam, init_pos_3d, duration);
	return fairy;
}

SwirlHandle ecls_swirl_3d_move_in(SwirlHandle swirl, Camera3D *cam, vec3 init_pos_3d, int duration) {
	ecls_base_3d_move_in(swirl.entity, &NOT_NULL(swirl.visual)->base, cam, init_pos_3d, duration);
	return swirl;
}

FairyHandle ecls_fairy_summon(FairyHandle fairy, int duration) {
	auto e = NOT_NULL(ENT_UNBOX(fairy.entity));  // TODO assert bound to current task
	auto visual = NOT_NULL(fairy.visual);

	assert(duration > 0);

	EnemyFlag tempflags;
	entrance_util_set_flags(e, &tempflags);

	DrawLayer elayer = e->ent.draw_layer;
	e->ent.draw_layer = LAYER_NODRAW;
	visual->summon.progress = 0;
	visual->summon.cloak = 0.75;
	visual->summon.mask_ofs_bits.bits = rng_u32();

	Projectile *circle = NOT_NULL(ENT_UNBOX(visual->circle));
	Color circle_basecolor = circle->color;
	Color circle_spawncolor = *color_mul(
		COLOR_COPY(&circle_basecolor),
		RGBA(2, 2, 2, 0)
	);

	float fairy_delay = 0.1f;

	for(int i = 1;;) {
		float f = i / (float)duration;

		visual->base.opacity = glm_ease_quint_in(clamp(f * 2.0f, 0.0f, 1.0f));
		visual->base.scale = lerpf(3.0f, 1.0f, glm_ease_back_out( glm_ease_sine_inout(f)));

		if(f >= fairy_delay) {
			visual->summon.progress = glm_ease_sine_inout(
				(f - fairy_delay) / (1.0f - fairy_delay) * 0.875f);
			e->ent.draw_layer = elayer;
		}

		if(LIKELY(circle = ENT_UNBOX(visual->circle))) {
			circle->color = *color_lerp(
				COLOR_COPY(&circle_spawncolor),
				&circle_basecolor,
				glm_ease_quad_in(f)
			);
		}

		if(i == duration) {
			break;
		}

		++i;
		YIELD;

		if(UNLIKELY((e = ENT_UNBOX(fairy.entity)) == NULL)) {
			return fairy;
		}
	}

	visual->summon.progress = 1;
	entrance_util_restore_flags(e, &tempflags);

	return fairy;
}

/*
 * Old-style wrappers
 */

#define SPAWN_WRAPPER(kind) \
	Enemy *(espawn_##kind)(cmplx pos, const ItemCounts *item_drops) { \
		return ENT_UNBOX(ecls_spawn_##kind(pos, item_drops).entity); \
	}

SPAWN_WRAPPER(swirl)
SPAWN_WRAPPER(fairy_blue)
SPAWN_WRAPPER(fairy_red)
SPAWN_WRAPPER(big_fairy)
SPAWN_WRAPPER(huge_fairy)
SPAWN_WRAPPER(super_fairy)
