/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "stage6_events.h"
#include "stage6.h"
#include <global.h>

Dialog *stage6_dialog(void) {
	Dialog *d = create_dialog(global.plr.cha == Marisa ? "dialog/marisa" : "dialog/youmu", "dialog/elly");

	dadd_msg(d, Left, "You are responsible for all this?");
	dadd_msg(d, Right, "Yes…");

	if(global.plr.cha == Marisa) {
		dadd_msg(d, Left, "I’m going to masterspark you now!");
		dadd_msg(d, Right, "What? Why do you want to fight?\nDon’t you even want to hear what\nI have to say?");
		dadd_msg(d, Left, "I understand that it’s a huge mess!\nCracking the border, a giant mansion, a giant tower…");
		dadd_msg(d, Left, "At first I was curious. But now\nI just want to finish this! Seriously.");
	} else {
		dadd_msg(d, Left, "Who are you and what is this?");
		dadd_msg(d, Right, "A kind person granted me an unknown power,\nand thanks to that I was able to\ncreate this little place for myself.");
		dadd_msg(d, Left, "Why did you create *this* kind of place for yourself?");
		dadd_msg(d, Right, "Because it is great for research!\nAnd it’s almost done! Just a matter of moments…");
		dadd_msg(d, Right, "And the true potential of my power will be\nunleashed!");
		dadd_msg(d, Left, "That means…\nI’ll better finish you off quickly?");
	}

	dadd_msg(d, Right, "Why do you have to be so ignorant?");
	dadd_msg(d, Right, "…\nSorry, this is more important than you!");

	dadd_msg(d, BGM, "stage6boss");
	return d;
}

int stage6_hacker(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 4, Power, 3, NULL);
		return 1;
	}

	FROM_TO(0, 70, 1)
		e->pos += e->args[0];

	FROM_TO(100, 180+40*global.diff, 3) {
		int i;
		for(i = 0; i < 6; i++) {
			complex n = sin(_i*0.2)*cexp(I*0.3*(i/2-1))*(1-2*(i&1));
			create_projectile1c("wave", e->pos + 120*n, rgb(1.0, 0.2-0.01*_i, 0.0), linear, (0.25-0.5*psin(global.frames+_i*46752+16463*i+467*sin(global.frames*_i*i)))*global.diff+creal(n)+2.0*I);
		}
	}

	FROM_TO(180+40*global.diff+60, 2000, 1)
		e->pos -= e->args[0];
	return 1;
}

int stage6_side(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 4, Power, 3, NULL);
		return 1;
	}

	if(t < 60 || t > 120)
		e->pos += e->args[0];

	AT(70) {
		int i;
		int c = 15+10*global.diff;
		for(i = 0; i < c; i++) {
			create_projectile2c((i%2 == 0) ? "rice" : "flea", e->pos+5*(i/2)*e->args[1], rgb(0.1*cabs(e->args[2]), 0.5, 1), accelerated, (1.0*I-2.0*I*(i&1))*(0.7+0.2*global.diff), 0.001*(i/2)*e->args[1]);
		}
		e->hp /= 4;
	}

	return 1;
}

int wait_proj(Projectile *p, int t) {
	if(t > creal(p->args[1])) {
		p->angle = carg(p->args[0]);
		p->pos += p->args[0];
	}

	return 1;
}

int stage6_flowermine(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 4, Power, 3, NULL);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(70, 200, 1)
		e->args[0] += 0.07*cexp(I*carg(e->args[1]-e->pos));

	FROM_TO(0, 1000, 7-global.diff) {
		create_projectile2c("rice", e->pos + 40*cexp(I*0.6*_i+I*carg(e->args[0])), rgb(1-psin(_i), 0.3, psin(_i)), wait_proj, I*cexp(I*0.6*_i)*(0.7+0.3*global.diff), 200)->angle = 0.6*_i;
	}

	return 1;
}

void ScaleFade(Projectile *p, int t) {
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	glScalef(creal(p->args[1]), creal(p->args[1]), 1);
	glRotatef(180/M_PI*p->angle, 0, 0, 1);
	if(t/creal(p->args[0]) != 0)
		glColor4f(1,1,1, 1.0 - (float)t/p->args[0]);

	draw_texture_p(0, 0, p->tex);

	if(t/creal(p->args[0]) != 0)
		glColor4f(1,1,1,1);
	glPopMatrix();
}

void scythe_common(Enemy *e, int t) {
	e->args[1] += cimag(e->args[1]);
}

int scythe_mid(Enemy *e, int t) {
	complex n;

	if(t < 0) {
		scythe_common(e, t);
		return 1;
	}

	if(t > 300) {
		return ACTION_DESTROY;
	}

	e->pos += (6-global.diff-0.005*I*t)*e->args[0];

	n = cexp(cimag(e->args[1])*I*t);
	create_projectile2c("bigball", e->pos + 80*n, rgb(0.2, 0.5-0.5*cimag(n), 0.5+0.5*creal(n)), wait_proj, global.diff*cexp(0.6*I)*n, 100)->draw=ProjDrawAdd;

	if(global.diff > D_Normal && t&1)
		create_projectile2c("ball", e->pos + 80*n, rgb(0, 0.2, 0.5), accelerated, n, 0.01*global.diff*cexp(I*carg(global.plr.pos - e->pos - 80*n)))->draw=ProjDrawAdd;

	scythe_common(e, t);
	return 1;
}

void Scythe(Enemy *e, int t) {
	Projectile *p = create_projectile_p(&global.particles, get_tex("stage6/scythe"), e->pos+I*6*sin(global.frames/25.0), 0, ScaleFade, timeout, 8, e->args[2], 0, 0);
	p->type = Particle;
	p->angle = creal(e->args[1]);

	create_particle2c("lasercurve", e->pos+100*creal(e->args[2])*frand()*cexp(2.0*I*M_PI*frand()), rgb(1,0.1,1), GrowFadeAdd, timeout_linear, 60, -I+1);
}

