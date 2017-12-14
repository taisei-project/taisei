/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "stage2_events.h"
#include "global.h"
#include "stage.h"
#include "enemy.h"

Dialog *stage2_dialog(void) {
	PlayerCharacter *pc = global.plr.mode->character;
	Dialog *d = create_dialog(pc->dialog_sprite_name, "dialog/hina");

	if(pc->id == PLR_CHAR_MARISA) {
		dadd_msg(d,Right, "I can’t let you pass any further than this. Please go back down the mountain.");
		dadd_msg(d,Left, "So, is that your misfortune talkin’?");
		dadd_msg(d,Right, "Exploring that tunnel is only going to lead you to ruin. I’ll protect you by driving you away!");
		dadd_msg(d,Left, "I can make dumb decisions on my own, thanks.");
		dadd_msg(d,Right, "A bad attitude like that is destined to be cursed from the beginning. I’ll send you packing home as a favor to you, so you don’t get hurt further by your terrible ideas!");
	} else if(pc->id == PLR_CHAR_YOUMU) {
		dadd_msg(d,Right, "I can’t let you pass any further than this. Please go back down the mountain.");
		dadd_msg(d,Left, "Are you a goddess? It’s nice of you to be looking out for me, but the Netherworld has been put at risk due to this incident.");
		dadd_msg(d,Left, "I have to keep going.");
		dadd_msg(d,Right, "The tunnel heads leads nowhere but to ruin. If you don’t return to the ghostly world where you come from, I’ll have to stop you by force!");
		dadd_msg(d,Left, "My mistress won’t like it if I tell her I was stopped by divine intervention. You’ll have to come up with another excuse.");
	}

	dadd_msg(d, BGM, "stage2boss");
	return d;
}

Dialog *stage2_post_dialog(void) {
	PlayerCharacter *pc = global.plr.mode->character;
	Dialog *d = create_dialog(pc->dialog_sprite_name, NULL);

	if(pc->id == PLR_CHAR_MARISA) {
		dadd_msg(d, Left, "Somehow I already feel luckier after beating ya. Fixin’ the border should be no sweat!");
	} else if(pc->id == PLR_CHAR_YOUMU) {
		dadd_msg(d, Left, "It’s strange, but fighting a god makes me feel like some of my burdens are lifted. I wonder if she decided to bless me after all?");
	}

	return d;
}

int stage2_great_circle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 5, Power, 4, NULL);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(50,70,1)
		e->args[0] *= 0.5;

	FROM_TO_SND("shot1_loop", 70, 190+global.diff*25, 5-global.diff/2) {
		int n, c = 5+2*(global.diff>D_Normal);

		const double partdist = 0.04*global.diff;
		const double bunchdist = 0.5;
		const int c2 = 5;


		for(n = 0; n < c; n++) {
			complex dir = cexp(I*(2*M_PI/c*n+partdist*(_i%c2-c2/2)+bunchdist*(_i/c2)));

			PROJECTILE("rice", e->pos+30*dir, rgb(0.6,0.0,0.3), asymptotic, {
				1.5*dir,
				_i%5
			});

			if(global.diff > D_Easy && _i%7 == 0) {
				play_sound("shot1");
				PROJECTILE("bigball", e->pos+30*dir, rgb(0.3,0.0,0.6), linear, {
					1.7*dir*cexp(0.3*I*frand())
				});
			}
		}
	}

	AT(210+global.diff*25) {
		e->hp = min(e->hp,200);
		e->args[0] = 2.0*I;
	}

	return 1;
}

int stage2_small_spin_circle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 2, NULL);
		return 1;
	}

	if(creal(e->args[0]) < 0)
		e->dir = 1;
	else
		e->dir = 0;

	e->pos += e->args[0];

	if(t < 100)
		e->args[0] += 0.0002*(VIEWPORT_W/2+I*VIEWPORT_H/2-e->pos);

	AT(50)
		e->pos0 = e->pos;

	FROM_TO(30,80+global.diff*5,5-global.diff/2) {
		if(_i % 2) {
			play_sound("shot1");
		}

		PROJECTILE("ball", e->pos, rgb(0.9,0.0,0.3), linear, {
			pow(global.diff,0.7)*(conj(e->pos-VIEWPORT_W/2)/100 + ((1-2*e->dir)+3.0*I))
		});
	}

	return 1;
}

