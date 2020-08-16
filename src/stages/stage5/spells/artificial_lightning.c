/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"
#include "../iku.h"

#include "common_tasks.h"
#include "global.h"

void iku_lightning(Boss *b, int time) {
	int t = time % 141;

	if(time == EVENT_DEATH) {
		enemy_kill_all(&global.enemies);
		return;
	}

	if(time < 0) {
		GO_TO(b, BOSS_DEFAULT_GO_POS, 0.03);
		return;
	}

	TIMER(&t);

	GO_TO(b,VIEWPORT_W/2+tanh(sin(time/100))*200+I*VIEWPORT_H/3+I*(cos(t/200)-1)*50,0.03);

	AT(0) {
		play_sound("charge_generic");
	}

	FROM_TO(0, 60, 1) {
		cmplx n = cexp(2.0*I*M_PI*frand());
		float l = 150*frand()+50;
		float s = 4+_i*0.01;
		float alpha = 0.5;

		PARTICLE(
			.sprite = "lightningball",
			.pos = b->pos+l*n,
			.color = RGBA(0.1*alpha, 0.1*alpha, 0.6*alpha, 0),
			.draw_rule = Fade,
			.rule = linear,
			.timeout = l/s,
			.args = { -s*n },
		);
	}

	if(global.diff >= D_Hard && time > 0 && !(time%100)) {
		int c = 7 + 2 * (global.diff == D_Lunatic);
		for(int i = 0; i<c; i++) {
			PROJECTILE(
				.proto = pp_bigball,
				.pos = b->pos,
				.color = RGBA(0.5, 0.1, 1.0, 0.0),
				.rule = zigzag_bullet,
				.args = { cexp(2*M_PI*I/c*i+I*carg(global.plr.pos-b->pos)) },
			);
		}

		play_sound("redirect");
		play_sound("shot_special1");
	}

	AT(100) {
		aniplayer_hard_switch(&b->ani, ((time/141)&1) ? "dashdown_left" : "dashdown_right",1);
		aniplayer_queue(&b->ani, "main", 0);
		int c = 40;
		int l = 200;
		int s = 10;

		for(int i=0; i < c; i++) {
			cmplx n = cexp(2.0*I*M_PI*frand());
			PARTICLE(
				.sprite = "smoke",
				.pos = b->pos,
				.color = RGBA(0.4, 0.4, 1.0, 0.0),
				.draw_rule = Fade,
				.rule = linear,
				.timeout = l/s,
				.args = { s*n },
			);
		}

		for(int i = 0; i < global.diff+1; i++){
			create_enemy1c(b->pos, ENEMY_IMMUNE, NULL, lightning_slave, 10*cexp(I*carg(global.plr.pos - b->pos)+2.0*I*M_PI/(global.diff+1)*i));
		}

		play_sound("shot_special1");
	}
}
