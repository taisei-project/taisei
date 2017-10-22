/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "stage5_events.h"
#include "stage5.h"
#include <global.h>
#include <float.h>

Dialog *stage5_post_mid_dialog(void) {
	PlayerCharacter *pc = global.plr.mode->character;
	Dialog *d = create_dialog(pc->dialog_sprite_name, NULL);

	if(pc->id == PLR_CHAR_MARISA) {
		dadd_msg(d, Left, "Hey, wait!  …Did I just see an oarfish?");
	} else if(pc->id == PLR_CHAR_YOUMU) {
		dadd_msg(d, Left, "A messenger of Heaven! If I follow her, I’ll\nsurely learn something about the incident!");
	}

	return d;
}

Dialog *stage5_boss_dialog(void) {
	PlayerCharacter *pc = global.plr.mode->character;
	Dialog *d = create_dialog(pc->dialog_sprite_name, "dialog/iku");

	if(pc->id == PLR_CHAR_MARISA) {
		dadd_msg(d,Left, "I finally caught up to ya! I should’ve\nknown that the Dragon’s messenger would be hard\nto chase down in the air.");
		dadd_msg(d,Left, "Are you part of the incident too?");
		dadd_msg(d,Right, "Weren’t those earlier bombs enough of\na deterrent? As this world is cutting\ninto the space of Heaven, only those\nauthorized are allowed to investigate.");
		dadd_msg(d,Right, "You’re not a Celestial or anyone else\nfrom Heaven. That means you cannot go\nfurther.");
		dadd_msg(d,Left, "C’mon, you’re good at readin’ the atmosphere,\nright? Then ya should know that I’m not gonna\nback down after comin’ this far.");
		dadd_msg(d,Right, "I don’t have time to reason with you,\nunfortunately. When it comes to an average\nhuman sticking out arrogantly, there’s only one reasonable\ncourse of action for a bolt of lightning to take.");
		dadd_msg(d,Right, "Prepare to be struck down from\nHeaven’s door!");
	} else if(pc->id == PLR_CHAR_YOUMU) {
		dadd_msg(d,Left, "You were quite difficult to pin down.\nDon’t worry; I’ll listen to whatever warning\nyou have before I continue forward.");
		dadd_msg(d,Right, "Hmm, you’re the groundskeeper of the\nNetherworld, correct?");
		dadd_msg(d,Left, "That’s right. My mistress sent me here to\ninvestigate since the world of spirits\nhas been put in jeopardy by this new world\ninfringing on its boundaries.");
		dadd_msg(d,Right, "I’m afraid I cannot let you pass. This new world\nis a great issue caused by an incredible new power.\nOnly a Celestial or greater is qualified to handle such a\ndangerous occurrence.");
		dadd_msg(d,Left, "That doesn’t seem fair considering I’ve solved\nincidents before. I know what I am doing,\nand Lady Yuyuko entrusted me with this.");
		dadd_msg(d,Right, "If your confidence will not allow you to back\ndown, then so be it. I will test you using all of\nHeaven’s might, and if you are unfit, you shall\nbe cast down from this Tower of Babel!");
		dadd_msg(d,Left, "I shall pass whatever test necessary if it\nwill allow me to fulfill the wishes of\nLady Yuyuko!");
	}
	dadd_msg(d, BGM, "stage5boss");
	return d;
}

Dialog *stage5_post_boss_dialog(void) {
	PlayerCharacter *pc = global.plr.mode->character;
	Dialog *d = create_dialog(pc->dialog_sprite_name, NULL);

	dadd_msg(d, Left, "I can see the top!");
	if(pc->id == PLR_CHAR_MARISA) {
		dadd_msg(d, Left, "I might not have the proper credentials,\nbut I can definitely solve this incident.\nJust sit back and leave it to me!");
	} else if(pc->id == PLR_CHAR_YOUMU) {
		dadd_msg(d, Left, "As you can see, I have cut through your\nchallenge. You can trust me to get to the\nheart of the matter and handle it swiftly and carefully.");
	}

	return d;
}