int scythe_intro(Enemy *e, int t) {
	if(t < 0) {
		scythe_common(e, t);
		return 0;
	}

	TIMER(&t);

	GO_TO(e,VIEWPORT_W/2+200.0*I, 0.05);


	FROM_TO(60, 119, 1) {
		e->args[1] -= 0.00333333*I;
		e->args[2] -= 0.007;
	}

	scythe_common(e, t);
	return 1;
}

void elly_intro(Boss *b, int t) {
	TIMER(&t);

	if(global.stage->type == STAGE_SPELL) {
		GO_TO(b, BOSS_DEFAULT_GO_POS, 0.1);

		AT(0) {
			create_enemy3c(VIEWPORT_W+200.0*I, ENEMY_IMMUNE, Scythe, scythe_intro, 0, 1+0.2*I, 1);
		}

		return;
	}

	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.01);

	AT(200) {
		create_enemy3c(VIEWPORT_W+200+200.0*I, ENEMY_IMMUNE, Scythe, scythe_intro, 0, 1+0.2*I, 1);
	}

	AT(300)
		global.dialog = stage6_dialog();
}

int scythe_infinity(Enemy *e, int t) {
	if(t < 0) {
		scythe_common(e, t);
		return 1;
	}

	TIMER(&t);
	FROM_TO(0, 40, 1) {
		GO_TO(e, VIEWPORT_W/2+200.0*I, 0.01);
		e->args[2] = min(0.8, creal(e->args[2])+0.0003*t*t);
		e->args[1] = creal(e->args[1]) + I*min(0.2, cimag(e->args[1])+0.0001*t*t);
	}

	FROM_TO(40, 3000, 1) {
		float w = min(0.15, 0.0001*(t-40));
		e->pos = VIEWPORT_W/2 + 200.0*I + 200*cos(w*(t-40)+M_PI/2.0) + I*80*sin(creal(e->args[0])*w*(t-40));

		create_projectile2c("ball", e->pos+80*cexp(I*creal(e->args[1])), rgb(cos(creal(e->args[1])), sin(creal(e->args[1])), cos(creal(e->args[1])+2.1)), asymptotic, (1+0.4*global.diff)*cexp(I*creal(e->args[1])), 3 + 0.2 * global.diff);
	}

	scythe_common(e, t);
	return 1;
}

int scythe_reset(Enemy *e, int t) {
	if(t < 0) {
		scythe_common(e, t);
		return 1;
	}

	if(t == 1)
		e->args[1] = fmod(creal(e->args[1]), 2*M_PI) + I*cimag(e->args[1]);

	GO_TO(e, BOSS_DEFAULT_GO_POS, 0.02);
	e->args[2] = max(0.6, creal(e->args[2])-0.01*t);
	e->args[1] += (0.19-creal(e->args[1]))*0.05;
	e->args[1] = creal(e->args[1]) + I*0.9*cimag(e->args[1]);

	scythe_common(e, t);
	return 1;
}

void elly_frequency(Boss *b, int t) {
	TIMER(&t);
	AT(EVENT_BIRTH) {
		global.enemies->birthtime = global.frames;
		global.enemies->logic_rule = scythe_infinity;
		global.enemies->args[0] = 2;
	}
	AT(EVENT_DEATH) {
		global.enemies->birthtime = global.frames;
		global.enemies->logic_rule = scythe_reset;
		global.enemies->args[0] = 0;
	}

}

int scythe_newton(Enemy *e, int t) {
	if(t < 0) {
		scythe_common(e, t);
		return 1;
	}

	TIMER(&t);

	FROM_TO(0, 100, 1)
		e->pos -= 0.2*I*_i;

	AT(100) {
		e->args[1] = 0.2*I;
// 		e->args[2] = 1;
	}

	FROM_TO(100, 10000, 1) {
		e->pos = VIEWPORT_W/2+I*VIEWPORT_H/2 + 500*cos(_i*0.06)*cexp(I*_i*0.01);
	}


	FROM_TO(100, 10000, 3) {
		float f = carg(global.plr.pos-e->pos);
		Projectile *p;
		for(p = global.projs; p; p = p->next) {
			if(p->type == FairyProj && cabs(p->pos-e->pos) < 50 && cabs(global.plr.pos-e->pos) > 50 && p->args[2] == 0) {
				e->args[3] += 1;
				//p->args[0] /= 2;
				if(global.diff > D_Normal && (int)(creal(e->args[3])+0.5) % (15-5*(global.diff == D_Lunatic)) == 0) {
					p->args[0] = cexp(I*f);
					p->clr = rgb(1,0,0.5);
					p->tex = get_tex("proj/bullet");
					p->args[1] = 0.005*I;
				} else {
					p->args[0] = 2*cexp(I*2*M_PI*frand());
					p->clr = rgba(frand(), 0, 1, 0.8);
				}
				p->args[2] = 1;
			}
		}
	}

	FROM_TO(100, 10000, 5-global.diff/2) {
		if(cabs(global.plr.pos-e->pos) > 50)
			create_projectile3c("rice", e->pos, rgb(0.3, 1, 0.8), linear, I,0,1);
	}

	scythe_common(e, t);
	return 1;
}

void elly_newton(Boss *b, int t) {
	TIMER(&t);

	AT(0) {
		global.enemies->birthtime = global.frames;
		global.enemies->logic_rule = scythe_newton;
	}

	AT(EVENT_DEATH) {
		global.enemies->birthtime = global.frames;
		global.enemies->logic_rule = scythe_reset;
	}

	FROM_TO(0, 100000, 20+10*(global.diff>D_Normal)) {
		float a = 2.7*_i+carg(global.plr.pos-b->pos);
		int x, y;
		float w = min(D_Hard,global.diff)/2.0+1.5;

		for(x = -w; x <= w; x++) {
			for(y = -w; y <= w; y++) {
				create_projectile2c("ball", b->pos+(x+I*y)*(18)*cexp(I*a), rgb(0, 0.5, 1), accelerated, 2*cexp(I*a), 0);
			}
		}
	}
}

int scythe_kepler(Enemy *e, int t) {
	if(t < 0) {
		scythe_common(e, t);
		return 1;
	}

	TIMER(&t);

	AT(20) {
		e->args[1] = 0.2*I;
	}

	FROM_TO(20, 10000, 1) {
		GO_TO(e, global.boss->pos + 100*cexp(I*_i*0.01),0.03);
	}


	scythe_common(e, t);
	return 1;
}