int stage2_aim(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Power, 2, NULL);
		return 1;
	}

	if(t < 70)
		e->pos += e->args[0];
	if(t > 150)
		e->pos -= e->args[0];

	AT(90) {
		if(global.diff > D_Normal) {
			play_sound("shot1");
			PROJECTILE("plainball", e->pos, rgb(0.6,0.0,0.8), asymptotic, {
				5*cexp(I*carg(global.plr.pos-e->pos)),
				-1
			});

			PROJECTILE("plainball", e->pos, rgb(0.2,0.0,0.1), linear, {
				3*cexp(I*carg(global.plr.pos-e->pos))
			});
		}
	}

	return 1;
}

int stage2_sidebox_trail(Enemy *e, int t) { // creal(a[0]): velocity, cimag(a[0]): angle, a[1]: d angle/dt, a[2]: time of acceleration
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 1, Power, 1, NULL);
		return 1;
	}

	e->pos += creal(e->args[0])*cexp(I*cimag(e->args[0]));

	FROM_TO((int) creal(e->args[2]),(int) creal(e->args[2])+M_PI*0.5/fabs(creal(e->args[1])),1)
		e->args[0] += creal(e->args[1])*I;

	FROM_TO(10,200,30-global.diff*4) {
		play_sound_ex("shot1", 5, false);

		float f = 0;
		if(global.diff > D_Normal)
			f = 0.03*global.diff*frand();

		PROJECTILE("rice", e->pos, rgb(0.9,0.0,0.9), linear, { 3*cexp(I*(cimag(e->args[0])+f+0.5*M_PI)) });
		PROJECTILE("rice", e->pos, rgb(0.9,0.0,0.9), linear, { 3*cexp(I*(cimag(e->args[0])-f-0.5*M_PI)) });
	}

	return 1;
}

int stage2_flea(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 2, NULL);
		return 1;
	}

	e->pos += e->args[0]*(1-e->args[1]);

	FROM_TO(80,90,1)
		e->args[1] += 0.1;

	FROM_TO(200,205,1)
		e->args[1] -= 0.2;


	FROM_TO(10, 400, 30-global.diff*3-t/70) {
		if(global.diff == D_Easy) {
			play_sound("shot1");
			PROJECTILE("flea", e->pos, rgb(0.3,0.2,1), asymptotic, {
				1.5*cexp(2.0*I*M_PI*frand()),
				1.5
			});
		}
	}

	return 1;
}

int stage2_accel_circle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 1, Power, 3, NULL);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(60,250, 20+10*(global.diff<D_Hard)) {
		e->args[0] *= 0.5;

		int i;
		for(i = 0; i < 6; i++) {
			play_sound("redirect");
			PROJECTILE("ball", e->pos, rgb(0.6,0.1,0.2), accelerated, {
				1.5*cexp(2.0*I*M_PI/6*i)+cexp(I*carg(global.plr.pos - e->pos)),
				-0.02*cexp(I*(2*M_PI/6*i+0.02*frand()*global.diff))
			});
		}
	}

	if(t > 270)
		e->args[0] -= 0.01*I;

	return 1;
}

static void wriggle_ani_flyin(Boss *w) {
	aniplayer_queue_pro(&w->ani,1,3,3,120,0);
	aniplayer_queue_pro(&w->ani,1,3,21,0,7);
	aniplayer_queue_pro(&w->ani,1,3,33,0,3);
	aniplayer_queue_pro(&w->ani,1,3,12,0,0);
}

