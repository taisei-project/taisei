/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "global.h"
#include "elly.h"
#include "draw.h"
#include "util/glm.h"
#include "spells/spells.h"

Boss* stage6_spawn_elly(cmplx pos) {
	Boss *b = create_boss("Elly", "elly", pos);
	boss_set_portrait(b, "elly", NULL, "normal");
	b->global_rule = elly_global_rule;
	return b;
}

static void scythe_particles(EllyScythe *s) {
	if(s->scale == 0) {
		return;
	}

	PARTICLE(
		.sprite_ptr = res_sprite("stage6/scythe"),
		.pos = s->pos+I*6*sin(global.frames/25.0),
		.draw_rule = pdraw_timeout_fade(1, 0),
		.timeout = 8,
		.angle = s->angle,
		.scale = s->scale,
		.flags = PFLAG_REQUIREDPARTICLE,
	);

	RNG_ARRAY(rand, 2);
	PARTICLE(
		.sprite = "smoothdot",
		.pos = s->pos + 100 * s->scale * vrng_real(rand[0]) * vrng_dir(rand[1]),
		.color = RGBA(1.0, 0.1, 1.0, 0.0),
		.draw_rule = pdraw_timeout_scalefade(0.4, 2, 1, 0),
		.timeout = 60,
		.flags = PFLAG_REQUIREDPARTICLE,
	);
}

TASK(scythe_visuals, { BoxedEllyScythe scythe; }) {
	EllyScythe *scythe = TASK_BIND(ARGS.scythe);

	for (;;) {
		scythe_particles(scythe);
		YIELD;
	}
}

TASK(scythe_update, { BoxedEllyScythe scythe; }) {
	EllyScythe *scythe = TASK_BIND(ARGS.scythe);

	for (;;) {
		move_update(&scythe->pos, &scythe->move);
		YIELD;
	}
}

static void stage6_init_elly_scythe(EllyScythe *scythe, cmplx pos) {
	scythe->pos = pos;
	scythe->scale = 0.7;
	INVOKE_TASK(scythe_visuals, ENT_BOX(scythe));
	INVOKE_TASK(scythe_update, ENT_BOX(scythe));
}

Boss *stage6_elly_init_scythe_attack(ScytheAttackTaskArgs *args) {
	if(ENT_UNBOX(args->scythe) == NULL) {
		Boss *b = NOT_NULL(ENT_UNBOX(args->base.boss));
		args->scythe = ENT_BOX(stage6_host_elly_scythe(b->pos));
	}

	return INIT_BOSS_ATTACK(&args->base);
}


EllyScythe *stage6_host_elly_scythe(cmplx pos) {
	EllyScythe *scythe = TASK_HOST_ENT(EllyScythe);
	TASK_HOST_EVENTS(scythe->events);
	stage6_init_elly_scythe(scythe, pos);
	return scythe;
}

DEFINE_EXTERN_TASK(stage6_elly_scythe_spin) {
	EllyScythe *scythe = TASK_BIND(ARGS.scythe);
	for(int i = 0; i != ARGS.duration; i++) {
		scythe->angle += ARGS.angular_velocity;
		YIELD;
	}
}

static void baryons_connector_particles(cmplx a, cmplx b) {
	Sprite *spr = res_sprite("stage6/baryon_connector");

	cmplx connector_pos = (a + b)/2;
	real connector_angle = carg(a - b) + M_PI/2;
	cmplx connector_scale = (cabs(a - b) - 70) / spr->w + I * 20 / spr->h;
	PARTICLE(
		.sprite_ptr = spr,
		.pos = connector_pos,
		.draw_rule = pdraw_timeout_fade(1, 0),
		.timeout = 4,
		.angle = connector_angle,
		.flags = PFLAG_REQUIREDPARTICLE,
		.scale = connector_scale
	);
}