int kepler_bullet(Projectile *p, int t) {
	TIMER(&t);
	int tier = round(creal(p->args[1]));
	AT(1) {
		char *tex;
		switch(tier) {
			case 0: tex = "proj/soul"; break;
			case 1: tex = "proj/bigball"; break;
			case 2: tex = "proj/ball"; break;
			default: tex = "proj/flea";
		}
		p->tex = get_tex(tex);

	}
	AT(EVENT_DEATH) {
		if(tier != 0)
			free_ref(p->args[2]);
	}
	if(t < 0)
		return 1;

	complex pos = p->pos0;

	if(tier != 0) {
		Projectile *parent = (Projectile *) REF(creal(p->args[2]));

		if(parent == 0) {
			p->pos += p->args[3];
			return 1;
		}
		pos = parent->pos;
	} else {
		pos += t*p->args[2];
	}

	complex newpos = pos + tanh(t/90.)*p->args[0]*cexp((1-2*(tier&1))*I*t*0.5/cabs(p->args[0]));
	complex vel = newpos-p->pos;
	p->pos = newpos;
	p->args[3] = vel;

	if(t%(30-5*global.diff) == 0) {
		p->args[1]+=1*I;
		int tau = global.frames-global.boss->current->starttime;
		complex phase = cexp(I*0.2*tau*tau);
		int n = global.diff/2+3+(frand()>0.3);
		if(global.diff == D_Easy)
			n=7;

		if(tier <= 1+(global.diff>D_Hard) && cimag(p->args[1])*(tier+1) < n) {
			create_projectile3c("flea",p->pos,rgb(0.3+0.3*tier,0.6-0.3*tier,1),kepler_bullet,cabs(p->args[0])*phase,tier+1,add_ref(p));
		}
	}
	return 1;
}


void elly_kepler(Boss *b, int t) {
	TIMER(&t);

	AT(0) {
		global.enemies->birthtime = global.frames;
		global.enemies->logic_rule = scythe_kepler;
		create_projectile3c("soul",b->pos,rgb(0.3,0.8,1),kepler_bullet,50+10*global.diff,0,0);
	}

	AT(EVENT_DEATH) {
		global.enemies->birthtime = global.frames;
		global.enemies->logic_rule = scythe_reset;
	}

	FROM_TO(0, 100000, 20) {
		int c = 2;
		for(int i = 0; i < c; i++) {
			complex n = cexp(I*2*M_PI/c*i+I*0.6*_i);
			create_projectile3c("soul",b->pos,rgb(0.3,0.8,1),kepler_bullet,50*n,0,(1.4+0.1*global.diff)*n);
		}

	}
}

void elly_frequency2(Boss *b, int t) {
	TIMER(&t);
	AT(0) {
		global.enemies->birthtime = global.frames;
		global.enemies->logic_rule = scythe_infinity;
		global.enemies->args[0] = 4;
	}
	AT(EVENT_DEATH) {
		global.enemies->birthtime = global.frames;
		global.enemies->logic_rule = scythe_reset;
		global.enemies->args[0] = 0;
	}

	FROM_TO(0, 2000, 3-global.diff/2) {
		complex n = sin(t*0.12*global.diff)*cexp(t*0.02*I*global.diff);
		create_projectile2c("plainball", b->pos+80*n, rgb(0,0,0.7), asymptotic, 2*n/cabs(n), 3);
	}
}

complex maxwell_laser(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->unclearable = true;
		l->shader = get_shader("laser_maxwell");
		return 0;
	}

	return l->pos + l->args[0]*(t+I*creal(l->args[2])*t*0.02*sin(0.1*t+cimag(l->args[2])));
}

void maxwell_laser_logic(Laser *l, int t) {
	static_laser(l, t);
	TIMER(&t);

	if(l->width < 3)
		l->width = 2.9;

	FROM_TO(60, 99, 1) {
		l->args[2] += 0.145+0.01*global.diff;
	}

	FROM_TO(00, 10000, 1) {
		l->args[2] -= 0.1*I+0.02*I*global.diff-0.05*I*(global.diff == D_Lunatic);
	}
}

void elly_maxwell(Boss *b, int t) {
	TIMER(&t);

	FROM_TO(40, 159, 5)
		create_laser(b->pos, 200, 10000, rgb(0,0.2,1), maxwell_laser, maxwell_laser_logic, cexp(2.0*I*M_PI/24*_i)*VIEWPORT_H*0.005, 200+15.0*I, 0, 0);

}

void Baryon(Enemy *e, int t) {
	Enemy *n;

	float alpha = 1 - 0.8/(exp((cabs(global.plr.pos-e->pos)-100)/10)+1);

	glColor4f(1.0,1.0,1.0,alpha);
	draw_texture(creal(e->pos), cimag(e->pos), "stage6/baryon");
	glColor4f(1.0,1.0,1.0,1.0);

	n = REF(e->args[1]);
	if(!n)
		return;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, get_tex("stage6/baryon_connector")->gltex);
	glPushMatrix();
	glTranslatef(creal(e->pos+n->pos)/2.0, cimag(e->pos+n->pos)/2.0, 0);
	glRotatef(180/M_PI*carg(e->pos-n->pos), 0, 0, 1);
	glScalef(cabs(e->pos-n->pos)-70, 20, 1);

	draw_quad();
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);

// 	create_particle2c("flare", e->pos+40*frand()*cexp(2.0*I*M_PI*frand()), rgb(0, 1, 1), GrowFadeAdd, timeout_linear, 50, 1-I);

	if(!(t % 10))
		create_particle1c("stain", e->pos+10*frand()*cexp(2.0*I*M_PI*frand()), rgb(0, 1*alpha, 0.7*alpha), FadeAdd, timeout, 50)->angle = 2*M_PI*frand();
}

