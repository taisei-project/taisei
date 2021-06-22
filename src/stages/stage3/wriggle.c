/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "wriggle.h"

#include "common_tasks.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

void stage3_draw_wriggle_spellbg(Boss *b, int time) {
	r_color4(1,1,1,1);
	fill_viewport(0, 0, 768.0/1024.0, "stage3/wspellbg");
	r_color4(1,1,1,0.5);
	r_blend(r_blend_compose(
		BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE, BLENDOP_SUB,
		BLENDFACTOR_ZERO,      BLENDFACTOR_ONE, BLENDOP_ADD
	));
	fill_viewport(sin(time) * 0.015, time / 50.0, 1, "stage3/wspellclouds");
	r_blend(BLEND_PREMUL_ALPHA);
	r_color4(0.5, 0.5, 0.5, 0.0);
	fill_viewport(0, time / 70.0, 1, "stage3/wspellswarm");
	r_blend(r_blend_compose(
		BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE, BLENDOP_SUB,
		BLENDFACTOR_ZERO,      BLENDFACTOR_ONE, BLENDOP_ADD
	));
	r_color4(1,1,1,0.4);
	fill_viewport(cos(time) * 0.02, time / 30.0, 1, "stage3/wspellclouds");

	r_blend(BLEND_PREMUL_ALPHA);
	r_color4(1, 1, 1, 1);
}

Boss *stage3_spawn_wriggle(cmplx pos) {
	Boss *wriggle = create_boss("Wriggle EX", "wriggleex", pos);
	boss_set_portrait(wriggle, "wriggle", NULL, "proud");
	wriggle->glowcolor = *RGBA_MUL_ALPHA(0.2, 0.4, 0.5, 0.5);
	wriggle->shadowcolor = *RGBA_MUL_ALPHA(0.4, 0.2, 0.6, 0.5);
	return wriggle;
}

void wriggle_slave_visual(Enemy *e, int time, bool render) {
	if(time < 0)
		return;

	if(render) {
		r_draw_sprite(&(SpriteParams) {
			.sprite = "fairy_circle",
			.rotation.angle = DEG2RAD * 7 * time,
			.scale.both = 0.7,
			.color = RGBA(0.8, 1.0, 0.4, 0),
			.pos = { creal(e->pos), cimag(e->pos) },
		});
	} else if(time % 5 == 0) {
		tsrand_fill(2);
		PARTICLE(
			.sprite = "smoothdot",
			.pos = 5*cexp(2*I*M_PI*afrand(0)),
			.color = RGBA(0.6, 0.6, 0.5, 0),
			.draw_rule = Shrink,
			.rule = enemy_flare,
			.timeout = 60,
			.args = {
				0.3*cexp(2*M_PI*I*afrand(1)),
				add_ref(e),
			},
		);
	}
}

DEPRECATED_DRAW_RULE
static void wriggle_slave_part_draw(Projectile *p, int t, ProjDrawRuleArgs args) {
	float b = 1 - t / (double)p->timeout;
	r_mat_mv_push();
	r_mat_mv_translate(creal(p->pos), cimag(p->pos), 0);
	ProjDrawCore(p, color_mul_scalar(COLOR_COPY(&p->color), b));
	r_mat_mv_pop();
}

static int wriggle_rocket_laserbullet(Projectile *p, int time) {
	if(time == EVENT_DEATH) {
		free_ref(p->args[0]);
		return ACTION_ACK;
	} else if(time < 0) {
		return ACTION_ACK;
	}

	if(time >= creal(p->args[1])) {
		if(p->args[2]) {
			cmplx dist = global.plr.pos - p->pos;
			cmplx accel = (0.1 + 0.2 * (global.diff / (float)D_Lunatic)) * dist / cabs(dist);
			float deathtime = sqrt(2*cabs(dist)/cabs(accel));

			Laser *l = create_lasercurve2c(p->pos, deathtime, deathtime, RGBA(0.4, 0.9, 1.0, 0.0), las_accel, 0, accel);
			l->width = 15;

			PROJECTILE(
				.proto = p->proto,
				.pos = p->pos,
				.color = &p->color,
				.draw_rule = p->draw_rule,
				.rule = wriggle_rocket_laserbullet,
				.args = {
					add_ref(l),
					deathtime,
				}
			);

			play_sound("redirect");
			play_sound("shot_special1");
		} else {
			int cnt = 22, i;
			float rot = (global.frames - global.boss->current->starttime) * 0.0037 * (global.diff);
			Color *c = HSLA(fmod(rot, M_PI*2)/(M_PI/2), 1.0, 0.5, 0);

			for(i = 0; i < cnt; ++i) {
				float f = (float)i/cnt;

				PROJECTILE(
					.proto = pp_thickrice,
					.pos = p->pos,
					.color = c,
					.rule = asymptotic,
					.args = {
						(1.0 + psin(M_PI*18*f)) * cexp(I*(2.0*M_PI*f+rot)),
						2 + 2 * global.diff
					},
				);
			}

			PARTICLE(
				.proto = pp_blast,
				.pos = p->pos,
				.color = c,
				.timeout = 35 - 5 * frand(),
				.draw_rule = GrowFade,
				.args = { 0, 1 + 0.5 * frand() },
				.angle = M_PI * 2 * frand(),
			);

			// FIXME: better sound
			play_sound("enemydeath");
			play_sound("shot1");
			play_sound("shot3");
		}

		return ACTION_DESTROY;
	}

	Laser *laser = (Laser*)REF(p->args[0]);

	if(!laser)
		return ACTION_DESTROY;

	p->pos = laser->prule(laser, time);

	return 1;
}