int stage5_greeter(Enemy *e, int t) {
	TIMER(&t)
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 2, Power, 2, NULL);
		return 1;
	}

	e->moving = true;
	e->dir = creal(e->args[0]) < 0;

	if(t < 50 || t > 200)
		e->pos += e->args[0];

	FROM_TO(100, 180, 20) {
		int i;

		for(i = -(int)global.diff; i <= (int)global.diff; i++) {
			create_projectile2c("bullet", e->pos, rgb(0.0,0.0,1.0), asymptotic, (3.5+(global.diff == D_Lunatic))*cexp(I*carg(global.plr.pos-e->pos) + 0.06*I*i), 5);
		}
	}

	return 1;
}

int stage5_lightburst(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 4, Power, 1, NULL);
		return 1;
	}

	if(t < 70 || t > 200)
		e->pos += e->args[0];

	FROM_TO(20, 170, 5) {
		int i;
		int c = 5+global.diff;
		for(i = 0; i < c; i++) {
			complex n = cexp(I*carg(global.plr.pos) + 2.0*I*M_PI/c*i);
			create_projectile2c("ball", e->pos + 50*n*cexp(-0.4*I*_i*global.diff), rgb(0.3, 0, 0.7), asymptotic, 3*n, 3);
		}
	}

	return 1;
}


int stage5_swirl(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 1, NULL);
		return 1;
	}

	if(t > creal(e->args[1]) && t < cimag(e->args[1]))
		e->args[0] *= e->args[2];

	e->pos += e->args[0];

	FROM_TO(0, 400, 26-global.diff*4) {
		create_projectile2c("bullet", e->pos, rgb(0.3, 0.4, 0.5), asymptotic, 2*e->args[0]*I/cabs(e->args[0]), 3);
		create_projectile2c("bullet", e->pos, rgb(0.3, 0.4, 0.5), asymptotic, -2*e->args[0]*I/cabs(e->args[0]), 3);
	}

	return 1;
}

int stage5_limiter(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 4, Power, 2, NULL);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(0, 1200, 3) {
		create_projectile2c("rice", e->pos, rgb(0.5,0.1,0.2), asymptotic, 10*cexp(I*carg(global.plr.pos-e->pos)+0.2*I-0.1*I*(global.diff/4)+3.0*I/(_i+1)), 2);
		create_projectile2c("rice", e->pos, rgb(0.5,0.1,0.2), asymptotic, 10*cexp(I*carg(global.plr.pos-e->pos)-0.2*I+0.1*I*(global.diff/4)-3.0*I/(_i+1)), 2);
	}


	return 1;
}

int stage5_laserfairy(Enemy *e, int t) {
	TIMER(&t)
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 5, Power, 5, NULL);
		return 1;
	}

	FROM_TO(0, 100, 1)
		e->pos += e->args[0];

	if(t > 700)
		e->pos -= e->args[0];

	FROM_TO(100, 700, (7-global.diff)*(1+(int)creal(e->args[1]))) {
		complex n = cexp(I*carg(global.plr.pos-e->pos)+(0.2-0.02*global.diff)*I*_i);
		float fac = (0.5+0.2*global.diff);
		create_lasercurve2c(e->pos, 100, 300, rgb(0.7, 0.3, 1), las_accel, fac*4*n, fac*0.05*n);
		create_projectile2c("plainball", e->pos, rgb(0.7, 0.3, 1), accelerated, fac*4*n, fac*0.05*n)->draw = ProjDrawAdd;
	}
	return 1;
}

int stage5_miner(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 2, NULL);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(0, 600, 5-global.diff/2) {
		tsrand_fill(2);
		create_projectile1c("rice", e->pos + 20*cexp(2.0*I*M_PI*afrand(0)), rgb(0,0,cabs(e->args[0])), linear, cexp(2.0*I*M_PI*afrand(1)));
	}

	return 1;
}

int stage5_explosion(Enemy *e, int t) {
	TIMER(&t)
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 5, Power, 5, Life, (int)creal(e->args[1]), NULL);
		return 1;
	}

	FROM_TO(0, 80, 1)
		e->pos += e->args[0];

	FROM_TO(90, 300, 7-global.diff)
		create_projectile2c("soul", e->pos, rgb(0,0,1), asymptotic, 4*cexp(0.5*I*_i), 3)->draw = ProjDrawAdd;

	FROM_TO(200, 720, 6-global.diff) {
		create_projectile2c("rice", e->pos, rgb(1,0,0), asymptotic, 2*cexp(-0.3*I*_i+frand()*I), 3);
		create_projectile2c("rice", e->pos, rgb(1,0,0), asymptotic, -2*cexp(-0.3*I*_i+frand()*I), 3);
	}

	FROM_TO(500, 800, 100-10*global.diff)
		create_laserline(e->pos, 10*cexp(I*carg(global.plr.pos-e->pos)+0.04*I*(1-2*frand())), 60, 120, rgb(1, 0.3, 1));

	return 1;
}

