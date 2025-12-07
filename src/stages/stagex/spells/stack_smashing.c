/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "bitarray.h"
#include "coroutine/taskdsl.h"
#include "projectile.h"
#include "random.h"
#include "spells.h"

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

TASK(stack_smash_bullet, { BoxedProjectile proj; cmplx origin; }) {
	cmplx axis = cnormalize(global.boss->pos - ARGS.origin);

	auto p = ENT_UNBOX(ARGS.proj);

	if(p == NULL) {
		return;
	}

	play_sfx("redirect");
	real distance_from_axis = re(I*axis * conj(p->pos-ARGS.origin));

	p->move = move_accelerated(-I*axis*distance_from_axis/BEATS, -sign(distance_from_axis)*0.001*I*axis);
}

TASK(stack_smash_arm, { cmplx pos; real angle; }) {
	ProjPrototype *protos[] = { pp_soul, pp_bigball, pp_ball, pp_plainball, pp_flea };

	real angle = ARGS.angle;
	real angle_diffusion = 0.2;
	int steps = 60;
	for(int t = 1;t < steps; t++) {
		real probabilities[] = { exp(-0.3*t), exp(-0.08*t), exp(-0.05*t), exp(-0.02*t), exp(-0.01*t) };

		for(int i = 1; i < ARRAY_SIZE(probabilities); i++) {
			probabilities[i] += probabilities[i-1];
		}
		real norm = probabilities[ARRAY_SIZE(probabilities)-1];
		for(int i = 0; i < ARRAY_SIZE(probabilities); i++) {
			probabilities[i] /= norm;
		}

		angle += angle_diffusion * rng_sreal();
		cmplx pos = ARGS.pos + 8*t*cdir(angle);

		real r = rng_real();
		int chosen;
		for(chosen = 0; chosen < ARRAY_SIZE(probabilities); chosen++) {
			if(r < probabilities[chosen]) {
				break;
			}
		}

		if(cabs(pos-global.plr.pos) < 40) {
			continue;
		}

		play_sfx("shot1_loop");


		auto p = PROJECTILE(protos[chosen], RGBA(0.5,0.3,1.0,0.0), .pos = pos);
		INVOKE_SUBTASK_DELAYED(2*BEATS-t,stack_smash_bullet, .proj = ENT_BOX(p), ARGS.pos);
		WAIT(1);
	}
	AWAIT_SUBTASKS;
}

DEFINE_EXTERN_TASK(stagex_spell_stack_smashing) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);
	boss->move = move_towards(boss->move.velocity, CMPLX(VIEWPORT_W/2, VIEWPORT_H/2), 0.02);

	real radius = 200;
	int count = 20;
	int final_count = 15;
	WAIT(BEATS);



	for(;;) {
		int count = 6;
	play_sfx("boon");
		for(int n = 0; n < count; n++) {
			INVOKE_SUBTASK(stack_smash_arm, boss->pos, M_TAU / count * n);
		}
		boss->move = move_towards(0,0.75*global.plr.pos + 0.25*boss->pos, 0.01);


		WAIT(3*BEATS);
	}
}
