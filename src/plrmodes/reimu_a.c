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

#define ORB_RETRACT_TIME 4

typedef struct ReimuAController {
	Player *plr;
	COEVENTS_ARRAY(
		slaves_expired
	) events;
} ReimuAController;

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
	Projectile *p = TASK_BIND_UNBOXED(PROJECTILE(
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

static Projectile *reimu_spirit_spawn_ofuda_particle(Projectile *p, int t, double vfactor) {
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
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		.scale = REIMU_SPIRIT_HOMING_SCALE,
	);
}

TASK(reimu_spirit_homing_impact, { BoxedProjectile p; }) {
	Projectile *ref = NOT_NULL(ENT_UNBOX(ARGS.p));

	Projectile *p = TASK_BIND_UNBOXED(PARTICLE(
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
	Projectile *p = TASK_BIND_UNBOXED(PROJECTILE(
		.proto = pp_ofuda,
		.pos = ARGS.pos,
		.color = RGBA(0.7, 0.63, 0.665, 0.7),
		.move = move_linear(ARGS.vel),
		.type = PROJ_PLAYER,
		.damage_type = DMG_PLAYER_SHOT,
		.damage = ARGS.damage,
		.shader_ptr = ARGS.shader,
		.scale = REIMU_SPIRIT_HOMING_SCALE,
		.flags = PFLAG_NOCOLLISIONEFFECT,
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
}

static Color *reimu_spirit_orb_color(Color *c, int i) {
	*c = *RGBA((0.2 + (i==0))/1.2, (0.2 + (i==1))/1.2, (0.2 + 1.5*(i==2))/1.2, 0.0);
	return c;
}

static void reimu_spirit_bomb_impact_balls(cmplx pos, int count) {
	real offset = rng_real();

	for(int i = 0; i < count; i++) {
		PARTICLE(
			.sprite_ptr = get_sprite("proj/glowball"),
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
	cmplx pos = ENT_UNBOX(ARGS.orb)->pos;

	play_sound("boom");
	play_sound("spellend");

	global.shake_view = 20;
	global.shake_view_fade = 0.6;

	double damage = 2000;
	double range = 300;

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
			.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		));
	}

	for(;;) {
		int live = 0;

		ENT_ARRAY_FOREACH_COUNTER(&impact_effects, int i, Projectile *p, {
			float t = (global.frames - p->birthtime) / p->timeout;
			float attack = min(1, vrng_range(rand[i], 7, 12) * t);
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

	Sprite *glowball = get_sprite("proj/glowball");
	ShaderProgram *shader = r_shader_get("sprite_bullet");

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
			cmplx32 offset = (10 + pow(t, 0.5)) * cdir(2.0 * M_PI / 3*i + sqrt(1 + t * t / 300));
			p->pos = pos + offset;
		});

		YIELD;
	}
}

TASK(reimu_spirit_bomb_orb, { BoxedPlayer plr; int index; real angle; }) {
	int index = ARGS.index;

	Player *plr = ENT_UNBOX(ARGS.plr);
	Projectile *orb = TASK_BIND_UNBOXED(PROJECTILE(
		.pos = plr->pos,
		.timeout = 200 + 20 * index,
		.type = PROJ_PLAYER,
		.damage = 1000,
		.damage_type = DMG_PLAYER_BOMB,
		.size = 10 * (1+I),
		.layer = LAYER_NODRAW,
		.flags = PFLAG_NOREFLECT | PFLAG_NOCOLLISION | PFLAG_NOMOVE | PFLAG_MANUALANGLE,
	));

	BoxedProjectile b_orb = ENT_BOX(orb);

	INVOKE_TASK(reimu_spirit_bomb_orb_visual, b_orb);
	INVOKE_TASK_WHEN(&orb->events.killed, reimu_spirit_bomb_orb_impact, b_orb);
	CANCEL_TASK_AFTER(&orb->events.killed, THIS_TASK);

	int circletime = 100 + 20 * index;
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
			play_sound("redirect");
		}

		cmplx target_circle = plr->pos + 10 * sqrt(t) * dir * (1 + 0.1 * sin(0.2 * t));
		dir *= cdir(0.12);

		double circlestrength = 1.0 / (1 + exp(t - circletime));

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
				.sprite_ptr = get_sprite("part/stain"),
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

static void reimu_spirit_bomb(Player *p) {
	int count = 6;

	for(int i = 0; i < count; i++) {
		INVOKE_TASK_DELAYED(1, reimu_spirit_bomb_orb, ENT_BOX(p), i, 2*M_PI/count*i);
	}

	global.shake_view = 4;
	play_sound("bomb_reimu_a");
	play_sound("bomb_marisa_b");
}

static void reimu_spirit_bomb_bg(Player *p) {
	if(!player_is_bomb_active(p)) {
		return;
	}

	double t = player_get_bomb_progress(p);
	float alpha = 0;
	if(t > 0)
		alpha = min(1,10*t);
	if(t > 0.7)
		alpha *= 1-pow((t-0.7)/0.3,4);

	reimu_common_bomb_bg(p, alpha);
	colorfill(0, 0.05 * alpha, 0.1 * alpha, alpha * 0.5);
}

TASK(reimu_spirit_ofuda, { cmplx pos; cmplx vel; real damage; }) {
	Projectile *ofuda = PROJECTILE(
		.proto = pp_ofuda,
		.pos = ARGS.pos,
		.color = RGBA_MUL_ALPHA(1, 1, 1, 0.5),
		.move = move_linear(ARGS.vel),
		.type = PROJ_PLAYER,
		.damage = ARGS.damage,
		.shader = "sprite_particle",
	);

	BoxedProjectile b_ofuda = ENT_BOX(ofuda);
	ProjectileList trails = { 0 };

	int t = 0;
	while((ofuda = ENT_UNBOX(b_ofuda)) || trails.first) {
		if(ofuda) {
			reimu_common_ofuda_swawn_trail(ofuda, &trails);
		}

		for(Projectile *p = trails.first; p; p = p->next) {
			p->color.g *= 0.95;
		}

		process_projectiles(&trails, false);
		YIELD;
		++t;
	}
}

static void reimu_spirit_shot(Player *p) {
}

static void reimu_spirit_yinyang_focused_visual(Enemy *e, int t, bool render) {
	if(render) {
		reimu_common_draw_yinyang(e, t, RGB(1.0, 0.8, 0.8));
	}
}

static void reimu_spirit_yinyang_unfocused_visual(Enemy *e, int t, bool render) {
	if(render) {
		reimu_common_draw_yinyang(e, t, RGB(0.95, 0.75, 1.0));
	}
}

TASK(reimu_spirit_slave_expire, { ReimuAController *ctrl; BoxedEnemy e; BoxedTask main_task; }) {
	Enemy *e = TASK_BIND(ARGS.e);
	Player *plr = ARGS.ctrl->plr;
	cotask_cancel(cotask_unbox(ARGS.main_task));

	cmplx pos0 = e->pos;
	real retract_time = ORB_RETRACT_TIME;
	e->move = (MoveParams) { 0 };

	for(int i = 1; i <= retract_time; ++i) {
		YIELD;
		e->pos = clerp(pos0, plr->pos, i / retract_time);
	}

	delete_enemy(&plr->slaves, e);
}

static Enemy *reimu_spirit_create_slave(ReimuAController *ctrl, EnemyVisualRule visual) {
	Player *plr = ctrl->plr;
	Enemy *e = create_enemy_p(
		&plr->slaves,
		plr->pos,
		ENEMY_IMMUNE,
		visual,
		NULL, 0, 0, 0, 0
	);
	e->ent.draw_layer = LAYER_PLAYER_SLAVE;
	INVOKE_TASK_WHEN(&ctrl->events.slaves_expired, reimu_spirit_slave_expire, ctrl, ENT_BOX(e), THIS_TASK);
	return e;
}

TASK(reimu_slave_shot_particles, {
	BoxedEnemy e;
	int num;
	cmplx dir;
	Color *color0;
	Color *color1;
	Sprite *sprite;
}) {
	Enemy *e = TASK_BIND(ARGS.e);
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
			.pos = e->pos,
			.sprite_ptr = sprite,
			.timeout = timeout,
		);
		YIELD;
	}
}

TASK(reimu_spirit_slave_needle_shot, {
	ReimuAController *ctrl;
	BoxedEnemy e;
	real damage;
}) {
	Player *plr = ARGS.ctrl->plr;
	Enemy *e = TASK_BIND(ARGS.e);

	ShaderProgram *shader = r_shader_get("sprite_particle");
	Sprite *particle_spr = get_sprite("part/stain");

	real damage = ARGS.damage;
	real delay = 3;

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);

		INVOKE_SUBTASK(reimu_slave_shot_particles,
			.e = ENT_BOX(e),
			.color0 = RGBA(0.5, 0, 0, 0),
			.color1 = RGBA(0.5, 0.25, 0, 0),
			.num = delay,
			.dir = -I,
			.sprite = particle_spr
		);

		INVOKE_TASK(reimu_spirit_needle,
			.pos = e->pos - 25.0*I,
			.vel = -20*I,
			.damage = damage,
			.shader = shader
		);

		WAIT(delay);
	}
}