void iku_slave_draw(Enemy *e, int t) {
	complex offset = (frand()-0.5)*10 + (frand()-0.5)*10.0*I;
	if(e->args[2] && !(t % 5)) {
		char *part = frand() > 0.5 ? "lightning0" : "lightning1";
		Projectile *p = create_particle1c(part, e->pos+3*offset, rgb(1.0, 1.0, 1.0), FadeAdd, timeout, 20);
		p->angle = frand()*2*M_PI;
	}

	if(!(t % 3)) {
		float alpha = 1;
		if(!e->args[2])
			alpha *= 0.03;
		create_particle3c("lightningball", e->pos, rgba(.1*alpha, .1*alpha, .6*alpha, 0.1*alpha), FadeAdd, enemy_flare, 50,offset*0.1,add_ref(e));
	}
}

void iku_mid_intro(Boss *b, int t) {
	TIMER(&t);

	b->pos += -1-7.0*I+10*t*(cimag(b->pos)<-200);

	FROM_TO(90, 110, 10) {
		create_enemy3c(b->pos, ENEMY_IMMUNE, iku_slave_draw, stage5_explosion, -2-0.5*_i+I*_i, _i == 1,1);
	}

	AT(960)
		killall(global.enemies);
}

Boss *create_iku_mid(void) {
	Boss *b = create_boss("Bombs?", "iku", 0, VIEWPORT_W+800.0*I);
	b->shadowcolor = rgba(0.2, 0.4, 0.5, 0.5);

	boss_add_attack(b, AT_SurvivalSpell, "Discharge Bombs", 16, 10, iku_mid_intro, NULL);

	return b;
}

int stage5_lightburst2(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 4, Power, 3, NULL);
		return 1;
	}

	if(t < 70 || t > 200)
		e->pos += e->args[0];

	FROM_TO(20, 170, 5) {
		int i;
		int c = 4+global.diff-(global.diff==D_Easy);
		for(i = 0; i < c; i++) {
			tsrand_fill(2);
			complex n = cexp(I*carg(global.plr.pos-e->pos) + 2.0*I*M_PI/c*i);
			create_projectile2c("bigball", e->pos + 50*n*cexp(-1.0*I*_i*global.diff), rgb(0.3, 0, 0.7+0.3*(_i&1)), asymptotic, 2.5*n+0.25*global.diff*afrand(0)*cexp(2.0*I*M_PI*afrand(1)), 3);
		}
	}

	return 1;
}

int stage5_superbullet(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 4, Power, 3, NULL);
		return 1;
	}

	FROM_TO(0, 70, 1)
		e->pos += e->args[0];

	FROM_TO(60, 200, 1) {
		complex n = cexp(I*M_PI*sin(_i/(8.0+global.diff)+frand()*0.1)+I*carg(global.plr.pos-e->pos));

		create_projectile2c("bullet", e->pos + 50*n, rgb(0.6, 0, 0), asymptotic, 2*n, 10);
	}

	FROM_TO(260, 400, 1)
		e->pos -= e->args[0];
	return 1;
}

void iku_intro(Boss *b, int t) {
	GO_TO(b, VIEWPORT_W/2+300.0*I, 0.02);

	if(t == 100)
		global.dialog = stage5_boss_dialog();
}

static void cloud_common(void) {
	tsrand_fill(4);
	float v = (afrand(2)+afrand(3))*0.5+1.0;
	create_projectile_p(&global.projs,get_tex("part/lightningball"), VIEWPORT_W*afrand(0)-15.0*I, rgba(0.2,0.0,0.4,0.6), PartDraw, accelerated, 1-2*afrand(1)+v*I, -0.01*I,0,0);
}

