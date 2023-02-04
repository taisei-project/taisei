/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"


static void baryons_final_blast(cmplx pos) {
	petal_explosion(24, pos);
	play_sfx("boom");
	stage_shake_view(15);

	for(uint i = 0; i < 3; ++i) {
		RNG_ARRAY(R, 6);
		PARTICLE(
			.sprite = "smoke",
			.pos = pos+10*vrng_real(R[0]) * vrng_dir(R[1]),
			.color = RGBA(0.2 * vrng_real(R[2]), 0.5, 0.4 * vrng_real(R[3]), 1.0),
			.move = move_asymptotic_simple(0.25 * vrng_dir(R[4]), 2),
			.draw_rule = pdraw_timeout_scalefade(1, 3, 1, 0),
			.timeout = vrng_range(R[5], 550, 650),
			.layer = LAYER_PARTICLE_HIGH | 0x40,
		);
	}

	{
		RNG_ARRAY(R, 8);
		
		PARTICLE(
			.sprite = "blast_huge_halo",
			.pos = pos + exp(4*vrng_real(R[0])) * vrng_dir(R[1]),
			.color = RGBA(vrng_range(R[2], 0.2, 0.3), vrng_range(R[3], 0.6, 1), vrng_range(R[4], 0.5, 1.0), 0),
			.timeout = 505 + 24 * vrng_real(R[5]),
			.draw_rule = pdraw_timeout_scalefade(0.25, 2.5 * vrng_range(R[6], 1.0, 1.2), 1, 0),
			.layer = LAYER_PARTICLE_HIGH | 0x41,
			.angle = vrng_angle(R[7]),
			.flags = PFLAG_REQUIREDPARTICLE,
		);
	}

	{
		RNG_ARRAY(R, 6);

		PARTICLE(
			.sprite = "blast_huge_rays",
			.color = color_add(RGBA(vrng_range(R[0], 0.2, 0.5), vrng_range(R[1], 0.5, 1.0), vrng_real(R[2]), 0.0), RGBA(1, 1, 1, 0)),
			.pos = pos,
			.timeout = 300 + 38 * vrng_real(R[3]),
			.draw_rule = pdraw_timeout_scalefade(0, vrng_range(R[4], 5.0, 6.0), 1, 0),
			.angle = vrng_angle(R[5]),
			.layer = LAYER_PARTICLE_HIGH | 0x42,
			.flags = PFLAG_REQUIREDPARTICLE,
		);
	}

	{
		RNG_ARRAY(R, 3);
		PARTICLE(
			.sprite = "blast_huge_halo",
			.pos = pos,
			.color = RGBA(3.0, 1.0, 2.0, 1.0),
			.timeout = 60 + 5 * vrng_sreal(R[0]),
			.draw_rule = pdraw_timeout_scalefade(0.25, vrng_range(R[1], 3.5, 4.2), 1, 0),
			.layer = LAYER_PARTICLE_HIGH | 0x41,
			.angle = vrng_angle(R[2]),
		);
	}
}

TASK(baryon_explode_movement, { BoxedEllyBaryons baryons; BoxedBoss boss; }) {
	EllyBaryons *baryons = TASK_BIND(ARGS.baryons); 

	for(int t = 0;; t++) {
		cmplx boss_pos = NOT_NULL(ENT_UNBOX(ARGS.boss))->pos;

		baryons->center_pos = boss_pos;

		for(int i = 0; i < NUM_BARYONS; i++) {
			baryons->target_poss[i] = boss_pos + stage6_elly_baryon_default_offset(i) * (1.0 + 0.2 * sin(t * 0.1)) * cdir(t * t * 0.0004);
		}

		YIELD;
	}

}

TASK(baryons_explode, { BoxedEllyBaryons baryons; BoxedBoss boss; }) {
	EllyBaryons *baryons = TASK_BIND(ARGS.baryons); 
	int lifetime = 120;
	int interval = 6;

	INVOKE_SUBTASK(baryon_explode_movement, ARGS.baryons, ARGS.boss);

	bool dead[NUM_BARYONS];
	memset(dead, 0, sizeof(dead));

	for(int t = 0;; t += WAIT(interval)) {
		for(int i = 0; i < NUM_BARYONS; i++) {
			if(dead[i]) {
				continue;
			}
			
			float angle = rng_angle();
			PARTICLE(
				.sprite = "blast_huge_halo",
				.pos = baryons->poss[i],
				.color = RGBA(3.0, 1.0, 2.0, 1.0),
				.timeout = 10,
				.draw_rule = pdraw_timeout_scalefade(t / 120.0 * rng_range(1.0, 1.2), 0, 1, 0),
				.layer = LAYER_PARTICLE_HIGH | 0x41,
				.angle = angle,
				.flags = PFLAG_REQUIREDPARTICLE,
			);

			if(rng_chance((t - lifetime) / 300.0)) {
				dead[i] = true; 
				baryons_final_blast(baryons->poss[i]);
			}
		}
	}
}

DEFINE_EXTERN_TASK(stage6_boss_baryons_explode) {
	STAGE_BOOKMARK(boss-baryons-explode);
	Boss *boss = stage6_elly_init_baryons_attack(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS.base);

	INVOKE_SUBTASK_DELAYED(20, baryons_explode, ARGS.baryons, ENT_BOX(boss));
	boss->move = move_towards(BOSS_DEFAULT_GO_POS, 0.04);

	for(int t = 0; t < 200; t++) {
		stage_shake_view(t / 200.0f);

		if(t > 30) {
			play_sfx_loop("charge_generic");
		}

		if(t == 42) {
			audio_bgm_stop(1.0);
		}

		YIELD;
	}

	// XXX: replace this by actual removal of baryons or the termination of the host task?
	EllyBaryons *baryons = NOT_NULL(ENT_UNBOX(ARGS.baryons));
	baryons->scale = 0;

	

	stage_shake_view(40);
	play_sfx("boom");

	real rad = 100 * rng_real();
	petal_explosion(100, boss->pos + rad * rng_dir());
}
