/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "stage3_events.h"
#include "global.h"
#include "stage.h"
#include "enemy.h"

static Dialog *stage3_dialog(void) {
	PlayerCharacter *pc = global.plr.mode->character;
	Dialog *d = create_dialog(pc->dialog_sprite_name, "dialog/wriggle");

	if(pc->id == PLR_CHAR_MARISA) {
		dadd_msg(d,Left, "Huh, didn’t expect a bug to make it this far down the tunnel.");
		dadd_msg(d,Right, "All of this beautiful light, and you didn’t expect me to be drawn to it?");
		dadd_msg(d,Left, "I expected ya to lose, that’s all.");
		dadd_msg(d,Right, "I’m nowhere near as weak as you think, human! In fact, being here has made me stronger than ever! That’s because I’m the one that made this tunnel and broke the barrier,");
		dadd_msg(d,Right, "in order to finally show you all the true merit of insect power!");
		dadd_msg(d,Left, "I can’t really say I believe ya on that point, bein’ that you’re just a bug ’n all.");
		dadd_msg(d,Right, "Ha! Watch me put on a grand light show that will put your dud fireworks to shame!");
	} else if(pc->id == PLR_CHAR_YOUMU) {
		dadd_msg(d,Left, "Huh, why is it that I feel like I’ve fought you before? But when I try to recall anything, my skin crawls and I seem to forget immediately.");
		dadd_msg(d,Right, "Maybe it’s because you realized the true power of us insects?");
		dadd_msg(d,Left, "No, I think it’s because I was too disgusted.");
		dadd_msg(d,Right, "How dare you! We make life possible for you humans, and in turn you disrespect us and call us derogatory things!");
		dadd_msg(d,Right, "Entering this light has given me great power. I’ll stomp you out in the same cruel way you humans step on bugs!");
		dadd_msg(d,Left, "If I cut legs off of an insect, do they squirm all on their own? How gross!");
	} else if(pc->id == PLR_CHAR_REIMU) {
		dadd_msg(d,Left, "Huh, a bug that managed to escape extermination.");
		dadd_msg(d,Right, "I heard that! ");
		dadd_msg(d,Right, "Do you really think I’ll let you get away with such a callous remark?");
		dadd_msg(d,Left, "You’re just an insect yōkai. What do you think you’re capable of?");
		dadd_msg(d,Right, "Why, this entire incident! You’re looking at the culprit right here, right now!");
		dadd_msg(d,Left, "You can’t be serious. Are you just drunk on all this light?");
		dadd_msg(d,Right, "I’m drunk on power! I’ll prove my mettle by defeating you once and for all!");
	}
	dadd_msg(d, BGM, "stage3boss");

	return d;
}

static Dialog *stage3_post_dialog(void) {
	PlayerCharacter *pc = global.plr.mode->character;
	Dialog *d = create_dialog(pc->dialog_sprite_name, NULL);

	if(pc->id == PLR_CHAR_MARISA) {
		dadd_msg(d, Left, "Whoops, looks like she burnt herself out on all that light she was all excited about. As I figured, the tunnel’s still here…");
		dadd_msg(d, Left, "so she wasn’t the culprit at all, just a bug that got caught inside. She should’a searched for a window instead of flyin’ right into the fire.");
	}
	if(pc->id == PLR_CHAR_YOUMU) {
		dadd_msg(d, Left, "I never see any insect infestations while tending Hakugyokurō’s gardens. That’s a convenience of being half-dead.");
	}
	if(pc->id == PLR_CHAR_REIMU) {
		dadd_msg(d,Left, "If humans get drunk from moonshine, then I guess bugs can become intoxicated by sunshine too.");
		dadd_msg(d,Left, "She was definitely kidding with all those claims, though. My intuition tells me nothing has changed.");
	}

	return d;

}

static int stage3_enterswirl(Enemy *e, int t) {
	TIMER(&t)

	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 1, Power, 1, NULL);

		float r, g;
		if(frand() > 0.5) {
			r = 0.3;
			g = 1.0;
		} else {
			r = 1.0;
			g = 0.3;
		}

		int cnt = 24 - (D_Lunatic - global.diff) * 4;
		for(int i = 0; i < cnt; ++i) {
			double a = (M_PI * 2.0 * i) / cnt;
			complex dir = cexp(I*a);

			PROJECTILE(e->args[1]? "ball" : "rice", e->pos, rgb(r, g, 1.0), asymptotic, {
				1.5 * dir,
				10 - 10 * psin(2 * a + M_PI/2)
			});
		}

		return 1;
	}

	AT(EVENT_BIRTH) {
		e->alpha = 0;
	}

	AT(60) {
		e->hp = 0;
	}

	e->pos += e->args[0];

	return 0;
}

static int stage3_slavefairy(Enemy *e, int t) {
	TIMER(&t)

	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 1, Power, 3, NULL);
		return 1;
	}

	AT(EVENT_BIRTH) {
		e->alpha = 0;
	}

	if(t < 120)
		GO_TO(e, e->args[0], 0.03)

	FROM_TO_SND("shot1_loop", 30, 120, 5 - global.diff) {
		float a = _i * 0.5;
		complex dir = cexp(I*a);

		PROJECTILE("wave",
			.pos = e->pos + dir * 10,
			.color = (_i % 2)? rgb(1.0, 0.3, 0.3) : rgb(0.3, 0.3, 1.0),
			.rule = accelerated,
			.args = {
				dir * 2,
				dir * -0.035,
			},
		);

		if(global.diff > D_Easy && e->args[1]) {
			PROJECTILE("ball", e->pos + dir * 10, rgb(1.0, 0.6, 0.3), linear, { dir * (1.0 + 0.5 * sin(a)) });
		}
	}

	if(t >= 120) {
		e->pos += 3 * e->args[2] + 2.0*I;
	}

	return 0;
}

static int stage3_slavefairy2(Enemy *e, int t) {
	TIMER(&t)

	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 1, Power, 3, NULL);
		return 1;
	}

	AT(EVENT_BIRTH) {
		e->alpha = 0;
	}

	int lifetime = 160;
	if(t < lifetime)
		GO_TO(e, e->args[0], 0.03)

	FROM_TO_SND("shot1_loop", 30, lifetime, 1) {
		double a = _i/sqrt(global.diff);

		if(t > 90) {
			a *= -1;
		}

		complex dir = cexp(I*a);
		PROJECTILE("wave", e->pos, (_i&1) ? rgb(1.0,0.3,0.3) : rgb(0.3,0.3,1.0), linear, { 2*dir });

		if(global.diff > D_Normal && _i % 3 == 0) {
			PROJECTILE("rice", e->pos, !(_i&1) ? rgb(1.0,0.3,0.3) : rgb(0.3,0.3,1.0), linear, { -2*dir });
		}
	}

	if(t >= lifetime) {
		e->pos += 3 * e->args[2] + 2.0*I;
	}

	return 1;
}