void iku_bolts(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);

	FROM_TO(0, 400, 2) {
		cloud_common();
	}

	FROM_TO(60, 400, 50) {
		int i, c = 10+global.diff;
		for(i = 0; i < c; i++) {
			create_projectile2c("ball", b->pos, rgb(0.4, 1, 1), asymptotic, (i+2)*0.4*cexp(I*carg(global.plr.pos-b->pos))+0.2*(global.diff-1)*frand(), 3)->draw = ProjDrawAdd;
		}
	}

	FROM_TO(0, 70, 1)
		GO_TO(b, 100+300.0*I, 0.02);

	FROM_TO(100, 200, 1)
		GO_TO(b, VIEWPORT_W/2+100.0*I, 0.02);

	FROM_TO(230, 300, 1)
		GO_TO(b, VIEWPORT_W-100+300.0*I, 0.02);

	FROM_TO(330, 400, 1)
		GO_TO(b, VIEWPORT_W/2+100.0*I, 0.02);

}

void iku_atmospheric(Boss *b, int time) {
	if(time < 0) {
		GO_TO(b, VIEWPORT_W/2+200.0*I, 0.06);
		return;
	}

	int t = time % 500;
	TIMER(&t);

	GO_TO(b,VIEWPORT_W/2+tanh(sin(time/100))*(200-100*(global.diff==D_Easy))+I*VIEWPORT_H/3+I*(cos(t/200)-1)*50,0.03);

	FROM_TO(0, 500, 23-2*global.diff) {
		tsrand_fill(4);
		complex p1 = VIEWPORT_W*afrand(0) + VIEWPORT_H/2*I*afrand(1);
		complex p2 = p1 + (120+20*global.diff)*cexp(0.5*I-afrand(2)*I)*(1-2*(afrand(3) > 0.5));

		int i;
		int c = 6+global.diff;

		for(i = -c*0.5; i <= c*0.5; i++) {
			create_projectile2c("ball", p1+(p2-p1)/c*i, rgb(1-1/(1+fabs(0.1*i)), 0.5-0.1*abs(i), 1), accelerated, 0, (0.004+0.001*global.diff)*cexp(I*carg(p2-p1)+I*M_PI/2+0.2*I*i))->draw = ProjDrawAdd;
		}
	}

	FROM_TO(0, 500, 7-global.diff) {
		if(global.diff >= D_Hard)
			create_projectile2c("thickrice", VIEWPORT_W*frand(), rgb(0,0.3,0.7), accelerated, 0, 0.01*I);
		else
			create_projectile1c("rice", VIEWPORT_W*frand(), rgb(0,0.3,0.7), linear, 2*I);
	}
}

complex bolts2_laser(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		return 0;
	}
	return creal(l->args[0])+I*cimag(l->pos) + sign(cimag(l->args[0]-l->pos))*0.06*I*t*t + (20+4*global.diff)*sin(t*0.025*global.diff+creal(l->args[0]));
}

void iku_bolts2(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);

	FROM_TO(0, 400, 2) {
		cloud_common();
	}

	FROM_TO(0, 400, 60)
		create_lasercurve1c(creal(global.plr.pos), 100, 200, rgb(0.3,1,1), bolts2_laser, global.plr.pos);

	FROM_TO(0, 400, 5-global.diff)
		if(frand() < 0.9)
			create_projectile1c("plainball", b->pos, rgb(0.2,0,0.8), linear, cexp(0.1*I*_i));

	FROM_TO(0, 70, 1)
		GO_TO(b, 100+200.0*I, 0.02);

	FROM_TO(230, 300, 1)
		GO_TO(b, VIEWPORT_W-100+200.0*I, 0.02);

}

int lightning_slave(Enemy *e, int t) {
	if(t < 0)
		return 1;
	if(t > 200)
		return ACTION_DESTROY;

	TIMER(&t);

	e->pos += e->args[0];

	FROM_TO(0,200,20)
		e->args[0] *= cexp(2.0*I*M_PI*frand());

	FROM_TO(0, 200, 3)
		if(cabs(e->pos-global.plr.pos) > 60) {
			tsrand_fill(2);
			Color clr = rgb(1-1/(1+0.01*_i), 0.5-0.01*_i, 1);
			create_projectile2c("wave", e->pos, clr, accelerated, (0.5+0.3*(_i&1)*(global.diff==D_Lunatic))*e->args[0]/cabs(e->args[0])*I, 0.001*global.diff*(1-2*afrand(0)+I*afrand(1)))->draw = ProjDrawAdd;
		}

	return 1;
}

