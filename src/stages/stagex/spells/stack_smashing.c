/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "bitarray.h"
#include "color.h"
#include "common_tasks.h"
#include "coroutine/taskdsl.h"
#include "global.h"
#include "lasers/rules.h"
#include "projectile.h"
#include "random.h"
#include "spells.h"
#include <cglm/cglm.h>

#define BEATS 86

static void spawn_circle(cmplx pos, real phase, real radius, int count, real collapse_time) {
	for(int i = 0; i < count; i++) {
		cmplx offset = radius*cdir(M_TAU/count*(i+phase));

		PROJECTILE(
			.pos = pos + offset,
			.proto = pp_bullet,
			.color = RGBA(0.5,0.2,1,1),
			.move = move_linear(-offset/collapse_time),
			.timeout = collapse_time,
			.max_viewport_dist = 2*radius,
		);
	}
}

static void draw_wriggle_proj(Projectile *p, int t, ProjDrawRuleArgs args) {
	Animation *ani = res_anim("boss/wriggle");
	AniSequence *seq = get_ani_sequence(ani, "fly");
 	r_draw_sprite(&(SpriteParams){
		.shader_ptr = res_shader("sprite_default"),
		.pos.as_cmplx = p->pos,
		.scale.as_cmplx = p->scale,
		.sprite_ptr = animation_get_frame(ani, seq, global.frames),
		.color = &p->color,
		.rotation.angle = p->angle+M_PI/2,
 	});
}

static cmplx stack_smash_bullet_pos(mat4 perspective, vec3 pos) {
	vec4 pos_shift = {};
	glm_vec3_copy(pos, pos_shift);
	pos_shift[2] -= 1000;
	pos_shift[3] = 1;

	vec4 projpos = {};
	glm_mat4_mulv(perspective, pos_shift, projpos);

	if(projpos[3] != 0) {
		return 200*(projpos[0] + I * projpos[1])/projpos[3];
	}
	return 0;
}

TASK(stack_smash_bullet, { ProjPrototype *proto; cmplx origin; float *pos; float *axis; int delay; }) {
	mat4 perspective = {};
	glm_perspective(glm_rad(30), 1, 10, 2000, perspective);

	vec3 axis;
	glm_vec3_copy(ARGS.axis, axis);
	vec3 pos0 = {};
	glm_vec3_copy(ARGS.pos, pos0);
	if(pos0[2] > 700) {
		return;
	}

	cmplx p0 = ARGS.origin + stack_smash_bullet_pos(perspective, pos0);

	if(cabs(global.plr.pos - p0) < 40) {
		return;
	}

	auto p = TASK_BIND(PROJECTILE(.proto = ARGS.proto, .pos = p0, .max_viewport_dist = 20));

	p->color = *color_lerp(RGBA(1,0,0,0), RGBA(0,0,1,0), 0.5 + 0.5 * tanh(ARGS.pos[2]/100));

	WAIT(ARGS.delay);

	cmplx prevp = p->pos;
	for(int t = 0; t < 2*BEATS; t += WAIT(1)) {

		real f= (sqrt(0.01 * t * t + 1) - 1);
		vec3 pos = {};
		glm_vec3_copy(pos0, pos);
		glm_vec3_rotate(pos, 0.1*f, axis);
		p->color = *color_lerp(RGBA(1,0,0,0), RGBA(0,0,1,0), 0.5 + 0.5 * tanh(pos[2]/100));

		cmplx cp = stack_smash_bullet_pos(perspective, pos);
		prevp = p->pos;
		p->pos = ARGS.origin + cp;
	}
	p->move = move_accelerated(p->pos-prevp, 0.01*(p->pos-prevp));
}

