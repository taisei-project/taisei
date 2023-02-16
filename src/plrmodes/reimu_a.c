/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "global.h"
#include "plrmodes.h"
#include "reimu.h"
#include "stagedraw.h"
#include "common_tasks.h"

#define SHOT_FORWARD_DMG 50
#define SHOT_FORWARD_DELAY 3
#define SHOT_VOLLEY_DMG 35
#define SHOT_VOLLEY_DELAY 16
#define SHOT_SLAVE_HOMING_DMG 175
#define SHOT_SLAVE_HOMING_DELAY 21
#define SHOT_SLAVE_NEEDLE_DMG 64
#define SHOT_SLAVE_NEEDLE_DELAY 3

#define ORB_RETRACT_TIME 4

typedef struct ReimuAController {
	Player *plr;
	int last_homing_fire_time;
	int last_needle_fire_time;
	COEVENTS_ARRAY(
		slaves_expired
	) events;
} ReimuAController;

DEFINE_ENTITY_TYPE(ReimuASlave, {
	Sprite *sprite;
	ShaderProgram *shader;
	cmplx pos;
	Color color;
	uint alive;
});

static void reimu_spirit_preload(void) {
	const int flags = RESF_DEFAULT;

	preload_resources(RES_SPRITE, flags,
		"yinyang",
		"proj/ofuda",
		"proj/needle",
		"proj/glowball",
		"proj/hakurei_seal",
		"part/ofuda_glow",
		"part/fantasyseal_impact",
	NULL);

	preload_resources(RES_TEXTURE, flags,
		"runes",
	NULL);

	preload_resources(RES_SHADER_PROGRAM, flags,
		"sprite_yinyang",
		"reimu_bomb_bg",
	NULL);

	preload_resources(RES_SFX, flags | RESF_OPTIONAL,
		"boom",
		"bomb_reimu_a",
		"bomb_marisa_b",
	NULL);
}

TASK(reimu_spirit_needle, { cmplx pos; cmplx vel; real damage; ShaderProgram *shader; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_needle,
		.pos = ARGS.pos,
		.color = RGBA(0.5, 0.5, 0.5, 0.5),
		.move = move_linear(ARGS.vel),
		.type = PROJ_PLAYER,
		.damage_type = DMG_PLAYER_SHOT,
		.damage = ARGS.damage,
		.shader_ptr = ARGS.shader,
	));

	Color trail_color = p->color;
	color_mul(&trail_color, RGBA_MUL_ALPHA(0.75, 0.5, 1, 0.5));
	color_mul_scalar(&trail_color, 0.6);
	trail_color .a = 0;

	MoveParams trail_move = move_linear(p->move.velocity * 0.8);
	ProjDrawRule trail_draw = pdraw_timeout_scalefade(0, 2, 1, 0);

	for(;;) {
		YIELD;

		PARTICLE(
			.sprite_ptr = p->sprite,
			.color = &trail_color,
			.timeout = 12,
			.pos = p->pos,
			.move = trail_move,
			.draw_rule = trail_draw,
			.layer = LAYER_PARTICLE_LOW,
			.flags = PFLAG_NOREFLECT,
		);
	}
}

#define REIMU_SPIRIT_HOMING_SCALE 0.75

static Projectile *reimu_spirit_spawn_ofuda_particle(Projectile *p, int t, real vfactor) {
	Color *c = HSLA_MUL_ALPHA(t * 0.1, 0.6, 0.7, 0.3);
	c->a = 0;

	return PARTICLE(
		.sprite = "ofuda_glow",
		// .color = rgba(0.5 + 0.5 + psin(global.frames * 0.75), psin(t*0.5), 1, 0.5),
		.color = c,
		.timeout = 12,
		.pos = p->pos,
		.angle = p->angle,
		.move = move_linear(p->move.velocity * rng_range(0.6, 1.0) * vfactor),
		.draw_rule = pdraw_timeout_scalefade(1, 1.5, 1, 0),
		.layer = LAYER_PARTICLE_LOW,
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE,
		.scale = REIMU_SPIRIT_HOMING_SCALE,
	);
}

