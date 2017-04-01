/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage2_events.h"
#include "global.h"
#include "stage.h"
#include "enemy.h"

void hina_spell_bg(Boss*, int);
void hina_amulet(Boss*, int);
void hina_bad_pick(Boss*, int);
void hina_wheel(Boss*, int);
void hina_monty(Boss*, int);

/*
 *	See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 */

AttackInfo stage2_spells[] = {
	{{ 0,  1,  2,  3},	AT_Spellcard, "Shard ~ Amulet of Harm", 26, 25000,
							hina_amulet, hina_spell_bg, BOSS_DEFAULT_GO_POS},
	{{ 4,  5,  6,  7},	AT_Spellcard, "Lottery Sign ~ Bad Pick", 30, 36000,
							hina_bad_pick, hina_spell_bg, BOSS_DEFAULT_GO_POS},
	{{ 8,  9, 10, 11},	AT_Spellcard, "Lottery Sign ~ Wheel of Fortune", 20, 36000,
							hina_wheel, hina_spell_bg, BOSS_DEFAULT_GO_POS},
	{{ 0,  1,  2,  3},	AT_ExtraSpell, "Lottery Sign ~ Monty Hall Danmaku", 60, 60000,
							hina_monty, hina_spell_bg, BOSS_DEFAULT_GO_POS},

	{{0}}
};

Dialog *stage2_dialog(void) {
	Dialog *d = create_dialog(global.plr.cha == Marisa ? "dialog/marisa" : "dialog/youmu", "dialog/hina");

	if(global.plr.cha == Marisa) {
		dadd_msg(d, Left, "Ha! What are you doing here?\nYou the culprit?");
		dadd_msg(d, Right, "Huh? No, you? Everone is going crazy, you know?\nThat’s why I’m here.");
		dadd_msg(d, Left, "Why? What happened?");
		dadd_msg(d, Right, "The border has been broken.");
		dadd_msg(d, Left, "Is that even possible?!");
		dadd_msg(d, Right, "Look, there is a way outside\nright behind us.");
		dadd_msg(d, Left, "But I’ve got a feeling that you\nwon’t let me pass, haha!");
	} else {
		dadd_msg(d, Left, "This must be the place…");
		dadd_msg(d, Right, "Hello? ");
		dadd_msg(d, Left, "You came here because of the\n“crack”, too? Where is it?");
		dadd_msg(d, Right, "Right behind us, but…");
		dadd_msg(d, Left, "Okay, if you’ll excuse me…");
		dadd_msg(d, Right, "No! Don’t make it more\ntroubling than it already is!");
	}

	dadd_msg(d, BGM, "bgm_stage2boss");
	return d;
}

Dialog *stage2_post_dialog(void) {
	Dialog *d = create_dialog(global.plr.cha == Marisa ? "dialog/marisa" : "dialog/youmu", NULL);

	dadd_msg(d, Right, "Well, let’s go then.");

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

	FROM_TO(70, 190+global.diff*25, 5-global.diff/2) {
		int n, c = 7;

		for(n = 0; n < c; n++) {
			complex dir = cexp(I*(2*M_PI/c*n+0.0001*(_i%5-3)+0.5*_i/5));
			create_projectile2c("rice", e->pos+30*dir, rgb(0.6,0.0,0.3), asymptotic, 1.5*dir, _i%5);

			if(global.diff > D_Easy && _i%7 == 0)
				create_projectile1c("bigball", e->pos+30*dir, rgb(0.3,0.0,0.6), linear, 1.7*dir*cexp(0.3*I*frand()));
		}
	}

	AT(210+global.diff*25) {
		e->args[0] = 2.0*I;
	}

	return 1;
}