TASK(stack_smash_arm, { cmplx pos; vec3 direction; }) {
	ProjPrototype *protos[] = { pp_soul, pp_bigball, pp_ball, pp_plainball, pp_flea };

	vec3 direction;
	glm_vec3_copy(ARGS.direction, direction);

	real stepsize = 20;

	real direction_diffusion = 0.02;
	int steps = 60;

	cmplx axis = cnormalize(global.plr.pos - ARGS.pos);
	vec3 axis3 = { re(axis), im(axis), 0.5};
	glm_vec3_normalize(axis3);
	for(int t = 1;t < steps; t++) {
		real probabilities[] = { exp(-0.3*t), exp(-0.08*t), exp(-0.05*t), exp(-0.03*t), exp(-0.001*t) };

		for(int i = 1; i < ARRAY_SIZE(probabilities); i++) {
			probabilities[i] += probabilities[i-1];
		}
		real norm = probabilities[ARRAY_SIZE(probabilities)-1];
		for(int i = 0; i < ARRAY_SIZE(probabilities); i++) {
			probabilities[i] /= norm;
		}


		vec3 perturbation = {};
		for(int i = 0; i < 3; i++) {
			// crude gaussian approx
			for(int n = 0; n < 12; n++) {
				perturbation[i] += 0.5 * rng_sreal();
			}
		}
		// project to plane perpendicular to direction
		vec3 perturbation_parallel = {};
		glm_vec3_proj(perturbation, direction, perturbation_parallel);
		glm_vec3_sub(perturbation, perturbation_parallel, perturbation);
		glm_vec3_scale(perturbation, direction_diffusion * glm_vec3_norm(perturbation), perturbation);
		glm_vec3_add(direction, perturbation, direction);
		glm_vec3_normalize(direction);


		real r = rng_real();
		int chosen;
		for(chosen = 0; chosen < ARRAY_SIZE(probabilities); chosen++) {
			if(r < probabilities[chosen]) {
				break;
			}
		}



		vec3 scaled_direction = {};
		glm_vec3_scale(direction, stepsize * t, scaled_direction);


		INVOKE_SUBTASK(stack_smash_bullet, protos[chosen], ARGS.pos, scaled_direction, axis3, BEATS);
		play_sfx("shot2");
		WAIT(1);
	}
	WAIT(BEATS-steps);
	for(int d = -1; d <= 1; d += 2) {
		Laser *l = create_laser(ARGS.pos, 150, 1000, RGBA(0.0,0.2,0.0,0), laser_rule_accelerated(d*axis, d*0.05*axis));
		l->width = 20;
	}

	play_sfx("boon");
	play_sfx("laser1");




	AWAIT_SUBTASKS;
}

DEFINE_EXTERN_TASK(stagex_spell_stack_smashing) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(boss->move.velocity, CMPLX(VIEWPORT_W/2, VIEWPORT_H/2), 0.02);

	INVOKE_SUBTASK(common_charge, .anchor = &boss->pos, .time = BEATS, .sound = COMMON_CHARGE_SOUNDS, .color = *RGBA(0,0,1,0));
	WAIT(BEATS);

    vec3 octahedron[] = {
        {1.0f, 1.0f, 0},
        {-1.0f, -1.0f, 0},
        {-1.0f, 1.0f, 0},
        {1.0f, -1.0f, 0},
        {0, 0, sqrt(2)},
        {0, 0, -sqrt(2)},
    };

    cmplx pos;

    for(;;) {
    	pos = global.plr.pos;
		int count = 6;
		play_sfx("boom");

		vec3 axis = { 0.1, 0.43, -1 };
		glm_vec3_normalize(axis);
		for(int n = 0; n < count; n++) {
			glm_vec3_rotate(octahedron[n], 0.4, axis);
			INVOKE_SUBTASK(stack_smash_arm, boss->pos, {octahedron[n][0], octahedron[n][1], octahedron[n][2]});
		}
		WAIT( BEATS);
		boss->move = move_towards(0,boss->pos * 0.3 + 0.7 *pos, 0.07);

		INVOKE_SUBTASK(common_charge, .anchor = &boss->pos, .time = 2*BEATS, .sound = COMMON_CHARGE_SOUNDS, .color = *RGBA(0,0,1,0));

		WAIT(2*BEATS);
	}
}