void BaryonCenter(Enemy *e, int t) {
	Enemy *l[2];
	int i;

	glBlendFunc(GL_SRC_ALPHA,GL_ONE);

	glPushMatrix();
	glTranslatef(creal(e->pos), cimag(e->pos), 0);
	glRotatef(2*t, 0, 0, 1);
	draw_texture(0, 0, "stage6/scythecircle");
	glPopMatrix();
	draw_texture(creal(e->pos), cimag(e->pos), "stage6/baryon");
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);


	l[0] = REF(creal(e->args[1]));
	l[1] = REF(cimag(e->args[1]));

	if(!l[0] || !l[1])
		return;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, get_tex("stage6/baryon_connector")->gltex);
	for(i = 0; i < 2; i++) {
		glPushMatrix();
		glTranslatef(creal(e->pos+l[i]->pos)/2.0, cimag(e->pos+l[i]->pos)/2.0, 0);
		glRotatef(180/M_PI*carg(e->pos-l[i]->pos), 0, 0, 1);
		glScalef(cabs(e->pos-l[i]->pos)-70, 20, 1);
		draw_quad();
		glPopMatrix();

	}
	glDisable(GL_TEXTURE_2D);
	create_particle2c("flare", e->pos+40*frand()*cexp(2.0*I*M_PI*frand()), rgb(0, 1, 1), GrowFadeAdd, timeout_linear, 50, 1-I);
	create_particle1c("stain", e->pos+40*frand()*cexp(2.0*I*M_PI*frand()), rgb(0, 1, 0.2), FadeAdd, timeout, 50)->angle = 2*M_PI*frand();
}

int baryon_unfold(Enemy *e, int t) {
	if(t < 0)
		return 1; // not catching death for references! because there will be no death!

	TIMER(&t);
	FROM_TO(0, 100, 1)
		e->pos += e->args[0];

	AT(100)
		e->pos0 = e->pos;
	return 1;
}

int baryon_center(Enemy *e, int t) {
	if(t == EVENT_DEATH) {
		free_ref(creal(e->args[1]));
		free_ref(cimag(e->args[1]));
	}

	return 1;
}

int scythe_explode(Enemy *e, int t) {
	if(t < 0) {
		scythe_common(e, t);
		return 0;
	}

	if(t < 50) {
		e->args[1] += 0.001*I*t;
		e->args[2] += 0.0015*t;
	}

	if(t >= 50)
		e->args[2] -= 0.002*(t-50);

	if(t == 100) {
		petal_explosion(100, e->pos);
		global.shake_view = 16;

		scythe_common(e, t);
		return ACTION_DESTROY;
	}

	scythe_common(e, t);
	return 1;
}

void elly_unbound(Boss *b, int t) {
	if(global.stage->type == STAGE_SPELL) {
		t += 100;
		GO_TO(b, BOSS_DEFAULT_GO_POS, 0.1)
	}

	TIMER(&t);

	AT(0) {
		global.enemies->birthtime = global.frames;
		global.enemies->logic_rule = scythe_explode;
	}

	AT(100) {
		int i;
		Enemy *e, *last = NULL, *first = NULL, *middle = NULL;

		for(i = 0; i < 6; i++) {
			e = create_enemy3c(b->pos, ENEMY_IMMUNE, Baryon, baryon_unfold, 1.5*cexp(2.0*I*M_PI/6*i), i != 0 ? add_ref(last) : 0, i);
			if(i == 0)
				first = e;
			else if(i == 3)
				middle = e;

			last = e;
		}

		first->args[1] = add_ref(last);

		create_enemy2c(b->pos, ENEMY_IMMUNE, BaryonCenter, baryon_center, 0, add_ref(first) + I*add_ref(middle));
	}

	if(t > 120)
		global.shake_view = max(0, 16-0.26*(t-120));
}

static void set_baryon_rule(EnemyLogicRule r) {
	Enemy *e;
	for(e = global.enemies; e; e = e->next) {
		if(e->draw_rule == Baryon) {
			e->birthtime = global.frames;
			e->logic_rule = r;
		}
	}
}

int eigenstate_proj(Projectile *p, int t) {
	if(t == creal(p->args[2])) {
		p->args[0] += p->args[3];
	}

	return asymptotic(p, t);
}

int baryon_eigenstate(Enemy *e, int t) {
	if(t < 0)
		return 1;

	e->pos = e->pos0 + 40*sin(0.03*t+M_PI*(creal(e->args[0]) > 0)) + 30*cimag(e->args[0])*I*sin(0.06*t);

	TIMER(&t);

	FROM_TO(100+20*(int)creal(e->args[2]), 100000, 150-12*global.diff) {
		int i, j;
		int c = 9;

		for(i = 0; i < c; i++) {
			complex n = cexp(2.0*I*_i+I*M_PI/2+I*creal(e->args[2]));
			for(j = 0; j < 3; j++)
				create_projectile4c("plainball", e->pos + 60*cexp(2.0*I*M_PI/c*i), rgb(j == 0, j == 1, j == 2), eigenstate_proj, 1*n, 1, 60, 0.6*I*n*(j-1)*cexp(0.4*I-0.1*I*global.diff))->draw = ProjDrawAdd;

		}
	}

	return 1;
}

int baryon_reset(Enemy *e, int t) {
	GO_TO(e, e->pos0, 0.1);
	return 1;
}

void elly_eigenstate(Boss *b, int t) {
	TIMER(&t);

	AT(0)
		set_baryon_rule(baryon_eigenstate);
	AT(EVENT_DEATH)
		set_baryon_rule(baryon_reset);
}

int baryon_nattack(Enemy *e, int t) {
	if(t < 0)
		return 1;

	TIMER(&t);

	e->pos = global.boss->pos + (e->pos-global.boss->pos)*cexp(0.006*I);

	FROM_TO(30, 10000, (7 - global.diff)) {
		float a = 0.2*_i + creal(e->args[2]) + 0.006*t;
		float ca = a + t/60.0f;
		create_projectile2c("ball", e->pos+40*cexp(I*a), rgb(cos(ca), sin(ca), cos(ca+2.1)), asymptotic, (1+0.2*global.diff)*cexp(I*a), 3);
	}

	return 1;
}