static void charge_effect(Enemy *e, int t, int chargetime) {
	TIMER(&t);

	FROM_TO(chargetime - 30, chargetime, 1) {
		complex n = cexp(2.0*I*M_PI*frand());
		float l = 50*frand()+25;
		float s = 4+_i*0.01;

		PARTICLE(
			.texture = "flare",
			.pos = e->pos+l*n,
			.color = rgb(0.5, 0.5, 0.25),
			.draw_rule = Fade,
			.rule = timeout_linear,
			.args = { l/s, -s*n },
			.flags =  PFLAG_DRAWADD
		);
	}
}

static int stage3_burstfairy(Enemy *e, int t) {
	TIMER(&t)

	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 2, Power, 5, NULL);
		return 1;
	}

	AT(EVENT_BIRTH) {
		e->alpha = 0;
	}

	int bursttime = creal(e->args[3]);
	int prebursttime = cimag(e->args[3]);
	int burstspan = 30;

	if(t < bursttime) {
		GO_TO(e, e->args[0], 0.08)
	}

	if(prebursttime > 0) {
		AT(prebursttime) {
			play_sound("shot_special1");

			int cnt = 6 + 4 * global.diff;
			for(int p = 0; p < cnt; ++p) {
				complex dir = cexp(I*M_PI*2*p/cnt);
				PROJECTILE("ball", e->args[0], rgb(0.2, 0.1, 0.5), asymptotic, {
					dir,
					10 + 4 * global.diff
				});
			}
		}
	}

	AT(bursttime) {
		play_sound("shot_special1");
	}

	int step = 3 - (global.diff > D_Normal);

	charge_effect(e, t, bursttime);

	FROM_TO_SND("shot1_loop", bursttime, bursttime + burstspan, step) {
		double phase = (t - bursttime) / (double)burstspan;
		complex dir = cexp(I*M_PI*phase);

		int cnt = 5 + global.diff;
		for(int p = 0; p < cnt; ++p) {
			for(int i = -1; i < 2; i += 2) {
				PROJECTILE("bullet", e->pos + dir * 10,
					.color = mix_colors(rgb(1.0, 0.0, 0.0), rgb(0.0, 0.0, 1.0), psin(M_PI * phase)),
					.rule = asymptotic,
					.args = {
						1.5 * dir * (1 + p / (cnt - 1.0)) * i,
						3 * global.diff
					},
				);
			}
		}

	}

	if(t >= bursttime + burstspan) {
		e->pos += 3 * e->args[2] + 2.0*I;
	} else if(t > bursttime) {
		// GO_TO(e, VIEWPORT_W+VIEWPORT_H*I - e->args[0], 0.05)
	}

	return 0;
}

static int stage3_chargefairy_proj(Projectile *p, int t) {
	if(t < 0) {
		return 0;
	}

	/*
	if(t == 0) {
		p->type = FakeProj;
	} else if(t == 20) {
		p->type = EnemyProj;
	}
	*/

	p->angle = carg(p->args[0]);
	t -= creal(p->args[2]);

	if(t == 0) {
		// FIXME: particle effect
		play_sound("redirect");
		play_sound("shot_special1");
	} else if(t > 0) {
		p->args[1] *= 0.8;
		p->pos += p->args[0] * (p->args[1] + 1);
	}

	return 0;
}

static int stage3_chargefairy(Enemy *e, int t) {
	TIMER(&t)

	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 5, Power, 3, NULL);
		return 1;
	}

	AT(EVENT_BIRTH) {
		e->alpha = 0;
	}

	AT(30) {
		e->args[3] = global.plr.pos;

	}

	int chargetime = e->args[1];
	int cnt = 19 - 4 * (D_Lunatic - global.diff);
	int step = 1;

	charge_effect(e, t, chargetime);

	FROM_TO_SND("shot1_loop", chargetime, chargetime + cnt * step - 1, step) {
		complex aim = e->args[3] - e->pos;
		aim /= cabs(aim);
		complex aim_norm = -cimag(aim) + I*creal(aim);

		int layers = 1 + global.diff;
		int i = _i;

		for(int layer = 0; layer < layers; ++layer) {
			if(layer&1) {
				i = cnt - 1 - i;
			}

			double f = i / (cnt - 1.0);
			int w = 100 - 20 * layer;
			complex o = e->pos + w * psin(M_PI*f) * aim + aim_norm * w*0.8 * (f - 0.5);
			complex paim = e->pos + (w+1) * aim - o;
			paim /= cabs(paim);

			PROJECTILE("wave", o,
				.color = mix_colors(rgb(1.0, 0.0, 0.0), rgb(0.0, 0.0, 1.0), f),
				.rule = stage3_chargefairy_proj,
				.args = {
					paim, 6 + global.diff - layer,
					chargetime + 30 - t
				},
			);
		}
	}

	if(t > chargetime + step * cnt * 2) {
		/*
		complex dir = e->pos - (VIEWPORT_W+VIEWPORT_H*I)/2;
		dir /= cabs(dir);
		e->pos += dir;
		*/
		e->pos += e->args[2];
	} else {
		GO_TO(e, e->args[0], 0.03);
	}

	return 0;
}

static int stage3_bigfairy(Enemy *e, int t) {
	TIMER(&t)

	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 5, Power, 5, NULL);
		if(creal(e->args[0]) && global.timer > 2800)
			spawn_items(e->pos, Bomb, 1, NULL);
		return 1;
	}

	AT(EVENT_BIRTH) {
		e->alpha = 0;
	}

	EnemyLogicRule slave = cimag(e->args[0]) ? stage3_slavefairy2 : stage3_slavefairy;

	FROM_TO(30, 600, 270) {
		create_enemy3c(e->pos, 900, Fairy, slave, e->pos + 70 + 50 * I, e->args[0], +1);
		create_enemy3c(e->pos, 900, Fairy, slave, e->pos - 70 + 50 * I, e->args[0], -1);
	}

	FROM_TO(120, 600, 270) {
		create_enemy3c(e->pos, 900, Fairy, slave, e->pos + 70 - 50 * I, e->args[0], +1);
		create_enemy3c(e->pos, 900, Fairy, slave, e->pos - 70 - 50 * I, e->args[0], -1);
	}

	AT(creal(e->args[1]))
		e->hp = 0;

	return 0;
}

static int stage3_bitchswirl(Enemy *e, int t) {
	TIMER(&t)

	AT(EVENT_BIRTH) {
		return 1;
	}

	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 1, Power, 1, NULL);
		return -1;
	}

	FROM_TO(0, 120, 20) {
		PROJECTILE("flea", e->pos, rgb(.7, 0.0, 0.5), accelerated, {
			2*cexp(I*carg(global.plr.pos - e->pos)),
			0.005*cexp(I*(M_PI*2 * frand())) * (global.diff > D_Easy)
		});
		play_sound("shot1");
	}

	e->pos += e->args[0] + e->args[1] * creal(e->args[2]);
	e->args[2] = creal(e->args[2]) * cimag(e->args[2]) + I * cimag(e->args[2]);

	return 0;
}

