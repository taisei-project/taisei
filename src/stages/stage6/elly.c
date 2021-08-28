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
#include "spells/spells.h"

Boss* stage6_spawn_elly(cmplx pos) {
	Boss *b = create_boss("Elly", "elly", pos);
	boss_set_portrait(b, "elly", NULL, "normal");
	b->global_rule = elly_global_rule;
	return b;
}

static void scythe_particles(EllyScythe *s) {
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

DEFINE_EXTERN_TASK(stage6_elly_scythe_nonspell) {
	EllyScythe *scythe = TASK_BIND(ARGS.scythe);

	cmplx center = VIEWPORT_W/2.0 + 200.0 * I;

	scythe->move = move_towards(center, 0.1);
	WAIT(40);
	scythe->move.retention = 0.9;

	for(int t = 0;; t++) {
		play_sfx_loop("shot1_loop");

		real theta = 0.01 * t * (1 + 0.001 * t) + M_PI/2;
		scythe->pos = center + 200 * cos(theta) * (1 + sin(theta) * I) / (1 + pow(sin(theta), 2));

		scythe->angle = 0.2 * t * (1 + 0*0.001 * t);

		cmplx dir = cdir(scythe->angle);
		real vel = difficulty_value(1.4, 1.8, 2.2, 2.6);
		
		cmplx color_dir = cdir(3 * theta);

		if(t > 50) {
			PROJECTILE(
				.proto = pp_ball,
				.pos = scythe->pos + 60 * dir * cdir(1.2),
				.color = RGB(creal(color_dir), cimag(color_dir), creal(color_dir * cdir(2.1))),
				.move = move_accelerated(0, 0.01 *vel * dir),
			);
		}

		YIELD;
	}
}

static void baryons_particles(EllyBaryons *baryons) {
	Boss *boss = NOT_NULL(ENT_UNBOX(baryons->boss));

	PARTICLE(
		.sprite_ptr = res_sprite("stage6/baryon_center"),
		.pos = boss->pos,
		.draw_rule = pdraw_timeout_fade(1, 0),
		.timeout = 4,
		.angle = baryons->center_angle,
		.scale = baryons->center_scale,
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
			.angle = baryons->angles[i],
			.flags = PFLAG_REQUIREDPARTICLE,
		);


		Sprite *spr = res_sprite("stage6/baryon_connector");
		cmplx connector_pos = (pos + pos_next)/2;
		real connector_angle = carg(pos - pos_next) + M_PI/2;
		cmplx connector_scale = (cabs(pos - pos_next) - 70) / spr->w + I * 20 / spr->h;
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
		YIELD;
	}
}


void stage6_init_elly_baryons(BoxedEllyBaryons baryons, BoxedBoss boss) {
	Boss *b = NOT_NULL(ENT_UNBOX(boss));
	EllyBaryons *eb = NOT_NULL(ENT_UNBOX(baryons));
	for(int i = 0; i < NUM_BARYONS; i++) {
		eb->poss[i] = b->pos;
	}
	eb->boss = boss;
	eb->center_scale = 0;
	INVOKE_TASK(baryons_visuals, baryons);
	INVOKE_TASK(baryons_update, baryons);
}

Boss *stage6_elly_init_baryons_attack(BaryonsAttackTaskArgs *args) {
	if(ENT_UNBOX(args->baryons) == NULL) {
		args->baryons = ENT_BOX(stage6_host_elly_baryons(args->base.boss));
		stage6_init_elly_baryons(args->baryons, args->base.boss);
	}

	return INIT_BOSS_ATTACK(&args->base);
}


EllyBaryons *stage6_host_elly_baryons(BoxedBoss boss) {
	EllyBaryons *baryons = TASK_HOST_ENT(EllyBaryons);
	TASK_HOST_EVENTS(baryons->events);
	return baryons;
}



// REMOVE
void scythe_draw(Enemy *, int, bool) {
}

// REMOVE
void scythe_common(Enemy *e, int t) {
	e->args[1] += cimag(e->args[1]);
}

// REMOVE
int wait_proj(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	if(t > creal(p->args[1])) {
		if(t == creal(p->args[1]) + 1) {
			play_sfx_ex("redirect", 4, false);
			spawn_projectile_highlight_effect(p);
		}

		p->angle = carg(p->args[0]);
		p->pos += p->args[0];
	}

	return ACTION_NONE;
}