TASK(reimu_spirit_slave_needle, {
	ReimuAController *ctrl;
	real distance;
	real rotation;
	real rotation_offset;
	real shot_damage;
}) {
	ReimuAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	Enemy *e = TASK_BIND_UNBOXED(reimu_spirit_create_slave(ctrl, reimu_spirit_yinyang_focused_visual));

	INVOKE_SUBTASK(reimu_spirit_slave_needle_shot, ctrl, ENT_BOX(e), ARGS.shot_damage);

	real dist = ARGS.distance;
	cmplx offset = dist * cdir(ARGS.rotation_offset);
	cmplx rot = cdir(ARGS.rotation);

	real target_speed = 0.005 * dist;
	e->move = move_towards(plr->pos + offset, 0);

	for(;;) {
		YIELD;
		e->move.attraction = approach(e->move.attraction, target_speed, target_speed / 12.0);
		e->move.attraction_point = plr->pos + offset;
		offset *= rot;
	}
}

TASK(reimu_spirit_slave_homing_shot, {
	ReimuAController *ctrl;
	BoxedEnemy e;
	cmplx vel;
	real damage;
}) {
	Player *plr = ARGS.ctrl->plr;
	Enemy *e = TASK_BIND(ARGS.e);

	ShaderProgram *shader = r_shader_get("sprite_particle");
	Sprite *particle_spr = get_sprite("part/stain");
	cmplx vel = ARGS.vel;
	cmplx pdir = cnormalize(vel);

	real damage = ARGS.damage;
	real delay = 12;

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);

		INVOKE_SUBTASK(reimu_slave_shot_particles,
			.e = ENT_BOX(e),
			.color0 = RGBA(0.5, 0.125, 0, 0),
			.color1 = RGBA(0.5, 0.125, 0.25, 0),
			.num = delay,
			.dir = pdir,
			.sprite = particle_spr
		);

		INVOKE_TASK(reimu_spirit_homing,
			.pos = e->pos,
			.vel = vel,
			.damage = damage,
			.shader = shader
		);

		WAIT(delay);
	}
}