static int stage3_cornerfairy(Enemy *e, int t) {
	TIMER(&t)

	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 5, Power, 5, NULL);
		return -1;
	}

	AT(EVENT_BIRTH) {
		e->alpha = 0;
	}

	FROM_TO(0, 120, 1)
		GO_TO(e, e->args[0], 0.01)

	FROM_TO_SND("shot1_loop", 140, 240, 1) {
		GO_TO(e, e->args[1], 0.025 * min((t - 120) / 42.0, 1))
		int d = 5; //(D_Lunatic - global.diff + 3);
		if(!(t % d)) {
			int i, cnt = 7+global.diff;

			for(i = 0; i < cnt; ++i) {
				float c = psin(t / 15.0);
				bool wave = global.diff > D_Easy && cabs(e->args[2]);

				PROJECTILE(
					.texture = wave ? "wave" : "thickrice",
					.pos = e->pos,
					.color = cabs(e->args[2])
							? rgb(0.5 - c*0.2, 0.3 + c*0.7, 1.0)
							: rgb(1.0 - c*0.5, 0.6, 0.5 + c*0.5),
					.rule = asymptotic,
					.args = {
						(1.8-0.4*wave*!!(e->args[2]))*cexp(I*((2*i*M_PI/cnt)+carg((VIEWPORT_W+I*VIEWPORT_H)/2 - e->pos))),
						1.5
					},
				);
			}
		}
	}

	AT(260) {
		return ACTION_DESTROY;
	}

	return 0;
}

static void scuttle_intro(Boss *boss, int time) {
	GO_TO(boss, VIEWPORT_W/2.0 + 100.0*I, 0.04);
}

static void scuttle_outro(Boss *boss, int time) {
	if(time == 0) {
		spawn_items(boss->pos, Point, 10, Power, 10, Life, 1, NULL);
	}

	boss->pos += pow(max(0, time)/30.0, 2) * cexp(I*(3*M_PI/2 + 0.5 * sin(time / 20.0)));
}

static int scuttle_poison(Projectile *p, int time) {
	int result = accelerated(p, time);

	if(time < 0)
		return 1;

	if(!(time % (57 - global.diff * 3)) && p->type != DeadProj) {
		float a = p->args[2];
		float t = p->args[3] + time;

		PROJECTILE(
			.texture = (frand() > 0.5)? "thickrice" : "rice",
			.pos = p->pos,
			.color = rgb(0.3, 0.7 + 0.3 * psin(a/3.0 + t/20.0), 0.3),
			.rule = accelerated,
			.args = {
				0,
				0.005*cexp(I*(M_PI*2 * sin(a/5.0 + t/20.0))),
			},
		);

		play_sound("redirect");
	}

	return result;
}

static int scuttle_lethbite_proj(Projectile *p, int time) {
	#define A0_PROJ_START 120
	#define A0_PROJ_CHARGE 20
	TIMER(&time)

	FROM_TO(A0_PROJ_START, A0_PROJ_START + A0_PROJ_CHARGE, 1)
		return 1;

	AT(A0_PROJ_START + A0_PROJ_CHARGE + 1) if(p->type != DeadProj) {
		p->args[1] = 3;
		p->args[0] = (3 + 2 * global.diff / (float)D_Lunatic) * cexp(I*carg(global.plr.pos - p->pos));

		int cnt = 3, i;
		for(i = 0; i < cnt; ++i) {
			tsrand_fill(2);
			Color clr = rgba(1.0,0.8,0.8,0.8);

			PARTICLE(
				.texture = "lasercurve",
				.color = clr,
				.draw_rule = EnemyFlareShrink,
				.rule = enemy_flare,
				.args = {
					100,
					cexp(I*(M_PI*anfrand(0))) * (1 + afrand(1)),
					add_ref(p)
				},
				.flags = PFLAG_DRAWADD,
			);

			float offset = global.frames/15.0;
			if(global.diff > D_Hard && global.boss) {
				offset = M_PI+carg(global.plr.pos-global.boss->pos);
			}

			PROJECTILE("thickrice", p->pos, rgb(0.4, 0.3, 1.0), linear, {
				-cexp(I*(i*2*M_PI/cnt + offset)) * (1.0 + (global.diff > D_Normal))
			});
		}

		play_sound("redirect");
		play_sound("shot1");
	}

	return asymptotic(p, time);
	#undef A0_PROJ_START
	#undef A0_PROJ_CHARGE
}

static void scuttle_lethbite(Boss *boss, int time) {
	int i;
	TIMER(&time)

	GO_TO(boss, VIEWPORT_W/2+VIEWPORT_W/3*sin(time/300) + I*cimag(boss->pos), 0.01)

	FROM_TO_INT(0, 90000, 72 + 6 * (D_Lunatic - global.diff), 0, 1) {
		int cnt = 21 - 1 * (D_Lunatic - global.diff);

		for(i = 0; i < cnt; ++i) {
			complex v = (2 - psin((max(3, global.diff+1)*2*M_PI*i/(float)cnt) + time)) * cexp(I*2*M_PI/cnt*i);
			PROJECTILE(
				.texture = "wave",
				.pos = boss->pos - v * 50,
				.color = _i % 2? rgb(0.7, 0.3, 0.0) : rgb(0.3, .7, 0.0),
				.rule = scuttle_lethbite_proj,
				.args = { v, 2.0 },
			);
		}

		// FIXME: better sound
		play_sound("shot_special1");
	}
}

void scuttle_deadly_dance(Boss *boss, int time) {
	int i;
	TIMER(&time)

	if(time < 0) {
		return;
	}

	boss->ani.stdrow=1;
	play_loop("shot1_loop");

	FROM_TO(0, 120, 1)
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/2, 0.03)

	if(time > 30) {
		double t = time * 1.5 * (0.4 + 0.3 * global.diff);
		double moverad = min(160, time/2.7);
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/2 + sin(t/50.0) * moverad * cexp(I * M_PI_2 * t/100.0), 0.03)

		if(!(time % 70)) {
			for(i = 0; i < 15; ++i) {
				double a = M_PI/(5 + global.diff) * i * 2;
				PROJECTILE(
					.texture = "wave",
					.pos = boss->pos,
					.color = rgb(0.3, 0.3 + 0.7 * psin(a*3 + time/50.0), 0.3),
					.rule = scuttle_poison,
					.args = {
						0,
						0.02 * cexp(I*(a+time/10.0)),
						a,
						time
					}
				);
			}

			play_sound("shot_special1");
		}

		if(global.diff > D_Easy && !(time % 35)) {
			int cnt = global.diff * 2;
			for(i = 0; i < cnt; ++i) {
				PROJECTILE("ball", boss->pos, rgb(1.0, 1.0, 0.3), asymptotic, {
					(0.5 + 3 * psin(time + M_PI/3*2*i)) * cexp(I*(time / 20.0 + M_PI/cnt*i*2)),
					1.5
				});
			}

			play_sound("shot1");
		}
	}

	if(!(time % 3)) {
		for(i = -1; i < 2; i += 2) {
			double c = psin(time/10.0);
			PROJECTILE(
				.texture = "crystal",
				.pos = boss->pos,
				.color = rgba(0.3 + c * 0.7, 0.6 - c * 0.3, 0.3, 0.7),
				.rule = linear,
				.args = {
					10 * cexp(I*(carg(global.plr.pos - boss->pos) + (M_PI/4.0 * i * (1-time/2500.0)) * (1 - 0.5 * psin(time/15.0))))
				}
			);
		}
	}
}