#define SAFE_RADIUS_DELAY 300
#define SAFE_RADIUS_BASE 100
#define SAFE_RADIUS_STRETCH 100
#define SAFE_RADIUS_MIN 60
#define SAFE_RADIUS_MAX 150
#define SAFE_RADIUS_SPEED 0.015
#define SAFE_RADIUS_PHASE 3*M_PI/2
#define SAFE_RADIUS_PHASE_FUNC(o) ((int)(creal(e->args[2])+0.5) * M_PI/3 + SAFE_RADIUS_PHASE + max(0, time - SAFE_RADIUS_DELAY) * SAFE_RADIUS_SPEED)
#define SAFE_RADIUS_PHASE_NORMALIZED(o) (fmod(SAFE_RADIUS_PHASE_FUNC(o) - SAFE_RADIUS_PHASE, 2*M_PI) / (2*M_PI))
#define SAFE_RADIUS_PHASE_NUM(o) ((int)((SAFE_RADIUS_PHASE_FUNC(o) - SAFE_RADIUS_PHASE) / (2*M_PI)))
#define SAFE_RADIUS(o) smoothreclamp(SAFE_RADIUS_BASE + SAFE_RADIUS_STRETCH * sin(SAFE_RADIUS_PHASE_FUNC(o)), SAFE_RADIUS_BASE - SAFE_RADIUS_STRETCH, SAFE_RADIUS_BASE + SAFE_RADIUS_STRETCH, SAFE_RADIUS_MIN, SAFE_RADIUS_MAX)

static void ricci_laser_logic(Laser *l, int t) {
	if(t == EVENT_BIRTH) {
		// remember maximum radius, start at 0 actual radius
		l->args[1] = 0 + creal(l->args[1]) * I;
		return;
	}

	if(t == EVENT_DEATH) {
		free_ref(l->args[2]);
		return;
	}

	Enemy *e = (Enemy*)REF(l->args[2]);

	if(e) {
		// attach to baryon
		l->pos = e->pos + l->args[3];
	}

	if(t > 0) {
		// expand then shrink radius
		l->args[1] = cimag(l->args[1]) * (I + sin(M_PI * t / (l->deathtime + l->timespan)));
	}

	return;
}

static int ricci_proj2(Projectile *p, int t) {
	TIMER(&t);

	AT(EVENT_DEATH) {
		Enemy *e = (Enemy*)REF(p->args[1]);

		if(!e) {
			return 1;
		}

		double rad = SAFE_RADIUS_MAX * (0.6 - 0.2 * (double)(D_Lunatic - global.diff) / 3);

		create_laser(p->pos, 12, 60, rgb(0.2, 1, 0.5), las_circle, ricci_laser_logic,  6*M_PI +  0*I, rad, add_ref(e), p->pos - e->pos);
		create_laser(p->pos, 12, 60, rgb(0.2, 0.4, 1), las_circle, ricci_laser_logic,  6*M_PI + 30*I, rad, add_ref(e), p->pos - e->pos);

		create_laser(p->pos, 1,  60, rgb(1.0, 0.0, 0), las_circle, ricci_laser_logic, -6*M_PI +  0*I, rad, add_ref(e), p->pos - e->pos)->width = 10;
		create_laser(p->pos, 1,  60, rgb(1.0, 0.0, 0), las_circle, ricci_laser_logic, -6*M_PI + 30*I, rad, add_ref(e), p->pos - e->pos)->width = 10;

		free_ref(p->args[1]);
		return 1;
	}

	if(t < 0) {
		return 1;
	}

	Enemy *e = (Enemy*)REF(p->args[1]);

	if(!e) {
		return ACTION_DESTROY;
	}

	p->pos = e->pos + p->pos0;

	if(t > 30)
		return ACTION_DESTROY;

	return 1;
}

int baryon_ricci(Enemy *e, int t) {
	int num = creal(e->args[2])+0.5;
	if(num % 2 == 1) {
		GO_TO(e, global.boss->pos,0.1);
	} else if(num == 0) {
		if(t < 150) {
			GO_TO(e, global.plr.pos, 0.1);
		} else {
			float s = 1.00 + 0.25 * (global.diff - D_Easy);
			complex d = e->pos - VIEWPORT_W/2-VIEWPORT_H*I*2/3 + 100*sin(s*t/200.)+25*I*cos(s*t*3./500.);
			e->pos += -0.5*d/cabs(d);
		}

	} else {
		e->pos = global.boss->pos+(global.enemies->pos-global.boss->pos)*cexp(I*2*M_PI*(1./6*creal(e->args[2])));
	}

	int time = global.frames - global.boss->current->starttime;

	if(num % 2 == 0 && SAFE_RADIUS_PHASE_NUM(e) > 0) {
		TIMER(&time);
		float phase = SAFE_RADIUS_PHASE_NORMALIZED(e);

		if(phase < 0.55 && phase > 0.15) {
			FROM_TO(150,100000,10) {
				int c = 3;
				complex n = cexp(2*M_PI*I * (0.25 + 1.0/c*_i));
				create_projectile2c("ball", 15*n, rgb(0,0.5,0.1),ricci_proj2,-n,add_ref(e))->draw = ProjDrawAdd;
			}
		}
	}

	return 1;
}

static int ricci_proj(Projectile *p, int t) {
	if(t < 0)
		return 1;

	if(t == 1)
		p->grazed = true;

	if(!global.boss)
		return ACTION_DESTROY;

	int time = global.frames-global.boss->current->starttime;

	complex shift = 0;
	Enemy *e;
	int i = 0;
	p->pos = p->pos0 + p->args[0]*t;

	double influence = 0;

	for(e = global.enemies; e; e = e->next, i++) {
		if(e->draw_rule != Baryon) {
			i--;
			continue;
		}
		if(i % 2 == 0) {
			double radius = SAFE_RADIUS(e);
			complex d = e->pos-p->pos;
			float s = 1.00 + 0.25 * (global.diff - D_Easy);
			int gaps = SAFE_RADIUS_PHASE_NUM(e) + 5;
			double r = cabs(d)/(1.0-0.15*sin(gaps*carg(d)+0.01*s*time));
			double range = 1/(exp((r-radius)/50)+1);
			shift += -1.1*(radius-r)/r*d*range;
			influence += range;
		}
	}

	p->pos = p->pos0 + p->args[0]*t+shift;

	p->angle = carg(p->args[0]);

	float r,g,b,a;
	parse_color(p->clr,&r,&g,&b, &a);
	a = 0.5 + 0.5 * max(0,tanh((time-80)/100.))*clamp(influence,0.2,1);
	a *= min(1, t / 20.0f);
	p->clr = rgba(r,g,cabs(shift)/20., a);

	return 1;
}


