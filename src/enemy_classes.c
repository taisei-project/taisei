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
#define ECLASS_HP_FAIRY         500
#define ECLASS_HP_BIG_FAIRY     1500
#define ECLASS_HP_HUGE_FAIRY    5600
#define ECLASS_HP_SUPER_FAIRY   15000

TASK(fairy_circle, {
	BoxedEnemy e;
	Sprite *sprite;
	float spin_rate;
	float scale_base;
	float scale_osc_ampl;
	float scale_osc_freq;
	Color color;
}) {
	Projectile *p = TASK_BIND(PARTICLE(
		.sprite_ptr = ARGS.sprite,
		.color = &ARGS.color,
		.flags = PFLAG_NOMOVE | PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE | PFLAG_NOAUTOREMOVE,
		.layer = LAYER_PARTICLE_LOW,
	));

	float scale_osc_phase = 0.0f;

	for(Enemy *e; (e = ENT_UNBOX(ARGS.e)); YIELD) {
		p->pos = enemy_visual_pos(e);
		p->opacity = e->alpha;
		p->angle += ARGS.spin_rate;
		p->scale = (ARGS.scale_base + ARGS.scale_osc_ampl * sinf(scale_osc_phase)) * (1.0f + I);
		scale_osc_phase += ARGS.scale_osc_freq;
	}

	kill_projectile(p);
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

	for(;;WAIT(period)) {
		cmplx offset = rng_sreal() * 12;
		offset += rng_sreal() * 10 * I;

		PARTICLE(
			.sprite_ptr = spr,
			.pos = e->pos + offset,
			.color = &ARGS.color,
			.draw_rule = pdraw_timeout_scalefade(2+2*I, 0.5+2*I, 1, 0),
			.angle = M_PI/2,
			.timeout = 50,
			.move = move_linear((-50.0*I-offset)/50.0 + 0.65 * e->move.velocity),
			.flags = PFLAG_MANUALANGLE,
			.layer = LAYER_PARTICLE_MID,
		);
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

	for(;;WAIT(period)) {
		cmplx offset = rng_sreal() * 12;
		offset += rng_sreal() * 10 * I;

		PARTICLE(
			.sprite_ptr = spr,
			.pos = e->pos + offset,
			.color = &ARGS.color,
			.draw_rule = pdraw_timeout_scalefade_exp(0.1 * (1+I), 2 * (1+I), 1, 0, 2),
			.angle = rng_angle(),
			.timeout = 180,
			.move = move_linear(0.65 * e->move.velocity),
			.flags = PFLAG_MANUALANGLE,
			.layer = LAYER_PARTICLE_MID,
		);
	}
}

static void draw_fairy(Enemy *e, int t, Animation *ani) {
	const char *seqname = !e->moving ? "main" : (e->dir ? "left" : "right");
	Sprite *spr = animation_get_frame(ani, get_ani_sequence(ani, seqname), t);

	r_draw_sprite(&(SpriteParams) {
		.color = RGBA_MUL_ALPHA(1, 1, 1, e->alpha),
		.sprite_ptr = spr,
		.pos.as_cmplx = e->pos,
	});
}

static void visual_swirl(Enemy *e, int t, bool render) {
	if(render) {
		r_draw_sprite(&(SpriteParams) {
			.color = RGBA_MUL_ALPHA(1, 1, 1, e->alpha),
			.sprite = "enemy/swirl",
			.pos.as_cmplx = e->pos,
			.rotation.angle = t * 10 * DEG2RAD,
		});
	}
}

static void visual_fairy_blue(Enemy *e, int t, bool render) {
	if(render) {
		draw_fairy(e, t, res_anim("enemy/fairy_blue"));
	}
}

static void visual_fairy_red(Enemy *e, int t, bool render) {
	if(render) {
		draw_fairy(e, t, res_anim("enemy/fairy_red"));
	}
}

static void visual_big_fairy(Enemy *e, int t, bool render) {
	if(render) {
		draw_fairy(e, t, res_anim("enemy/bigfairy"));
	}
}

static void visual_huge_fairy(Enemy *e, int t, bool render) {
	if(render) {
		draw_fairy(e, t, res_anim("enemy/hugefairy"));
	}
}

static void visual_super_fairy(Enemy *e, int t, bool render) {
	if(render) {
		draw_fairy(e, t, res_anim("enemy/bigfairy"));
	}
}

// Deprecated declarations defined in enemy.h; TODO remove these later
void Swirl(Enemy *e, int t, bool r) { visual_swirl(e, t, r); }
void Fairy(Enemy *e, int t, bool r) { visual_fairy_blue(e, t, r); }
void BigFairy(Enemy *e, int t, bool r) { visual_big_fairy(e, t, r); }

static Enemy *spawn(cmplx pos, const ItemCounts *item_drops, real hp, EnemyVisualRule visual) {
	Enemy *e = create_enemy_p(&global.enemies, pos, hp, visual, NULL, 0, 0, 0, 0);

	if(item_drops) {
		INVOKE_TASK_WHEN(&e->events.killed, common_drop_items, &e->pos, *item_drops);
	}

	return e;
}

Enemy *(espawn_swirl)(cmplx pos, const ItemCounts *item_drops) {
	Enemy *e = spawn(pos, item_drops, ECLASS_HP_SWIRL, visual_swirl);
	return e;
}

Enemy *(espawn_fairy_blue)(cmplx pos, const ItemCounts *item_drops) {
	Enemy *e = spawn(pos, item_drops, ECLASS_HP_FAIRY, visual_fairy_blue);
	INVOKE_TASK(fairy_circle, ENT_BOX(e),
		.sprite = res_sprite("fairy_circle"),
		.spin_rate = 10 * DEG2RAD,
		.scale_base = 0.8f,
		.scale_osc_ampl = 1.0f / 6.0f,
		.scale_osc_freq = 0.1f,
		.color = *RGB(1, 1, 1)
	);
	return e;
}

Enemy *(espawn_fairy_red)(cmplx pos, const ItemCounts *item_drops) {
	Enemy *e = spawn(pos, item_drops, ECLASS_HP_FAIRY, visual_fairy_red);
	INVOKE_TASK(fairy_circle, ENT_BOX(e),
		.sprite = res_sprite("fairy_circle_red"),
		.spin_rate = 10 * DEG2RAD,
		.scale_base = 0.8f,
		.scale_osc_ampl = 1.0f / 6.0f,
		.scale_osc_freq = 0.1f,
		.color = *RGB(1, 1, 1)
	);
	return e;
}

Enemy *(espawn_big_fairy)(cmplx pos, const ItemCounts *item_drops) {
	Enemy *e = spawn(pos, item_drops, ECLASS_HP_BIG_FAIRY, visual_big_fairy);
	INVOKE_TASK(fairy_flame_emitter, ENT_BOX(e),
		.period = 5,
		.color = *RGBA(0.0, 0.2, 0.3, 0.0)
	);
	INVOKE_TASK(fairy_circle, ENT_BOX(e),
		.sprite = res_sprite("fairy_circle_big"),
		.spin_rate = 5 * DEG2RAD,
		.scale_base = 0.75f,
		.scale_osc_ampl = 0.15f,
		.scale_osc_freq = 1.0f / 15.0f,
		.color = *RGBA(1, 1, 1, 0.8)
	);
	return e;
}

Enemy *(espawn_huge_fairy)(cmplx pos, const ItemCounts *item_drops) {
	Enemy *e = spawn(pos, item_drops, ECLASS_HP_HUGE_FAIRY, visual_huge_fairy);
	INVOKE_TASK(fairy_flame_emitter, ENT_BOX(e),
		.period = 6,
		.color = *RGBA(0.0, 0.2, 0.3, 0.0)
	);
	INVOKE_TASK(fairy_flame_emitter, ENT_BOX(e),
		.period = 6,
		.color = *RGBA(0.3, 0.0, 0.2, 0.0)
	);
	INVOKE_TASK(fairy_circle, ENT_BOX(e),
		.sprite = res_sprite("fairy_circle_big"),
		.spin_rate = 5 * DEG2RAD,
		.scale_base = 0.85f,
		.scale_osc_ampl = 0.1f,
		.scale_osc_freq = 1.0f / 15.0f,
		.color = *RGBA(1, 1, 1, 0.95)
	);
	return e;
}

Enemy *(espawn_super_fairy)(cmplx pos, const ItemCounts *item_drops) {
	Enemy *e = spawn(pos, item_drops, ECLASS_HP_SUPER_FAIRY, visual_super_fairy);
	INVOKE_TASK(fairy_flame_emitter, ENT_BOX(e),
		.period = 5,
		.color = *RGBA(0.2, 0.0, 0.3, 0.0)
	);
	INVOKE_TASK(fairy_stardust_emitter, ENT_BOX(e),
		.period = 15,
		.color = *RGBA(0.0, 0.0, 0.0, 0.8)
	);
	INVOKE_TASK(fairy_circle, ENT_BOX(e),
		.sprite = res_sprite("fairy_circle_big_and_mean"),
		.spin_rate = 5 * DEG2RAD,
		.scale_base = 0.9f,
		.scale_osc_ampl = 0.1f,
		.scale_osc_freq = 1.0f / 15.0f,
		.color = *RGBA(1, 1, 1, 0.6)
	);
	return e;
}
