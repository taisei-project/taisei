/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "kurumi.h"

#include "audio/audio.h"
#include "global.h"
#include "intl/intl.h"

DEFINE_EXTERN_TASK(stage4_boss_nonspell_burst) {
	Boss *b = TASK_BIND(ARGS.boss);

	int count = ARGS.count;
	for(int i = 0; i < ARGS.duration; i += WAIT(100)) {
		play_sfx("shot_special1");
		aniplayer_queue(&b->ani, "muda", 4);
		aniplayer_queue(&b->ani, "main", 0);

		for(int j = 0; j < count; j++) {
			PROJECTILE(
				.proto = pp_bigball,
				.pos = b->pos,
				.color = RGBA(0.5, 0.0, 0.5, 0.0),
				.move = move_asymptotic_simple(cdir(M_TAU / count * j), 3),
			);
		}
	}
}

DEFINE_EXTERN_TASK(stage4_boss_nonspell_redirect) {
	Projectile *p = TASK_BIND(ARGS.proj);
	p->move = ARGS.new_move;
	p->color.b *= -1;
	play_sfx_ex("redirect", 10, false);
	spawn_projectile_highlight_effect(p);
}

TASK(kurumi_shadowcolor, { BoxedBoss boss; }) {
	auto b = TASK_BIND(ARGS.boss);

	// TODO: less braindead way of doing this. Perhaps a boss event on attack switch?
	for(;;YIELD) {
		if(b->current && ATTACK_IS_SPELL(b->current->type)) {
			b->shadowcolor = *RGBA_MUL_ALPHA(0.0, 0.4, 0.5, 0.5);
		} else {
			b->shadowcolor = *RGBA_MUL_ALPHA(1.0, 0.1, 0.0, 0.5);
		}
	}
}

Boss *stage4_spawn_kurumi(cmplx pos) {
	Boss* b = create_boss(_("Kurumi"), "kurumi", pos);
	boss_set_portrait(b, "kurumi", NULL, "normal");
	b->glowcolor = *RGB(0.5, 0.1, 0.0);
	INVOKE_TASK(kurumi_shadowcolor, ENT_BOX(b));
	return b;
}

DEFINE_EXTERN_TASK(stage4_boss_slave_visual) {
	for(;;) {
		PARTICLE(
			.sprite = "stain",
			.pos = *ARGS.pos,
			.color = RGBA(0.3, 0.0, 0.0, 0.0),
			.draw_rule = pdraw_timeout_fade(1, 0),
			.angle = rng_angle(),
			.scale = 0.4,
			.timeout = 30,
			.flags = PFLAG_REQUIREDPARTICLE,
		);
		WAIT(ARGS.interval);
	}
}

void kurumi_spell_bg(Boss *b, int time) {
	float f = 0.5+0.5*sin(time/80.0);

	r_mat_mv_push();
	r_mat_mv_translate(VIEWPORT_W / 2.0, VIEWPORT_H / 2.0, 0);
	r_mat_mv_scale(0.6, 0.6, 1);
	r_color3(f, 1 - f, 1 - f);
	draw_sprite(0, 0, "stage4/kurumibg1");
	r_mat_mv_pop();
	r_color4(1, 1, 1, 0);
	fill_viewport(time / 300.0, time / 300.0, 0.5, "stage4/kurumibg2");
	r_color4(1, 1, 1, 1);
}