static void wriggle_intro_stage2(Boss *w, int t) {
	if(t < 0)
		return;
	w->pos = VIEWPORT_W/2 + 100.0*I + 300*(1.0-t/(4.0*FPS))*cexp(I*(3-t*0.04));
}

int wriggle_bug(Projectile *p, int t) {
	if(t < 0)
		return 1;

	p->pos += p->args[0];
	p->angle = carg(p->args[0]);

	if(global.boss && global.boss->current && !((global.frames - global.boss->current->starttime - 30) % 200)) {
		play_sound("redirect");
		p->args[0] *= cexp(I*(M_PI/3)*nfrand());
		PARTICLE("flare", p->pos, 0, timeout, { 15, 5 }, .draw_rule = GrowFade);
	}

	return 1;
}

void wriggle_small_storm(Boss *w, int time) {
	int t = time % 400;
	TIMER(&t);

	if(time < 0)
		return;

	FROM_TO_SND("shot1_loop", 0,400,5-global.diff) {
		PROJECTILE("rice", w->pos, rgb(1,0.5,0.2), wriggle_bug, { 2*cexp(I*_i*2*M_PI/20) });
		PROJECTILE("rice", w->pos, rgb(1,0.5,0.2), wriggle_bug, { 2*cexp(I*_i*2*M_PI/20+I*M_PI) });
	}

	GO_AT(w, 60, 120, 1)
	GO_AT(w, 180, 240, -1)

	if(!(t%200)) {
		int i;
		aniplayer_queue(&w->ani,1,0,0)->speed=4;
		play_sound("shot_special1");

		for(i = 0; i < 10+global.diff; i++) {
			PROJECTILE("bigball", w->pos, rgb(0.1,0.3,0.0), asymptotic, {
				2*cexp(I*i*2*M_PI/(10+global.diff)),
				2
			});
		}
	}
}

void wiggle_mid_flee(Boss *w, int t) {
	w->ani.stdrow = 1;
	if(t >= 0) {
		GO_TO(w, VIEWPORT_W/2 - 3.0 * t - 300 * I, 0.01)
	}
}

Boss *create_wriggle_mid(void) {
	Boss* wriggle = create_boss("Wriggle", "wriggle", NULL, VIEWPORT_W + 150 - 30.0*I);
	wriggle->glowcolor = rgba(0.2, 0.4, 0.5, 0.5);
	wriggle->shadowcolor = rgba(0.4, 0.2, 0.6, 0.5);
	wriggle_ani_flyin(wriggle);
	boss_add_attack(wriggle, AT_Move, "Introduction", 4, 0, wriggle_intro_stage2, NULL);
	boss_add_attack(wriggle, AT_Normal, "Small Bug Storm", 20, 26000, wriggle_small_storm, NULL);
	boss_add_attack(wriggle, AT_Move, "Flee", 5, 0, wiggle_mid_flee, NULL);;

	boss_start_attack(wriggle, wriggle->attacks);
	return wriggle;
}

void hina_intro(Boss *h, int time) {
	TIMER(&time);

	AT(100)
		global.dialog = stage2_dialog();

	aniplayer_queue(&h->ani,0,2,0);
	aniplayer_queue(&h->ani,1,0,0);
	GO_TO(h, VIEWPORT_W/2 + 100.0*I, 0.05);
}

void hina_cards1(Boss *h, int time) {
	int t = time % 500;
	TIMER(&t);

	if(time < 0)
		return;

	h->ani.stdrow = 1;
	FROM_TO(0, 500, 2-(global.diff > D_Normal)) {
		play_sound_ex("shot1", 4, false);
		PROJECTILE("card", h->pos+50*cexp(I*t/10), rgb(0.8,0.0,0.0), asymptotic, { (1.6+0.4*global.diff)*cexp(I*t/5.0), 3 });
		PROJECTILE("card", h->pos-50*cexp(I*t/10), rgb(0.0,0.0,0.8), asymptotic, {-(1.6+0.4*global.diff)*cexp(I*t/5.0), 3 });
	}
}

