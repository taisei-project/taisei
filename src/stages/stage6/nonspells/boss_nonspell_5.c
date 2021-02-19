/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

static int baryon_explode(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		free_ref(e->args[1]);
		petal_explosion(24, e->pos);
		play_sfx("boom");
		stage_shake_view(15);

		for(uint i = 0; i < 3; ++i) {
			PARTICLE(
				.sprite = "smoke",
				.pos = e->pos+10*frand()*cexp(2.0*I*M_PI*frand()),
				.color = RGBA(0.2 * frand(), 0.5, 0.4 * frand(), 1.0),
				.rule = asymptotic,
				.draw_rule = ScaleFade,
				.args = { cexp(I*2*M_PI*frand()) * 0.25, 2, (1 + 3*I) },
				.timeout = 600 + 50 * nfrand(),
				.layer = LAYER_PARTICLE_HIGH | 0x40,
			);
		}

		PARTICLE(
			.sprite = "blast_huge_halo",
			.pos = e->pos + cexp(4*frand() + I*M_PI*frand()*2),
			.color = RGBA(0.2 + frand() * 0.1, 0.6 + 0.4 * frand(), 0.5 + 0.5 * frand(), 0),
			.timeout = 500 + 24 * frand() + 5,
			.draw_rule = ScaleFade,
			.args = { 0, 0, (0.25 + 2.5*I) * (1 + 0.2 * frand()) },
			.layer = LAYER_PARTICLE_HIGH | 0x41,
			.angle = frand() * 2 * M_PI,
			.flags = PFLAG_REQUIREDPARTICLE,
		);

		PARTICLE(
			.sprite = "blast_huge_rays",
			.color = color_add(RGBA(0.2 + frand() * 0.3, 0.5 + 0.5 * frand(), frand(), 0.0), RGBA(1, 1, 1, 0)),
			.pos = e->pos,
			.timeout = 300 + 38 * frand(),
			.draw_rule = ScaleFade,
			.args = { 0, 0, (0 + 5*I) * (1 + 0.2 * frand()) },
			.angle = frand() * 2 * M_PI,
			.layer = LAYER_PARTICLE_HIGH | 0x42,
			.flags = PFLAG_REQUIREDPARTICLE,
		);

		PARTICLE(
			.sprite = "blast_huge_halo",
			.pos = e->pos,
			.color = RGBA(3.0, 1.0, 2.0, 1.0),
			.timeout = 60 + 5 * nfrand(),
			.draw_rule = ScaleFade,
			.args = { 0, 0, (0.25 + 3.5*I) * (1 + 0.2 * frand()) },
			.layer = LAYER_PARTICLE_HIGH | 0x41,
			.angle = frand() * 2 * M_PI,
		);

		return 1;
	}

	GO_TO(e, global.boss->pos + (e->pos0-global.boss->pos)*(1.0+0.2*sin(t*0.1)) * cexp(I*t*t*0.0004), 0.05);

	if(!(t % 6)) {
		PARTICLE(
			.sprite = "blast_huge_halo",
			.pos = e->pos,
			.color = RGBA(3.0, 1.0, 2.0, 1.0),
			.timeout = 10,
			.draw_rule = ScaleFade,
			.args = { 0, 0, (t / 120.0 + 0*I) * (1 + 0.2 * frand()) },
			.layer = LAYER_PARTICLE_HIGH | 0x41,
			.angle = frand() * 2 * M_PI,
			.flags = PFLAG_REQUIREDPARTICLE,
		);
	}

	if(t > 120 && frand() < (t - 120) / 300.0) {
		e->hp = 0;
		return 1;
	}

	return 1;
}

void elly_baryon_explode(Boss *b, int t) {
	TIMER(&t);

	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.05);

	AT(20) {
		set_baryon_rule(baryon_explode);
	}

	AT(42) {
		audio_bgm_stop(1.0);
	}

	FROM_TO(0, 200, 1) {
		stage_shake_view(_i / 200.0f);

		if(_i > 30) {
			play_sfx_loop("charge_generic");
		}
	}

	AT(200) {
		tsrand_fill(2);
		stage_shake_view(40);
		play_sfx("boom");
		petal_explosion(100, b->pos + 100*afrand(0)*cexp(2.0*I*M_PI*afrand(1)));
		enemy_kill_all(&global.enemies);
	}
}