TASK(reimu_spirit_homing_impact, { BoxedProjectile p; }) {
	Projectile *ref = NOT_NULL(ENT_UNBOX(ARGS.p));

	Projectile *p = TASK_BIND(PARTICLE(
		.proto = ref->proto,
		.color = &ref->color,
		.timeout = 32,
		.pos = ref->pos,
		.angle = ref->angle,
		.angle_delta = 0.2,
		.draw_rule = pdraw_timeout_scalefade(1, 1.5, 1, 0),
		.layer = LAYER_PARTICLE_HIGH,
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE,
		.scale = REIMU_SPIRIT_HOMING_SCALE,
	));

	for(int t = global.frames - ref->birthtime;; ++t) {
		Projectile *trail = reimu_spirit_spawn_ofuda_particle(p, t, 0);
		trail->timeout = 6;
		trail->angle = p->angle;
		trail->ent.draw_layer = LAYER_PLAYER_FOCUS - 1; // TODO: add a layer for "super high" particles?
		YIELD;
	}
}

static inline real reimu_spirit_homing_aimfactor(real t, real maxt) {
	real q = pow(t / maxt, 3);
	return 4 * q * (1 - q);
}

TASK(reimu_spirit_homing, { cmplx pos; cmplx vel; real damage; ShaderProgram *shader; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_ofuda,
		.pos = ARGS.pos,
		.color = RGBA(0.7, 0.63, 0.665, 0.7),
		.move = move_linear(ARGS.vel),
		.type = PROJ_PLAYER,
		.damage_type = DMG_PLAYER_SHOT,
		.damage = ARGS.damage,
		.shader_ptr = ARGS.shader,
		.scale = REIMU_SPIRIT_HOMING_SCALE,
		.flags = PFLAG_NOCOLLISIONEFFECT | PFLAG_NOAUTOREMOVE,
	));

	INVOKE_TASK_WHEN(&p->events.killed, reimu_spirit_homing_impact, ENT_BOX(p));

	cmplx target = p->pos + p->move.velocity * hypot(VIEWPORT_W, VIEWPORT_H);
	real speed = cabs(p->move.velocity);
	real homing_time = 60;

	for(real t = 0; t < homing_time; ++t) {
		target = plrutil_homing_target(p->pos, target);
		cmplx aim = cnormalize(target - p->pos);
		real s = speed * (0.5 + 0.5 * pow((t + 1) / homing_time, 2));
		aim *= s * 0.25 * reimu_spirit_homing_aimfactor(t, homing_time);
		p->move.velocity = s * cnormalize(p->move.velocity + aim);
		reimu_spirit_spawn_ofuda_particle(p, t, 0.25);
		YIELD;
	}

	p->flags &= ~PFLAG_NOAUTOREMOVE;
}

static Color *reimu_spirit_orb_color(Color *c, int i) {
	*c = *RGBA((0.2 + (i==0))/1.2, (0.2 + (i==1))/1.2, (0.2 + 1.5*(i==2))/1.2, 0.0);
	return c;
}

static void reimu_spirit_bomb_impact_balls(cmplx pos, int count) {
	real offset = rng_real();

	for(int i = 0; i < count; i++) {
		PARTICLE(
			.sprite_ptr = res_sprite("proj/glowball"),
			.shader = "sprite_bullet",
			.color = HSLA(3 * (float)i / count + offset, 1, 0.5, 0),
			.timeout = 60,
			.pos = pos,
			.angle = rng_angle(),
			.move = move_linear(cdir(2 * M_PI / count * (i + offset)) * 15),
			.draw_rule = pdraw_timeout_fade(1, 0),
			.layer = LAYER_BOSS,
			.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE,
		);
	}
}