int wriggle_spell_slave(Enemy *e, int time) {
	TIMER(&time)

	float angle = e->args[2] * (time / 70.0 + e->args[1]);
	cmplx dir = cexp(I*angle);
	Boss *boss = (Boss*)REF(e->args[0]);

	if(!boss)
		return ACTION_DESTROY;

	AT(EVENT_BIRTH) {
		e->ent.draw_layer = LAYER_BULLET - 1;
	}

	AT(EVENT_DEATH) {
		free_ref(e->args[0]);
		return 1;
	}

	GO_TO(e, boss->pos + 100 * sin(time / 100.0) * dir, 0.03)

	if(!(time % 2)) {
		float c = 0.5 * psin(time / 25.0);

		PROJECTILE(
			// FIXME: add prototype, or shove it into the basic ones somehow,
			// or just replace this with some thing else
			.sprite_ptr = get_sprite("part/smoothdot"),
			.size = 16 + 16*I,
			.collision_size = 7.2 + 7.2*I,

			.pos = e->pos,
			.color = RGBA(1.0 - c, 0.5, 0.5 + c, 0),
			.draw_rule = wriggle_slave_part_draw,
			.timeout = 60,
			.shader = "sprite_default",
			.flags = PFLAG_NOCLEAR | PFLAG_NOCLEAREFFECT | PFLAG_NOCOLLISIONEFFECT | PFLAG_NOSPAWNEFFECTS,
		);
	}

	// moonlight rocket rockets
	int rocket_period = (160 + 20 * (D_Lunatic - global.diff));

	if(!creal(e->args[3]) && !((time + rocket_period/2) % rocket_period)) {
		Laser *l;
		float dt = 60;

		l = create_lasercurve4c(e->pos, dt, dt, RGBA(1.0, 1.0, 0.5, 0.0), las_sine_expanding, 2.5*dir, M_PI/20, 0.2, 0);
		PROJECTILE(
			.proto = pp_ball,
			.pos = e->pos,
			.color = RGB(1.0, 0.4, 0.6),
			.rule = wriggle_rocket_laserbullet,
			.args = {
				add_ref(l), dt-1, 1
			}
		);

		l = create_lasercurve4c(e->pos, dt, dt, RGBA(0.5, 1.0, 0.5, 0.0), las_sine_expanding, 2.5*dir, M_PI/20, 0.2, M_PI);
		PROJECTILE(
			.proto = pp_ball,
			.pos = e->pos,
			.color = RGB(1.0, 0.4, 0.6),
			.rule = wriggle_rocket_laserbullet,
			.args = {
				add_ref(l), dt-1, 1
			},
		);

		play_sound("laser1");
	}

	// night ignite balls
	if(creal(e->args[3]) && global.diff > D_Easy) {
		FROM_TO(300, 1000000, 180) {
			int cnt = 5, i;
			for(i = 0; i < cnt; ++i) {
				PROJECTILE(
					.proto = pp_ball,
					.pos = e->pos,
					.color = RGBA(0.5, 1.0, 0.5, 0),
					.rule = accelerated,
					.args = {
						0, 0.02 * cexp(I*i*2*M_PI/cnt)
					},
				);

				if(global.diff > D_Hard) {
					PROJECTILE(
						.proto = pp_ball,
						.pos = e->pos,
						.color = RGBA(1.0, 1.0, 0.5, 0),
						.rule = accelerated,
						.args = {
							0, 0.01 * cexp(I*i*2*M_PI/cnt)
						},
					);
				}
			}

			// FIXME: better sound
			play_sound("shot_special1");
		}
	}

	return 1;
}