TASK(reimu_spirit_slave_homing, {
	ReimuAController *ctrl;
	cmplx offset;
	cmplx shot_vel;
	real shot_damage;
}) {
	ReimuAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	Enemy *e = TASK_BIND_UNBOXED(reimu_spirit_create_slave(ctrl, reimu_spirit_yinyang_unfocused_visual));

	INVOKE_SUBTASK(reimu_spirit_slave_homing_shot, ctrl, ENT_BOX(e), ARGS.shot_vel, ARGS.shot_damage);

	cmplx offset = ARGS.offset;
	real target_speed = 0.005 * cabs(offset);
	e->move = move_towards(plr->pos + offset, 0);

	for(;;) {
		YIELD;
		e->move.attraction = approach(e->move.attraction, target_speed, target_speed / 12.0);
		e->move.attraction_point = plr->pos + offset;
	}
}

static void reimu_spirit_spawn_slaves_unfocused(ReimuAController *ctrl, int power_rank) {
	real dmg_homing = 100;  // 120 - 12 * power_rank;
	cmplx sv = -10 * I;

	switch(power_rank) {
		case 0:
			break;
		case 1:
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, 50*I, sv, dmg_homing);
			break;
		case 2:
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, +40, sv * cdir(+M_PI/24), dmg_homing);
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, -40, sv * cdir(-M_PI/24), dmg_homing);
			break;
		case 3:
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, 50*I, sv, dmg_homing);
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, +40, sv * cdir(+M_PI/24), dmg_homing);
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, -40, sv * cdir(-M_PI/24), dmg_homing);
			break;
		case 4:
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, +40, sv * cdir(+M_PI/24), dmg_homing);
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, -40, sv * cdir(-M_PI/24), dmg_homing);
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, +80, sv * cdir(+M_PI/16), dmg_homing);
			INVOKE_TASK(reimu_spirit_slave_homing, ctrl, -80, sv * cdir(-M_PI/16), dmg_homing);
			break;
		default:
			UNREACHABLE;
	}
}