int spin_circle(Projectile *p, int t) { // a[0]: angular velocity, a[1]: center, a[2]: center speed
	if(t < 0)
		return 1;

	p->pos += p->args[0]*cimag(p->args[1]-p->pos) - p->args[0]*creal(p->args[1]-p->pos)*I;

	p->args[1] += p->args[2];

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

	FROM_TO(50,80+global.diff*5,5)
		create_projectile3c("ball", e->pos, rgb(0.9,0.0,0.3), spin_circle, 0.02 - 0.04*(!e->dir), e->pos0 + _i*10*((1-2*e->dir)+1.0*I), (1-2*e->dir)+1.0*I);

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
			create_projectile2c("plainball", e->pos, rgb(0.6,0.0,0.8), asymptotic, 5*cexp(I*carg(global.plr.pos-e->pos)), -1);
			create_projectile1c("plainball", e->pos, rgb(0.2,0.0,0.1), linear, 3*cexp(I*carg(global.plr.pos-e->pos)));
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
		float f = 0;
		if(global.diff > D_Normal)
			f = 0.03*global.diff*frand();

		create_projectile1c("rice", e->pos, rgb(0.9,0.0,0.9), linear, 3*cexp(I*(cimag(e->args[0])+f+0.5*M_PI)));
		create_projectile1c("rice", e->pos, rgb(0.9,0.0,0.9), linear, 3*cexp(I*(cimag(e->args[0])-f-0.5*M_PI)));
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


	FROM_TO(10, 400, 20-global.diff*2-t/70) {
		create_projectile2c("flea", e->pos, rgb(0.2,0.2,1), asymptotic, 1.5*cexp(2.0*I*M_PI*frand()), 1.5);
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

	FROM_TO(60,250, 10) {
		e->args[0] *= 0.5;

		int i;
		for(i = 0; i < 6; i++) {
			create_projectile2c("ball", e->pos, rgb(0.6,0.1,0.2), accelerated, 1.5*cexp(2.0*I*M_PI/6*i)+cexp(I*carg(global.plr.pos - e->pos)), -0.02*cexp(I*(2*M_PI/6*i+0.02*frand()*global.diff)));
		}
	}

	if(t > 270)
		e->args[0] -= 0.01*I;

	return 1;
}

void wriggle_intro(Boss *w, int t) {
	if(t != EVENT_DEATH)
		w->pos = VIEWPORT_W/2 + 100.0*I + 400*(1.0-t/(4.0*FPS))*cexp(I*(3-t*0.04));
}

int wriggle_bug(Projectile *p, int t) {
	if(t < 0)
		return 1;

	p->pos += p->args[0];
	p->angle = carg(p->args[0]);


	if(t > 70 && frand() < 0.01)
		p->args[0] *= cexp(I*M_PI/3);

	return 1;
}

void wriggle_small_storm(Boss *w, int time) {
	int t = time % 400;
	TIMER(&t);

	if(time < 0)
		return;

	FROM_TO(0,400,5-global.diff) {
		create_projectile1c("rice", w->pos, rgb(1,0.5,0.2), wriggle_bug, 2*cexp(I*_i*2*M_PI/20));
		create_projectile1c("rice", w->pos, rgb(1,0.5,0.2), wriggle_bug, 2*cexp(I*_i*2*M_PI/20+I*M_PI));
	}

	GO_AT(w, 60, 120, 1)
	GO_AT(w, 180, 240, -1)

	if(!(t%200)) {
		int i;
		for(i = 0; i < 10+global.diff; i++)
			create_projectile2c("bigball", w->pos, rgb(0.1,0.3,0.0), asymptotic, 2*cexp(I*i*2*M_PI/(10+global.diff)), 2);
	}
}

Boss *create_wriggle_mid(void) {
	Boss* wriggle = create_boss("Wriggle", "wriggle", "dialog/wriggle", VIEWPORT_W + 150 - 30.0*I);
	boss_add_attack(wriggle, AT_Move, "Introduction", 4, 0, wriggle_intro, NULL);
	boss_add_attack(wriggle, AT_Normal, "Small Bug Storm", 20, 20000, wriggle_small_storm, NULL);

	start_attack(wriggle, wriggle->attacks);
	return wriggle;
}

void hina_intro(Boss *h, int time) {
	TIMER(&time);

	AT(100)
		global.dialog = stage2_dialog();

	GO_TO(h, VIEWPORT_W/2 + 100.0*I, 0.05);
}

void hina_cards1(Boss *h, int time) {
	int t = time % 500;
	TIMER(&t);

	if(time < 0)
		return;

	FROM_TO(0, 500, 2-(global.diff > D_Normal)) {
		create_projectile2c("card", h->pos+50*cexp(I*t/10), rgb(0.8,0.0,0.0),  asymptotic, (1.6+0.4*global.diff)*cexp(I*t/5.0), 3);
		create_projectile2c("card", h->pos-50*cexp(I*t/10), rgb(0.0,0.0,0.8),  asymptotic, -(1.6+0.4*global.diff)*cexp(I*t/5.0), 3);
	}
}

void hina_amulet(Boss *h, int time) {
	int t = time % 200;

	if(time < 0)
		return;

	if(time < 100)
		GO_TO(h, VIEWPORT_W/2 + 200.0*I, 0.02);

	TIMER(&t);

	FROM_TO(0,30*global.diff,1) {
			float f = _i/30.0;
			complex n = cexp(I*2*M_PI*f+0.7*time/200*I);

			create_projectile2c("crystal", h->pos+30*log(1+_i/2.0)*n, rgb(0.8,0,0), accelerated, 2*n*I, -0.01*n);
			create_projectile2c("crystal", h->pos+30*log(1+_i/2.0)*n, rgb(0.8,0,0.5), accelerated, -2*n*I, -0.01*n);
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
			create_projectile2c("bigball", h->pos, rgb(0.7, 0, 0.7), asymptotic, 2*cexp(I*2*M_PI*i/20.0), 3);
		}
	}
}