void scuttle_spellbg(Boss *h, int time) {
	float a = 1.0;

	if(time < 0)
		a += (time / (float)ATTACK_START_DELAY);
	float s = 0.3 + 0.7 * a;

	glColor4f(.1, .1, .1, a);
	draw_texture(VIEWPORT_W/2, VIEWPORT_H/2, "stage3/spellbg2");
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	fill_screen(-time/200.0 + 0.5, time/400.0+0.5, s, "stage3/spellbg1");

	glColor4f(1, 1, 1, 0.1);
	fill_screen(time/300.0 + 0.5, -time/340.0+0.5, s*0.5, "stage3/spellbg1");
	fill_screen(time/220.0 + 0.5, -time/400.0+0.5, s*0.5, "stage3/spellbg1");


	glColor4f(1, 1, 1, 1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void wriggle_spellbg(Boss *b, int time) {
	glColor4f(1,1,1,1);
	fill_screen(0, 0, 768.0/1024.0, "stage3/wspellbg");
	glColor4f(1,1,1,0.5);
	glBlendEquation(GL_FUNC_SUBTRACT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	fill_screen(sin(time) * 0.015, time / 50.0, 1, "stage3/wspellclouds");
	glBlendEquation(GL_FUNC_ADD);
	fill_screen(0, time / 70.0, 1, "stage3/wspellswarm");
	glBlendEquation(GL_FUNC_SUBTRACT);
	glColor4f(1,1,1,0.4);
	fill_screen(cos(time) * 0.02, time / 30.0, 1, "stage3/wspellclouds");

	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1,1,1,1);
}

Boss* stage3_spawn_scuttle(complex pos) {
	Boss *scuttle = create_boss("Scuttle", "scuttle", 0, pos);
	scuttle->glowcolor = rgba(0.5, 0.6, 0.3, 0.5);
	scuttle->shadowcolor = rgba(0.7, 0.3, 0.1, 0.5);
	return scuttle;
}

Boss* stage3_create_midboss(void) {
	Boss *scuttle = stage3_spawn_scuttle(VIEWPORT_W/2 - 200.0*I);

	boss_add_attack(scuttle, AT_Move, "Introduction", 1, 0, scuttle_intro, NULL);
	boss_add_attack(scuttle, AT_Normal, "Lethal Bite", 11, 20000, scuttle_lethbite, NULL);
	boss_add_attack_from_info(scuttle, &stage3_spells.mid.deadly_dance, false);
	boss_add_attack(scuttle, AT_Move, "Runaway", 2, 1, scuttle_outro, NULL);
	scuttle->zoomcolor = rgb(0.4, 0.1, 0.4);

	boss_start_attack(scuttle, scuttle->attacks);
	return scuttle;
}

static void wriggle_slave_visual(Enemy *e, int time, bool render) {
	if(time < 0)
		return;

	if(render) {
		glPushMatrix();
		glTranslatef(creal(e->pos),cimag(e->pos),0);
		glRotatef(7*time,0,0,1);
		glColor4f(0.8,1,0.4,1);
		glScalef(0.7,0.7,1);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		draw_texture(0,0,"fairy_circle");
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor3f(1,1,1);
		glPopMatrix();
	} else if(time % 5 == 0) {
		tsrand_fill(2);
		PARTICLE(
			.texture = "lasercurve",
			.pos = 5*cexp(2*I*M_PI*afrand(0)),
			.color = rgba(1,1,0.8,0.6),
			.draw_rule = EnemyFlareShrink,
			.rule = enemy_flare,
			.args = {
				60,
				0.3*cexp(2*M_PI*I*afrand(1)),
				add_ref(e),
			},
			.flags = PFLAG_DRAWADD,
		);
	}
}

static int wriggle_rocket_laserbullet(Projectile *p, int time) {
	if(time == EVENT_DEATH) {
		free_ref(p->args[0]);
		return 1;
	} else if(time < 0)
		return 1;

	if(time >= creal(p->args[1])) {
		if(p->args[2]) {
			complex dist = global.plr.pos - p->pos;
			complex accel = (0.1 + 0.2 * (global.diff / (float)D_Lunatic)) * cexp(I*carg(dist));
			float deathtime = sqrt(2*cabs(dist)/cabs(accel));

			Laser *l = create_lasercurve2c(p->pos, deathtime, deathtime, rgb(0.4, 0.9, 1.0), las_accel, 0, accel);
			l->width = 15;

			PROJECTILE(
				.texture_ptr = p->tex,
				.pos = p->pos,
				.color = p->color,
				.draw_rule = p->draw_rule,
				.rule = wriggle_rocket_laserbullet,
				.args = {
					add_ref(l),
					deathtime - 1
				}
			);

			play_sound("redirect");
			play_sound("shot_special1");
		} else {
			int cnt = floor(2 + frand() * global.diff), i;

			for(i = 0; i < cnt; ++i) {
				PROJECTILE(
					.texture = "thickrice",
					.pos = p->pos,
					.color = (i % 2) ? rgb(1.0, 0.4, 0.6) : rgb(0.6, 0.4, 1.0),
					.rule = asymptotic,
					.args = {
						(0.1 + frand()) * cexp(I*(2*i*M_PI/cnt+time)),
						5
					},
					.flags = PFLAG_DRAWADD,
				);
			}

			// FIXME: better sound
			play_sound("enemydeath");
			play_sound("shot1");
		}

		return ACTION_DESTROY;
	} else if(time < 0)
		return 1;

	Laser *laser = (Laser*)REF(p->args[0]);

	if(!laser)
		return ACTION_DESTROY;

	p->pos = laser->prule(laser, time);

	return 1;
}

static void wriggle_slave_part_draw(Projectile *p, int t) {
	Texture *tex = p->tex;
	glBindTexture(GL_TEXTURE_2D, tex->gltex);
	float b = 1 - t / p->args[0];
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	ProjDrawCore(p, multiply_colors(p->color, rgba(b, b, b, 1)));
	draw_texture_p(0,0, p->tex);
	glPopMatrix();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static int wriggle_spell_slave(Enemy *e, int time) {
	TIMER(&time)

	float angle = e->args[2] * (time / 70.0 + e->args[1]);
	complex dir = cexp(I*angle);
	Boss *boss = (Boss*)REF(e->args[0]);

	if(!boss)
		return ACTION_DESTROY;

	AT(EVENT_DEATH) {
		free_ref(e->args[0]);
		return 1;
	}

	GO_TO(e, boss->pos + 100 * sin(time / 100.0) * dir, 0.03)

	if(!(time % 2)) {
		float c = 0.5 * psin(time / 25.0);

		PROJECTILE(
			.texture_ptr = get_tex("part/lasercurve"),
			.pos = e->pos,
			.color = rgb(1.0 - c, 0.5, 0.5 + c),
			.draw_rule = wriggle_slave_part_draw,
			.rule = timeout,
			.args = { 120 },
			.color_transform_rule = proj_clrtransform_particle,
		);
	}

	// moonlight rocket rockets
	if(!creal(e->args[3]) && !(time % 140)) {
		Laser *l;
		float dt = 60;

		l = create_lasercurve4c(e->pos, dt, dt, rgb(1.0, 1.0, 0.5), las_sine_expanding, 2.5*dir, M_PI/20, 0.2, 0);
		PROJECTILE("ball", e->pos, rgb(1.0, 0.4, 0.6), wriggle_rocket_laserbullet, {
			add_ref(l), dt-1, 1
		});

		l = create_lasercurve4c(e->pos, dt, dt, rgb(0.5, 1.0, 0.5), las_sine_expanding, 2.5*dir, M_PI/20, 0.2, M_PI);
		PROJECTILE("ball", e->pos, rgb(1.0, 0.4, 0.6), wriggle_rocket_laserbullet, {
			add_ref(l), dt-1, 1
		});

		play_sound("laser1");
	}

	// night ignite balls
	if(creal(e->args[3]) && global.diff > D_Easy) {
		FROM_TO(300, 1000000, 180) {
			int cnt = 5, i;
			for(i = 0; i < cnt; ++i) {
				PROJECTILE("ball", e->pos, rgb(0.5, 1.0, 0.5), accelerated,
					.args = {
						0, 0.02 * cexp(I*i*2*M_PI/cnt)
					},
					.flags = PFLAG_DRAWADD,
				);

				if(global.diff > D_Hard) {
					PROJECTILE("ball", e->pos, rgb(1.0, 1.0, 0.5), accelerated,
						.args = {
							0, 0.01 * cexp(I*i*2*M_PI/cnt)
						},
						.flags = PFLAG_DRAWADD,
					);
				}
			}

			// FIXME: better sound
			play_sound("shot_special1");
		}
	}

	return 1;
}

void wriggle_moonlight_rocket(Boss *boss, int time) {
	int i, j, cnt = 1 + global.diff;
	TIMER(&time)

	AT(EVENT_DEATH) {
		killall(global.enemies);
		return;
	}

	if(time < 0)
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/2.5, 0.05)
	else if(time == 0) {
		for(j = -1; j < 2; j += 2) for(i = 0; i < cnt; ++i)
			create_enemy3c(boss->pos, ENEMY_IMMUNE, wriggle_slave_visual, wriggle_spell_slave, add_ref(boss), i*2*M_PI/cnt, j);
	}
}

static int wriggle_ignite_laserbullet(Projectile *p, int time) {
	if(time == EVENT_DEATH) {
		free_ref(p->args[0]);
		return 1;
	} else if(time < 0)
		return 1;

	if(time < 0)
		return 1;

	Laser *laser = (Laser*)REF(p->args[0]);

	if(laser) {
		p->args[3] = laser->prule(laser, time - p->args[1]) - p->pos;
	}

	p->angle = carg(p->args[3]);
	p->pos = p->pos + p->args[3];

	return 1;
}

static void wriggle_ignite_warnlaser_logic(Laser *l, int time) {
	if(time == EVENT_BIRTH) {
		l->width = 0;
		return;
	}

	if(time < 0) {
		return;
	}

	if(time == 90) {
		play_sound_ex("laser1", 30, false);
	}

	l->width = laser_charge(l, time, 90, 10);
	l->color = mix_colors(rgb(1, 0.2, 0.2), rgb(0.2, 0.2, 1), time / l->deathtime);
}

static void wriggle_ignite_warnlaser(Laser *l) {
	float f = 6;
	create_laser(l->pos, 90, 120, 0, l->prule, wriggle_ignite_warnlaser_logic, f*l->args[0], l->args[1], f*l->args[2], l->args[3]);
}

void wriggle_night_ignite(Boss *boss, int time) {
	TIMER(&time)

	float dfactor = global.diff / (float)D_Lunatic;

	if(time == EVENT_DEATH) {
		killall(global.enemies);
		return;
	}

	if(time < 0) {
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/3, 0.05)
		return;
	}

	AT(0) for(int j = -1; j < 2; j += 2) for(int i = 0; i < 7; ++i) {
		create_enemy4c(boss->pos, ENEMY_IMMUNE, wriggle_slave_visual, wriggle_spell_slave, add_ref(boss), i*2*M_PI/7, j, 1);
	}

	FROM_TO_INT(0, 1000000, 180, 120, 10) {
		float dt = 200;
		float lt = 100 * dfactor;

		float a = _ni*M_PI/2.5 + _i + time;
		float b = 0.3;
		float c = 0.3;

		complex vel = 2 * cexp(I*a);
		double amp = M_PI/5;
		double freq = 0.05;

		Laser *l1 = create_lasercurve3c(boss->pos, lt, dt, rgb(b, b, 1.0), las_sine_expanding, vel, amp, freq);
		wriggle_ignite_warnlaser(l1);

		Laser *l2 = create_lasercurve3c(boss->pos, lt * 1.5, dt, rgb(1.0, b, b), las_sine_expanding, vel, amp, freq - 0.002 * min(global.diff, D_Hard));
		wriggle_ignite_warnlaser(l2);

		Laser *l3 = create_lasercurve3c(boss->pos, lt, dt, rgb(b, b, 1.0), las_sine_expanding, vel, amp, freq - 0.004 * min(global.diff, D_Hard));
		wriggle_ignite_warnlaser(l3);

		for(int i = 0; i < 5 + 15 * dfactor; ++i) {
			PROJECTILE("plainball",	boss->pos, rgb(c, c, 1.0), wriggle_ignite_laserbullet, { add_ref(l1), i }, .flags = PFLAG_DRAWADD);
			PROJECTILE("bigball",	boss->pos, rgb(1.0, c, c), wriggle_ignite_laserbullet, { add_ref(l2), i }, .flags = PFLAG_DRAWADD);
			PROJECTILE("plainball",	boss->pos, rgb(c, c, 1.0), wriggle_ignite_laserbullet, { add_ref(l3), i }, .flags = PFLAG_DRAWADD);

			// FIXME: better sound
			play_sound("shot1");
		}

		// FIXME: better sound
		play_sound_ex("shot_special1", 1, false);
	}
}

static void wriggle_singularity_laser_logic(Laser *l, int time) {
	if(time == EVENT_BIRTH) {
		l->width = 0;
		l->speed = 0;
		l->timeshift = l->timespan;
		l->unclearable = true;
		return;
	}

	if(time == 120) {
		play_sound("laser1");
	}

	l->width = laser_charge(l, time, 120, 10 + 10 * psin(l->args[0] + time / 60.0));
	l->args[3] = time / 10.0;
	l->args[0] *= cexp(I*(M_PI/500.0) * (0.6 + 0.4 * global.diff));

	l->color = hsl((carg(l->args[0]) + M_PI) / (M_PI * 2), 1.0, 0.5);
}

void wriggle_light_singularity(Boss *boss, int time) {
	TIMER(&time)

	AT(EVENT_DEATH) {
		return;
	}

	time -= 120;

	if(time < 0) {
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/2, 0.05)
		return;
	}

	AT(0) {
		int cnt = 2 + global.diff;
		for(int i = 0; i < cnt; ++i) {
			complex vel = 2 * cexp(I*(M_PI / 4 + M_PI * 2 * i / (double)cnt));
			double amp = (4.0/cnt) * (M_PI/5.0);
			double freq = 0.05;

			create_laser(boss->pos, 200, 10000, rgb(0.0, 0.2, 1.0), las_sine_expanding,
				wriggle_singularity_laser_logic, vel, amp, freq, 0);
		}

		play_sound("charge_generic");
		boss->ani.stdrow = 0;
	}

	if(time > 120) {
		play_loop("shot1_loop");
	}

	if(time == 0) {
		return;
	}

	if(!((time+30) % 300)) {
		aniplayer_queue(&boss->ani, 2, 0, 0);
	}

	if(!(time % 300)) {
		char *ptype = NULL;

		switch(time / 300 - 1) {
			case 0:	 ptype = "thickrice";	break;
			case 1:	 ptype = "rice";		break;
			case 2:	 ptype = "bullet";		break;
			case 3:	 ptype = "wave";		break;
			case 4:	 ptype = "ball";		break;
			case 5:	 ptype = "plainball";	break;
			case 6:	 ptype = "bigball";		break;
			default: ptype = "soul";		break;
		}

		int cnt = 6 + 2 * global.diff;
		float colorofs = frand();

		for(int i = 0; i < cnt; ++i) {
			double a = ((M_PI*2.0*i)/cnt);
			complex dir = cexp(I*a);

			PROJECTILE(
				.texture = ptype,
				.pos = boss->pos,
				.color = hsl(a/(M_PI*2) + colorofs, 1.0, 0.5),
				.rule = asymptotic,
				.args = {
					dir * (1.2 - 0.2 * global.diff),
					20
				},
				.flags = PFLAG_DRAWADD,
			);
		}

		play_sound("shot_special1");
	} else if(!(time % 150)) {
		aniplayer_queue(&boss->ani, 1, 1, 0);
	}
}