void elly_ricci(Boss *b, int t) {
	TIMER(&t);
	AT(0)
		set_baryon_rule(baryon_ricci);
	AT(EVENT_DEATH)
		set_baryon_rule(baryon_reset);

	int c = 15;
	float v = 3;
	int interval = 5;

	FROM_TO(30, 100000, interval) {
		for(int i = 0; i < c; i++) {
			int w = VIEWPORT_W;
			complex pos = 0.5 * w/(float)c + fmod(w/(float)c*(i+0.5*_i),w) + (VIEWPORT_H+10)*I;

			Projectile *p = create_projectile1c("ball", pos, rgba(0.5, 0.0, 0,0), ricci_proj, -v*I);
			p->draw = ProjDrawNoFlareAdd;
			p->maxviewportdist = SAFE_RADIUS_MAX;
		}
	}
}

#undef SAFE_RADIUS
#undef SAFE_RADIUS_DELAY
#undef SAFE_RADIUS_BASE
#undef SAFE_RADIUS_STRETCH
#undef SAFE_RADIUS_MIN
#undef SAFE_RADIUS_MAX
#undef SAFE_RADIUS_SPEED
#undef SAFE_RADIUS_PHASE
#undef SAFE_RADIUS_PHASE_FUNC
#undef SAFE_RADIUS_PHASE_NORMALIZED
#undef SAFE_RADIUS_PHASE_NUM

void elly_baryonattack(Boss *b, int t) {
	TIMER(&t);
	AT(0)
		set_baryon_rule(baryon_nattack);
	AT(EVENT_DEATH)
		set_baryon_rule(baryon_reset);
}

void elly_baryonattack2(Boss *b, int t) {
	TIMER(&t);
	AT(0)
		set_baryon_rule(baryon_nattack);
	AT(EVENT_DEATH)
		set_baryon_rule(baryon_reset);

	FROM_TO(100, 100000, 200-5*global.diff) {

		if(_i % 2) {
			int cnt = 5;
			for(int i = 0; i < cnt; ++i) {
				float a = M_PI/4;
				a = a * (i/(float)cnt) - a/2;
				complex n = cexp(I*(a+carg(global.plr.pos-b->pos)));

				for(int j = 0; j < 3; ++j) {
					create_projectile2c("bigball", b->pos, rgb(0,0.2,0.9), asymptotic, n, 2 * j);
				}
			}
		} else {
			int x, y;
			int w = 1+(global.diff > D_Normal);
			complex n = cexp(I*carg(global.plr.pos-b->pos));

			for(x = -w; x <= w; x++)
				for(y = -w; y <= w; y++)
					create_projectile2c("bigball", b->pos+25*(x+I*y)*n, rgb(0,0.2,0.9), asymptotic, n, 3);
		}
	}
}

void lhc_laser_logic(Laser *l, int t) {
	Enemy *e;

	static_laser(l, t);

	TIMER(&t);
	AT(EVENT_DEATH) {
		free_ref(l->args[2]);
		return;
	}

	e = REF(l->args[2]);

	if(e)
		l->pos = e->pos;
}

int baryon_lhc(Enemy *e, int t) {
	int t1 = t % 400;
	int g = (int)creal(e->args[2]);
	if(g == 0 || g == 3)
		return 1;
	TIMER(&t1);

	AT(1) {
		e->args[3] = 100.0*I+400.0*I*((t/400)&1);

		if(g == 2 || g == 5) {
			create_laser(e->pos, 200, 300, rgb(0.1+0.9*(g>3),0,1-0.9*(g>3)), las_linear, lhc_laser_logic, (1-2*(g>3))*VIEWPORT_W*0.005, 200+30.0*I, add_ref(e), 0);
		}
	}

	GO_TO(e, VIEWPORT_W*(creal(e->pos0) > VIEWPORT_W/2)+I*cimag(e->args[3]) + (100-0.4*t1)*I*(1-2*(g > 3)), 0.02);

	return 1;
}

void elly_lhc(Boss *b, int t) {
	TIMER(&t);

	AT(0)
		set_baryon_rule(baryon_lhc);
	AT(EVENT_DEATH)
		set_baryon_rule(baryon_reset);

	FROM_TO(280, 10000, 400) {
		int i;
		int c = 30+10*global.diff;
		complex pos = VIEWPORT_W/2 + 100.0*I+400.0*I*((t/400)&1);

		global.shake_view = 16;

		for(i = 0; i < c; i++) {
			complex v = 3*cexp(2.0*I*M_PI*frand());
			tsrand_fill(4);
			create_lasercurve2c(pos, 70+20*global.diff, 300, rgb(1, 1, 1), las_accel, v, 0.02*frand()*copysign(1,creal(v)))->width=15;
			create_projectile1c("soul", pos, rgb(0.4, 0, 1.0), linear, (1+2.5*afrand(0))*cexp(2.0*I*M_PI*afrand(1)))->draw = ProjDrawAdd;
			create_projectile1c("bigball", pos, rgb(1, 0, 0.4), linear, (1+2.5*afrand(2))*cexp(2.0*I*M_PI*afrand(3)))->draw = ProjDrawAdd;
		}
	}

	FROM_TO(0, 100000,7-global.diff)
		create_projectile2c("ball", b->pos, rgb(0, 0.4,1), asymptotic, cexp(2.0*I*_i), 3)->draw = ProjDrawAdd;

	FROM_TO(300, 10000, 400) {
		global.shake_view = 0;
	}
}

int baryon_explode(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		free_ref(e->args[1]);
		petal_explosion(35, e->pos);
		return 1;
	}

	GO_TO(e, global.boss->pos + (e->pos0-global.boss->pos)*(1.5+0.2*sin(t*0.05)), 0.04);

	if(frand() < 0.02) {
		e->hp = 0;
		return 1;
	}

	return 1;
}