void hina_amulet(Boss *h, int time) {
	int t = time % 200;

	if(time < 0)
		return;

	if(time < 100)
		GO_TO(h, VIEWPORT_W/2 + 200.0*I, 0.02);

	TIMER(&t);

	complex d = global.plr.pos - h->pos;
	h->ani.stdrow = 0;
	FROM_TO_SND("shot1_loop", 0,200*(global.diff+0.5)/(D_Lunatic+0.5),1) {
		h->ani.stdrow = 1;
		float f = _i/30.0;
		complex n = cexp(I*2*M_PI*f+I*carg(d)+0.7*time/200*I)/sqrt(0.5+global.diff);

		float speed = 1.0 + 0.75 * max(0, (int)global.diff - D_Normal);
		float accel = 1.0 + 1.20 * max(0, (int)global.diff - D_Normal);

		complex p = h->pos+30*log(1+_i/2.0)*n;

		const char *t0 = "ball";
		const char *t1 = global.diff == D_Easy ? t0 : "crystal";

		PROJECTILE(t0, p, rgb(0.8, 0.0, 0.0), accelerated, { speed *  2*n*I, accel * -0.01*n });
		PROJECTILE(t1, p, rgb(0.8, 0.0, 0.5), accelerated, { speed * -2*n*I, accel * -0.01*n });
	}
}

void hina_cards2(Boss *h, int time) {
	int t = time % 500;
	TIMER(&t);

	if(time < 0)
		return;

	hina_cards1(h, time);

	GO_AT(h, 100, 200, 2);
	GO_AT(h, 260, 460, -2);
	GO_AT(h, 460, 500, 5);

	AT(100) {
		int i;
		for(i = 0; i < 30; i++) {
			play_sound("shot_special1");
			PROJECTILE("bigball", h->pos, rgb(0.7, 0, 0.7), asymptotic, {
				2*cexp(I*2*M_PI*i/20.0),
				3
			});
		}
	}
}

#define SLOTS 5

static int bad_pick_bullet(Projectile *p, int t) {
	if(t < 0)
		return 1;

	p->pos += p->args[0];
	p->args[0] += p->args[1];

	// reflection
	int slot = (int)(creal(p->pos)/(VIEWPORT_W/SLOTS)+1)-1; // correct rounding for slot == -1
	int targetslot = creal(p->args[2])+0.5;
	if(slot != targetslot)
		p->args[0] = copysign(creal(p->args[0]),targetslot-slot)+I*cimag(p->args[0]);

	return 1;
}

void hina_bad_pick(Boss *h, int time) {
	int t = time % 500;
	int i, j;

	TIMER(&t);

	if(time < 0)
		return;

	GO_TO(h, VIEWPORT_W/SLOTS*((113*(time/500))%SLOTS+0.5)+ 100.0*I, 0.02);

	FROM_TO(100, 500, 5) {
		play_sound_ex("shot1", 4, false);

		for(i = 1; i < SLOTS; i++) {
			PROJECTILE("crystal", VIEWPORT_W/SLOTS*i, rgb(0.5,0,0.6), linear, { 7.0*I });
		}

		if(global.diff >= D_Hard) {
			double shift = 0;
			if(global.diff == D_Lunatic)
				shift = 0.3*max(0,t-200);
			for(i = 1; i < SLOTS; i++) {
				double height = VIEWPORT_H/SLOTS*i+shift;
				if(height > VIEWPORT_H-40)
					height -= VIEWPORT_H-40;
				PROJECTILE("crystal", (i&1)*VIEWPORT_W+I*height, rgb(0.5,0,0.6), linear, { 5.0*(1-2*(i&1)) });
			}
		}
	}

	AT(200) {
		aniplayer_queue(&h->ani,1,1,0);
		play_sound("shot_special1");

		int win = tsrand()%SLOTS;
		for(i = 0; i < SLOTS; i++) {
			if(i == win)
				continue;

			float cnt = (1+min(D_Normal,global.diff)) * 5;
			for(j = 0; j < cnt; j++) {
				complex o = VIEWPORT_W/SLOTS*(i + j/(cnt-1));

				PROJECTILE("ball", o, rgb(0.7,0,0.0), bad_pick_bullet,
					.args = {
						0,
						0.005*nfrand() + 0.005*I * (1 + psin(i + j + global.frames)),
						i
					},
					.flags = PFLAG_DRAWADD,
				);
			}
		}
	}
}