TASK(reimu_spirit_bomb_orb_impact, { BoxedProjectile orb; }) {
	cmplx pos = NOT_NULL(ENT_UNBOX(ARGS.orb))->pos;

	play_sfx("boom");
	play_sfx("spellend");

	stage_shake_view(200);

	real damage = 2000;
	real range = 300;

	ent_area_damage(pos, range, &(DamageInfo){damage, DMG_PLAYER_BOMB}, NULL, NULL);
	stage_clear_hazards_at(pos, range, CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW);

	reimu_spirit_bomb_impact_balls(pos, 21);

	int num_impacts = 3;
	int t = global.frames;
	BoxedProjectileArray impact_effects = ENT_ARRAY(Projectile, 3);
	RNG_ARRAY(rand, num_impacts);
	Color base_colors[3];

	for(int i = 0; i < 3; ++i) {
		base_colors[i] = *reimu_spirit_orb_color(&(Color){0}, i);

		PARTICLE(
			.sprite = "blast",
			.color = color_mul_scalar(COLOR_COPY(&base_colors[i]), 2),
			.pos = pos + 30 * cexp(I*2*M_PI/num_impacts*(i+t*0.1)),
			.timeout = 40,
			.draw_rule = pdraw_timeout_scalefade(0, 7.5, 1, 0),
			.layer = LAYER_BOSS + 2,
			.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		);

		ENT_ARRAY_ADD(&impact_effects, PARTICLE(
			.sprite = "fantasyseal_impact",
			.color = reimu_spirit_orb_color(&(Color){0}, i),
			.pos = pos + 2 * cexp(I*2*M_PI/num_impacts*(i+t*0.1)),
			.timeout = 120,
			.layer = LAYER_BOSS + 1,
			.angle = -M_PI/2,
			.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE,
		));
	}

	for(;;) {
		int live = 0;

		ENT_ARRAY_FOREACH_COUNTER(&impact_effects, int i, Projectile *p, {
			float t = (global.frames - p->birthtime) / p->timeout;
			float attack = fmin(1, vrng_f32_range(rand[i], 7, 12) * t);
			float decay = t;

			Color c = base_colors[i];
			color_lerp(&c, RGBA(0.2, 0.1, 0, 1.0), decay);
			color_mul_scalar(&c, powf(1.0f - decay, 2.0f) * 0.75f);
			p->color = c;
			p->scale = (0.75f + 0.25f / (powf(decay, 3.0f) + 1.0f)) + sqrtf(5.0f * (1.0f - attack));

			++live;
		});

		if(!live) {
			break;
		}

		YIELD;
	}
}

TASK(reimu_spirit_bomb_orb_visual_kill, { BoxedProjectileArray components; }) {
	ENT_ARRAY_FOREACH(&ARGS.components, Projectile *p, {
		kill_projectile(p);
	});
}

TASK(reimu_spirit_bomb_orb_visual, { BoxedProjectile orb; }) {
	Projectile *orb = TASK_BIND(ARGS.orb);
	DECLARE_ENT_ARRAY(Projectile, components, 3);

	Sprite *glowball = res_sprite("proj/glowball");
	ShaderProgram *shader = res_shader("sprite_bullet");

	for(int i = 0; i < components.capacity; ++i) {
		ENT_ARRAY_ADD(&components, PARTICLE(
			.sprite_ptr = glowball,
			.shader_ptr = shader,
			.color = reimu_spirit_orb_color(&(Color){0}, i),
			.opacity = 0.7,
			.layer = LAYER_PLAYER_FOCUS - 1,
			.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		));
	}

	INVOKE_TASK_AFTER(&orb->events.killed, reimu_spirit_bomb_orb_visual_kill, components);
	CANCEL_TASK_AFTER(&orb->events.killed, THIS_TASK);

	for(;;) {
		cmplx pos = orb->pos;

		ENT_ARRAY_FOREACH_COUNTER(&components, int i, Projectile *p, {
			real t = global.frames - p->birthtime;
			cmplxf offset = (10 + pow(t, 0.5)) * cdir(2.0 * M_PI / 3*i + sqrt(1 + t * t / 300));
			p->pos = pos + offset;
		});

		YIELD;
	}
}