static int zigzag_bullet(Projectile *p, int t) {
	int l = 50;
	p->pos = p->pos0+(abs(((2*t)%l)-l/2)*I+t)*2*p->args[0];

	if(t%2 == 0)
		create_particle1c("lightningball", p->pos, rgb(0.1,0.1,0.6), FadeAdd, timeout, 15);

	return 1;
}

void iku_lightning(Boss *b, int time) {
	int t = time % 141;

	if(time == EVENT_DEATH) {
		killall(global.enemies);
		return;
	}

	if(time < 0) {
		GO_TO(b, BOSS_DEFAULT_GO_POS, 0.03);
		return;
	}

	TIMER(&t);

	GO_TO(b,VIEWPORT_W/2+tanh(sin(time/100))*200+I*VIEWPORT_H/3+I*(cos(t/200)-1)*50,0.03);

	FROM_TO(0, 60, 1) {
		complex n = cexp(2.0*I*M_PI*frand());
		float l = 150*frand()+50;
		float s = 4+_i*0.01;
		float alpha = 0.5;
		create_particle2c("lightningball", b->pos+l*n, rgb(0.1*alpha,0.1*alpha,0.6*alpha), FadeAdd, timeout_linear, l/s, -s*n);
	}

	if(global.diff == D_Lunatic && time > 0 && !(time%100)) {
		int c = 7;
		for(int i = 0; i<c; i++)
			create_projectile1c("bigball", b->pos, rgb(0.5,0.1,1), zigzag_bullet, cexp(2*M_PI*I/c*i+I*carg(global.plr.pos-b->pos)))->draw = ProjDrawAdd;

	}

	AT(100) {
		int c = 40;
		int l = 200;
		int s = 10;
		float alpha = 1;
		for(int i=0; i < c; i++) {
			complex n = cexp(2.0*I*M_PI*frand());
			create_particle2c("smoke", b->pos, rgb(0.4*alpha,0.4*alpha,1*alpha), FadeAdd, timeout_linear, l/s, s*n);
		}
		for(int i = 0; i < global.diff+1; i++)
			create_enemy1c(b->pos, ENEMY_IMMUNE, NULL, lightning_slave, 10*cexp(I*carg(global.plr.pos - b->pos)+2.0*I*M_PI/(global.diff+1)*i));
	}
}

void iku_bolts3(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);

	FROM_TO(0, 400, 2) {
		cloud_common();
	}

	FROM_TO(60, 400, 60) {
		int i, c = 10+global.diff;
		complex n = cexp(I*carg(global.plr.pos-b->pos)+0.1*I-0.2*I*frand());
		for(i = 0; i < c; i++) {
			create_projectile2c("ball", b->pos, rgb(0.4, 1, 1), asymptotic, (i+2)*0.4*n+0.2*(global.diff-1)*frand(), 3)->draw = ProjDrawAdd;
		}
	}

	FROM_TO(0, 400, 5-global.diff)
		if(frand() < 0.9)
			create_projectile1c("plainball", b->pos, rgb(0.2,0,0.8), linear, cexp(0.1*I*_i));

	FROM_TO(0, 70, 1)
		GO_TO(b, 100+200.0*I, 0.02);

	FROM_TO(230, 300, 1)
		GO_TO(b, VIEWPORT_W-100+200.0*I, 0.02);

}


static complex induction_bullet_traj(Projectile *p, float t) {
	return p->pos0 + p->args[0]*t*cexp(p->args[1]*t);
}

int induction_bullet(Projectile *p, int time) {
	if(time < 0)
		return 1;
	float t = time*sqrt(global.diff);

	if(global.diff > D_Normal && !p->args[2]) {
		t = time*0.6;
		t = 230-t;
		complex pos = induction_bullet_traj(p,t);
		if(fabs(creal(pos)/VIEWPORT_W-0.5) > 0.5 || fabs(cimag(pos)/VIEWPORT_H-0.5) > 0.5)
			return 1;
		if(t < 0)
			return ACTION_DESTROY;
	}
	p->pos = induction_bullet_traj(p,t);
	p->angle = carg(p->args[0]*cexp(p->args[1]*t)*(1+p->args[1]*t));
	return 1;
}

complex cathode_laser(Laser *l, float t) {
	return l->pos + l->args[0]*t*cexp(l->args[1]*t);
}