#undef SLOTS

void hina_wheel(Boss *h, int time) {
	int t = time % 400;
	TIMER(&t);

	static int dir = 0;

	if(time < 0)
		return;

	GO_TO(h, VIEWPORT_W/2+VIEWPORT_H/2*I, 0.02);

	h->ani.stdrow = 1;
	if(time < 60) {
		if(time == 0) {
			if(global.diff > D_Normal) {
				dir = 1 - 2 * (tsrand()%2);
			} else {
				dir = 1;
			}
		}

		return;
	}

	FROM_TO_SND("shot1_loop", 0, 400, 5-(int)min(global.diff, D_Normal)) {
		int i;
		float speed = 10;
		if(time > 500)
			speed = 1+9*exp(-(time-500)/100.0);

		float d = max(0, (int)global.diff - D_Normal);

		for(i = 1; i < 6+d; i++) {
			float a = dir * 2*M_PI/(5+d)*(i+(1 + 0.4 * d)*time/100.0+(1 + 0.2 * d)*frand()*time/1700.0);
			PROJECTILE("crystal", h->pos, rgb(log(1+time*1e-3),0,0.2), linear, { speed*cexp(I*a) });
		}
	}
}

static int timeout_deadproj_linear(Projectile *p, int time) {
	if(time > creal(p->args[0]))
		p->type = DeadProj;
	p->pos += p->args[1];
	p->angle = carg(p->args[1]);
	return 1;
}

int hina_monty_slave(Enemy *s, int time) {
	if(time < 0) {
		return 1;
	}

	if(time > 60 && time < 720-140 + 20*(global.diff-D_Lunatic) && !(time % (int)(max(2 + (global.diff < D_Normal), (120 - 0.5 * time))))) {
		play_loop("shot1_loop");

		PROJECTILE("crystal", s->pos,
			.color = rgb(0.5 + 0.5 * psin(time*0.2), 0.3, 1.0 - 0.5 * psin(time*0.2)),
			.rule = asymptotic,
			.args = {
				5*I + 1 * (sin(time) + I * cos(time)),
				4
			}
		);

		if(global.diff > D_Easy) {
			PROJECTILE("crystal", s->pos,
				.color = rgb(0.5 + 0.5 * psin(time*0.2), 0.3, 1.0 - 0.5 * psin(time*0.2)),
				.rule = timeout_deadproj_linear,
				.args = {
					500,
					-0.5*I + 1 * (sin(time) + I * cos(time))
				}
			);
		}
	}

	return 1;
}