TASK(reimu_spirit_bomb_orb, { BoxedPlayer plr; int index; real angle; }) {
	int index = ARGS.index;

	Player *plr = NOT_NULL(ENT_UNBOX(ARGS.plr));
	Projectile *orb = TASK_BIND(PROJECTILE(
		.pos = plr->pos,
		.timeout = 160 + 20 * index,
		.type = PROJ_PLAYER,
		.damage = 1000,
		.damage_type = DMG_PLAYER_BOMB,
		.size = 10 * (1+I),
		.layer = LAYER_NODRAW,
		.flags = PFLAG_NOREFLECT | PFLAG_NOCOLLISION | PFLAG_NOMOVE | PFLAG_MANUALANGLE | PFLAG_NOAUTOREMOVE,
	));

	BoxedProjectile b_orb = ENT_BOX(orb);

	INVOKE_TASK(reimu_spirit_bomb_orb_visual, b_orb);
	INVOKE_TASK_WHEN(&orb->events.killed, reimu_spirit_bomb_orb_impact, b_orb);
	CANCEL_TASK_AFTER(&orb->events.killed, THIS_TASK);

	int circletime = 60 + 20 * index;
	cmplx target_homing = plr->pos;
	cmplx dir = cdir(ARGS.angle);
	cmplx vel = 0;

	for(int t = 0;; ++t) {
		if(!player_is_bomb_active(plr)) {
			kill_projectile(orb);
			return;
		}

		if(t == circletime) {
			target_homing = global.plr.pos - 256*I;
			orb->flags &= ~PFLAG_NOCOLLISION;
			play_sfx("redirect");
		}

		cmplx target_circle = plr->pos + 10 * sqrt(t) * dir * (1 + 0.1 * sin(0.2 * t));
		dir *= cdir(0.12);

		real circlestrength = 1.0 / (1 + exp(t - circletime));

		target_homing = plrutil_homing_target(orb->pos, target_homing);
		cmplx homing = target_homing - orb->pos;
		cmplx v = 0.3 * (circlestrength * (target_circle - orb->pos) + 0.2 * (1 - circlestrength) * (homing + 2*homing/(cabs(homing)+0.01)));
		vel += (v - vel) * 0.1;
		orb->pos += vel;

		for(int i = 0; i < 3; i++) {
			cmplx trail_pos = orb->pos + 10 * cdir(2*M_PI/3*(i+t*0.1));
			cmplx trail_vel = global.plr.pos - trail_pos;
			trail_vel *= 3 * circlestrength / cabs(trail_vel);

			PARTICLE(
				.sprite_ptr = res_sprite("part/stain"),
				// .color = reimu_spirit_orb_color(&(Color){0}, i),
				.color = HSLA(t/orb->timeout, 0.3, 0.3, 0.0),
				.pos = trail_pos,
				.angle = rng_angle(),
				.angle_delta = -0.05,
				.timeout = 30,
				.draw_rule = pdraw_timeout_scalefade(0.4, 0, 1, 0),
				.move = move_linear(trail_vel),
				.flags = PFLAG_NOREFLECT | PFLAG_MANUALANGLE,
			);
		}

		YIELD;
	}
}

TASK(reimu_spirit_bomb_background, { ReimuAController *ctrl; }) {
	ReimuAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	CoEvent *draw_event = &stage_get_draw_events()->background_drawn;

	for(;;) {
		WAIT_EVENT_OR_DIE(draw_event);

		float t = player_get_bomb_progress(plr);

		if(t >= 1) {
			break;
		}

		float alpha = 0.0f;

		if(t > 0) {
			alpha = fminf(1.0f, 10.0f * t);
		}

		if(t > 0.7) {
			alpha *= 1.0f - powf((t - 0.7f) / 0.3f, 4.0f);
		}

		reimu_common_bomb_bg(plr, alpha);
		colorfill(0.0f, 0.05f * alpha, 0.1f * alpha, alpha * 0.5f);
	}
}