static void wriggle_fstorm_proj_draw(Projectile *p, int time) {
	float f = 1-min(time/60.0,1);
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	glRotatef(p->angle*180/M_PI+90, 0, 0, 1);
	ProjDrawCore(p, p->color);

	if(f > 0) {
		p->tex = get_tex("proj/ball");
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		glScalef(f,f,f);
		ProjDrawCore(p,time);
		glScalef(1/f,1/f,1/f);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		p->tex = get_tex("proj/rice");
	}
	glPopMatrix();
}

static int wriggle_fstorm_proj(Projectile *p, int time) {
	if(time < 0)
		return 1;

	if(cabs(global.plr.pos-p->pos) > 100) {
		p->args[2]+=1;
	} else {
		p->args[2]-=1;
		if(creal(p->args[2]) < 0)
			p->args[2] = 0;
	}

	int turntime = rint(creal(p->args[0]));
	int t = rint(creal(p->args[2]));
	if(t < turntime) {
		float f = t/(float)turntime;
		p->color = rgb(0.3+0.7*(1 - pow(1 - f, 4)), 0.3+0.3*f*f, 0.7-0.7*f);
	}

	if(t == turntime && global.boss) {
		p->args[1] = global.boss->pos-p->pos;
		p->args[1] *= 2/cabs(p->args[1]);
		p->angle = carg(p->args[1]);
		p->birthtime = global.frames;
		p->draw_rule = wriggle_fstorm_proj_draw;
		p->tex = get_tex("proj/rice");

		for(int i = 0; i < 3; ++i) {
			tsrand_fill(2);
			PARTICLE("flare", p->pos, 0, timeout_linear,
				.args = {
					60, (1+afrand(0))*cexp(I*tsrand_a(1))
				},
				.draw_rule = Shrink,
			);
		}

		play_sound_ex("redirect", 3, false);
	}

	p->pos += p->args[1];
	return 1;
}

