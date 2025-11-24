/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "nonspells.h"
#include "stagetext.h"
#include "i18n/i18n.h"

#include "common_tasks.h"

TASK(scythe_explode, { BoxedEllyScythe scythe; }) {
	EllyScythe *scythe = TASK_BIND(ARGS.scythe);
	INVOKE_TASK(common_easing_animated, &scythe->angle, 1000, 100, glm_ease_quad_in);
	INVOKE_TASK(common_easing_animate, &scythe->scale, 1.7, 70, glm_ease_quad_inout);
	INVOKE_TASK_DELAYED(70, common_easing_animate, &scythe->scale, 0, 30, glm_ease_linear);

	WAIT(100);
	petal_explosion(100, scythe->pos);
	play_sfx("boom");
	stage6_despawn_elly_scythe(scythe);
}

TASK(baryons_spawn, { BoxedBoss boss; BoxedEllyBaryons baryons; }) {
	stage6_init_elly_baryons(ARGS.baryons, ARGS.boss);
	int duration = 100;
	EllyBaryons *baryons = TASK_BIND(ARGS.baryons);
	INVOKE_SUBTASK(common_easing_animate, &baryons->scale, 1, duration, glm_ease_bounce_out);

	real angle_offset = -5;

	INVOKE_SUBTASK(common_easing_animated, &angle_offset, 0, duration, glm_ease_quad_out);

	for(int t = 0; t < duration; t++) {
		Boss *b = NOT_NULL(ENT_UNBOX(ARGS.boss));
		for(int i = 0; i < NUM_BARYONS; i++) {
			real radius = 3*200.0/duration * clamp(t - duration/(real)NUM_BARYONS/2*i, 0, duration/3.0);
			baryons->target_poss[i] = b->pos + radius * cdir((1-2*(i&1)) * angle_offset + M_TAU/NUM_BARYONS*i);
		}
		YIELD;
	}
}

DEFINE_EXTERN_TASK(stage6_boss_paradigm_shift) {
	STAGE_BOOKMARK(boss-baryons);
	Boss *boss = INIT_BOSS_ATTACK(&ARGS.base);
	BEGIN_BOSS_ATTACK(&ARGS.base);

	play_sfx("bossdeath");

	INVOKE_TASK(scythe_explode, ARGS.scythe);

	elly_clap(boss, 150);

	WAIT(80);
	audio_bgm_stop(0.5);
	WAIT(20);

	stage_unlock_bgm("stage6boss_phase1");
	stage_start_bgm("stage6boss_phase2");
	stagetext_add(_("Paradigm Shift!"), VIEWPORT_W/2.0 + I * (VIEWPORT_H/2.0+64), ALIGN_CENTER, res_font("big"), RGB(1, 1, 1), 0, 120, 10, 30);
	stage_shake_view(200);

	INVOKE_TASK(baryons_spawn, ENT_BOX(boss), ARGS.baryons);
}