TASK(reimu_spirit_bomb, { ReimuAController *ctrl; }) {
	ReimuAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	INVOKE_SUBTASK(reimu_spirit_bomb_background, ctrl);
	YIELD;

	const int orb_count = 6;
	for(int i = 0; i < orb_count; i++) {
		INVOKE_TASK_DELAYED(1, reimu_spirit_bomb_orb, ENT_BOX(plr), i, M_TAU/orb_count*i);
	}

	// keep bg renderer alive
	WAIT(plr->bomb_endtime - global.frames);
}

TASK(reimu_spirit_bomb_handler, { ReimuAController *ctrl; }) {
	ReimuAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	BoxedTask bomb_task = { 0 };

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.bomb_used);
		play_sfx("bomb_reimu_a");
		play_sfx("bomb_marisa_b");
		CANCEL_TASK(bomb_task);
		bomb_task = cotask_box(INVOKE_SUBTASK(reimu_spirit_bomb, ctrl));
	}
}

TASK(reimu_spirit_ofuda, { cmplx pos; cmplx vel; real damage; ShaderProgram *shader; }) {
	Projectile *ofuda = TASK_BIND(PROJECTILE(
		.proto = pp_ofuda,
		.pos = ARGS.pos,
		.color = RGBA_MUL_ALPHA(1, 1, 1, 0.5),
		.move = move_linear(ARGS.vel),
		.type = PROJ_PLAYER,
		.damage = ARGS.damage,
		.shader_ptr = ARGS.shader,
	));

	for(;;YIELD) {
		reimu_common_ofuda_swawn_trail(ofuda);
	}
}

static void reimu_spirit_draw_slave(EntityInterface *ent) {
	ReimuASlave *slave = ENT_CAST(ent, ReimuASlave);
	r_draw_sprite(&(SpriteParams) {
		.color = &slave->color,
		.pos.as_cmplx = slave->pos,
		.rotation.angle = global.frames * -6 * DEG2RAD,
		.shader_ptr = slave->shader,
		.sprite_ptr = slave->sprite,
	});
}

static ReimuASlave *reimu_spirit_create_slave(ReimuAController *ctrl, const Color *color) {
	Player *plr = ctrl->plr;
	ReimuASlave *slave = TASK_HOST_ENT(ReimuASlave);
	slave->sprite = res_sprite("yinyang");
	slave->shader = res_shader("sprite_yinyang");
	slave->pos = plr->pos;
	slave->color = *color;
	slave->ent.draw_layer = LAYER_PLAYER_SLAVE;
	slave->ent.draw_func = reimu_spirit_draw_slave;
	slave->alive = 1;

	INVOKE_SUBTASK_WHEN(&ctrl->events.slaves_expired, common_set_bitflags,
		.pflags = &slave->alive,
		.mask = 0, .set = 0
	);

	return slave;
}

TASK(reimu_slave_shot_particles, {
	BoxedReimuASlave slave;
	int num;
	cmplx dir;
	Color *color0;
	Color *color1;
	Sprite *sprite;
}) {
	ReimuASlave *slave = TASK_BIND(ARGS.slave);
	Color color0 = *ARGS.color0;
	Color color1 = *ARGS.color1;
	int num = ARGS.num;
	Sprite *sprite = ARGS.sprite;
	ProjDrawRule draw = pdraw_timeout_scalefade(0.25, 0, 1, 0);
	cmplx dir = ARGS.dir;

	for(int i = 0; i < num; ++i) {
		MoveParams move = move_linear(dir * rng_range(5, 10));
		real timeout = rng_range(8, 10);
		real angle = rng_angle();

		PARTICLE(
			.angle = angle,
			.color = COLOR_COPY(color_lerp(&color0, &color1, rng_f32())),
			.draw_rule = draw,
			.flags = PFLAG_NOREFLECT | PFLAG_MANUALANGLE,
			.move = move,
			.pos = slave->pos + move.velocity * 0.5,
			.sprite_ptr = sprite,
			.timeout = timeout,
		);
		YIELD;
	}
}