static void reimu_spirit_spawn_slaves_focused(ReimuAController *ctrl, int power_rank) {
	real dmg_needle = 90; // 92 - 10 * power_rank;

	switch(power_rank) {
		case 0:
			break;
		case 1:
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 60, 0.10, 0, dmg_needle);
			break;
		case 2:
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 60, 0.10, 0, dmg_needle);
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 60, 0.10, M_TAU/2, dmg_needle);
			break;
		case 3:
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 60, 0.10, 0*M_TAU/3, dmg_needle);
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 60, 0.10, 1*M_TAU/3, dmg_needle);
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 60, 0.10, 2*M_TAU/3, dmg_needle);
			break;
		case 4:
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 60, 0.10, 0, dmg_needle);
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 60, 0.10, M_PI, dmg_needle);
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 80, -0.05, 0, dmg_needle);
			INVOKE_TASK(reimu_spirit_slave_needle, ctrl, 80, -0.05, M_PI, dmg_needle);
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
	int power_rank = plr->power / 100;

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
			if(plr->slaves.first) {
				reimu_spirit_kill_slaves(ctrl);
				WAIT(ORB_RETRACT_TIME);
			}

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
	int old_power = plr->power / 100;

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.power_changed);
		int new_power = plr->power / 100;
		if(old_power != new_power) {
			reimu_spirit_respawn_slaves(ctrl);
			old_power = new_power;
		}
	}
}

TASK(reimu_spirit_shot_forward, { ReimuAController *ctrl; }) {
	Player *plr = ARGS.ctrl->plr;
	real dir = 1;

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);
		play_loop("generic_shot");
		INVOKE_TASK(reimu_spirit_ofuda,
			.pos = plr->pos + 10 * dir - 15.0*I,
			.vel = -20*I,
			.damage = 100 - 8 * (plr->power / 100)
		);
		dir = -dir;
		WAIT(3);
	}
}

TASK(reimu_spirit_shot_volley_bullet, { Player *plr; cmplx offset; cmplx vel; real damage; ShaderProgram *shader; }) {
	play_loop("generic_shot");

	PROJECTILE(
		.proto = pp_hakurei_seal,
		.pos = ARGS.plr->pos + ARGS.offset,
		.color = color_mul_scalar(RGBA(1, 1, 1, 0.5), 0.7),
		.move = move_linear(ARGS.vel),
		.type = PROJ_PLAYER,
		.damage = ARGS.damage,
		.shader = "sprite_particle",
		.shader_ptr = ARGS.shader,
	);
}

TASK(reimu_spirit_shot_volley, { ReimuAController *ctrl; }) {
	Player *plr = ARGS.ctrl->plr;
	ShaderProgram *shader = r_shader_get("sprite_particle");

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);

		int power_rank = plr->power / 100;
		real damage = 60 - 5 * power_rank;

		for(int pwr = 0; pwr <= power_rank; ++pwr) {
			int delay = 5 * pwr;
			real spread = M_PI/32 * (1 + 0.35 * pwr);

			INVOKE_SUBTASK_DELAYED(delay, reimu_spirit_shot_volley_bullet,
				plr,
				.offset = -I + 5,
				.vel = -18 * I * cdir(spread),
				.damage = damage,
				.shader = shader
			);

			INVOKE_SUBTASK_DELAYED(delay, reimu_spirit_shot_volley_bullet,
				plr,
				.offset = -I - 5,
				.vel = -18 * I * cdir(-spread),
				.damage = damage,
				.shader = shader
			);
		}

		WAIT(16);
	}
}

TASK(reimu_spirit_controller, { BoxedPlayer plr; }) {
	ReimuAController ctrl;
	ctrl.plr = TASK_BIND(ARGS.plr);
	COEVENT_INIT_ARRAY(ctrl.events);

	INVOKE_SUBTASK(reimu_spirit_focus_handler, &ctrl);
	INVOKE_SUBTASK(reimu_spirit_power_handler, &ctrl);
	INVOKE_SUBTASK(reimu_spirit_shot_forward, &ctrl);
	INVOKE_SUBTASK(reimu_spirit_shot_volley, &ctrl);

	WAIT_EVENT(&TASK_EVENTS(THIS_TASK)->finished);
	COEVENT_CANCEL_ARRAY(ctrl.events);
}

static void reimu_spirit_init(Player *plr) {
	reimu_common_bomb_buffer_init();
	INVOKE_TASK(reimu_spirit_controller, ENT_BOX(plr));
}

static double reimu_spirit_property(Player *plr, PlrProperty prop) {
	double base_value = reimu_common_property(plr, prop);

	switch(prop) {
		case PLR_PROP_SPEED: {
			float p = player_get_bomb_progress(plr);

			if(p < 0.5) {
				return base_value * p * 2;
			}
		}

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
		.bomb = reimu_spirit_bomb,
		.bombbg = reimu_spirit_bomb_bg,
		.shot = reimu_spirit_shot,
		.preload = reimu_spirit_preload,
	},
};
