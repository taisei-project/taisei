/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "enemy.h"
#include "boss.h"

#define FERMIONTIME 1000
#define HIGGSTIME 1700
#define YUKAWATIME 2200
#define SYMMETRYTIME (HIGGSTIME+200)
#define BREAKTIME (YUKAWATIME+400)

Boss* stage6_spawn_elly(cmplx pos);

DEFINE_ENTITY_TYPE(EllyScythe, {
	cmplx pos;
	real angle;
	float scale;

	MoveParams move;

	COEVENTS_ARRAY(
		despawned
	) events;
});

EllyScythe *stage6_host_elly_scythe(cmplx pos);

DEFINE_TASK_INTERFACE_WITH_BASE(ScytheAttack, BossAttack, {
	BoxedEllyScythe scythe;
});
typedef TASK_IFACE_ARGS_TYPE(ScytheAttack) ScytheAttackTaskArgs;
Boss *stage6_elly_init_scythe_attack(ScytheAttackTaskArgs *args);


// duration < 0 means infinite duration
DECLARE_EXTERN_TASK(stage6_elly_scythe_spin, { BoxedEllyScythe scythe; real angular_velocity; int duration; });
DECLARE_EXTERN_TASK(stage6_elly_scythe_nonspell, { BoxedEllyScythe scythe; });

#define NUM_BARYONS 6

DEFINE_ENTITY_TYPE(EllyBaryons, {
	cmplx poss[NUM_BARYONS];
	cmplx target_poss[NUM_BARYONS];
	real relaxation_rate;

	real angles[NUM_BARYONS];

	cmplx center_pos;
	float center_angle;
	float center_scale;

	COEVENTS_ARRAY(
	        despawned
	) events;
});

EllyBaryons *stage6_host_elly_baryons(BoxedBoss boss);
void stage6_init_elly_baryons(BoxedEllyBaryons baryons, BoxedBoss boss);

DEFINE_TASK_INTERFACE_WITH_BASE(BaryonsAttack, BossAttack, {
	BoxedEllyBaryons baryons;
});

typedef TASK_IFACE_ARGS_TYPE(BaryonsAttack) BaryonsAttackTaskArgs;
Boss *stage6_elly_init_baryons_attack(BaryonsAttackTaskArgs *args);


void scythe_draw(Enemy *, int, bool);
void scythe_common(Enemy*, int);
int scythe_reset(Enemy*, int);
int scythe_infinity(Enemy*, int);
int scythe_explode(Enemy*, int);
void elly_clap(Boss*, int);
void elly_spawn_baryons(cmplx);

void set_baryon_rule(EnemyLogicRule);
int baryon_reset(Enemy*, int);

int wait_proj(Projectile*, int);

Enemy* find_scythe(void);