int curvature_bullet(Projectile *p, int t) {
	if(REF(p->args[1]) == 0) {
		return 0;
	}
	float vx, vy, x, y;
	complex v = ((Enemy *)REF(p->args[1]))->args[0]*0.00005;
	vx = creal(v);
	vy = cimag(v);
	x = creal(p->pos-global.plr.pos);
	y = cimag(p->pos-global.plr.pos);

	float f1 = (1+2*(vx*x+vy*y)+x*x+y*y);
	float f2 = (1-vx*vx+vy*vy);
	float f3 = f1-f2*(x*x+y*y);
	p->pos = global.plr.pos + (f1*v+f2*(x+I*y))/f3+p->args[0]/(1+2*(x*x+y*y)/VIEWPORT_W/VIEWPORT_W);

	if(t == EVENT_DEATH)
		free_ref(p->args[1]);

	return 1;
}

int curvature_orbiter(Projectile *p, int t) {
	const double w = 0.03;
	if(REF(p->args[1]) != 0 && p->args[3] == 0) {
		p->pos = ((Projectile *)REF(p->args[1]))->pos+p->args[0]*cexp(I*t*w);

		p->args[2] = p->args[0]*I*w*cexp(I*t*w);
	} else {
		p->pos += p->args[2];
	}


	if(t == EVENT_DEATH)
		free_ref(p->args[1]);

	return 1;
}

static double saw(double t) {
	return cos(t)+cos(3*t)/9+cos(5*t)/25;
}

int curvature_slave(Enemy *e, int t) {
	e->args[0] = -(e->args[1] - global.plr.pos);
	e->args[1] = global.plr.pos;

	if(t % (2+(global.diff < D_Hard)) == 0) {
		tsrand_fill(2);
		complex pos = VIEWPORT_W*afrand(0)+I*VIEWPORT_H*afrand(1);
		if(cabs(pos - global.plr.pos) > 50) {
			tsrand_fill(2);
			float speed = 0.5/(1+(global.diff < D_Hard));
			create_projectile2c("flea",pos,rgb(0.1*afrand(0), 0.6,1), curvature_bullet, speed*cexp(2*M_PI*I*afrand(1)), add_ref(e));
		}
	}
	if(global.diff > D_Easy && t % (60-8*global.diff) == 0) {
			tsrand_fill(2);
			complex pos = VIEWPORT_W*afrand(0)+I*VIEWPORT_H*afrand(1);
			if(cabs(pos - global.plr.pos) > 100)
				create_projectile1c("ball",pos,rgb(1, 0.4,1), linear, cexp(I*carg(global.plr.pos-pos)))->draw = ProjDrawAdd;


	}

	if(global.diff >= D_Hard && !(t%20)) {
		Projectile *p = create_projectile1c("bigball",global.boss->pos,rgb(0.5, 0.4,1), linear, 4*I*cexp(I*M_PI*2/5*saw(t/100.)));
		p->draw = ProjDrawAdd;
		if(global.diff == D_Lunatic) {
			create_projectile2c("plainball",global.boss->pos,rgb(0.2, 0.4,1), curvature_orbiter, 40*cexp(I*t/400),add_ref(p))->draw = ProjDrawAdd;
		}



	}

	return 1;
}

void elly_curvature(Boss *b, int t) {
	TIMER(&t);

	AT(50) {
		create_enemy2c(b->pos,ENEMY_IMMUNE, 0, curvature_slave,0,global.plr.pos);
	}

	GO_TO(b, VIEWPORT_W/2+100*I+VIEWPORT_W/3*round(sin(t/200)), 0.04);

	AT(EVENT_DEATH) {
		killall(global.enemies);
	}

}
void elly_baryon_explode(Boss *b, int t) {
	TIMER(&t);

	AT(0)
		start_fall_over();

	AT(20)
		set_baryon_rule(baryon_explode);

	FROM_TO(0, 200, 1) {
		tsrand_fill(2);
		petal_explosion(1, b->pos + 100*afrand(0)*cexp(2.0*I*M_PI*afrand(1)));
	}

	AT(200) {
		tsrand_fill(2);
		global.shake_view = 10;
		petal_explosion(100, b->pos + 100*afrand(0)*cexp(2.0*I*M_PI*afrand(1)));
		killall(global.enemies);
	}

	AT(220) {
		global.shake_view = 0;
	}
}

void ScaleFadeSub(Projectile *proj, int t) {
	glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	ScaleFade(proj, t);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
}

int theory_proj(Projectile *p, int t) {

	if(t < 0)
		return 1;

	p->pos += p->args[0];
	p->angle = carg(p->args[0]);

	if(!cimag(p->args[1])) {
		float re = creal(p->pos);
		float im = cimag(p->pos);

		if(re <= 0 || re >= VIEWPORT_W)
			p->args[0] = -creal(p->args[0]) + I*cimag(p->args[0]);
		else if(im <= 0 || im >= VIEWPORT_H)
			p->args[0] = creal(p->args[0]) - I*cimag(p->args[0]);
		else
			return 1;

		p->args[0] *= 0.4+0.1*global.diff;

		switch((int)creal(p->args[1])) {
		case 0:
			p->tex = get_tex("proj/ball");
			break;
		case 1:
			p->tex = get_tex("proj/bigball");
			break;
		case 2:
			p->tex = get_tex("proj/bullet");
			break;
		case 3:
			p->tex = get_tex("proj/plainball");
			break;
		}

		p->clr = rgb(cos(p->angle), sin(p->angle), cos(p->angle+2.1));
		p->args[1] += I;
	}

	return 1;
}

int scythe_theory(Enemy *e, int t) {
	complex n;

	if(t < 0) {
		scythe_common(e, t);
		return 1;
	}

	if(t > 300) {
		scythe_common(e, t);
		return ACTION_DESTROY;
	}

	e->pos += (3-0.01*I*t)*e->args[0];

	n = cexp(cimag(e->args[1])*I*t);

	TIMER(&t);
	FROM_TO(0, 300, 4)
		create_projectile2c("ball", e->pos + 80*n, rgb(carg(n), 1-carg(n), 1/carg(n)), wait_proj, (0.6+0.3*global.diff)*cexp(0.6*I)*n, 100)->draw=ProjDrawAdd;

	scythe_common(e, t);
	return 1;
}