TASK(reimu_spirit_slave_needle_shot, {
	ReimuAController *ctrl;
	BoxedReimuASlave slave;
}) {
	ReimuAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	ReimuASlave *slave = TASK_BIND(ARGS.slave);
	WAIT(imax(0, SHOT_SLAVE_HOMING_DELAY - (global.frames - ctrl->last_needle_fire_time)));

	ShaderProgram *shader = res_shader("sprite_particle");
	Sprite *particle_spr = res_sprite("part/stain");

	real damage = SHOT_SLAVE_NEEDLE_DMG;
	real delay = SHOT_SLAVE_NEEDLE_DELAY;

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);

		INVOKE_SUBTASK(reimu_slave_shot_particles,
			.slave = ENT_BOX(slave),
			.color0 = RGBA(0.5, 0, 0, 0),
			.color1 = RGBA(0.5, 0.25, 0, 0),
			.num = delay,
			.dir = -I,
			.sprite = particle_spr
		);

		INVOKE_TASK(reimu_spirit_needle,
			.pos = slave->pos - 25.0*I,
			.vel = -20*I,
			.damage = damage,
			.shader = shader
		);

		ctrl->last_needle_fire_time = global.frames;
		ctrl->last_homing_fire_time = imax(ctrl->last_homing_fire_time, global.frames - SHOT_SLAVE_HOMING_DELAY / 2);

		WAIT(delay);
	}
}

TASK(reimu_spirit_slave_needle, {
	ReimuAController *ctrl;
	real distance;
	real rotation;
	real rotation_offset;
}) {
	ReimuAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	ReimuASlave *slave = reimu_spirit_create_slave(ctrl, RGB(1.0, 0.8, 0.8));

	INVOKE_SUBTASK(reimu_spirit_slave_needle_shot, ctrl, ENT_BOX(slave));

	real dist = ARGS.distance;
	cmplx offset = dist * cdir(ARGS.rotation_offset);
	cmplx rot = cdir(ARGS.rotation);

	real target_speed = 0.005 * dist;
	MoveParams move = move_towards(plr->pos + offset, 0);

	do {
		move.attraction = approach(creal(move.attraction), target_speed, target_speed / 12.0);
		move.attraction_point = plr->pos + offset;
		offset *= rot;
		move_update(&slave->pos, &move);
		YIELD;
	} while(slave->alive);

	plrutil_slave_retract(ENT_BOX(plr), &slave->pos, ORB_RETRACT_TIME);
}

TASK(reimu_spirit_slave_homing_shot, {
	ReimuAController *ctrl;
	BoxedReimuASlave slave;
	cmplx vel;
}) {
	ReimuAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	ReimuASlave *slave = TASK_BIND(ARGS.slave);
	WAIT(imax(0, SHOT_SLAVE_HOMING_DELAY - (global.frames - ctrl->last_homing_fire_time)));

	ShaderProgram *shader = res_shader("sprite_particle");
	Sprite *particle_spr = res_sprite("part/stain");
	cmplx vel = ARGS.vel;
	cmplx pdir = cnormalize(vel);

	real damage = SHOT_SLAVE_HOMING_DMG;
	int delay = SHOT_SLAVE_HOMING_DELAY;

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);

		INVOKE_SUBTASK(reimu_slave_shot_particles,
			.slave = ENT_BOX(slave),
			.color0 = RGBA(0.5, 0.125, 0, 0),
			.color1 = RGBA(0.5, 0.125, 0.25, 0),
			.num = delay,
			.dir = pdir,
			.sprite = particle_spr
		);

		INVOKE_TASK(reimu_spirit_homing,
			.pos = slave->pos,
			.vel = vel,
			.damage = damage,
			.shader = shader
		);

		ctrl->last_homing_fire_time = global.frames;

		WAIT(delay);
	}
}