void hina_bad_pick(Boss *h, int time) {
	int t = time % 400;
	int i, j;

	TIMER(&t);

	if(time < 0)
		return;

	GO_TO(h, VIEWPORT_W/5*(time/400+0.6)+ 100.0*I, 0.02);

	FROM_TO(100, 500, 5) {

		for(i = 1; i < 5; i++) {
			create_projectile1c("crystal", VIEWPORT_W/5*i, rgb(0.2,0,0.2), linear, 7.0*I);
		}
	}

	AT(200) {
		int win = tsrand()%5;
		for(i = 0; i < 5; i++) {
			if(i == win)
				continue;

			float cnt = (2.0+global.diff) * 5;
			for(j = 0; j < cnt; j++) {
				complex o = VIEWPORT_W/5*(i + j/(cnt-1));
				create_projectile2c("ball", o, rgb(0.7,0,0.0), accelerated, 0, 0.005*nfrand() + 0.01*I * (1 + (1 * psin(i + j + global.frames))))->draw = ProjDrawAdd;
			}
		}
	}
}

void hina_wheel(Boss *h, int time) {
	int t = time % 400;
	TIMER(&t);

	if(time < 0)
		return;

	GO_TO(h, VIEWPORT_W/2+VIEWPORT_H/2*I, 0.02);

	if(time < 60)
		return;

	FROM_TO(0, 400, 5-global.diff) {
		int i;
		float speed = 10;
		if(time > 500)
			speed = 1+9*exp(-(time-500)/100.0);

		for(i = 1; i < 6; i++) {
			create_projectile1c("crystal", h->pos, rgb(log(1+time*1e-3),0,0.2), linear, speed*cexp(I*2*M_PI/5*(i+time/100.0+frand()*time/1700.0)));
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
		create_projectile2c("crystal", s->pos, rgb(0.5 + 0.5 * psin(time*0.2), 0.3, 1.0 - 0.5 * psin(time*0.2)),
							asymptotic, 5*I + 1 * (sin(time) + I * cos(time)), 4);

		if(global.diff > D_Easy) {
			create_projectile2c("crystal", s->pos, rgb(0.5 + 0.5 * psin(time*0.2), 0.3, 1.0 - 0.5 * psin(time*0.2)),
								timeout_deadproj_linear, 500, -0.5*I + 1 * (sin(time) + I * cos(time)));
		}
	}

	return 1;
}

void hina_monty_slave_draw(Enemy *s, int time) {
	Swirl(s, time);

	Texture *soul = get_tex("proj/soul");
	Shader *shader = get_shader("bullet_color");
	double scale = fabs(swing(clamp(time / 60.0, 0, 1), 3)) * 1.25;

	float clr1[] = {1.0, 0.0, 0.0, 1.0};
	float clr2[] = {0.0, 0.0, 1.0, 1.0};
	float clr3[] = {psin(time*0.05), 0.0, 1.0 - psin(time*0.05), 1.0};

	glUseProgram(shader->prog);

	glPushMatrix();
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);

	glTranslatef(creal(s->pos), cimag(s->pos), 0);

	glUniform4fv(uniloc(shader, "color"), 1, clr1);
	glPushMatrix();
	glRotatef(time, 0, 0, 1);
	glScalef(scale * (0.6 + 0.6 * psin(time*0.1)), scale * (0.7 + 0.5 * psin(time*0.1 + M_PI)), 0);
	draw_texture_p(0, 0, soul);
	glPopMatrix();

	glUniform4fv(uniloc(shader, "color"), 1, clr2);
	glPushMatrix();
	glRotatef(time, 0, 0, 1);
	glScalef(scale * (0.7 + 0.5 * psin(time*0.1 + M_PI)), scale * (0.6 + 0.6 * psin(time*0.1)), 0);
	draw_texture_p(0, 0, soul);
	glPopMatrix();

	glUniform4fv(uniloc(shader, "color"), 1, clr3);
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

		for(int i = 0; i < 2; ++i) {
			int x = cwidth * (1 + i);
			create_laserline_ab(x, x + VIEWPORT_H*I, 15, 30, 60, rgb(0.3, 1.0, 1.0));
			create_laserline_ab(x, x + VIEWPORT_H*I, 20, 240, 600, rgb(1.0, 0.3, 1.0));
		}

		if(global.enemies)
			global.enemies->hp = 0;
	}

	AT(120) {
		do slave_pos = tsrand() % 3; while(slave_pos == plr_pos || slave_pos == good_pos);
		while(bad_pos == slave_pos || bad_pos == good_pos) bad_pos = tsrand() % 3;

		complex o = cwidth * (0.5 + slave_pos) + VIEWPORT_H/2.0*I - 200.0*I;

		create_laserline_ab(h->pos, o, 15, 30, 60, rgb(1.0, 0.3, 0.3));
	}

	AT(140) {
		create_enemy4c(cwidth * (0.5 + slave_pos) + VIEWPORT_H/2.0*I - 200.0*I, ENEMY_IMMUNE, hina_monty_slave_draw, hina_monty_slave, 0, 0, 0, 1);
	}

	AT(190) {
		targetpos = cwidth * (0.5 + good_pos) + VIEWPORT_H/2.0*I - 200.0*I;
	}

	FROM_TO(220, 360 + 60 * max(0, (double)global.diff - D_Easy), 60) {
		float cnt = (2.0+global.diff) * 5;
		for(int i = 0; i < cnt; i++) {
			bool top = ((global.diff > D_Hard) && (_i % 2));
			complex o = !top*VIEWPORT_H*I + cwidth*(bad_pos + i/(double)(cnt - 1));
			create_projectile2c("ball", o, top ? rgb(0, 0, 0.7) : rgb(0.7, 0, 0), accelerated, 0,
				(top ? -0.5 : 1) * 0.004 * (sin((M_PI * 4 * i / (cnt - 1)))*0.1*global.diff - I*(1 + psin(i + global.frames)))
			)->draw = ProjDrawAdd;
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

		FROM_TO_INT(start, start + cycle_dur * ncycles - 1, cycle_dur, burst_dur, step) {
			double p = _ni / (double)(cnt-1);
			double c = p;
			double m = 0.60 + 0.01 * global.diff;

			p *= m;
			if(_i % 2) {
				p = 1.0 - p;
			}

			complex o = cwidth * (p + 0.5/(cnt-1) - 0.5) + h->pos;
			if(global.diff > D_Normal)
				create_projectile2c("card", o, rgb(c * 0.8, 0, (1 - c) * 0.8), accelerated, -2.5*I, 0.05*I);
			else
				create_projectile1c("card", o, rgb(c * 0.8, 0, (1 - c) * 0.8), linear, 2.5*I);
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
	draw_animation(creal(h->pos), cimag(h->pos), 0, "fire");

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}

Boss *create_hina(void) {
	Boss* hina = create_boss("Kagiyama Hina", "hina", "dialog/hina", VIEWPORT_W + 150 + 100.0*I);
	boss_add_attack(hina, AT_Move, "Introduction", 2, 0, hina_intro, NULL);
	boss_add_attack(hina, AT_Normal, "Cards1", 20, 15000, hina_cards1, NULL);
	boss_add_attack_from_info(hina, stage2_spells+0, false);
	boss_add_attack(hina, AT_Normal, "Cards2", 17, 15000, hina_cards2, NULL);
	boss_add_attack_from_info(hina, stage2_spells+1, false);
	boss_add_attack_from_info(hina, stage2_spells+2, false);

	start_attack(hina, hina->attacks);
	return hina;
}

void stage2_events(void) {
	TIMER(&global.timer);

	AT(0) {
		start_bgm("bgm_stage2");
	}

	AT(300) {
		create_enemy1c(VIEWPORT_W/2-10.0*I, 7000+500*global.diff, BigFairy, stage2_great_circle, 2.0*I);
	}

	FROM_TO(650-50*global.diff, 750+25*(4-global.diff), 50) {
		create_enemy1c(VIEWPORT_W*((_i)%2)+50.0*I, 2000, Fairy, stage2_small_spin_circle, 2-4*(_i%2)+1.0*I);
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

	FROM_TO(1950, 2500, 60) {
		create_enemy3c(VIEWPORT_W-40+(VIEWPORT_H+20)*I, 200, Fairy, stage2_sidebox_trail, 5 - 0.5*M_PI*I, -0.02, 83-global.diff*3);
		create_enemy3c(40+(VIEWPORT_H+20)*I, 200, Fairy, stage2_sidebox_trail, 5 - 0.5*M_PI*I, 0.02, 80-global.diff*3);
	}

	AT(2500) {
		create_enemy1c(VIEWPORT_W/4-10.0*I, 2000, Fairy, stage2_accel_circle, 2.0*I);
		create_enemy1c(VIEWPORT_W/4*3-10.0*I, 2000, Fairy, stage2_accel_circle, 2.0*I);
	}

	AT(2800)
		global.boss = create_wriggle_mid();

	FROM_TO(2900, 3400, 50) {
		create_enemy3c(VIEWPORT_W-80+(VIEWPORT_H+20)*I, 200, Fairy, stage2_sidebox_trail, 3 - 0.5*M_PI*I, -0.02, 90);
		create_enemy3c(80+(VIEWPORT_H+20)*I, 200, Fairy, stage2_sidebox_trail, 3 - 0.5*M_PI*I, 0.02, 90);
	}

	FROM_TO(3000, 3600, 300) {
		create_enemy1c(VIEWPORT_W/2-60*(_i-1)-10.0*I, 7000+500*global.diff, BigFairy, stage2_great_circle, 2.0*I);
	}

	FROM_TO(3700, 4500, 40)
		create_enemy1c(VIEWPORT_W*frand()-10.0*I, 150, Fairy, stage2_flea, 2.5*I);

	FROM_TO(4000, 4600, 100)
		create_enemy1c(VIEWPORT_W/2+100-200*frand()-10.0*I, 2000, Fairy, stage2_accel_circle, 2.0*I);

	AT(5100) {
		global.boss = create_hina();
	}

	AT(5180) {
		global.dialog = stage2_post_dialog();
	}

	AT(5240 - FADE_TIME) {
		stage_finish(GAMEOVER_WIN);
	}
}
