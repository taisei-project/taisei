/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "wriggle.h"

#include "common_tasks.h"
#include "i18n/i18n.h"

void stage3_draw_wriggle_spellbg(Boss *b, int time) {
	r_state_push();
	r_blend(BLEND_NONE);
	r_shader("stage3_wriggle_bg");
	r_uniform_sampler("tex_bg", "stage3/wspellbg");
	r_uniform_sampler("tex_clouds", "stage3/wspellclouds");
	r_uniform_sampler("tex_swarm", "stage3/wspellswarm");
	r_uniform_float("time", time / 60.0f);
	r_mat_mv_push();
	r_mat_mv_scale(VIEWPORT_W, VIEWPORT_H, 1);
	r_mat_mv_translate(0.5, 0.5, 0);
	r_draw_quad();
	r_mat_mv_pop();
	r_state_pop();
}

Boss *stage3_spawn_wriggle(cmplx pos) {
	Boss *wriggle = create_boss(_("Wriggle EX"), "wriggleex", pos);
	boss_set_portrait(wriggle, "wriggle", NULL, "proud");
	wriggle->glowcolor = *RGBA_MUL_ALPHA(0.2, 0.4, 0.5, 0.5);
	wriggle->shadowcolor = *RGBA_MUL_ALPHA(0.4, 0.2, 0.6, 0.5);
	return wriggle;
}

static void wriggle_slave_draw(EntityInterface *e) {
	WriggleSlave *slave = ENT_CAST(e, WriggleSlave);
	int time = global.frames - slave->spawn_time;
	r_draw_sprite(&(SpriteParams) {
		.pos.as_cmplx = slave->pos,
		.sprite_ptr = slave->sprites.circle,
		.rotation.angle = DEG2RAD * 7 * time,
		.scale.as_cmplx = slave->scale,
		.color = &slave->color,
	});
}

TASK(wriggle_slave_particles, { BoxedWriggleSlave slave; }) {
	WriggleSlave *slave = TASK_BIND(ARGS.slave);

	int period = 5;
	WAIT(rng_irange(0, period));

	for(;;WAIT(period)) {
		cmplx vel = 2 * rng_dir();

		PARTICLE(
			.sprite_ptr = slave->sprites.particle,
			.pos = slave->pos,
			.color = RGBA(0.6, 0.6, 0.5, 0),
			.draw_rule = pdraw_timeout_scale(2, 0.01),
			.timeout = 20,
			.move = move_linear(vel),
		);
	}
}

void stage3_init_wriggle_slave(WriggleSlave *slave, cmplx pos) {
	slave->pos = pos;
	slave->spawn_time = global.frames;
	slave->ent.draw_layer = LAYER_BOSS - 1;
	slave->ent.draw_func = wriggle_slave_draw;
	slave->sprites.circle = res_sprite("fairy_circle");
	slave->sprites.particle = res_sprite("part/smoothdot");

	INVOKE_TASK(wriggle_slave_particles, ENT_BOX(slave));
}

WriggleSlave *stage3_host_wriggle_slave(cmplx pos) {
	WriggleSlave *slave = TASK_HOST_ENT(WriggleSlave);
	TASK_HOST_EVENTS(slave->events);
	stage3_init_wriggle_slave(slave, pos);

	// TODO spawn animation
	// INVOKE_TASK(wriggle_slave_fadein, ENT_BOX(slave));
	slave->color = *RGBA(0.8, 1.0, 0.4, 0);
	slave->scale = (1 + I) * 0.7;

	// TODO despawn animation
	// INVOKE_TASK_AFTER(&slave->events.despawned, wriggle_slave_fadeout, ENT_BOX(slave));

	return slave;
}

void stage3_despawn_wriggle_slave(WriggleSlave *slave) {
	coevent_signal_once(&slave->events.despawned);
}

DEFINE_EXTERN_TASK(wriggle_slave_damage_trail) {
	WriggleSlave *slave = TASK_BIND(ARGS.slave);
	ShaderProgram *pshader = res_shader("sprite_default");

	for(;;WAIT(2)) {
		float t = (global.frames - slave->spawn_time) / 25.0;
		float c = 0.5f * psinf(t);

		PROJECTILE(
			// XXX: Do we want this to be a special snowflake without a ProjPrototype?
			.sprite_ptr = slave->sprites.particle,
			.size = 16 + 16*I,
			.collision_size = 7.2 + 7.2*I,

			.pos = slave->pos,
			.color = RGBA(1.0 - c, 0.5, 0.5 + c, 0),
			.draw_rule = pdraw_timeout_fade(1, 0),
			.timeout = 60,
			.shader_ptr = pshader,
			.flags = PFLAG_NOCLEAR | PFLAG_NOCLEAREFFECT | PFLAG_NOCOLLISIONEFFECT | PFLAG_NOSPAWNEFFECTS,
		);
	}
}

DEFINE_EXTERN_TASK(wriggle_slave_follow) {
	WriggleSlave *slave = TASK_BIND(ARGS.slave);
	Boss *boss = NOT_NULL(ENT_UNBOX(ARGS.boss));

	MoveParams move = move_towards(0, 0, 0.03);
	cmplx dir = cdir(ARGS.rot_initial);
	cmplx r = cdir(ARGS.rot_speed);

	for(;(boss = ENT_UNBOX(ARGS.boss)); YIELD) {
		real t = global.frames - slave->spawn_time;
		move.attraction_point = boss->pos + 100 * sin(t / 100) * dir;
		move_update(&slave->pos, &move);
		if(ARGS.out_dir) {
			*ARGS.out_dir = dir;
		}
		dir *= r;
	}
}