void hina_monty_slave_visual(Enemy *s, int time, bool render) {
	Swirl(s, time, render);

	if(!render) {
		return;
	}

	Texture *soul = get_tex("proj/soul");
	Shader *shader = recolor_get_shader();
	double scale = fabs(swing(clamp(time / 60.0, 0, 1), 3)) * 1.25;

	ColorTransform ct;

	Color clr1 = rgba(1.0, 0.0, 0.0, 1.0);
	Color clr2 = rgba(0.0, 0.0, 1.0, 1.0);
	Color clr3 = rgba(psin(time*0.05), 0.0, 1.0 - psin(time*0.05), 1.0);

	glUseProgram(shader->prog);

	glPushMatrix();
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);

	glTranslatef(creal(s->pos), cimag(s->pos), 0);

	static_clrtransform_bullet(clr1, &ct);
	recolor_apply_transform(&ct);
	glPushMatrix();
	glRotatef(time, 0, 0, 1);
	glScalef(scale * (0.6 + 0.6 * psin(time*0.1)), scale * (0.7 + 0.5 * psin(time*0.1 + M_PI)), 0);
	draw_texture_p(0, 0, soul);
	glPopMatrix();

	static_clrtransform_bullet(clr2, &ct);
	recolor_apply_transform(&ct);
	glPushMatrix();
	glRotatef(time, 0, 0, 1);
	glScalef(scale * (0.7 + 0.5 * psin(time*0.1 + M_PI)), scale * (0.6 + 0.6 * psin(time*0.1)), 0);
	draw_texture_p(0, 0, soul);
	glPopMatrix();

	static_clrtransform_bullet(clr3, &ct);
	recolor_apply_transform(&ct);
	glPushMatrix();
	glRotatef(-time, 0, 0, 1);
	// glScalef(scale * (0.7 + 0.5 * psin(time*0.1 + M_PI)), scale * (0.6 + 0.6 * psin(time*0.1)), 0);
	glScalef(scale, scale, 0);
	draw_texture_p(0, 0, soul);
	glPopMatrix();

	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glPopMatrix();

	glUseProgram(0);
}

void hina_monty(Boss *h, int time) {
	int t = time % 720;
	TIMER(&t);

	static short slave_pos, bad_pos, good_pos, plr_pos;
	static int cwidth = VIEWPORT_W / 3.0;
	static complex targetpos;

	if(time == EVENT_DEATH) {
		killall(global.enemies);
		return;
	}

	if(time < 0) {
		targetpos = VIEWPORT_W/2.0 + VIEWPORT_H/2.0 * I;
		return;
	}

	AT(0) {
		plr_pos = creal(global.plr.pos) / cwidth;
		bad_pos = tsrand() % 3;
		do good_pos = tsrand() % 3; while(good_pos == bad_pos);

		play_sound("laser1");

		for(int i = 0; i < 2; ++i) {
			int x = cwidth * (1 + i);
			create_laserline_ab(x, x + VIEWPORT_H*I, 15, 30, 60, rgb(0.3, 1.0, 1.0));
			create_laserline_ab(x, x + VIEWPORT_H*I, 20, 240, 600, rgb(1.0, 0.3, 1.0));
		}

		if(global.enemies) {
			global.enemies->hp = 0;
		}
	}

	AT(120) {
		do slave_pos = tsrand() % 3; while(slave_pos == plr_pos || slave_pos == good_pos);
		while(bad_pos == slave_pos || bad_pos == good_pos) bad_pos = tsrand() % 3;

		complex o = cwidth * (0.5 + slave_pos) + VIEWPORT_H/2.0*I - 200.0*I;

		play_sound("laser1");
		create_laserline_ab(h->pos, o, 15, 30, 60, rgb(1.0, 0.3, 0.3));
		aniplayer_queue(&h->ani,1,0,0);
	}

	AT(140) {
		play_sound("shot_special1");
		create_enemy4c(cwidth * (0.5 + slave_pos) + VIEWPORT_H/2.0*I - 200.0*I, ENEMY_IMMUNE, hina_monty_slave_visual, hina_monty_slave, 0, 0, 0, 1);
	}

	AT(190) {
		targetpos = cwidth * (0.5 + good_pos) + VIEWPORT_H/2.0*I - 200.0*I;
	}

	AT(240) {
		// main laser barrier activation
		play_sound("laser1");
	}

	FROM_TO(220, 360 + 60 * max(0, (double)global.diff - D_Easy), 60) {
		play_sound("shot_special1");

		float cnt = (2.0+global.diff) * 5;
		for(int i = 0; i < cnt; i++) {
			bool top = ((global.diff > D_Hard) && (_i % 2));
			complex o = !top*VIEWPORT_H*I + cwidth*(bad_pos + i/(double)(cnt - 1));

			PROJECTILE("ball", o,
				.color = top ? rgb(0, 0, 0.7) : rgb(0.7, 0, 0),
				.rule = accelerated,
				.args = {
					0,
					(top ? -0.5 : 1) * 0.004 * (sin((M_PI * 4 * i / (cnt - 1)))*0.1*global.diff - I*(1 + psin(i + global.frames)))
				},
				.flags = PFLAG_DRAWADD,
			);
		}
	}

	{
		const int step = 2;
		const int cnt = 10;
		const int burst_dur = (cnt - 1) * step;
		const int cycle_dur = burst_dur+10*(D_Hard-global.diff);
		const int start = 210;
		const int end = 540;
		const int ncycles = (end - start) / cycle_dur;

		AT(start)
			h->ani.stdrow = 1;
		AT(end)
			h->ani.stdrow = 0;
		FROM_TO_INT(start, start + cycle_dur * ncycles - 1, cycle_dur, burst_dur, step) {
			play_sound("shot1");

			double p = _ni / (double)(cnt-1);
			double c = p;
			double m = 0.60 + 0.01 * global.diff;

			p *= m;
			if(_i % 2) {
				p = 1.0 - p;
			}

			complex o = cwidth * (p + 0.5/(cnt-1) - 0.5) + h->pos;

			if(global.diff > D_Normal) {
				PROJECTILE("card", o, rgb(c * 0.8, 0, (1 - c) * 0.8), accelerated, { -2.5*I, 0.05*I });
			} else {
				PROJECTILE("card", o, rgb(c * 0.8, 0, (1 - c) * 0.8), linear, { 2.5*I });
			}
		}
	}

	FROM_TO(240, 390, 5) {
		create_item(VIEWPORT_H*I + cwidth*(good_pos + frand()), -50.0*I, _i % 2 ? Point : Power);
	}

	AT(600) {
		targetpos = cwidth * (0.5 + slave_pos) + VIEWPORT_H/2.0*I;
	}

	GO_TO(h, targetpos, 0.06);
}

