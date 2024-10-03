/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
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

typedef struct BaseEnemyVisualParams {
	union {
		Sprite *spr;
		Animation *ani;
	};
	float scale;
	float opacity;
	struct {
		cmplxf pos;
		float blendfactor;
	} fakepos;
} BaseEnemyVisualParams;

typedef struct FairyVisualParams {
	BaseEnemyVisualParams base;
	BoxedProjectile circle;
	struct {
		float progress;
		float cloak;
		FloatBits mask_ofs_bits;
	} summon;
} FairyVisualParams;

static cmplxf visual_pos(const EnemyDrawParams *dp, BaseEnemyVisualParams *vp) {
	return clerpf(dp->pos, vp->fakepos.pos, vp->fakepos.blendfactor);
}

static cmplxf visual_pos_e(Enemy *e) {
	EnemyDrawParams edp = { .pos = enemy_visual_pos(e) };
	return visual_pos(&edp, e->visual.drawdata);
}

SpriteParams ecls_anyfairy_sprite_params(
	Enemy *fairy,
	EnemyDrawParams draw_params,
	SpriteParamsBuffer *out_spbuf
) {
	FairyVisualParams *vp = fairy->visual.drawdata;
	auto ani = vp->base.ani;
	const char *seqname = !fairy->moving ? "main" : (fairy->dir ? "left" : "right");
	Sprite *spr = animation_get_frame(ani, get_ani_sequence(ani, seqname), draw_params.time);

	float o = vp->base.opacity;
	float b = 1.0f - vp->base.fakepos.blendfactor;
	out_spbuf->color = *RGBA(o*b, o*b, o*b, o);
	out_spbuf->shader_params.vector[0] = vp->summon.progress;
	out_spbuf->shader_params.vector[1] = vp->summon.cloak;
	out_spbuf->shader_params.vector[2] = vp->summon.mask_ofs_bits.val;
	out_spbuf->shader_params.vector[3] = global.frames / 60.0f;

	return (SpriteParams) {
		.color = &out_spbuf->color,
		.sprite_ptr = spr,
		.pos.as_cmplx = visual_pos(&draw_params, &vp->base),
		.scale = { vp->base.scale, vp->base.scale },
		.shader_ptr = res_shader("sprite_fairy"),
		.aux_textures = { res_texture("fractal_noise") },
		.shader_params = &out_spbuf->shader_params,
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
	Enemy *e = NOT_NULL(ENT_UNBOX(ARGS.e));

	Projectile *circle = TASK_BIND(PARTICLE(
		.sprite_ptr = ARGS.sprite,
		.color = &ARGS.color,
		.flags = PFLAG_NOMOVE | PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE | PFLAG_NOAUTOREMOVE,
		.layer = LAYER_NODRAW,
	));

	FairyVisualParams *fvp = e->visual.drawdata;
	fvp->circle = ENT_BOX(circle);
	YIELD;
	circle->ent.draw_layer = LAYER_PARTICLE_LOW;

	float scale_osc_phase = 0.0f;

	for(;(e = ENT_UNBOX(ARGS.e)); YIELD) {
		assert(fvp == e->visual.drawdata);

		if(fvp->summon.progress >= 1 && fvp->summon.cloak > 0) {
			fapproach_asymptotic_p(&fvp->summon.cloak, 0, 0.1, 1e-2);
		}

		circle->pos = visual_pos_e(e);
		circle->angle += ARGS.spin_rate;
		float s = ARGS.scale_base + ARGS.scale_osc_ampl * sinf(scale_osc_phase);
		s *= fvp->base.scale;
		circle->scale = CMPLXF(s, s);
		circle->opacity = fvp->base.opacity * (1 - fvp->base.fakepos.blendfactor);
		circle->ent.draw_layer = fvp->base.fakepos.blendfactor
			? LAYER_ENEMY_CIRCLE_BACKGROUND
			: LAYER_PARTICLE_LOW,
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

	cmplx old_pos = visual_pos_e(e);
	FairyVisualParams *fvp = e->visual.drawdata;

	for(int t = 0;; ++t, YIELD) {
		cmplx epos = visual_pos_e(e);

		ENT_ARRAY_FOREACH(&parts, Projectile *p, {
			p->move.attraction_point += epos - old_pos - I + rng_sreal() * 0.5;
		});

		ENT_ARRAY_COMPACT(&parts);

		old_pos = epos;

		if(!(t % period)) {
			cmplx offset = rng_sreal() * 8;
			offset += rng_sreal() * 10 * I;
			cmplx spawn_pos = epos + fvp->base.scale * offset;

			ENT_ARRAY_ADD(&parts, PARTICLE(
				.sprite_ptr = spr,
				.pos = spawn_pos,
				.color = &ARGS.color,
				.draw_rule = pdraw_timeout_scalefade(2+2*I, 0.5+2*I, 1, 0),
				.angle = M_PI/2 + rng_sreal() * M_PI/16,
				.timeout = 50,
				.move = move_towards(0, spawn_pos, 0.3),
				.flags = PFLAG_MANUALANGLE,
				.layer = fvp->base.fakepos.blendfactor
					? LAYER_ENEMY_PARTICLE_BACKGROUND
					: LAYER_PARTICLE_MID,
				.scale = fvp->base.scale,
				.opacity = fvp->base.opacity,
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

	cmplx old_pos = visual_pos_e(e);
	FairyVisualParams *fvp = e->visual.drawdata;

	for(int t = 0;; ++t, YIELD) {
		cmplx epos = visual_pos_e(e);

		ENT_ARRAY_FOREACH(&parts, Projectile *p, {
			p->move.attraction_point += epos - old_pos;
			p->scale = (1 + I) * fvp->base.scale;
			p->opacity = fvp->base.opacity;
		});

		ENT_ARRAY_COMPACT(&parts);

		old_pos = epos;

		if(!(t % period)) {
			RNG_ARRAY(rng, 4);
			cmplx ofs = vrng_sreal(rng[0]) * 12 + vrng_sreal(rng[1]) * 10 * I;
			cmplx pos = epos + fvp->base.scale * ofs;

			ENT_ARRAY_ADD(&parts, PARTICLE(
				.sprite_ptr = spr,
				.pos = pos,
				.color = &ARGS.color,
				.draw_rule = pdraw_timeout_scalefade_exp(0.1 * (1+I), 2 * (1+I), 1, 0, 2),
				.angle = vrng_angle(rng[0]),
				.timeout = 180,
				.move = move_towards(0, pos, 0.18 + 0.01 * vrng_sreal(rng[1])),
				.flags = PFLAG_MANUALANGLE,
				.layer = fvp->base.fakepos.blendfactor
					? LAYER_ENEMY_PARTICLE_BACKGROUND
					: LAYER_PARTICLE_MID,
				.scale = fvp->base.scale,
				.opacity = fvp->base.opacity,
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
	cmplx pos, const ItemCounts *item_drops, real hp,
	EnemyDrawFunc draw, EnemyDrawOrder draw_order, size_t drawdata_alloc
) {
	assert(drawdata_alloc > 0);
	Enemy *e = create_enemy(pos, hp, (EnemyVisual) { draw });
	e->ent.draw_layer = LAYER_ENEMY | ((drawlayer_low_t)draw_order << 8);

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
#define spawn(_pos, _items, _hp, _draw, _order, ...) ({ \
	auto _e = _spawn(_pos, _items, _hp, _draw, _order, sizeof(__VA_ARGS__)); \
	*(typeof(__VA_ARGS__)*)(_e->visual.drawdata) = __VA_ARGS__; \
	_e; \
});

static void swirl_draw(Enemy *e, EnemyDrawParams p) {
	BaseEnemyVisualParams *vp = e->visual.drawdata;
	float o = vp->opacity;
	float b = 1.0f - vp->fakepos.blendfactor;
	r_draw_sprite(&(SpriteParams) {
		.color = RGBA(o*b*b, o*b*b, o*b*b, o),
		.sprite_ptr = vp->spr,
		.shader_ptr = res_shader("sprite_particle"),
		.shader_params = &(ShaderCustomParams) { 1.0f },
		.pos.as_cmplx = visual_pos(&p, vp),
		.rotation.angle = p.time * 10 * DEG2RAD,
		.scale = { vp->scale, vp->scale },
	});
}

Enemy *(espawn_swirl)(cmplx pos, const ItemCounts *item_drops) {
	return spawn(pos, item_drops, ECLASS_HP_SWIRL, swirl_draw, ECLASS_ORDER_SWIRL,
		(BaseEnemyVisualParams) {
			.spr = res_sprite("enemy/swirl"),
			.scale = 1,
			.opacity = 1,
		}
	);
}

static Enemy *spawn_fairy(
	cmplx pos, const ItemCounts *item_drops, real hp, EnemyDrawOrder order, Animation *ani
) {
	return spawn(pos, item_drops, hp, anyfairy_draw, order, (FairyVisualParams) {
		.base = {
			.ani = ani,
			.scale = 1,
			.opacity = 1,
		},
		.summon.progress = 1,
	});
}

static Enemy *espawn_fairy_weak(
	cmplx pos, const ItemCounts *item_drops, Animation *fairy_ani, Sprite *circle_spr
) {
	auto e = spawn_fairy(pos, item_drops, ECLASS_HP_FAIRY, ECLASS_ORDER_FAIRY, fairy_ani);
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
	auto e = spawn_fairy(pos, item_drops,
		ECLASS_HP_BIG_FAIRY, ECLASS_ORDER_BIG_FAIRY, res_anim("enemy/bigfairy")
	);
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
	auto e = spawn_fairy(pos, item_drops,
		ECLASS_HP_HUGE_FAIRY, ECLASS_ORDER_HUGE_FAIRY, res_anim("enemy/hugefairy")
	);
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
	auto e = spawn_fairy(pos, item_drops,
		ECLASS_HP_SUPER_FAIRY, ECLASS_ORDER_SUPER_FAIRY, res_anim("enemy/superfairy")
	);
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

float ecls_anyenemy_set_scale(Enemy *e, float s) {
	BaseEnemyVisualParams *vp = e->visual.drawdata;
	float olds = vp->scale;
	vp->scale = s;
	return olds;
}

float ecls_anyenemy_get_scale(Enemy *e) {
	BaseEnemyVisualParams *vp = e->visual.drawdata;
	return vp->scale;
}

float ecls_anyenemy_set_opacity(Enemy *e, float o) {
	BaseEnemyVisualParams *vp = e->visual.drawdata;
	float oldo = vp->opacity;
	vp->opacity = o;
	return oldo;
}

float ecls_anyenemy_get_opacity(Enemy *e) {
	BaseEnemyVisualParams *vp = e->visual.drawdata;
	return vp->opacity;
}

static inline void spawnanim_set_flags(Enemy *e, EnemyFlag *tempflags) {
	EnemyFlag flags = EFLAG_NO_HIT | EFLAG_NO_HURT | EFLAG_INVULNERABLE;
	*tempflags = flags & ~e->flags;
	e->flags |= flags;
}

static inline void spawnanim_restore_flags(Enemy *e, EnemyFlag *tempflags) {
	e->flags &= ~*tempflags;
}

void ecls_anyenemy_fake3dmovein(
	Enemy *e,
	Camera3D *cam,
	vec3 initpos_3d,
	int duration
) {
	assert(duration > 0);

	BaseEnemyVisualParams *vp = e->visual.drawdata;
	vec3 initpos_projected;
	camera3d_project(cam, initpos_3d, initpos_projected);
	vp->fakepos.pos = CMPLXF(initpos_projected[0], initpos_projected[1]);

	EnemyFlag tempflags;
	spawnanim_set_flags(e, &tempflags);

	DrawLayer layer = e->ent.draw_layer;
	e->ent.draw_layer = LAYER_ENEMY_BACKGROUND | (layer & LAYER_LOW_MASK);

	for(int i = 1;;) {
		float f = i / (float)duration;
		vp->fakepos.blendfactor = (1.0 - glm_ease_sine_inout(f));
		vp->scale = glm_ease_sine_inout(f);
		vp->opacity = glm_ease_sine_out(f);

		if(i == duration) {
			break;
		}

		++i;
		YIELD;
	}

	spawnanim_restore_flags(e, &tempflags);
	e->ent.draw_layer = layer;
}

void ecls_anyfairy_summon(Enemy *e, int duration) {
	assert(duration > 0);

	FairyVisualParams *vp = e->visual.drawdata;

	EnemyFlag tempflags;
	spawnanim_set_flags(e, &tempflags);

	DrawLayer elayer = e->ent.draw_layer;
	e->ent.draw_layer = LAYER_NODRAW;
	vp->summon.progress = 0;
	vp->summon.cloak = 0.75;
	vp->summon.mask_ofs_bits.bits = rng_u32();

	Projectile *circle = NOT_NULL(ENT_UNBOX(vp->circle));
	Color circle_basecolor = circle->color;
	Color circle_spawncolor = *color_mul(
		COLOR_COPY(&circle_basecolor),
		RGBA(2, 2, 2, 0)
	);

	float fairy_delay = 0.1f;

	for(int i = 1;;) {
		float f = i / (float)duration;

		vp->base.opacity = glm_ease_quint_in(clamp(f * 2.0f, 0.0f, 1.0f));
		vp->base.scale = lerpf(3.0f, 1.0f, glm_ease_back_out( glm_ease_sine_inout(f)));

		if(f >= fairy_delay) {
			vp->summon.progress = glm_ease_sine_inout(
				(f - fairy_delay) / (1.0f - fairy_delay) * 0.875f);
			e->ent.draw_layer = elayer;
		}

		if((circle = ENT_UNBOX(vp->circle))) {
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
	}

	vp->summon.progress = 1;
	spawnanim_restore_flags(e, &tempflags);
}
