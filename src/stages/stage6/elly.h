/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "boss.h"

#define FERMIONTIME 1000
#define HIGGSTIME 1700
#define YUKAWATIME 2200
#define SYMMETRYTIME (HIGGSTIME+200)
#define BREAKTIME (YUKAWATIME+400)

#define ELLY_DEFAULT_POS (BOSS_DEFAULT_GO_POS)
#define ELLY_SCYTHE_RESTING_OFS (8)

Boss* stage6_spawn_elly(cmplx pos);

DEFINE_ENTITY_TYPE(EllyScythe, {
	cmplx pos;
	real angle;
	real spin;
	real resting_angle;

	float scale;
	float opacity;

	MoveParams move;

	COEVENTS_ARRAY(
		despawned
	) events;
});

EllyScythe *stage6_host_elly_scythe(cmplx pos);
void stage6_despawn_elly_scythe(EllyScythe *scythe);

DEFINE_TASK_INTERFACE_WITH_BASE(ScytheAttack, BossAttack, {
	BoxedEllyScythe scythe;
});
typedef TASK_IFACE_ARGS_TYPE(ScytheAttack) ScytheAttackTaskArgs;
Boss *stage6_elly_init_scythe_attack(ScytheAttackTaskArgs *args);


#define NUM_BARYONS 6

#define MAX_BARYON_PARTICLES 128

DEFINE_ENTITY_TYPE(EllyBaryons, {
	cmplx poss[NUM_BARYONS];
	cmplx target_poss[NUM_BARYONS];
	real angles[NUM_BARYONS];
	real relaxation_rate;

	cmplx center_pos;
	float center_angle;
	float scale;

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

cmplx stage6_elly_baryon_default_offset(int i);

void elly_clap(Boss*, int);
