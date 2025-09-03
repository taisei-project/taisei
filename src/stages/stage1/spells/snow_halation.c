/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "spells.h"
#include "i18n/i18n.h"

static Color *halation_color(Color *out_clr, float phase) {
	if(phase < 0.5) {
		*out_clr = *color_lerp(
			RGBA(0.4, 0.4, 0.75, 0),
			RGBA(0.4, 0.4, 0.3, 0),
			phase * phase
		);
	} else {
		*out_clr = *color_lerp(
			RGBA(0.4, 0.4, 0.3, 0),
			RGBA(1.0, 0.3, 0.2, 0),
			(phase - 0.5) * 2
		);
	}

	return out_clr;
}

TASK(halation_laser_color, { BoxedLaser laser; float max_width; }) {
	Laser *l = TASK_BIND(ARGS.laser);
	float max_width = ARGS.max_width;

	for(;;) {
		halation_color(&l->color, l->width / max_width);
		YIELD;
	}
}

static Laser *create_halation_laser(cmplx a, cmplx b, float width, float charge, float dur, const Color *clr) {
	Laser *l;

	if(clr == NULL) {
		Color c = { };
		l = create_laserline_ab(a, b, width, charge, dur, &c);
		INVOKE_TASK(halation_laser_color, ENT_BOX(l), width);
	} else {
		l = create_laserline_ab(a, b, width, charge, dur, clr);
	}

	return l;
}

TASK(halation_orb_trail, { BoxedProjectile orb; }) {
	Projectile *orb = TASK_BIND(ARGS.orb);

	for(;;) {
		stage1_spawn_stain(orb->pos, rng_angle(), 20);
		WAIT(4);
	}
}

TASK(halation_orb, {
	cmplx pos[4];
	int activation_time;
}) {
	Projectile *orb = TASK_BIND(PROJECTILE(
		.proto = pp_plainball,
		.pos = ARGS.pos[0],
		.max_viewport_dist = 200,
		.flags = PFLAG_NOCLEAR | PFLAG_NOCOLLISION,
		.move = move_from_towards(ARGS.pos[0], ARGS.pos[2], 0.1),
	));

	halation_color(&orb->color, 0);

	INVOKE_SUBTASK(halation_orb_trail, ENT_BOX(orb));
	CANCEL_TASK_AFTER(&orb->events.cleared, THIS_TASK);

	int activation_time = ARGS.activation_time;
	int phase_time = 60;
	cmplx *pos = ARGS.pos;

	// TODO modernize lasers

	WAIT(activation_time);
	create_halation_laser(pos[2], pos[3], 15, phase_time * 0.5, phase_time * 2.0, &orb->color);
	create_halation_laser(pos[0], pos[2], 15, phase_time, phase_time * 1.5, NULL);

	WAIT(phase_time / 2);
	play_sfx("laser1");
	WAIT(phase_time / 2);
	create_halation_laser(pos[0], pos[1], 12, phase_time, phase_time * 1.5, NULL);

	WAIT(phase_time);
	play_sfx("shot1");
	create_halation_laser(pos[0], pos[3], 15, phase_time, phase_time * 1.5, NULL);
	create_halation_laser(pos[1], pos[3], 15, phase_time, phase_time * 1.5, NULL);

	WAIT(phase_time);
	play_sfx("shot1");
	create_halation_laser(pos[0], pos[1], 12, phase_time, phase_time * 1.5, NULL);
	create_halation_laser(pos[0], pos[2], 15, phase_time, phase_time * 1.5, NULL);

	WAIT(phase_time);
	play_sfx("shot1");
	play_sfx("shot_special1");

	Color colors[] = {
		// PRECISE colors, VERY important!!!
		{ 226/255.0, 115/255.0,  45/255.0, 1 },
		{  54/255.0, 179/255.0, 221/255.0, 1 },
		{ 140/255.0, 147/255.0, 149/255.0, 1 },
		{  22/255.0,  96/255.0, 165/255.0, 1 },
		{ 241/255.0, 197/255.0,  31/255.0, 1 },
		{ 204/255.0,  53/255.0,  84/255.0, 1 },
		{ 116/255.0,  71/255.0, 145/255.0, 1 },
		{  84/255.0, 171/255.0,  72/255.0, 1 },
		{ 213/255.0,  78/255.0, 141/255.0, 1 },
	};

	int pcount = sizeof(colors)/sizeof(Color);
	float rot = rng_angle();

	for(int i = 0; i < pcount; ++i) {
		PROJECTILE(
			.proto = pp_crystal,
			.pos = orb->pos,
			.color = colors+i,
			.move = move_asymptotic_simple(cdir(rot + M_PI * 2 * (i + 1.0) / pcount), 3),
		);
	}

	kill_projectile(orb);
}

TASK(halation_chase, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	boss->move = move_from_towards(boss->pos, global.plr.pos, 0.05);
	for(;;) {
		aniplayer_queue(&boss->ani, "(9)", 0);
		boss->move.attraction_point = global.plr.pos;
		YIELD;
	}
}

DEFINE_EXTERN_TASK(stage1_spell_snow_halation) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	boss->move = move_from_towards(boss->pos, VIEWPORT_W/2.0 + 100.0*I, 0.05);
	BEGIN_BOSS_ATTACK(&ARGS);

	cmplx center;
	real rotation = 0;
	real cage_radius = 200;
	int cheater = 0;

	const int orbs = difficulty_value(0, 0, 10, 14);

	for(;;) {
		WAIT(60);
		center = global.plr.pos;
		rotation += M_PI/2;

		cmplx orb_positions[orbs];
		for(int i = 0; i < orbs; ++i) {
			orb_positions[i] = cage_radius * cdir(rotation + (i + 0.5) / orbs * M_TAU) + center;
		}

		for(int i = 0; i < orbs; ++i) {
			play_sfx_loop("shot1_loop");
			INVOKE_TASK(halation_orb,
				.pos = {
					orb_positions[i],
					orb_positions[(i + orbs/2) % orbs],
					orb_positions[(i + orbs/2 - 1) % orbs],
					orb_positions[(i + orbs/2 - 2) % orbs],
				},
				.activation_time = 35 - i
			);
			YIELD;
		}

		WAIT(60);

		if(cabs(global.plr.pos - center) > 200) {
			const char *text[] = {
				_("What are you doing??"),
				_("Dodge it properly!"),
				_("I bet you can’t do it! Idiot!"),
				_("I spent so much time on this attack!"),
				_("Maybe it is too smart for secondaries!"),
				_("I think you don’t even understand the timer!"),
				_("You- You Idiootttt!"),
			};

			if(cheater < ARRAY_SIZE(text)) {
				stagetext_add(text[cheater], global.boss->pos+100*I, ALIGN_CENTER, res_font("standard"), RGB(1,1,1), 0, 100, 10, 20);

				if(++cheater == ARRAY_SIZE(text)) {
					INVOKE_SUBTASK(halation_chase, ENT_BOX(boss));
				}
			}
		}

		WAIT(240);
	}
}