void hina_spell_bg(Boss *h, int time) {

	glPushMatrix();
	glTranslatef(VIEWPORT_W/2, VIEWPORT_H/2,0);
	glPushMatrix();
	glScalef(0.6,0.6,1);
	draw_texture(0, 0, "stage2/spellbg1");
	glPopMatrix();
	glBlendFunc(GL_ZERO, GL_SRC_COLOR);
	glRotatef(time*5, 0,0,1);
	draw_texture(0, 0, "stage2/spellbg2");
	glPopMatrix();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	play_animation(get_ani("fire"),creal(h->pos), cimag(h->pos), 0);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}

Boss* stage2_spawn_hina(complex pos) {
	Boss *hina = create_boss("Kagiyama Hina", "hina", "dialog/hina", pos);
	hina->glowcolor = rgba(0.7, 0.2, 0.3, 0.5);
	hina->shadowcolor = hina->glowcolor;
	return hina;
}

Boss *create_hina(void) {
	Boss *hina = stage2_spawn_hina(VIEWPORT_W + 150 + 100.0*I);

	boss_add_attack(hina, AT_Move, "Introduction", 2, 0, hina_intro, NULL);
	boss_add_attack(hina, AT_Normal, "Cards1", 20, 25000, hina_cards1, NULL);
	boss_add_attack_from_info(hina, &stage2_spells.boss.amulet_of_harm, false);
	boss_add_attack(hina, AT_Normal, "Cards2", 17, 30000, hina_cards2, NULL);
	boss_add_attack_from_info(hina, &stage2_spells.boss.bad_pick, false);

	if(global.diff < D_Hard) {
		boss_add_attack_from_info(hina, &stage2_spells.boss.wheel_of_fortune_easy, false);
	} else {
		boss_add_attack_from_info(hina, &stage2_spells.boss.wheel_of_fortune_hard, false);
	}

	boss_add_attack_from_info(hina, &stage2_spells.extra.monty_hall_danmaku, false);

	boss_start_attack(hina, hina->attacks);
	return hina;
}