TASK(reimu_spirit_slave_homing, {
	ReimuAController *ctrl;
	cmplx offset;
	cmplx shot_vel;
}) {
	ReimuAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	ReimuASlave *slave = reimu_spirit_create_slave(ctrl, RGB(0.95, 0.75, 1.0));

	INVOKE_SUBTASK(reimu_spirit_slave_homing_shot, ctrl, ENT_BOX(slave), ARGS.shot_vel);

	cmplx offset = ARGS.offset;
	real target_speed = 0.005 * cabs(offset);
	MoveParams move = move_towards(plr->pos + offset, 0);

	do {
		move.attraction = approach(creal(move.attraction), target_speed, target_speed / 12.0);
		move.attraction_point = plr->pos + offset;
		move_update(&slave->pos, &move);
		YIELD;
	} while(slave->alive);

	plrutil_slave_retract(ENT_BOX(plr), &slave->pos, ORB_RETRACT_TIME);
}

static void reimu_spirit_spawn_slaves_unfocused(ReimuAController *ctrl, int power_rank) {
	cmplx sv = -8 * I;

	switch(power_rank) {
		case 0:
			break;
		case 1:
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, 50*I, sv);
			break;
		case 2:
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, +40, sv * cdir(+M_PI/24));
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, -40, sv * cdir(-M_PI/24));
			break;
		case 3:
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, 50*I, sv);
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, +40, sv * cdir(+M_PI/24));
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, -40, sv * cdir(-M_PI/24));
			break;
		case 4:
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, +40, sv * cdir(+M_PI/24));
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, -40, sv * cdir(-M_PI/24));
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, +80, sv * cdir(+M_PI/16));
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, -80, sv * cdir(-M_PI/16));
			break;
		default:
			UNREACHABLE;
	}
}

static void reimu_spirit_spawn_slaves_focused(ReimuAController *ctrl, int power_rank) {
	switch(power_rank) {
		case 0:
			break;
		case 1:
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 52, 0.12, 0);
			break;
		case 2:
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 52, 0.12, 0);
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 52, 0.12, M_TAU/2);
			break;
		case 3:
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 52, 0.12, 0*M_TAU/3);
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 52, 0.12, 1*M_TAU/3);
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 52, 0.12, 2*M_TAU/3);
			break;
		case 4:
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 52, 0.12, 0);
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 52, 0.12, M_PI);
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 78, -0.06, 0);
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 78, -0.06, M_PI);
			break;
		default:
			UNREACHABLE;
	}
}

static void reimu_spirit_kill_slaves(ReimuAController *ctrl) {
	coevent_signal(&ctrl->events.slaves_expired);
}

static void reimu_spirit_respawn_slaves(ReimuAController *ctrl) {
	Player *plr = ctrl->plr;
	int power_rank = player_get_effective_power(plr) / 100;

	reimu_spirit_kill_slaves(ctrl);

	if(plr->inputflags & INFLAG_FOCUS) {
		reimu_spirit_spawn_slaves_focused(ctrl, power_rank);
	} else {
		reimu_spirit_spawn_slaves_unfocused(ctrl, power_rank);
	}
}

TASK(reimu_spirit_focus_handler, { ReimuAController *ctrl; }) {
	ReimuAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	reimu_spirit_respawn_slaves(ctrl);

	uint prev_inputflags = 0;

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.inputflags_changed);

		// focus state changed?
		while((prev_inputflags ^ plr->inputflags) & INFLAG_FOCUS) {
			reimu_spirit_kill_slaves(ctrl);
			WAIT(ORB_RETRACT_TIME * 2);

			// important to record these at the time of respawning
			prev_inputflags = plr->inputflags;
			reimu_spirit_respawn_slaves(ctrl);

			// "shift-spam" protection - minimum time until next respawn
			WAIT(ORB_RETRACT_TIME * 2);
		}
	}
}

TASK(reimu_spirit_power_handler, { ReimuAController *ctrl; }) {
	ReimuAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	int old_power = player_get_effective_power(plr) / 100;

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.effective_power_changed);
		int new_power = player_get_effective_power(plr) / 100;
		if(old_power != new_power) {
			reimu_spirit_respawn_slaves(ctrl);
			old_power = new_power;
		}
	}
}