void wriggle_firefly_storm(Boss *boss, int time) {
	TIMER(&time)

	if(time < 0) {
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/2, 0.05)
		return;
	}

	bool lun = global.diff == D_Lunatic;

	FROM_TO_SND("shot1_loop", 30, 9000, 2) {
		boss->ani.stdrow=1;

		int i, cnt = 2;
		for(i = 0; i < cnt; ++i) {
			float r = tanh(sin(_i/200.));
			float v = lun ? cos(_i/150.)/pow(cosh(atanh(r)),2) : 0.5;
			complex pos = 230*cexp(I*(_i*0.301+2*M_PI/cnt*i))*r;

			PROJECTILE(
				.texture = (global.diff >= D_Hard) && !(i%10) ? "bigball" : "ball",
				.pos = boss->pos+pos,
				.color = rgb(0.2,0.2,0.6),
				.rule = wriggle_fstorm_proj,
				.args = {
					(global.diff == D_Easy) ? 40 : 100-25*(!lun)-20*(global.diff == D_Normal),
					cexp(I*(!lun)*0.6)*pos/cabs(pos)*(1+v)
				},
			);
		}
	}
}

static int wriggle_nonspell_slave(Enemy *e, int time) {
	TIMER(&time)

	int level = e->args[3];
	float angle = e->args[2] * (time / 70.0 + e->args[1]);
	complex dir = cexp(I*angle);
	Boss *boss = (Boss*)REF(e->args[0]);

	if(!boss)
		return ACTION_DESTROY;

	AT(EVENT_DEATH) {
		free_ref(e->args[0]);
		return 1;
	}

	if(time < 0)
		return 1;

	GO_TO(e, boss->pos + (100 + 20 * e->args[2] * sin(time / 100.0)) * dir, 0.03)

	int d = 10 - global.diff;
	if(level > 2)
		d += 4;

	if(!(time % d)) {
		play_sound("shot1");

		PROJECTILE("rice", e->pos, rgb(0.7, 0.2, 0.1), linear, { 3 * cexp(I*carg(boss->pos - e->pos)) });

		if(!(time % (d*2)) || level > 1) {
			PROJECTILE("thickrice", e->pos, rgb(0.7, 0.7, 0.1), linear, { 2.5 * cexp(I*carg(boss->pos - e->pos)) });
		}

		if(level > 2) {
			PROJECTILE("wave", e->pos, rgb(0.3, 0.1 + 0.6 * psin(time / 25.0), 0.7), linear, { 2 * cexp(I*carg(boss->pos - e->pos)) });
		}
	}

	return 1;
}

static void wriggle_nonspell_common(Boss *boss, int time, int level) {
	TIMER(&time)
	int i, j, cnt = 3 + global.diff;

	AT(0) for(j = -1; j < 2; j += 2) for(i = 0; i < cnt; ++i)
		create_enemy4c(boss->pos, ENEMY_IMMUNE, wriggle_slave_visual, wriggle_nonspell_slave, add_ref(boss), i*2*M_PI/cnt, j, level);

	AT(EVENT_DEATH) {
		killall(global.enemies);
		return;
	}

	if(time < 0) {
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/3, 0.05)
		return;
	}

	FROM_TO(120, 240, 1)
		GO_TO(boss, VIEWPORT_W/3 + VIEWPORT_H*I/3, 0.03)

	FROM_TO(360, 480, 1)
		GO_TO(boss, VIEWPORT_W - VIEWPORT_W/3 + VIEWPORT_H*I/3, 0.03)

	FROM_TO(600, 720, 1)
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/3, 0.03)
}