int scythe_reset(Enemy *e, int t) {
	if(t < 0) {
		scythe_common(e, t);
		return 1;
	}

	if(t == 1)
		e->args[1] = fmod(creal(e->args[1]), 2*M_PI) + I*cimag(e->args[1]);

	GO_TO(e, BOSS_DEFAULT_GO_POS, 0.05);
	e->args[2] = fmax(0.6, creal(e->args[2])-0.01*t);
	e->args[1] += (0.19-creal(e->args[1]))*0.05;
	e->args[1] = creal(e->args[1]) + I*0.9*cimag(e->args[1]);

	scythe_common(e, t);
	return 1;
}

attr_returns_nonnull
Enemy* find_scythe(void) {
	for(Enemy *e = global.enemies.first; e; e = e->next) {
		if(e->visual_rule == scythe_draw) {
			return e;
		}
	}

	UNREACHABLE;
}

void elly_clap(Boss *b, int claptime) {
	aniplayer_queue(&b->ani, "clapyohands", 1);
	aniplayer_queue_frames(&b->ani, "holdyohands", claptime);
	aniplayer_queue(&b->ani, "unclapyohands", 1);
	aniplayer_queue(&b->ani, "main", 0);
}

static int baryon_unfold(Enemy *baryon, int t) {
	if(t < 0)
		return 1; // not catching death for references! because there will be no death!

	TIMER(&t);

	int extent = 100;
	float timeout;

	if(global.stage->type == STAGE_SPELL) {
		timeout = 92;
	} else {
		timeout = 142;
	}

	FROM_TO(0, timeout, 1) {
		for(Enemy *e = global.enemies.first; e; e = e->next) {
			if(e->visual_rule == baryon_center_draw) {
				float f = t / (float)timeout;
				float x = f;
				float g = sin(2 * M_PI * log(log(x + 1) + 1));
				float a = g * pow(1 - x, 2 * x);
				f = 1 - pow(1 - f, 3) + a;

				baryon->pos = baryon->pos0 = e->pos + baryon->args[0] * f * extent;
				return 1;
			}
		}
	}

	return 1;
}

static int elly_baryon_center(Enemy *e, int t) {
	if(t == EVENT_DEATH) {
		free_ref(creal(e->args[1]));
		free_ref(cimag(e->args[1]));
	}

	if(global.boss) {
		e->pos = global.boss->pos;
	}

	return 1;
}

int scythe_explode(Enemy *e, int t) {
	if(t < 0) {
		scythe_common(e, t);
		return 0;
	}

	if(t < 50) {
		e->args[1] += 0.001*I*t;
		e->args[2] += 0.0015*t;
	}

	if(t >= 50)
		e->args[2] -= 0.002*(t-50);

	if(t == 100) {
		petal_explosion(100, e->pos);
		stage_shake_view(16);
		play_sfx("boom");

		scythe_common(e, t);
		return ACTION_DESTROY;
	}

	scythe_common(e, t);
	return 1;
}

void elly_spawn_baryons(cmplx pos) {
	int i;
	Enemy *e, *last = NULL, *first = NULL, *middle = NULL;

	for(i = 0; i < 6; i++) {
		e = create_enemy3c(pos, ENEMY_IMMUNE, baryon, baryon_unfold, 1.5*cexp(2.0*I*M_PI/6*i), i != 0 ? add_ref(last) : 0, i);
		e->ent.draw_layer = LAYER_BACKGROUND;

		if(i == 0) {
			first = e;
		} else if(i == 3) {
			middle = e;
		}

		last = e;
	}

	first->args[1] = add_ref(last);
	e = create_enemy2c(pos, ENEMY_IMMUNE, baryon_center_draw, elly_baryon_center, 0, add_ref(first) + I*add_ref(middle));
	e->ent.draw_layer = LAYER_BACKGROUND;
}


void set_baryon_rule(EnemyLogicRule r) {
	Enemy *e;

	for(e = global.enemies.first; e; e = e->next) {
		if(e->visual_rule == baryon) {
			e->birthtime = global.frames;
			e->logic_rule = r;
		}
	}
}

int baryon_reset(Enemy* baryon, int t) {
	if(t < 0) {
		return 1;
	}

	for(Enemy *e = global.enemies.first; e; e = e->next) {
		if(e->visual_rule == baryon_center_draw) {
			cmplx targ_pos = baryon->pos0 - e->pos0 + e->pos;
			GO_TO(baryon, targ_pos, 0.1);

			return 1;
		}
	}

	GO_TO(baryon, baryon->pos0, 0.1);
	return 1;
}