void iku_cathode(Boss *b, int t) {
	GO_TO(b, VIEWPORT_W/2+200.0*I, 0.02);

	TIMER(&t)

	FROM_TO(50, 1800, 70-global.diff*10) {
		int i;
		int c = 5+global.diff;

		for(i = 0; i < c; i++) {
			create_projectile3c("bigball", b->pos, rgb(0.2, 0.4, 1), induction_bullet, 2*cexp(2.0*I*M_PI*frand()), 0.01*I*(1-2*(_i&1)),1)->draw = ProjDrawAdd;
			create_lasercurve2c(b->pos, 60, 200, rgb(0.4, 1, 1), cathode_laser, 2*cexp(2.0*I*M_PI*M_PI*frand()), 0.015*I*(1-2*(_i&1)));
		}
	}
}

void iku_induction(Boss *b, int t) {
	if(t < 0) {
		GO_TO(b, VIEWPORT_W/2+200.0*I, 0.03);
		return;
	}

	TIMER(&t);

	FROM_TO(0, 1800, 8) {
		int i,j;
		int c = 6;
		int c2 = 6-(global.diff/4);
		for(i = 0; i < c; i++) {
			for(j = 0; j < 2; j++) {
				Color clr = rgb(1-1/(1+0.1*(_i%c2)), 0.5-0.1*(_i%c2), 1);
				float shift = 0.6*(_i/c2);
				float a = -0.0002*(global.diff-D_Easy);
				if(global.diff == D_Hard)
					a += 0.0005;
				create_projectile2c("ball", b->pos, clr, induction_bullet, 2*cexp(2.0*I*M_PI/c*i+I*M_PI/2+I*shift), (0.01+0.001*global.diff)*I*(1-2*j)+a)->draw = ProjDrawAdd;
			}
		}

	}
}

void iku_spell_bg(Boss *b, int t);

Enemy* iku_extra_find_next_slave(complex from, double playerbias) {
	Enemy *nearest = NULL, *e;
	double dist, mindist = DBL_MAX;

	complex org = from + playerbias * cexp(I*(carg(global.plr.pos - from)));

	for(e = global.enemies; e; e = e->next) {
		if(e->args[2]) {
			continue;
		}

		dist = cabs(e->pos - org);

		if(dist < mindist) {
			nearest = e;
			mindist = dist;
		}
	}

	return nearest;
}


void iku_extra_slave_draw(Enemy *e, int t) {
	iku_slave_draw(e, t);

	if(e->args[2] && !(t % 5)) {
		complex offset = (frand()-0.5)*30 + (frand()-0.5)*20.0*I;
		create_particle3c("lasercurve", offset, e->args[1] ? rgb(1.0, 0.5, 0.0) : rgb(0.0, 0.5, 0.5), EnemyFlareShrink, enemy_flare, 50, (-50.0*I-offset)/50.0, add_ref(e));
	}
}

int iku_extra_trigger_bullet(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		free_ref(p->args[1]);
		return 1;
	}

	Enemy *target = REF(p->args[1]);

	if(!target) {
		return ACTION_DESTROY;
	}

	if(creal(p->args[2]) < 0) {
		linear(p, t);
		if(cabs(p->pos - target->pos) < 5) {
			p->pos = target->pos;
			target->args[1] = 1;
			p->args[2] = 55 - 5 * global.diff;
			target->args[3] = global.frames + p->args[2];
		}
	} else {
		p->args[2] = approach(creal(p->args[2]), 0, 1);
	}

	if(creal(p->args[2]) == 0) {
		int cnt = 6 + 2 * global.diff;
		for(int i = 0; i < cnt; ++i) {
			complex dir = cexp(I*(t + i*2*M_PI/cnt));
			create_projectile2c("bigball", p->pos, rgb(1, 0.5, 0), asymptotic, 1.1*dir, 5)->draw = ProjDrawAdd;
			create_projectile2c("bigball", p->pos, rgb(0, 0.5, 1), asymptotic, dir, 10)->draw = ProjDrawAdd;
		}
		global.shake_view += 5;
		global.shake_view_fade = 0.2;
		return ACTION_DESTROY;
	}

	p->angle = global.frames + t;

	char *part = frand() > 0.5 ? "lightning0" : "lightning1";
	complex ofs = nfrand();
	ofs += I * nfrand();
	Projectile *prt = create_particle2c(part, p->pos + 3 * ofs, rgb(1.0, 0.7 + 0.2 * nfrand(), 0.4), GrowFadeAdd, timeout, 20, 2.4);
	prt->angle = frand() * 2 * M_PI;

	return 1;
}