static void stage3_boss_nonspell1(Boss *boss, int time) {
	wriggle_nonspell_common(boss, time, 1);
}

static void stage3_boss_nonspell2(Boss *boss, int time) {
	wriggle_nonspell_common(boss, time, 2);
}

static void stage3_boss_nonspell3(Boss *boss, int time) {
	wriggle_nonspell_common(boss, time, 3);
}

static void stage3_boss_intro(Boss *boss, int time) {
	if(time == 110)
		global.dialog = stage3_dialog();

	GO_TO(boss, VIEWPORT_W/2.0 + 100.0*I, 0.03);
}

Boss* stage3_spawn_wriggle_ex(complex pos) {
	Boss *wriggle = create_boss("Wriggle EX", "wriggleex", "dialog/wriggle", pos);
	wriggle->glowcolor = rgba(0.2, 0.4, 0.5, 0.5);
	wriggle->shadowcolor = rgba(0.4, 0.2, 0.6, 0.5);
	return wriggle;
}

Boss* stage3_create_boss(void) {
	Boss *wriggle = stage3_spawn_wriggle_ex(VIEWPORT_W/2 - 200.0*I);

	boss_add_attack(wriggle, AT_Move, "Introduction", 2, 0, stage3_boss_intro, NULL);
	boss_add_attack(wriggle, AT_Normal, "", 11, 35000, stage3_boss_nonspell1, NULL);
	boss_add_attack_from_info(wriggle, &stage3_spells.boss.moonlight_rocket, false);
	boss_add_attack(wriggle, AT_Normal, "", 20, 35000, stage3_boss_nonspell2, NULL);
	boss_add_attack_from_info(wriggle, &stage3_spells.boss.wriggle_night_ignite, false);
	boss_add_attack(wriggle, AT_Normal, "", 20, 35000, stage3_boss_nonspell3, NULL);
	boss_add_attack_from_info(wriggle, &stage3_spells.boss.firefly_storm, false);
	boss_add_attack_from_info(wriggle, &stage3_spells.extra.light_singularity, false);

	boss_start_attack(wriggle, wriggle->attacks);
	return wriggle;
}

void stage3_skip(int t);