void elly_theory(Boss *b, int time) {
	int t = time % 500;
	int i;

	if(time == EVENT_BIRTH)
		global.shake_view = 10;
	if(time < 20)
		GO_TO(b, VIEWPORT_W/2+300.0*I, 0.05);
	if(time == 0)
		global.shake_view = 0;

	TIMER(&t);

	FROM_TO(0, 500, 10)
		for(i = 0; i < 3; i++) {
			tsrand_fill(5);
			create_particle2c("stain", b->pos+80*afrand(0)*cexp(2.0*I*M_PI*afrand(1)), rgba(1-afrand(2),0.8,0.5,1), FadeAdd, timeout, 60, 1+3*afrand(3))->angle = 2*M_PI*afrand(4);
		}

	FROM_TO(20, 70, 30-global.diff) {
		int c = 20+2*global.diff;
		for(i = 0; i < c; i++) {
			complex n = cexp(2.0*I*M_PI/c*i);
			create_projectile2c("soul", b->pos, rgb(0.2, 0, 0.9), accelerated, n, (0.01+0.002*global.diff)*n+0.01*I*n*(1-2*(_i&1)))->draw = ProjDrawAdd;
		}
	}

	FROM_TO(120, 240, 10-global.diff) {
		int x, y;
		int w = 2;
		complex n = cexp(0.7*I*_i+0.2*I*frand());
		for(x = -w; x <= w; x++)
			for(y = -w; y <= w; y++)
				create_projectile2c("ball", b->pos + (15+5*global.diff)*(x+I*y)*n, rgb(0, 0.5, 1), accelerated, 2*n, 0.026*n);
	}

	FROM_TO(250, 299, 10) {
		create_enemy3c(b->pos, ENEMY_IMMUNE, Scythe, scythe_theory, cexp(2.0*I*M_PI/5*_i+I*(time/500)), 0.2*I, 1);
	}

}

void elly_spellbg_classic(Boss *b, int t) {
	fill_screen(0,0,0.7,"stage6/spellbg_classic");
	glBlendFunc(GL_ZERO,GL_SRC_COLOR);
	glColor4f(1,1,1,0);
	fill_screen(0,-t*0.005,0.7,"stage6/spellbg_chalk");
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1,1,1,1);
}

void elly_spellbg_modern(Boss *b, int t) {
	fill_screen(0,0,0.6,"stage6/spellbg_modern");
	glBlendFunc(GL_ZERO,GL_SRC_COLOR);
	glColor4f(1,1,1,0);
	fill_screen(0,-t*0.005,0.7,"stage6/spellbg_chalk");
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1,1,1,1);
}

Boss *create_elly(void) {
	Boss *b = create_boss("Elly", "elly", "dialog/elly", -200.0*I);

	boss_add_attack(b, AT_Move, "Catch the Scythe", 6, 0, elly_intro, NULL);
	boss_add_attack(b, AT_Normal, "Frequency", 40, 50000, elly_frequency, NULL);
	boss_add_attack_from_info(b, &stage6_spells.scythe.occams_razor, false);
	boss_add_attack(b, AT_Normal, "Frequency2", 40, 50000, elly_frequency2, NULL);
	boss_add_attack_from_info(b, &stage6_spells.scythe.orbital_clockwork, false);
	boss_add_attack_from_info(b, &stage6_spells.scythe.wave_theory, false);
	boss_add_attack(b, AT_Move, "Unbound", 3, 10, elly_unbound, NULL);
	boss_add_attack_from_info(b, &stage6_spells.baryon.many_world_interpretation, false);
	boss_add_attack(b, AT_Normal, "Baryon", 40, 50000, elly_baryonattack, NULL);
	boss_add_attack_from_info(b, &stage6_spells.baryon.spacetime_curvature, false);
	boss_add_attack(b, AT_Normal, "Baryon", 40, 50000, elly_baryonattack2, NULL);
	boss_add_attack_from_info(b, &stage6_spells.baryon.higgs_boson_uncovered, false);
	boss_add_attack(b, AT_Move, "Explode", 6, 10, elly_baryon_explode, NULL);
	boss_add_attack_from_info(b, &stage6_spells.extra.curvature_domination, false);
	boss_add_attack_from_info(b, &stage6_spells.final.theory_of_everything, false);
	boss_start_attack(b, b->attacks);

	return b;
}

void stage6_events(void) {
	TIMER(&global.timer);

	AT(0) {
		start_bgm("stage6");
	}

	AT(100)
		create_enemy1c(VIEWPORT_W/2, 6000, BigFairy, stage6_hacker, 2.0*I);

	FROM_TO(500, 700, 10)
		create_enemy3c(VIEWPORT_W*(_i&1), 2000, Fairy, stage6_side, 2.0*I+0.1*(1-2*(_i&1)),1-2*(_i&1),_i);

	FROM_TO(720, 940, 10) {
		complex p = VIEWPORT_W/2+(1-2*(_i&1))*20*(_i%10);
		create_enemy3c(p, 2000, Fairy, stage6_side, 2.0*I+1*(1-2*(_i&1)),I*cexp(I*carg(global.plr.pos-p))*(1-2*(_i&1)),_i*psin(_i));
	}

	FROM_TO(1380, 1660, 20)
		create_enemy2c(200.0*I, 600, Fairy, stage6_flowermine, 2*cexp(0.5*I*M_PI/9*_i)+1, 0);

	FROM_TO(1600, 2000, 20)
		create_enemy3c(VIEWPORT_W/2, 600, Fairy, stage6_flowermine, 2*cexp(0.5*I*M_PI/9*_i+I*M_PI/2)+1.0*I, VIEWPORT_H/2*I+VIEWPORT_W, 1);

	AT(2300)
		create_enemy3c(200.0*I-200, ENEMY_IMMUNE, Scythe, scythe_mid, 1, 0.2*I, 1);

	AT(3800)
		global.boss = create_elly();

	AT(3900 - FADE_TIME) {
		stage_finish(GAMEOVER_WIN);
	}
}