TASK(reimu_spirit_shot_forward, { ReimuAController *ctrl; }) {
	ReimuAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	real dir = 1;
	ShaderProgram *shader = res_shader("sprite_particle");

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);
		play_sfx_loop("generic_shot");
		INVOKE_TASK(reimu_spirit_ofuda,
			.pos = plr->pos + 10 * dir - 15.0*I,
			.vel = -20*I,
			.damage = SHOT_FORWARD_DMG,
			.shader = shader
		);
		dir = -dir;
		WAIT(SHOT_FORWARD_DELAY);
	}
}

TASK(reimu_spirit_shot_volley_bullet, { Player *plr; cmplx offset; cmplx vel; real damage; ShaderProgram *shader; }) {
	play_sfx_loop("generic_shot");

	PROJECTILE(
		.proto = pp_hakurei_seal,
		.pos = ARGS.plr->pos + ARGS.offset,
		.color = color_mul_scalar(RGBA(1, 1, 1, 0.5), 0.7),
		.move = move_linear(ARGS.vel),
		.type = PROJ_PLAYER,
		.damage = ARGS.damage,
		.shader_ptr = ARGS.shader,
	);
}

TASK(reimu_spirit_shot_volley, { ReimuAController *ctrl; }) {
	Player *plr = ARGS.ctrl->plr;
	ShaderProgram *shader = res_shader("sprite_particle");

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);

		int power_rank = player_get_effective_power(plr) / 100;
		real damage = SHOT_VOLLEY_DMG;

		for(int pwr = 0; pwr <= power_rank; ++pwr) {
			int delay = 5 * pwr;
			real spread = M_PI/32 * (1 + 0.35 * pwr);

			INVOKE_SUBTASK_DELAYED(delay, reimu_spirit_shot_volley_bullet,
				.plr = plr,
				.offset = -I + 5,
				.vel = -18 * I * cdir(spread),
				.damage = damage,
				.shader = shader
			);

			INVOKE_SUBTASK_DELAYED(delay, reimu_spirit_shot_volley_bullet,
				.plr = plr,
				.offset = -I - 5,
				.vel = -18 * I * cdir(-spread),
				.damage = damage,
				.shader = shader
			);
		}

		WAIT(SHOT_VOLLEY_DELAY);
	}
}

TASK(reimu_spirit_controller, { BoxedPlayer plr; }) {
	ReimuAController *ctrl = TASK_MALLOC(sizeof(ReimuAController));
	ctrl->plr = TASK_BIND(ARGS.plr);
	TASK_HOST_EVENTS(ctrl->events);

	INVOKE_SUBTASK(reimu_spirit_focus_handler, ctrl);
	INVOKE_SUBTASK(reimu_spirit_power_handler, ctrl);
	INVOKE_SUBTASK(reimu_spirit_shot_forward, ctrl);
	INVOKE_SUBTASK(reimu_spirit_shot_volley, ctrl);
	INVOKE_SUBTASK(reimu_spirit_bomb_handler, ctrl);

	STALL;
}

static void reimu_spirit_init(Player *plr) {
	reimu_common_bomb_buffer_init();
	INVOKE_TASK(reimu_spirit_controller, ENT_BOX(plr));
}

static double reimu_spirit_property(Player *plr, PlrProperty prop) {
	real base_value = reimu_common_property(plr, prop);

	switch(prop) {
		// fallthrough
		default: {
			return base_value;
		}
	}
}

PlayerMode plrmode_reimu_a = {
	.name = "Yōkai Buster",
	.description = "The tried-and-true: homing amulets and sharp needles. You don’t have the luxury of time to think about aiming.",
	.spellcard_name = "Spirit Sign “Fantasy Seal —Burst—”",
	.character = &character_reimu,
	.dialog = &dialog_tasks_reimu,
	.shot_mode = PLR_SHOT_REIMU_SPIRIT,
	.procs = {
		.property = reimu_spirit_property,
		.init = reimu_spirit_init,
		.preload = reimu_spirit_preload,
	},
};