void stage3_events(void) {
	TIMER(&global.timer);

	AT(0) {
		stage_start_bgm("stage3");
		stage3_skip(getenvint("STAGE3_TEST", 0));
	}

	FROM_TO(160, 300, 10) {
		tsrand_fill(2);
		create_enemy1c(VIEWPORT_W/2 + 20 * anfrand(0) + (VIEWPORT_H/4 + 20 * anfrand(1))*I, 200, Swirl, stage3_enterswirl, 3 * (I + sin(M_PI*global.frames/15.0)));
	}

	AT(360) {
		create_enemy2c(VIEWPORT_W/2 + (VIEWPORT_H/3)*I, 10000, BigFairy, stage3_bigfairy, 0+1*I, 600 - 30 * (D_Lunatic - global.diff));
	}

	FROM_TO(600, 800-30*(D_Lunatic-global.diff), 20) {
		create_enemy3c(-20 + 20*I, 50, Swirl, stage3_bitchswirl, 5, 1*I, 5+0.95*I);
	}

	FROM_TO(800, 1000-30*(D_Lunatic-global.diff), 20) {
		complex f = 5 + (0.93 + 0.01 * _i) * I;
		create_enemy3c(-20 + 20*I, 50, Swirl, stage3_bitchswirl, 5, 1*I, f);
		create_enemy3c(VIEWPORT_W+20 + 20*I, 50, Swirl, stage3_bitchswirl, -5, 1*I, f);
	}

	{
		int cnt = 4;
		int step = 20;
		int start = 1030;
		FROM_TO(start, start + step * cnt - 1, step) {
			int i = _i % cnt;
			double span = 300 - 60 * (i/2);
			complex pos1 = VIEWPORT_W/2;
			complex pos2 = VIEWPORT_W/2 + span * (-0.5 + (i&1)) + (VIEWPORT_H/3 + 100*(i/2))*I;
			create_enemy4c(pos1, 700, Fairy, stage3_burstfairy, pos2, i&1, 0, start + step * 6 - global.timer);
		}
	}

	{
		int cnt = 6;
		int step = 60;
		int start = 1200;
		FROM_TO(start, start + cnt * step - 1, step) {
			int i = _i % 4;
			double span = 300 - 60 * (i/2);
			complex pos = VIEWPORT_W/2 + span * (-0.5 + (i&1)) + (VIEWPORT_H/3 + 100*(i/2))*I;

			complex exitdir = pos - (VIEWPORT_W+VIEWPORT_H*I)/2;
			exitdir /= cabs(exitdir);

			create_enemy3c(pos, 1000, Fairy, stage3_chargefairy, pos, 30, exitdir);
		}
	}

	FROM_TO(1530, 1575, 10) {
		create_enemy3c(20 - 20*I, 50, Swirl, stage3_bitchswirl, 5*I, 0, 0);
	}

	FROM_TO(1575, 1620, 10) {
		create_enemy3c(VIEWPORT_W-20 - 20*I, 50, Swirl, stage3_bitchswirl, 5*I, 0, 0);
	}

	AT(1600) {
		create_enemy2c(VIEWPORT_W/2 + (VIEWPORT_H/3)*I, 10000, BigFairy, stage3_bigfairy, 1, 600 - 30 * (D_Lunatic - global.diff));
	}

	FROM_TO(1800, 2200, 10) {
		if(global.enemies == 0) {
			int cnt = 2;
			for(int i = 0; i <= cnt;i++) {
				complex pos1 = VIEWPORT_W/2+VIEWPORT_W/3*nfrand() + VIEWPORT_H/5*I;
				complex pos2 = VIEWPORT_W/2+50*(i-cnt/2)+VIEWPORT_H/3*I;
				create_enemy3c(pos1, 700, Fairy, stage3_slavefairy2, pos2, i&1,0.5*(i-cnt/2));
			}
		}
	}

	{
		int cnt = 3;
		int step = 60;
		int start = 2000;

		FROM_TO(start, start + cnt * step - 1, step) {
			create_enemy3c(-20 + (VIEWPORT_H+20)*I, 1000, Fairy, stage3_chargefairy, 30 + VIEWPORT_H/2.0*I, 60, -1*I);
			create_enemy3c(VIEWPORT_W+20 + (VIEWPORT_H+20)*I, 1000, Fairy, stage3_chargefairy, VIEWPORT_W-30 + VIEWPORT_H/2.0*I, 60, -1*I);
		}
	}

	AT(2400) {
		double offs = -50;

		complex p1 = 0+0*I;
		complex p2 = VIEWPORT_W+0*I;
		complex p3 = VIEWPORT_W+VIEWPORT_H*I;
		complex p4 = 0+VIEWPORT_H*I;

		create_enemy2c(p1, 500, Fairy, stage3_cornerfairy, p1 - offs - offs*I, p2 + offs - offs*I);
		create_enemy2c(p2, 500, Fairy, stage3_cornerfairy, p2 + offs - offs*I, p3 + offs + offs*I);
		create_enemy2c(p3, 500, Fairy, stage3_cornerfairy, p3 + offs + offs*I, p4 - offs + offs*I);
		create_enemy2c(p4, 500, Fairy, stage3_cornerfairy, p4 - offs + offs*I, p1 - offs - offs*I);
	}

	if(global.diff > D_Normal) AT(2490) {
		double offs = -50;

		complex p1 = VIEWPORT_W/2+0*I;
		complex p2 = VIEWPORT_W+VIEWPORT_H/2*I;
		complex p3 = VIEWPORT_W/2+VIEWPORT_H*I;
		complex p4 = 0+VIEWPORT_H/2*I;

		create_enemy2c(p1, 500, Fairy, stage3_cornerfairy, p1 - offs*I, p2 + offs);
		create_enemy2c(p2, 500, Fairy, stage3_cornerfairy, p2 + offs,   p3 + offs*I);
		create_enemy2c(p3, 500, Fairy, stage3_cornerfairy, p3 + offs*I, p4 - offs);
		create_enemy2c(p4, 500, Fairy, stage3_cornerfairy, p4 - offs,   p1 + offs*I);
	}

	int midboss_time = STAGE3_MIDBOSS_TIME;

	AT(2740) {
		global.boss = stage3_create_midboss();
	}

	AT(2741) {
		if(global.boss) {
			global.timer += global.frames - global.boss->birthtime - 1;
		}
	}

	FROM_TO(2741, 2740 + midboss_time - 60, 10) {
		if(_i&1) {
			create_enemy3c(-20 + (VIEWPORT_H-20)*I, 50, Swirl, stage3_bitchswirl, 5, -1*I, 25+0.95*I);
		} else {
			create_enemy3c(VIEWPORT_W+20 + (VIEWPORT_H-20)*I, 50, Swirl, stage3_bitchswirl, -5, -1*I, 25+0.95*I);
		}
	}

	FROM_TO(2740 + midboss_time, 3000 + midboss_time, 10+2*(D_Lunatic-global.diff)) {
		tsrand_fill(3);
		create_enemy2c(VIEWPORT_W/2 + 20 * anfrand(0) + (VIEWPORT_H/4 + 20 * anfrand(1))*I, 200, Swirl, stage3_enterswirl, I * 3 + anfrand(2) * 3, 1);
	}

	AT(3000 + midboss_time) {
		create_enemy2c(VIEWPORT_W - VIEWPORT_W/3 + (VIEWPORT_H/5)*I, 8000, BigFairy, stage3_bigfairy, 2, 480 - 30 * (D_Lunatic - global.diff));
	}

	if(global.diff > D_Easy) {
		FROM_TO(3000 + midboss_time, 3100 + midboss_time, 20+4*(D_Lunatic-global.diff)) {
			create_enemy3c(VIEWPORT_W-20 + (VIEWPORT_H+20)*I, 50, Swirl, stage3_bitchswirl, -5*I, 0, 0);
		}
	}

	AT(3500 + midboss_time) {
		create_enemy2c(VIEWPORT_W/3 + (VIEWPORT_H/5)*I, 8000, BigFairy, stage3_bigfairy, 2, 480 - 30 * (D_Lunatic - global.diff));
	}

	{
		int cnt = 4;
		int step = 60;
		FROM_TO(3500 + midboss_time, 3500 + cnt * step - 1 + midboss_time, step) {
			complex pos = VIEWPORT_W - VIEWPORT_W/3 + (VIEWPORT_H/5)*I;
			complex spos = creal(pos) - 20 * I;
			create_enemy3c(spos, 1000, Fairy, stage3_chargefairy, pos, 30, 2*I);
		}
	}

	{
		int cnt = 4;
		int step = 70;
		int start = 4000 + midboss_time;
		FROM_TO(start, start + step * cnt - 1, step) {
			int i = _i % cnt;
			double span = 300 - 60 * (1-i/2);
			complex pos1 = VIEWPORT_W/2;
			complex pos2 = VIEWPORT_W/2 + span * (-0.5 + (i&1)) + (VIEWPORT_H/3 + 100*(i/2))*I;
			create_enemy4c(pos1, 3000, BigFairy, stage3_burstfairy, pos2, i&1, 0,
				(start + 300 - global.timer) + I *
				(30)
			);
		}
	}

	FROM_TO(3700 + midboss_time, 3800 + midboss_time, 20+4*(D_Lunatic-global.diff)) {
		create_enemy3c(20 + (VIEWPORT_H+20)*I, 50, Swirl, stage3_bitchswirl, -5*I, 0, 0);
	}

	AT(4330 + midboss_time) {
		double offs = -50;

		complex p1 = 0+0*I;
		complex p2 = VIEWPORT_W+0*I;
		complex p3 = VIEWPORT_W+VIEWPORT_H*I;
		complex p4 = 0+VIEWPORT_H*I;

		create_enemy3c(p1, 500, Fairy, stage3_cornerfairy, p1 - offs - offs*I, p2 + offs - offs*I, 1);
		create_enemy3c(p2, 500, Fairy, stage3_cornerfairy, p2 + offs - offs*I, p3 + offs + offs*I, 1);
		create_enemy3c(p3, 500, Fairy, stage3_cornerfairy, p3 + offs + offs*I, p4 - offs + offs*I, 1);
		create_enemy3c(p4, 500, Fairy, stage3_cornerfairy, p4 - offs + offs*I, p1 - offs - offs*I, 1);
	}

	if(global.diff > D_Normal) AT(4480 + midboss_time) {
		double offs = -50;

		complex p1 = VIEWPORT_W/2+0*I;
		complex p2 = VIEWPORT_W+VIEWPORT_H/2*I;
		complex p3 = VIEWPORT_W/2+VIEWPORT_H*I;
		complex p4 = 0+VIEWPORT_H/2*I;

		create_enemy3c(p1, 500, Fairy, stage3_cornerfairy, p1 - offs*I, p2 + offs, 0);
		create_enemy3c(p2, 500, Fairy, stage3_cornerfairy, p2 + offs,   p3 + offs*I, 0);
		create_enemy3c(p3, 500, Fairy, stage3_cornerfairy, p3 + offs*I, p4 - offs, 0);
		create_enemy3c(p4, 500, Fairy, stage3_cornerfairy, p4 - offs,   p1 + offs*I, 0);
	}

	FROM_TO(4760 + midboss_time, 4940 + midboss_time, 10+2*(D_Lunatic-global.diff)) {
		create_enemy3c(VIEWPORT_W-20 - 20.0*I, 50, Swirl, stage3_bitchswirl, 5*I, 0, 0);
		create_enemy3c(20 + -20.0*I, 50, Swirl, stage3_bitchswirl, 5*I, 0, 0);
	}

	AT(5300 + midboss_time) {
		global.boss = stage3_create_boss();
	}
	AT(5400 + midboss_time) {
		global.dialog = stage3_post_dialog();
	}

	AT(5700 + midboss_time - FADE_TIME) {
		stage_finish(GAMEOVER_WIN);
	}
}