void iku_extra_fire_trigger_bullet(void) {
	Enemy *e = iku_extra_find_next_slave(global.boss->pos, 250);

	if(!e) {
		return;
	}

	Boss *b = global.boss;

	create_projectile3c("soul", b->pos, rgb(0.2, 0.2, 1.0), iku_extra_trigger_bullet,
		3*cexp(I*carg(e->pos - b->pos)), add_ref(e), -1);
}

int iku_extra_slave(Enemy *e, int t) {
	GO_TO(e, e->args[0], 0.05);

	if(e->args[1]) {
		if(creal(e->args[1]) < 2) {
			e->args[1] += 1;
			return 0;
		}

		if(global.frames == creal(e->args[3])) {
			complex o2 = e->args[2];
			e->args[2] = 0;
			Enemy *new = iku_extra_find_next_slave(e->pos, 75);
			e->args[2] = o2;

			if(new && e != new) {
				e->args[1] = 0;
				e->args[2] = new->args[2] = 600;
				new->args[1] = 1;
				new->args[3] = global.frames + 55 - 5 * global.diff;

				create_laserline_ab(e->pos, new->pos, 10, 30, e->args[2], rgb(0.3, 1, 1))->in_background = true;

				if(global.diff > D_Easy) {
					int cnt = floor(global.diff * 2.5), i;
					double r = frand() * 2 * M_PI;

					for(i = 0; i < cnt; ++i) {
						create_projectile2c("rice", e->pos, rgb(1, 1, 0), asymptotic, 2*cexp(I*(r+i*2*M_PI/cnt)), 2)->draw = ProjDrawAdd;
					}
				}
			} else {
				Enemy *o;
				Laser *l;
				int cnt = 6 + 2 * global.diff, i;

				global.shake_view = 0;
				global.shake_view_fade = 0.2;

				e->args[2] = 1;

				for(o = global.enemies; o; o = o->next) {
					if(!o->args[2])
						continue;

					for(i = 0; i < cnt; ++i) {
						create_projectile2c("ball", o->pos, rgb(0, 1, 1), asymptotic, 1.5*cexp(I*(t + i*2*M_PI/cnt)), 5)->draw = ProjDrawAdd;
					}

					o->args[1] = 0;
					o->args[2] = 0;

					global.shake_view += 1;
				}

				for(l = global.lasers; l; l = l->next) {
					l->deathtime = global.frames - l->birthtime + 20;
				}

				iku_extra_fire_trigger_bullet();
			}
		}
	}

	if(e->args[2]) {
		e->args[2] -= 1;
	}

	return 0;
}

void iku_extra(Boss *b, int t) {
	TIMER(&t);

	AT(EVENT_DEATH) {
		killall(global.enemies);
	}

	if(t < 0) {
		GO_TO(b, VIEWPORT_W/2+50.0*I, 0.02);
		return;
	}

	AT(0) {
		int i, j;
		int cnt = 5;
		double step = VIEWPORT_W / (double)cnt;

		for(i = 0; i < cnt; ++i) {
			for(j = 0; j < cnt; ++j) {
				complex epos = step * (0.5 + i) + (step * j + 125) * I;
				create_enemy4c(b->pos, ENEMY_IMMUNE, iku_extra_slave_draw, iku_extra_slave, epos, 0, 0, 1);
			}
		}
	}

	AT(60) {
		iku_extra_fire_trigger_bullet();
	}
}

Boss* stage5_spawn_iku(complex pos) {
	Boss *b = create_boss("Nagae Iku", "iku", "dialog/iku", pos);
	b->shadowcolor = rgba(0.2, 0.4, 0.5, 0.5);
	return b;
}