void stage2_events(void) {
	TIMER(&global.timer);

	AT(0) {
		stage_start_bgm("stage2");
	}

	AT(300) {
		create_enemy1c(VIEWPORT_W/2-10.0*I, 7500, BigFairy, stage2_great_circle, 2.0*I);
	}

	FROM_TO(650-50*global.diff, 750+25*(4-global.diff), 50) {
		create_enemy1c(VIEWPORT_W*((_i)%2), 1000, Fairy, stage2_small_spin_circle, 2-4*(_i%2)+1.0*I);
	}

	FROM_TO(850, 1000, 15)
		create_enemy1c(VIEWPORT_W/2+25*(_i-5)-20.0*I, 200, Fairy, stage2_aim, (2+frand()*0.3)*I);

	FROM_TO(960, 1200, 20)
		create_enemy3c(VIEWPORT_W-80+(VIEWPORT_H+20)*I, 200, Fairy, stage2_sidebox_trail, 3 - 0.5*M_PI*I, -0.02, 90);

	FROM_TO(1140, 1400, 20)
		create_enemy3c(200-20.0*I, 200, Fairy, stage2_sidebox_trail, 3+0.5*I*M_PI, -0.05, 70);

	AT(1300)
		create_enemy1c(150-10.0*I, 4000, BigFairy, stage2_great_circle, 2.5*I);

	AT(1500)
		create_enemy1c(VIEWPORT_W-150-10.0*I, 4000, BigFairy, stage2_great_circle, 2.5*I);

	FROM_TO(1700, 2000, 30)
		create_enemy1c(VIEWPORT_W*frand()-20.0*I, 200, Fairy, stage2_flea, 1.7*I);

	if(global.diff > D_Easy) {
		FROM_TO(1950, 2500, 60) {
			create_enemy3c(VIEWPORT_W-40+(VIEWPORT_H+20)*I, 200, Fairy, stage2_sidebox_trail, 5 - 0.5*M_PI*I, -0.02, 83-global.diff*3);
			create_enemy3c(40+(VIEWPORT_H+20)*I, 200, Fairy, stage2_sidebox_trail, 5 - 0.5*M_PI*I, 0.02, 80-global.diff*3);
		}
	}

	AT(2300) {
		create_enemy1c(VIEWPORT_W/4-10.0*I, 2000, Fairy, stage2_accel_circle, 2.0*I);
		create_enemy1c(VIEWPORT_W/4*3-10.0*I, 2000, Fairy, stage2_accel_circle, 2.0*I);
	}

	AT(2800)
		global.boss = create_wriggle_mid();

	FROM_TO(2900, 3400, 50) {
		if(global.diff > D_Normal)
			create_enemy3c(VIEWPORT_W-80+(VIEWPORT_H+20)*I, 200, Fairy, stage2_sidebox_trail, 3 - 0.5*M_PI*I, -0.02, 90);
		create_enemy3c(80+(VIEWPORT_H+20)*I, 200, Fairy, stage2_sidebox_trail, 3 - 0.5*M_PI*I, 0.02, 90);
	}

	FROM_TO(3000, 3600, 300) {
		create_enemy1c(VIEWPORT_W/2-60*(_i-1)-10.0*I, 7000+500*global.diff, BigFairy, stage2_great_circle, 2.0*I);
	}

	FROM_TO(3700, 4500, 40)
		create_enemy1c(VIEWPORT_W*frand()-10.0*I, 150, Fairy, stage2_flea, 2.5*I);

	FROM_TO(4000, 4600, 100+100*(global.diff<D_Hard))
		create_enemy1c(VIEWPORT_W/2+100-200*frand()-10.0*I, 2000, Fairy, stage2_accel_circle, 2.0*I);

	AT(5100) {
		global.boss = create_hina();
	}

	AT(5180) {
		global.dialog = stage2_post_dialog();
	}

	AT(5340 - FADE_TIME) {
		stage_finish(GAMEOVER_WIN);
	}
}