static void baryons_particles(EllyBaryons *baryons) {
	if(baryons->scale == 0) {
		return;
	}

	PARTICLE(
		.sprite_ptr = res_sprite("stage6/baryon_center"),
		.pos = baryons->center_pos,
		.draw_rule = pdraw_timeout_fade(1, 0),
		.timeout = 4,
		.angle = baryons->center_angle,
		.scale = baryons->scale,
		.flags = PFLAG_REQUIREDPARTICLE,
	);

	PARTICLE(
		.sprite_ptr = res_sprite("stage6/baryon"),
		.pos = baryons->center_pos,
		.angle = M_TAU/12,
		.draw_rule = pdraw_timeout_fade(1, 0),
		.timeout = 4,
		.flags = PFLAG_REQUIREDPARTICLE,
	);


	for(int i = 0; i < NUM_BARYONS; i++) {
		cmplx pos = baryons->poss[i];
		cmplx pos_next = baryons->poss[(i+1) % NUM_BARYONS];

		PARTICLE(
			.sprite_ptr = res_sprite("stage6/baryon"),
			.pos = pos,
			.draw_rule = pdraw_timeout_fade(1, 0),
			.timeout = 4,
			.scale = baryons->scale,
			.angle = baryons->angles[i]+M_TAU/12,
			.flags = PFLAG_REQUIREDPARTICLE,
		);

		baryons_connector_particles(pos, pos_next);
	}

	baryons_connector_particles(baryons->center_pos, baryons->poss[0]);
	baryons_connector_particles(baryons->center_pos, baryons->poss[3]);
}

TASK(baryons_visuals, { BoxedEllyBaryons baryons; }) {
	EllyBaryons *baryons = TASK_BIND(ARGS.baryons);

	for (;;) {
		baryons_particles(baryons);
		YIELD;
	}
}

TASK(baryons_update, { BoxedEllyBaryons baryons; }) {
	EllyBaryons *baryons = TASK_BIND(ARGS.baryons);

	for (;;) {
		baryons->center_angle += 0.035;

		for(int i = 0; i < NUM_BARYONS; i++) {
			cmplx d = (baryons->target_poss[i] - baryons->poss[i]);
			if(cabs2(d) > 0) {
				baryons->poss[i] += baryons->relaxation_rate * (d / sqrt(cabs(d)) + 0.01*d);
			}
		}

		YIELD;
	}
}

TASK(baryons_quick_intro, { BoxedEllyBaryons baryons; }) {
	EllyBaryons *baryons = TASK_BIND(ARGS.baryons);

	int duration = 30;
	for(int t = 0; t < duration; t++) {
		baryons->scale = glm_ease_quad_in(t / (float) duration);
		YIELD;
	}
}

void stage6_init_elly_baryons(BoxedEllyBaryons baryons, BoxedBoss boss) {
	Boss *b = NOT_NULL(ENT_UNBOX(boss));
	EllyBaryons *eb = NOT_NULL(ENT_UNBOX(baryons));
	for(int i = 0; i < NUM_BARYONS; i++) {
		eb->target_poss[i] = b->pos + 150 * cdir(M_TAU / NUM_BARYONS * i);
		eb->poss[i] = b->pos;
	}
	eb->center_pos = b->pos;
	eb->scale = 0;
	eb->relaxation_rate = 0.5;
	INVOKE_TASK(baryons_visuals, baryons);
	INVOKE_TASK(baryons_update, baryons);
}

Boss *stage6_elly_init_baryons_attack(BaryonsAttackTaskArgs *args) {
	if(ENT_UNBOX(args->baryons) == NULL) {
		args->baryons = ENT_BOX(stage6_host_elly_baryons(args->base.boss));
		stage6_init_elly_baryons(args->baryons, args->base.boss);
		INVOKE_TASK(baryons_quick_intro, args->baryons);
	}

	return INIT_BOSS_ATTACK(&args->base);
}


EllyBaryons *stage6_host_elly_baryons(BoxedBoss boss) {
	EllyBaryons *baryons = TASK_HOST_ENT(EllyBaryons);
	TASK_HOST_EVENTS(baryons->events);
	return baryons;
}



// REMOVE
void scythe_draw(Enemy *e, int t, bool b) {
}

// REMOVE
void scythe_common(Enemy *e, int t) {
	e->args[1] += cimag(e->args[1]);
}


void elly_clap(Boss *b, int claptime) {
	aniplayer_queue(&b->ani, "clapyohands", 1);
	aniplayer_queue_frames(&b->ani, "holdyohands", claptime);
	aniplayer_queue(&b->ani, "unclapyohands", 1);
	aniplayer_queue(&b->ani, "main", 0);
}