Boss* create_iku(void) {
	Boss *b = stage5_spawn_iku(VIEWPORT_W/2-200.0*I);

	boss_add_attack(b, AT_Move, "Introduction", 3, 0, iku_intro, NULL);
	boss_add_attack(b, AT_Normal, "Bolts1", 20, 24000, iku_bolts, NULL);
	boss_add_attack_from_info(b, &stage5_spells.boss.atmospheric_discharge, false);
	boss_add_attack(b, AT_Normal, "Bolts2", 25, 27000, iku_bolts2, NULL);
	boss_add_attack_from_info(b, &stage5_spells.boss.artificial_lightning, false);
	boss_add_attack(b, AT_Normal, "Bolts3", 20, 30000, iku_bolts3, NULL);
	boss_add_attack_from_info(b, &stage5_spells.boss.natural_cathode, false);

	if(global.diff < D_Hard) {
		boss_add_attack_from_info(b, &stage5_spells.boss.induction_field, false);
	} else {
		boss_add_attack_from_info(b, &stage5_spells.boss.inductive_resonance, false);
	}

	boss_add_attack_from_info(b, &stage5_spells.extra.overload, false);

	return b;
}

void stage5_events(void) {
	TIMER(&global.timer);

	AT(0) {
		stage_start_bgm("stage5");
	}

	FROM_TO(60, 120, 10)
		create_enemy1c(VIEWPORT_W+70.0*I+50*_i*I, 300, Fairy, stage5_greeter, -3);

	FROM_TO(270, 320, 25)
		create_enemy1c(VIEWPORT_W/4+VIEWPORT_W/2*_i, 2000, BigFairy, stage5_lightburst, 2.0*I);

	FROM_TO(500, 600, 10) {
		tsrand_fill(2);
		create_enemy3c(200.0*I*afrand(0), 500, Swirl, stage5_swirl, 4+I, 70+20*afrand(1)+200.0*I, cexp(-0.05*I));
	}

	FROM_TO(700, 800, 10) {
		tsrand_fill(3);
		create_enemy3c(VIEWPORT_W+200.0*I*afrand(0), 500, Swirl, stage5_swirl, -4+afrand(1)*I, 70+20*afrand(2)+200.0*I, cexp(0.05*I));
	}

	FROM_TO(870+50*(global.diff==D_Easy), 1000, 50)
		create_enemy1c(VIEWPORT_W/4+VIEWPORT_W/2*(_i&1), 2000, BigFairy, stage5_limiter, I);

	AT(1000)
		create_enemy1c(VIEWPORT_W/2, 9000, BigFairy, stage5_laserfairy, 2.0*I);

	FROM_TO(1900+100*(global.diff < D_Hard), 2200, 60-5*global.diff) {
		tsrand_fill(2);
		create_enemy1c(VIEWPORT_W+200.0*I*afrand(0), 500, Swirl, stage5_miner, -3+2.0*I*afrand(1));
	}

	FROM_TO(1500, 2400, 80)
		create_enemy1c(VIEWPORT_W*(_i&1)+100.0*I, 300, Fairy, stage5_greeter, 3-6*(_i&1));

	FROM_TO(2200, 2600, 60-5*global.diff)
		create_enemy1c(VIEWPORT_W/(6+global.diff)*_i, 200, Swirl, stage5_miner, 3.0*I);

	AT(2900)
		global.boss = create_iku_mid();

	AT(2920)
		global.dialog = stage5_post_mid_dialog();

	FROM_TO(3000, 3200, 100)
		create_enemy1c(VIEWPORT_W/2 + VIEWPORT_W/6*(1-2*(_i&1)), 2000, BigFairy, stage5_lightburst2, -1+2*(_i&1) + 2.0*I);

	FROM_TO(3300, 4000, 90-10*global.diff)
		create_enemy1c(200.0*I+VIEWPORT_W*(_i&1), 1500, Fairy, stage5_superbullet, 3-6*(_i&1));

	AT(3400) {
		create_enemy2c(VIEWPORT_W/4, 6000, BigFairy, stage5_laserfairy, 2.0*I, 1);
		create_enemy2c(VIEWPORT_W/4*3, 6000, BigFairy, stage5_laserfairy, 2.0*I, 1);
	}

	FROM_TO(4200, 5000, 20) {
		float f = frand();
		create_enemy3c(VIEWPORT_W/2, 400, Swirl, stage5_swirl, 2*cexp(I*M_PI*f)+I, 60 + 100.0*I, cexp(0.01*I*(1-2*(f<0.5))));
	}

	AT(5300)
		global.boss = create_iku();

	AT(5320)
		global.dialog = stage5_post_boss_dialog();

	AT(5550 - FADE_TIME) {
		stage_finish(GAMEOVER_WIN);
	}
}
