/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "stage6_events.h"
#include "stage6.h"
#include "global.h"
#include "stagetext.h"

static Dialog *stage6_dialog_pre_boss(void) {
	PlayerMode *pm = global.plr.mode;
	Dialog *d = create_dialog(pm->character->dialog_sprite_name, "dialog/elly");
	pm->dialog->stage6_pre_boss(d);
	dadd_msg(d, BGM, "stage6boss_phase1");
	return d;
}

static Dialog *stage6_dialog_pre_final(void) {
	PlayerMode *pm = global.plr.mode;
	Dialog *d = create_dialog(pm->character->dialog_sprite_name, "dialog/elly");
	pm->dialog->stage6_pre_final(d);
	return d;
}

int stage6_hacker(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, Point, 4, Power, 3, NULL);
		return 1;
	}

	FROM_TO(0, 70, 1)
		e->pos += e->args[0];

	FROM_TO_SND("shot1_loop",100, 180+40*global.diff, 3) {
		int i;
		for(i = 0; i < 6; i++) {
			complex n = sin(_i*0.2)*cexp(I*0.3*(i/2-1))*(1-2*(i&1));
			PROJECTILE("wave", e->pos + 120*n, RGB(1.0, 0.2-0.01*_i, 0.0), linear, {
				(0.25-0.5*psin(global.frames+_i*46752+16463*i+467*sin(global.frames*_i*i)))*global.diff+creal(n)+2.0*I
			});
		}
	}

	FROM_TO(180+40*global.diff+60, 2000, 1)
		e->pos -= e->args[0];
	return 1;
}

int stage6_side(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, Point, 4, Power, 3, NULL);
		return 1;
	}

	if(t < 60 || t > 120)
		e->pos += e->args[0];

	AT(70) {
		int i;
		int c = 15+10*global.diff;
		play_sound_ex("shot1",4,true);
		for(i = 0; i < c; i++) {
			PROJECTILE(
				.sprite = (i%2 == 0) ? "rice" : "flea",
				.pos = e->pos+5*(i/2)*e->args[1],
				.color = RGB(0.1*cabs(e->args[2]), 0.5, 1),
				.rule = accelerated,
				.args = {
					(1.0*I-2.0*I*(i&1))*(0.7+0.2*global.diff),
					0.001*(i/2)*e->args[1]
				}
			);
		}
		e->hp /= 4;
	}

	return 1;
}

int wait_proj(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	if(t > creal(p->args[1])) {
		if(t == creal(p->args[1]) + 1) {
			play_sound_ex("redirect", 4, false);
			spawn_projectile_highlight_effect(p);
		}

		p->angle = carg(p->args[0]);
		p->pos += p->args[0];
	}

	return ACTION_NONE;
}

int stage6_flowermine(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_KILLED) {
		spawn_items(e->pos, Point, 4, Power, 3, NULL);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(70, 200, 1)
		e->args[0] += 0.07*cexp(I*carg(e->args[1]-e->pos));

	FROM_TO(0, 1000, 7-global.diff) {
		PROJECTILE(
			.sprite = "rice",
			.pos = e->pos + 40*cexp(I*0.6*_i+I*carg(e->args[0])),
			.color = RGB(1-psin(_i), 0.3, psin(_i)),
			.rule = wait_proj,
			.args = {
				I*cexp(I*0.6*_i)*(0.7+0.3*global.diff),
				200
			},
			.angle = 0.6*_i,
		);
		play_sound("shot1");
	}

	return 1;
}

void scythe_common(Enemy *e, int t) {
	e->args[1] += cimag(e->args[1]);
}

int scythe_mid(Enemy *e, int t) {
	TIMER(&t);
	complex n;

	if(t < 0) {
		scythe_common(e, t);
		return 1;
	}

	if(creal(e->pos) > VIEWPORT_W + 100) {
		return ACTION_DESTROY;
	}

	e->pos += (6-global.diff-0.005*I*t)*e->args[0];

	n = cexp(cimag(e->args[1])*I*t);
	FROM_TO_SND("shot1_loop",0,300,1) {}
	PROJECTILE(
		.sprite = "bigball",
		.pos = e->pos + 80*n,
		.color = RGBA(0.2, 0.5-0.5*cimag(n), 0.5+0.5*creal(n), 0.0),
		.rule = wait_proj,
		.args = {
			global.diff*cexp(0.6*I)*n,
			100
		},
	);

	if(global.diff > D_Normal && t&1) {
		Projectile *p = PROJECTILE("ball", e->pos + 80*n, RGBA(0, 0.2, 0.5, 0.0), accelerated,
			.args = {
				n,
				0.01*global.diff*cexp(I*carg(global.plr.pos - e->pos - 80*n))
			},
		);

		if(projectile_in_viewport(p)) {
			play_sound("shot1");
		}
	}

	scythe_common(e, t);
	return 1;
}

void ScytheTrail(Projectile *p, int t) {
	r_mat_push();
	r_mat_translate(creal(p->pos), cimag(p->pos), 0);
	r_mat_rotate_deg(p->angle*180/M_PI+90, 0, 0, 1);
	r_mat_scale(creal(p->args[1]), creal(p->args[1]), 1);

	float a = (1.0 - t/(double)p->timeout) * (1.0 - cimag(p->args[1]));
	ProjDrawCore(p, RGBA(a, a, a, a));
	r_mat_pop();
}

void Scythe(Enemy *e, int t, bool render) {
	if(render) {
		return;
	}

	PARTICLE(
		.sprite_ptr = get_sprite("stage6/scythe"),
		.pos = e->pos+I*6*sin(global.frames/25.0),
		.draw_rule = ScytheTrail,
		.timeout = 8,
		.args = { 0, e->args[2] },
		.angle = creal(e->args[1]) - M_PI/2,
		.flags = PFLAG_REQUIREDPARTICLE,
	);

	PARTICLE(
		.sprite = "smoothdot",
		.pos = e->pos+100*creal(e->args[2])*frand()*cexp(2.0*I*M_PI*frand()),
		.color = RGBA(1.0, 0.1, 1.0, 0.0),
		.draw_rule = GrowFade,
		.rule = linear,
		.timeout = 60,
		.args = { -I+1, -I+1 }, // XXX: what the fuck?
		.flags = PFLAG_REQUIREDPARTICLE,
	);
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
		global.dialog = stage6_dialog_pre_boss();
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

	FROM_TO_SND("shot1_loop",40, 3000, 1) {
		float w = min(0.15, 0.0001*(t-40));
		e->pos = VIEWPORT_W/2 + 200.0*I + 200*cos(w*(t-40)+M_PI/2.0) + I*80*sin(creal(e->args[0])*w*(t-40));

		PROJECTILE(
			.sprite = "ball",
			.pos = e->pos+80*cexp(I*creal(e->args[1])),
			.color = RGB(cos(creal(e->args[1])), sin(creal(e->args[1])), cos(creal(e->args[1])+2.1)),
			.rule = asymptotic,
			.args = {
				(1+0.4*global.diff)*cexp(I*creal(e->args[1])),
				3 + 0.2 * global.diff
			}
		);
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

	GO_TO(e, BOSS_DEFAULT_GO_POS, 0.05);
	e->args[2] = max(0.6, creal(e->args[2])-0.01*t);
	e->args[1] += (0.19-creal(e->args[1]))*0.05;
	e->args[1] = creal(e->args[1]) + I*0.9*cimag(e->args[1]);

	scythe_common(e, t);
	return 1;
}

static Enemy* find_scythe(void) {
	for(Enemy *e = global.enemies.first; e; e = e->next) {
		if(e->visual_rule == Scythe) {
			return e;
		}
	}

	return NULL;
}

void elly_frequency(Boss *b, int t) {
	TIMER(&t);
	Enemy *scythe;

	AT(EVENT_BIRTH) {
		scythe = find_scythe();
		aniplayer_queue(&b->ani, "snipsnip", 0);
		scythe->birthtime = global.frames;
		scythe->logic_rule = scythe_infinity;
		scythe->args[0] = 2;
	}

	AT(EVENT_DEATH) {
		scythe = find_scythe();
		scythe->birthtime = global.frames;
		scythe->logic_rule = scythe_reset;
		scythe->args[0] = 0;
	}
}

static void elly_clap(Boss *b, int claptime) {
	aniplayer_queue(&b->ani, "clapyohands", 1);
	aniplayer_queue_frames(&b->ani, "holdyohands", claptime);
	aniplayer_queue(&b->ani, "unclapyohands", 1);
	aniplayer_queue(&b->ani, "main", 0);
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
//      e->args[2] = 1;
	}

	FROM_TO(100, 10000, 1) {
		e->pos = VIEWPORT_W/2+I*VIEWPORT_H/2 + 400*cos(_i*0.04)*cexp(I*_i*0.01);
	}


	FROM_TO(100, 10000, 3) {
		Projectile *p;
		for(p = global.projs.first; p; p = p->next) {
			if(
				p->type == EnemyProj &&
				cabs(p->pos-e->pos) < 50 &&
				cabs(global.plr.pos-e->pos) > 50 &&
				p->args[2] == 0 &&
				p->sprite != get_sprite("proj/apple")
			) {
				e->args[3] += 1;
				//p->args[0] /= 2;
				play_sound_ex("redirect",4,false);
				p->birthtime=global.frames;
				p->pos0=p->pos;
				p->args[0] = (2+0.125*global.diff)*cexp(I*2*M_PI*frand());
				p->color = *RGBA_MUL_ALPHA(frand(), 0, 1, 0.8);
				p->args[2] = 1;
				spawn_projectile_highlight_effect(p);
			}
		}
	}

	/*
	FROM_TO(100, 10000, 5-global.diff/2) {
		if(cabs(global.plr.pos-e->pos) > 50)
			PROJECTILE("rice", e->pos, RGB(0.3, 1, 0.8), linear, { I });
	}
	*/

	scythe_common(e, t);
	return 1;
}

static int newton_apple(Projectile *p, int t) {
	int r = accelerated(p, t);

	if(t >= 0) {
		p->angle += M_PI/16 * sin(creal(p->args[2]) + t / 30.0);
	}

	return r;
}

void elly_newton(Boss *b, int t) {
	TIMER(&t);

	AT(0) {
		Enemy *scythe = find_scythe();
		scythe->birthtime = global.frames;
		scythe->logic_rule = scythe_newton;
		elly_clap(b,100);
	}

	AT(EVENT_DEATH) {
		Enemy *scythe = find_scythe();
		scythe->birthtime = global.frames;
		scythe->logic_rule = scythe_reset;
	}

	FROM_TO(30 + 60 * (D_Lunatic - global.diff), 10000000, 30 - 6 * global.diff) {
		Sprite *apple = get_sprite("proj/apple");
		Color *c = NULL;

		switch(tsrand() % 3) {
			case 0: c = RGB(0.8, 0.0, 0.1); break;
			case 1: c = RGB(0.4, 0.6, 0.0); break;
			case 2: c = RGB(0.8, 0.6, 0.0); break;
		}

		complex apple_pos = clamp(creal(global.plr.pos) + nfrand() * 64, apple->w*0.5, VIEWPORT_W - apple->w*0.5);

		PROJECTILE(
			.pos = apple_pos,
			.proto = pp_apple,
			.rule = newton_apple,
			.args = {
				0, 0.05*I, M_PI*2*frand()
			},
			.color = c,
			.layer = LAYER_BULLET | 0xffff, // force them to render on top of the other bullets
		);

		play_sound("shot3");
	}

	FROM_TO(0, 100000, 20+10*(global.diff>D_Normal)) {
		float a = 2.7*_i+carg(global.plr.pos-b->pos);
		int x, y;
		float w = global.diff/2.0+1.5;

		play_sound("shot_special1");
		for(x = -w; x <= w; x++) {
			for(y = -w; y <= w; y++) {
				PROJECTILE("ball", b->pos+(x+I*y)*25*cexp(I*a), RGB(0, 0.5, 1), linear, { (2+(_i==0))*cexp(I*a) });
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

static ProjPrototype* kepler_pick_bullet(int tier) {
	switch(tier) {
		case 0:  return pp_soul;
		case 1:  return pp_bigball;
		case 2:  return pp_ball;
		default: return pp_flea;
	}
}

int kepler_bullet(Projectile *p, int t) {
	TIMER(&t);
	int tier = round(creal(p->args[1]));

	AT(EVENT_DEATH) {
		if(tier != 0) {
			free_ref(p->args[2]);
		}
	}

	if(t < 0) {
		return ACTION_ACK;
	}

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

		play_sound("redirect");
		if(tier <= 1+(global.diff>D_Hard) && cimag(p->args[1])*(tier+1) < n) {
			PROJECTILE(
				.proto = kepler_pick_bullet(tier + 1),
				.pos = p->pos,
				.color = RGB(0.3 + 0.3 * tier, 0.6 - 0.3 * tier, 1.0),
				.rule = kepler_bullet,
				.args = {
					cabs(p->args[0])*phase,
					tier+1,
					add_ref(p)
				}
			);
		}
	}
	return ACTION_NONE;
}


void elly_kepler(Boss *b, int t) {
	TIMER(&t);

	AT(0) {
		Enemy *scythe = find_scythe();
		scythe->birthtime = global.frames;
		scythe->logic_rule = scythe_kepler;

		PROJECTILE("soul", b->pos, RGB(0.3,0.8,1), kepler_bullet, { 50+10*global.diff });
		elly_clap(b,20);
	}

	AT(EVENT_DEATH) {
		Enemy *scythe = find_scythe();
		scythe->birthtime = global.frames;
		scythe->logic_rule = scythe_reset;
	}

	FROM_TO(0, 100000, 20) {
		int c = 2;
		play_sound("shot_special1");
		for(int i = 0; i < c; i++) {
			complex n = cexp(I*2*M_PI/c*i+I*0.6*_i);

			PROJECTILE(
				.proto = kepler_pick_bullet(0),
				.pos = b->pos,
				.color = RGB(0.3,0.8,1),
				.rule = kepler_bullet,
				.args = { 50*n, 0, (1.4+0.1*global.diff)*n }
			);
		}
	}
}

void elly_frequency2(Boss *b, int t) {
	TIMER(&t);

	AT(0) {
		Enemy *scythe = find_scythe();
		aniplayer_queue(&b->ani, "snipsnip", 0);
		scythe->birthtime = global.frames;
		scythe->logic_rule = scythe_infinity;
		scythe->args[0] = 4;
	}

	AT(EVENT_DEATH) {
		Enemy *scythe = find_scythe();
		scythe->birthtime = global.frames;
		scythe->logic_rule = scythe_reset;
		scythe->args[0] = 0;
	}

	FROM_TO_SND("shot1_loop",0, 2000, 3-global.diff/2) {
		complex n = sin(t*0.12*global.diff)*cexp(t*0.02*I*global.diff);
		PROJECTILE("plainball", b->pos+80*n, RGB(0,0,0.7), asymptotic, { 2*n/cabs(n), 3 });
	}
}

complex maxwell_laser(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->unclearable = true;
		l->shader = r_shader_get_optional("lasers/maxwell");
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

	AT(250) {
		elly_clap(b,50);
			play_sound("laser1");

	}
	FROM_TO(40, 159, 5) {
		create_laser(b->pos, 200, 10000, RGBA(0, 0.2, 1, 0.0), maxwell_laser, maxwell_laser_logic, cexp(2.0*I*M_PI/24*_i)*VIEWPORT_H*0.005, 200+15.0*I, 0, 0);
	}

}

static void draw_baryon_connector(complex a, complex b) {
	Sprite *spr = get_sprite("stage6/baryon_connector");
	r_mat_push();
	r_mat_translate(creal(a+b)/2.0, cimag(a+b)/2.0, 0);
	r_mat_rotate_deg(180/M_PI*carg(a-b), 0, 0, 1);
	r_mat_scale((cabs(a-b)-70) / spr->w, 20 / spr->h, 1);
	draw_sprite_batched_p(0, 0, spr);
	r_mat_pop();
}

void Baryon(Enemy *e, int t, bool render) {
	Enemy *n;

	if(!render) {
		if(!(t % 10) && global.boss && cabs(e->pos - global.boss->pos) > 2) {
			PARTICLE(
				.sprite = "stain",
				.pos = e->pos+10*frand()*cexp(2.0*I*M_PI*frand()),
				.color = RGBA(0, 1, 0.7, 0.0),
				.draw_rule = Fade,
				.timeout = 50,
				.angle = 2*M_PI*frand(),
			);
		}

		return;
	}

	draw_sprite_batched(creal(e->pos), cimag(e->pos), "stage6/baryon");

	n = REF(e->args[1]);

	if(!n) {
		return;
	}

	draw_baryon_connector(e->pos, n->pos);
}

void BaryonCenter(Enemy *e, int t, bool render) {
	Enemy *l[2];
	int i;

	if(!render) {
		complex p = e->pos+40*frand()*cexp(2.0*I*M_PI*frand());

		PARTICLE("flare", p, RGBA(0.0, 1.0, 1.0, 0.0),
			.draw_rule = GrowFade,
			.rule = linear,
			.timeout = 50,
			.args = { 1-I },
		);

		PARTICLE("stain", p, RGBA(0.0, 1.0, 0.2, 0.0),
			.draw_rule = Fade,
			.timeout = 50,
			.angle = 2*M_PI*frand(),
		);

		return;
	}

	r_color4(1, 1, 1, 0);
	r_mat_push();
	r_mat_translate(creal(e->pos), cimag(e->pos), 0);
	r_mat_rotate_deg(2*t, 0, 0, 1);
	draw_sprite_batched(0, 0, "stage6/scythecircle");
	r_mat_pop();
	draw_sprite_batched(creal(e->pos), cimag(e->pos), "stage6/baryon");
	r_color4(1, 1, 1, 1);

	l[0] = REF(creal(e->args[1]));
	l[1] = REF(cimag(e->args[1]));

	if(!l[0] || !l[1])
		return;

	for(i = 0; i < 2; i++) {
		draw_baryon_connector(e->pos, l[i]->pos);
	}
}

int baryon_unfold(Enemy *baryon, int t) {
	if(t < 0)
		return 1; // not catching death for references! because there will be no death!

	TIMER(&t);

	int extent = 100;
	float timeout;

	if(global.stage->type == STAGE_SPELL) {
		timeout = 92;
	} else {
		timeout = 142;
	}

	FROM_TO(0, timeout, 1) {
		for(Enemy *e = global.enemies.first; e; e = e->next) {
			if(e->visual_rule == BaryonCenter) {
				float f = t / (float)timeout;
				float x = f;
				float g = sin(2 * M_PI * log(log(x + 1) + 1));
				float a = g * pow(1 - x, 2 * x);
				f = 1 - pow(1 - f, 3) + a;

				baryon->pos = baryon->pos0 = e->pos + baryon->args[0] * f * extent;
				return 1;
			}
		}
	}

	return 1;
}

int baryon_center(Enemy *e, int t) {
	if(t == EVENT_DEATH) {
		free_ref(creal(e->args[1]));
		free_ref(cimag(e->args[1]));
	}

	if(global.boss) {
		e->pos = global.boss->pos;
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
		play_sound("boom");

		scythe_common(e, t);
		return ACTION_DESTROY;
	}

	scythe_common(e, t);
	return 1;
}

void elly_spawn_baryons(complex pos) {
	int i;
	Enemy *e, *last = NULL, *first = NULL, *middle = NULL;

	for(i = 0; i < 6; i++) {
		e = create_enemy3c(pos, ENEMY_IMMUNE, Baryon, baryon_unfold, 1.5*cexp(2.0*I*M_PI/6*i), i != 0 ? add_ref(last) : 0, i);
		e->ent.draw_layer = LAYER_BACKGROUND;

		if(i == 0) {
			first = e;
		} else if(i == 3) {
			middle = e;
		}

		last = e;
	}

	first->args[1] = add_ref(last);
	e = create_enemy2c(pos, ENEMY_IMMUNE, BaryonCenter, baryon_center, 0, add_ref(first) + I*add_ref(middle));
	e->ent.draw_layer = LAYER_BACKGROUND;
}

void elly_paradigm_shift(Boss *b, int t) {
	if(global.stage->type == STAGE_SPELL) {
		GO_TO(b, BOSS_DEFAULT_GO_POS, 0.1)
	} else if(t == 0) {
		play_sound("bossdeath");
	}

	TIMER(&t);

	AT(0) {
		Enemy *scythe = find_scythe();
		scythe->birthtime = global.frames;
		scythe->logic_rule = scythe_explode;
		elly_clap(b,150);
	}

	if(global.stage->type != STAGE_SPELL) {
		AT(80) {
			fade_bgm(0.5);
		}
	}

	AT(100) {
		if(global.stage->type != STAGE_SPELL) {
			stage_start_bgm("stage6boss_phase2");
			stagetext_add("Paradigm Shift!", VIEWPORT_W/2+I*(VIEWPORT_H/2+64), ALIGN_CENTER, get_font("big"), RGB(1, 1, 1), 0, 120, 10, 30);
		}

		elly_spawn_baryons(b->pos);
	}

	if(t > 120)
		global.shake_view = max(0, 16-0.26*(t-120));
}

static void set_baryon_rule(EnemyLogicRule r) {
	Enemy *e;
	for(e = global.enemies.first; e; e = e->next) {
		if(e->visual_rule == Baryon) {
			e->birthtime = global.frames;
			e->logic_rule = r;
		}
	}
}

int eigenstate_proj(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

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

		play_sound("shot_special1");
		play_sound_delayed("redirect",4,true,60);
		for(i = 0; i < c; i++) {
			complex n = cexp(2.0*I*_i+I*M_PI/2+I*creal(e->args[2]));
			for(j = 0; j < 3; j++) {
				PROJECTILE(.sprite = "plainball",
					.pos = e->pos + 60*cexp(2.0*I*M_PI/c*i),
					.color = RGBA(j == 0, j == 1, j == 2, 0.0),
					.rule = eigenstate_proj,
					.args = {
						1*n,
						1,
						60,
						0.6*I*n*(j-1)*cexp(0.4*I-0.1*I*global.diff)
					},
				);
			}
		}
	}

	return 1;
}

int baryon_reset(Enemy *baryon, int t) {
	if(t < 0) {
		return 1;
	}

	for(Enemy *e = global.enemies.first; e; e = e->next) {
		if(e->visual_rule == BaryonCenter) {
			complex targ_pos = baryon->pos0 - e->pos0 + e->pos;
			GO_TO(baryon, targ_pos, 0.1);

			return 1;
		}
	}

	GO_TO(baryon, baryon->pos0, 0.1);
	return 1;
}

void elly_eigenstate(Boss *b, int t) {
	TIMER(&t);

	AT(0) {
		aniplayer_queue(&b->ani, "snipsnip", 0);
		set_baryon_rule(baryon_eigenstate);
	}

	AT(EVENT_DEATH)
		set_baryon_rule(baryon_reset);
}

int broglie_particle(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		free_ref(p->args[0]);
		return ACTION_ACK;
	}

	/*
	if(t == EVENT_BIRTH) {
		// hidden and no collision detection until scattertime
		p->type = FakeProj;
		p->draw = ProjNoDraw;
	}
	*/

	if(t < 0) {
		return ACTION_ACK;
	}

	int scattertime = creal(p->args[1]);

	if(t < scattertime) {
		Laser *laser = (Laser*)REF(p->args[0]);

		if(laser) {
			complex oldpos = p->pos;
			p->pos = laser->prule(laser, min(t, cimag(p->args[1])));

			if(oldpos != p->pos) {
				p->angle = carg(p->pos - oldpos);
			}
		}
	} else {
		if(t == scattertime && p->type != DeadProj) {
			p->type = EnemyProj;
			p->draw_rule = ProjDraw;
			p->flags &= ~(PFLAG_NOCLEARBONUS | PFLAG_NOCLEAREFFECT);

			double angle_ampl = creal(p->args[3]);
			double angle_freq = cimag(p->args[3]);

			p->angle += angle_ampl * sin(t * angle_freq) * cos(2 * t * angle_freq);
			p->args[2] = -cabs(p->args[2]) * cexp(I*p->angle);

			play_sound("redirect");
		}

		p->angle = carg(p->args[2]);
		p->pos = p->pos + p->args[2];
	}

	return 1;
}

#define BROGLIE_PERIOD (420 - 30 * global.diff)

void broglie_laser_logic(Laser *l, int t) {
	double hue = cimag(l->args[3]);

	if(t == EVENT_BIRTH) {
		l->color = *HSLA(hue, 1.0, 0.5, 0.0);
	}

	if(t < 0) {
		return;
	}

	int dt = l->timespan * l->speed;
	float charge = min(1, pow((double)t / dt, 4));
	l->color = *HSLA(hue, 1.0, 0.5 + 0.2 * charge, 0.0);
	l->width_exponent = 1.0 - 0.5 * charge;
}

int broglie_charge(Projectile *p, int t) {
	if(t == EVENT_BIRTH) {
		play_sound_ex("shot3", 10, false);
	}

	if(t < 0) {
		return ACTION_ACK;
	}

	int firetime = creal(p->args[1]);

	if(t == firetime) {
		play_sound_ex("laser1", 10, true);
		play_sound("boom");

		global.shake_view = 5.0;
		global.shake_view_fade = 0.25;

		Color clr = p->color;
		clr.a = 0;

		PARTICLE(
			.sprite = "blast",
			.pos = p->pos,
			.color = &clr,
			.timeout = 35,
			.draw_rule = GrowFade,
			.args = { 0, 2.4 },
		);

		int attack_num = creal(p->args[2]);
		double hue = creal(p->args[3]);

		p->pos -= p->args[0] * 15;
		complex aim = cexp(I*p->angle);

		double s_ampl = 30 + 2 * attack_num;
		double s_freq = 0.10 + 0.01 * attack_num;

		for(int lnum = 0; lnum < 2; ++lnum) {
			Laser *l = create_lasercurve4c(p->pos, 75, 100, RGBA(1, 1, 1, 0), las_sine,
				5*aim, s_ampl, s_freq, lnum * M_PI +
				I*(hue + lnum * (M_PI/12)/(M_PI/2)));

			l->width = 20;
			l->lrule = broglie_laser_logic;

			int pnum = 0;
			double inc = pow(1.10 - 0.10 * (global.diff - D_Easy), 2);

			for(double ofs = 0; ofs < l->deathtime; ofs += inc, ++pnum) {
				bool fast = global.diff == D_Easy || pnum & 1;

				PROJECTILE(
					.sprite = fast ? "thickrice" : "rice",
					.pos = l->prule(l, ofs),
					.color = HSLA(hue + lnum * (M_PI/12)/(M_PI/2), 1.0, 0.5, 0.0),
					.rule = broglie_particle,
					.args = {
						add_ref(l),
						I*ofs + l->timespan + ofs - 10,
						fast ? 2.0 : 1.5,
						(1 + 2 * ((global.diff - 1) / (double)(D_Lunatic - 1))) * M_PI/11 + s_freq*10*I
					},
					.draw_rule = ProjNoDraw,
					.flags = PFLAG_NOCLEARBONUS | PFLAG_NOCLEAREFFECT | PFLAG_NOSPAWNZOOM,
					.type = FakeProj,
				);
			}
		}

		return ACTION_DESTROY;
	} else {
		float f = pow(clamp((140 - (firetime - t)) / 90.0, 0, 1), 8);
		complex o = p->pos - p->args[0] * 15;
		p->args[0] *= cexp(I*M_PI*0.2*f);
		p->pos = o + p->args[0] * 15;

		if(f > 0.1) {
			play_loop("charge_generic");

			complex n = cexp(2.0*I*M_PI*frand());
			float l = 50*frand()+25;
			float s = 4+f;

			Color *clr = color_lerp(RGB(1, 1, 1), &p->color, clamp((1 - f * 0.5), 0.0, 1.0));
			clr->a = 0;

			PARTICLE(
				.sprite = "flare",
				.pos = p->pos+l*n,
				.color = clr,
				.draw_rule = Fade,
				.rule = linear,
				.timeout = l/s,
				.args = { -s*n },
			);
		}
	}

	return ACTION_NONE;
}

int baryon_broglie(Enemy *e, int t) {
	if(t < 0) {
		return 1;
	}

	if(!global.boss) {
		return ACTION_DESTROY;
	}

	int period = BROGLIE_PERIOD;
	int t_real = t;
	int attack_num = t_real / period;
	t %= period;

	TIMER(&t);
	Enemy *master = NULL;

	for(Enemy *en = global.enemies.first; en; en = en->next) {
		if(en->visual_rule == Baryon) {
			master = en;
			break;
		}
	}

	assert(master != NULL);

	if(master != e) {
		if(t_real < period) {
			GO_TO(e, global.boss->pos, 0.03);
		} else {
			e->pos = global.boss->pos;
		}

		return 1;
	}

	int delay = 140;
	int step = 15;
	int cnt = 3;
	int fire_delay = 120;

	static double aim_angle;

	AT(delay) {
		elly_clap(global.boss,fire_delay);
		aim_angle = carg(e->pos - global.boss->pos);
	}

	FROM_TO(delay, delay + step * cnt - 1, step) {
		double a = 2*M_PI * (0.25 + 1.0/cnt*_i);
		complex n = cexp(I*a);
		double hue = (attack_num * M_PI + a + M_PI/6) / (M_PI*2);

		PROJECTILE(
			.sprite = "ball",
			.pos = e->pos + 15*n,
			.color = HSLA(hue, 1.0, 0.55, 0.0),
			.rule = broglie_charge,
			.args = {
				n,
				(fire_delay - step * _i),
				attack_num,
				hue
			},
			.flags = PFLAG_NOCLEAR,
			.angle = (2*M_PI*_i)/cnt + aim_angle,
		);
	}

	if(t < delay /*|| t > delay + fire_delay*/) {
		complex target_pos = global.boss->pos + 100 * cexp(I*carg(global.plr.pos - global.boss->pos));
		GO_TO(e, target_pos, 0.03);
	}

	return 1;
}

void elly_broglie(Boss *b, int t) {
	TIMER(&t);

	AT(0) {
		set_baryon_rule(baryon_broglie);
	}

	AT(EVENT_DEATH) {
		set_baryon_rule(baryon_reset);
	}

	if(t < 0) {
		return;
	}

	int period = BROGLIE_PERIOD;
	double ofs = 100;

	complex positions[] = {
		VIEWPORT_W-ofs + ofs*I,
		VIEWPORT_W-ofs + ofs*I,
		ofs + (VIEWPORT_H-ofs)*I,
		ofs + ofs*I,
		ofs + ofs*I,
		VIEWPORT_W-ofs + (VIEWPORT_H-ofs)*I,
	};

	if(t/period > 0) {
		GO_TO(b, positions[(t/period) % (sizeof(positions)/sizeof(complex))], 0.02);
	}
}

int baryon_nattack(Enemy *e, int t) {
	if(t < 0)
		return 1;

	TIMER(&t);

	e->pos = global.boss->pos + (e->pos-global.boss->pos)*cexp(0.006*I);

	FROM_TO_SND("shot1_loop",30, 10000, (7 - global.diff)) {
		float a = 0.2*_i + creal(e->args[2]) + 0.006*t;
		float ca = a + t/60.0f;
		PROJECTILE(
			.sprite = "ball",
			.pos = e->pos+40*cexp(I*a),
			.color = RGB(cos(ca), sin(ca), cos(ca+2.1)),
			.rule = asymptotic,
			.args = {
				(1+0.2*global.diff)*cexp(I*a),
				3
			}
		);
	}

	return 1;
}

/* !!! You are entering Akari danmaku code !!! */
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

	AT(EVENT_BIRTH) {
		play_sound("shot3");
	}

	AT(EVENT_DEATH) {
		Enemy *e = (Enemy*)REF(p->args[1]);

		if(!e) {
			return ACTION_ACK;
		}

		// play_sound_ex("shot3",8, false);
		play_sound("shot_special1");

		double rad = SAFE_RADIUS_MAX * (0.6 - 0.2 * (double)(D_Lunatic - global.diff) / 3);

		create_laser(p->pos, 12, 60, RGBA(0.2, 1, 0.5, 0), las_circle, ricci_laser_logic,  6*M_PI +  0*I, rad, add_ref(e), p->pos - e->pos);
		create_laser(p->pos, 12, 60, RGBA(0.2, 0.4, 1, 0), las_circle, ricci_laser_logic,  6*M_PI + 30*I, rad, add_ref(e), p->pos - e->pos);

		create_laser(p->pos, 1,  60, RGBA(1.0, 0.0, 0, 0), las_circle, ricci_laser_logic, -6*M_PI +  0*I, rad, add_ref(e), p->pos - e->pos)->width = 10;
		create_laser(p->pos, 1,  60, RGBA(1.0, 0.0, 0, 0), las_circle, ricci_laser_logic, -6*M_PI + 30*I, rad, add_ref(e), p->pos - e->pos)->width = 10;

		free_ref(p->args[1]);
		return ACTION_ACK;
	}

	if(t < 0) {
		return ACTION_ACK;
	}

	Enemy *e = (Enemy*)REF(p->args[1]);

	if(!e) {
		return ACTION_DESTROY;
	}

	p->pos = e->pos + p->pos0;

	if(t > 30)
		return ACTION_DESTROY;

	return ACTION_NONE;
}

int baryon_ricci(Enemy *e, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

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
		for(Enemy *reference = global.enemies.first; reference; reference = reference->next) {
			if(reference->logic_rule == baryon_ricci && (int)(creal(reference->args[2])+0.5) == 0) {
				e->pos = global.boss->pos+(reference->pos-global.boss->pos)*cexp(I*2*M_PI*(1./6*creal(e->args[2])));
			}
		}
	}

	int time = global.frames - global.boss->current->starttime;

	if(num % 2 == 0 && SAFE_RADIUS_PHASE_NUM(e) > 0) {
		TIMER(&time);
		float phase = SAFE_RADIUS_PHASE_NORMALIZED(e);

		if(phase < 0.55 && phase > 0.15) {
			FROM_TO(150,100000,10) {
				int c = 3;
				complex n = cexp(2*M_PI*I * (0.25 + 1.0/c*_i));
				PROJECTILE(
					.sprite = "ball",
					.pos = 15*n,
					.color = RGBA(0.0, 0.5, 0.1, 0.0),
					.rule = ricci_proj2,
					.args = {
						-n,
						add_ref(e),
					},
				);
			}
		}
	}

	return 1;
}

static int ricci_proj(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	if(!global.boss) {
		p->prevpos = p->pos;
		return ACTION_DESTROY;
	}

	if(p->type == DeadProj) {
		// p->pos += p->args[0];
		p->prevpos = p->pos0 = p->pos;
		p->birthtime = global.frames;
		p->rule = asymptotic;
		p->args[0] = cexp(I*M_PI*2*frand());
		p->args[1] = 9;
		return ACTION_NONE;
	}

	int time = global.frames-global.boss->current->starttime;

	complex shift = 0;
	p->pos = p->pos0 + p->args[0]*t;

	double influence = 0;

	for(Enemy *e = global.enemies.first; e; e = e->next) {
		if(e->logic_rule != baryon_ricci) {
			continue;
		}

		int num = creal(e->args[2])+0.5;

		if(num % 2 == 0) {
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
	p->prevpos = p->pos;

	float a = 0.5 + 0.5 * max(0,tanh((time-80)/100.))*clamp(influence,0.2,1);
	a *= min(1, t / 20.0f);

	/*
	p->color = derive_color(p->color, CLRMASK_B|CLRMASK_A,
		rgba(0, 0, cabs(shift)/20.0, a)
	);
	*/

	p->color.r = 0.5;
	p->color.g = 0;
	p->color.b = cabs(shift)/20.0;
	p->color.a = 0;
	// HACK: default bullet shader multiplies final color by (1 - param[0]).
	// This is currently the only way to influence the white part, sadly.
	p->shader_params.vector[0] = (1 - a);

	return ACTION_NONE;
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

	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.03);

	FROM_TO(30, 100000, interval) {
		for(int i = 0; i < c; i++) {
			int w = VIEWPORT_W;
			complex pos = 0.5 * w/(float)c + fmod(w/(float)c*(i+0.5*_i),w) + (VIEWPORT_H+10)*I;

			PROJECTILE("ball", pos, RGBA(0, 0, 0, 0), ricci_proj, { -v*I },
				.flags = PFLAG_NOSPAWNZOOM | PFLAG_NOCLEAR | PFLAG_GRAZESPAM,
				.max_viewport_dist = SAFE_RADIUS_MAX,
			);
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
/* Thank you for visiting Akari danmaku code (tm) */

void elly_baryonattack(Boss *b, int t) {
	TIMER(&t);
	AT(0)
		set_baryon_rule(baryon_nattack);
	AT(EVENT_DEATH)
		set_baryon_rule(baryon_reset);
}

void elly_baryonattack2(Boss *b, int t) {
	TIMER(&t);
	AT(0) {
		aniplayer_queue(&b->ani, "snipsnip", 0);
		set_baryon_rule(baryon_nattack);
	}
	AT(EVENT_DEATH)
		set_baryon_rule(baryon_reset);

	FROM_TO(100, 100000, 200-5*global.diff) {
		play_sound("shot_special1");

		if(_i % 2) {
			int cnt = 5;
			for(int i = 0; i < cnt; ++i) {
				float a = M_PI/4;
				a = a * (i/(float)cnt) - a/2;
				complex n = cexp(I*(a+carg(global.plr.pos-b->pos)));

				for(int j = 0; j < 3; ++j) {
					PROJECTILE("bigball", b->pos, RGB(0,0.2,0.9), asymptotic, { n, 2 * j });
				}
			}
		} else {
			int x, y;
			int w = 1+(global.diff > D_Normal);
			complex n = cexp(I*carg(global.plr.pos-b->pos));

			for(x = -w; x <= w; x++)
				for(y = -w; y <= w; y++)
					PROJECTILE("bigball", b->pos+25*(x+I*y)*n, RGB(0,0.2,0.9), asymptotic, { n, 3 });
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
			play_sound_delayed("laser1",10,true,200);

			Laser *l = create_laser(e->pos, 200, 300, RGBA(0.1+0.9*(g>3), 0, 1-0.9*(g>3), 0), las_linear, lhc_laser_logic, (1-2*(g>3))*VIEWPORT_W*0.005, 200+30.0*I, add_ref(e), 0);
			l->unclearable = true;
		}
	}

	GO_TO(e, VIEWPORT_W*(creal(e->pos0) > VIEWPORT_W/2)+I*cimag(e->args[3]) + (100-0.4*t1)*I*(1-2*(g > 3)), 0.02);

	return 1;
}

void elly_lhc(Boss *b, int t) {
	TIMER(&t);

	AT(0)
		set_baryon_rule(baryon_lhc);
	AT(EVENT_DEATH) {
		set_baryon_rule(baryon_reset);
		global.shake_view_fade=1;
	}

	FROM_TO(260, 10000, 400) {
		elly_clap(b,100);
	}

	FROM_TO(280, 10000, 400) {
		int i;
		int c = 30+10*global.diff;
		complex pos = VIEWPORT_W/2 + 100.0*I+400.0*I*((t/400)&1);

		global.shake_view = 16;
		play_sound("boom");

		for(i = 0; i < c; i++) {
			complex v = 3*cexp(2.0*I*M_PI*frand());
			tsrand_fill(4);
			create_lasercurve2c(pos, 70+20*global.diff, 300, RGBA(1, 1, 1, 0), las_accel, v, 0.02*frand()*copysign(1,creal(v)))->width=15;

			PROJECTILE("soul",    pos, RGBA(0.4, 0.0, 1.0, 0.0), linear,
				.args = { (1+2.5*afrand(0))*cexp(2.0*I*M_PI*afrand(1)) },
			);
			PROJECTILE("bigball", pos, RGBA(1.0, 0.0, 0.4, 0.0), linear,
				.args = { (1+2.5*afrand(2))*cexp(2.0*I*M_PI*afrand(3)) },
			);
		}
	}

	FROM_TO(0, 100000,7-global.diff) {
		play_sound_ex("shot2",10,false);
		PROJECTILE("ball", b->pos, RGBA(0.0, 0.4, 1.0, 0.0), asymptotic,
			.args = { cexp(2.0*I*_i), 3 },
		);
	}

	FROM_TO(300, 10000, 400) {
		global.shake_view = 0;
	}
}

int baryon_explode(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		free_ref(e->args[1]);
		petal_explosion(24, e->pos);
		play_sound("boom");
		global.shake_view = max(15, global.shake_view);

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
	}/* else {
		PARTICLE(
			.sprite = "smoke",
			.pos = e->pos+10*frand()*cexp(2.0*I*M_PI*frand()),
			.color = RGBA(0.2 * frand(), 0.5, 0.4 * frand(), 0.1 * frand()),
			.rule = asymptotic,
			.draw_rule = ScaleFade,
			.args = { cexp(I*2*M_PI*frand()) * (1 + t/300.0), (1 + t / 200.0), (1 + 0.5*I) * (t / 200.0) },
			.timeout = 60,
			.layer = LAYER_PARTICLE_HIGH | 0x40,
		);
	}*/

	if(t > 120 && frand() < (t - 120) / 300.0) {
		e->hp = 0;
		return 1;
	}

	return 1;
}

int baryon_curvature(Enemy *e, int t) {
	int num = creal(e->args[2])+0.5;
	int odd = num&1;
	complex bpos = global.boss->pos;
	complex target = (1-2*odd)*(300+100*sin(t*0.01))*cexp(I*(2*M_PI*(num+0.5*odd)/6+0.6*sqrt(1+t*t/600.)));
	GO_TO(e,bpos+target, 0.1);

	if(global.diff > D_Easy && t % (80-4*global.diff) == 0) {
		tsrand_fill(2);
		complex pos = e->pos+60*anfrand(0)+I*60*anfrand(1);

		if(cabs(pos - global.plr.pos) > 100) {
			PROJECTILE("ball", pos, RGBA(1.0, 0.4, 1.0, 0.0), linear,
				.args = { cexp(I*carg(global.plr.pos-pos)) },
			  );
		}
	}

	return 1;
}

int curvature_bullet(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

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

	return ACTION_NONE;
}

int curvature_orbiter(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	const double w = 0.03;
	if(REF(p->args[1]) != 0 && p->args[3] == 0) {
		p->pos = ((Projectile *)REF(p->args[1]))->pos+p->args[0]*cexp(I*t*w);

		p->args[2] = p->args[0]*I*w*cexp(I*t*w);
	} else {
		p->pos += p->args[2];
	}

	if(t == EVENT_DEATH)
		free_ref(p->args[1]);

	return ACTION_NONE;
}

static double saw(double t) {
	return cos(t)+cos(3*t)/9+cos(5*t)/25;
}

int curvature_slave(Enemy *e, int t) {
	e->args[0] = -(e->args[1] - global.plr.pos);
	e->args[1] = global.plr.pos;
	play_loop("shot1_loop");

	if(t % (2+(global.diff < D_Hard)) == 0) {
		tsrand_fill(2);
		complex pos = VIEWPORT_W*afrand(0)+I*VIEWPORT_H*afrand(1);
		if(cabs(pos - global.plr.pos) > 50) {
			tsrand_fill(2);
			float speed = 0.5/(1+(global.diff < D_Hard));

			PROJECTILE(
				.sprite = "flea",
				.pos = pos,
				.color = RGB(0.1*afrand(0), 0.6,1),
				.rule = curvature_bullet,
				.args = {
					speed*cexp(2*M_PI*I*afrand(1)),
					add_ref(e)
				}
			);
		}
	}

	if(global.diff >= D_Hard && !(t%20)) {
		play_sound_ex("shot2",10,false);
		Projectile *p =PROJECTILE(
			.sprite = "bigball",
			.pos = global.boss->pos,
			.color = RGBA(0.5, 0.4, 1.0, 0.0),
			.rule = linear,
			.args = { 4*I*cexp(I*M_PI*2/5*saw(t/100.)) },
		);

		if(global.diff == D_Lunatic) {
			PROJECTILE(
				.sprite = "plainball",
				.pos = global.boss->pos,
				.color = RGBA(0.2, 0.4, 1.0, 0.0),
				.rule = curvature_orbiter,
				.args = {
					40*cexp(I*t/400),
					add_ref(p)
				},
			);
		}
	}

	return 1;
}

void elly_curvature(Boss *b, int t) {
	TIMER(&t);

	AT(EVENT_BIRTH) {
		set_baryon_rule(baryon_curvature);
		return;
	}

	AT(EVENT_DEATH) {
		set_baryon_rule(baryon_reset);

		for(Enemy *e = global.enemies.first; e; e = e->next) {
			if(e->logic_rule == curvature_slave) {
				e->hp = 0;
			}
		}

		return;
	}

	AT(50) {
		create_enemy2c(b->pos, ENEMY_IMMUNE, 0, curvature_slave, 0, global.plr.pos);
	}

	GO_TO(b, VIEWPORT_W/2+100*I+VIEWPORT_W/3*round(sin(t/200)), 0.04);

}
void elly_baryon_explode(Boss *b, int t) {
	TIMER(&t);

	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.05);

	global.shake_view_fade = 0.5;

	AT(20) {
		set_baryon_rule(baryon_explode);
	}

	AT(42) {
		fade_bgm(1.0);
	}

	FROM_TO(0, 200, 1) {
		// tsrand_fill(2);
		// petal_explosion(1, b->pos + 100*afrand(0)*cexp(2.0*I*M_PI*afrand(1)));
		global.shake_view = max(global.shake_view, 5 * _i / 200.0);

		if(_i > 30) {
			play_loop("charge_generic");
		}
	}

	AT(200) {
		tsrand_fill(2);
		global.shake_view += 30;
		global.shake_view_fade = 0.05;
		play_sound("boom");
		petal_explosion(100, b->pos + 100*afrand(0)*cexp(2.0*I*M_PI*afrand(1)));
		enemy_kill_all(&global.enemies);
	}
}

static complex wrap_around(complex *pos) {
	// This function only works approximately. If more than one of these conditions are true,
	// dir has to correspond to the wall that was passed first for the
	// current preview display to work.
	//
	// with some clever geometry this could be either fixed here or in the
	// preview calculation, but the spell as it is currently seems to work
	// perfectly with this simplified version.

	complex dir = 0;
	if(creal(*pos) < -10) {
		*pos += VIEWPORT_W;
		dir += -1;
	}
	if(creal(*pos) > VIEWPORT_W+10) {
		*pos -= VIEWPORT_W;
		dir += 1;
	}
	if(cimag(*pos) < -10) {
		*pos += I*VIEWPORT_H;
		dir += -I;
	}
	if(cimag(*pos) > VIEWPORT_H+10) {
		*pos -= I*VIEWPORT_H;
		dir += I;
	}

	return dir;
}

static int elly_toe_boson_effect(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	p->angle = creal(p->args[3]) * max(0, t) / (double)p->timeout;
	return ACTION_NONE;
}

static Color* boson_color(Color *out_clr, int pos, int warps) {
	float f = pos / 3.0;
	*out_clr = *HSL((warps-0.3*(1-f))/3.0, 1+f, 0.5);
	return out_clr;
}

static int elly_toe_boson(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	p->angle = carg(p->args[0]);

	int activate_time = creal(p->args[2]);
	int num_in_trail = cimag(p->args[2]);
	assert(activate_time >= 0);

	Color thiscolor_additive = p->color;
	thiscolor_additive.a = 0;

	if(t < activate_time) {
		float fract = clamp(2 * (t+1) / (float)activate_time, 0, 1);

		float a = 0.1;
		float x = fract;
		float modulate = sin(x * M_PI) * (0.5 + cos(x * M_PI));
		fract = (pow(a, x) - 1) / (a - 1);
		fract *= (1 + (1 - x) * modulate);

		p->pos = p->args[3] * fract + p->pos0 * (1 - fract);

		if(t % 3 == 0) {
			PARTICLE(
				.sprite = "smoothdot",
				.pos = p->pos,
				.color = &thiscolor_additive,
				.rule = linear,
				.timeout = 10,
				.draw_rule = Fade,
				.args = { 0, p->args[0] },
			);
		}

		return ACTION_NONE;
	}

	if(t == activate_time) {
		play_sound("shot_special1");
		play_sound("redirect");

		for(int i = 0; i < 3; ++i) {
			PARTICLE(
				.sprite = "stain",
				.pos = p->pos,
				.color = RGBA(i==0, i==1, i==2, 0.0),
				.rule = elly_toe_boson_effect,
				.draw_rule = ScaleFade,
				.timeout = 60,
				.args = {
					0,
					p->args[0] * 1.0,
					0.5 * (0.8 + 2.0 * I),
					M_PI*2*frand(),
				},
				.angle = 0,
			);
		}
	}

	p->pos += p->args[0];
	complex prev_pos = p->pos;
	int warps_left = creal(p->args[1]);
	int warps_initial = cimag(p->args[1]);

	if(wrap_around(&p->pos) != 0) {
		p->prevpos = p->pos; // don't lerp

		if(warps_left-- < 1) {
			p->type = FakeProj; // prevent invisible collision at would-be warp location
			return ACTION_DESTROY;
		}

		p->args[1] = warps_left + warps_initial * I;

		if(num_in_trail == 3) {
			play_sound_ex("warp", 0, false);
			// play_sound("redirect");
		}

		PARTICLE(
			.sprite = "myon",
			.pos = prev_pos,
			.color = &thiscolor_additive,
			.timeout = 50,
			.draw_rule = ScaleFade,
			.args = { 0, 0, 5.00 },
			.angle = M_PI*2*frand(),
		);

		PARTICLE(
			.sprite = "stardust",
			.pos = prev_pos,
			.color = &thiscolor_additive,
			.timeout = 60,
			.draw_rule = ScaleFade,
			.args = { 0, 0, 3 * I },
			.angle = M_PI*2*frand(),
		);

		boson_color(&p->color, num_in_trail, warps_initial - warps_left);
		thiscolor_additive = p->color;
		thiscolor_additive.a = 0;

		PARTICLE("stardust", p->pos, &thiscolor_additive, elly_toe_boson_effect,
			.draw_rule = ScaleFade,
			.timeout = 30,
			.args = { 0, p->args[0] * 2, 3 * I, M_PI*2*frand() },
			.angle = 0,
		);
	}

	float tLookahead = 40;
	complex posLookahead = p->pos+p->args[0]*tLookahead;
	complex dir = wrap_around(&posLookahead);
	if(dir != 0 && t%3 == 0 && warps_left > 0) {
		complex pos0 = posLookahead - VIEWPORT_W/2*(1-creal(dir))-I*VIEWPORT_H/2*(1-cimag(dir));

		// Re [a b^*] behaves like the 2D vector scalar product
		float tOvershoot = creal(pos0*conj(dir))/creal(p->args[0]*conj(dir));
		posLookahead -= p->args[0]*tOvershoot;

		Color *clr = boson_color(&(Color){0}, num_in_trail, warps_initial - warps_left + 1);
		clr->a = 0;

		PARTICLE(
			.sprite = "smoothdot",
			.pos = posLookahead,
			.color = clr,
			.timeout = 10,
			.rule = linear,
			.draw_rule = Fade,
			.args = { p->args[0] },
			.flags = PFLAG_REQUIREDPARTICLE,
		);
	}

	return ACTION_NONE;
}

#define FERMIONTIME 1000
#define HIGGSTIME 1700
#define YUKAWATIME 2200
#define SYMMETRYTIME (HIGGSTIME+200)
#define BREAKTIME (YUKAWATIME+400)

static bool elly_toe_its_yukawatime(complex pos) {
	int t = global.frames-global.boss->current->starttime;

	if(pos == 0)
		return t>YUKAWATIME;

	int sectors = 4;

	pos -= global.boss->pos;
	if(((int)(carg(pos)/2/M_PI*sectors+sectors+0.005*t))%2)
		return t > YUKAWATIME+0.2*cabs(pos);

	return false;
}

static int elly_toe_fermion_yukawa_effect(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		free_ref(p->args[0]);
	}

	if(t < 0) {
		return ACTION_ACK;
	}

	Projectile *master = REF(p->args[0]);

	if(master) {
		p->pos = master->pos;
	}

	return ACTION_NONE;
}

static int elly_toe_fermion(Projectile *p, int t) {
	if(t == EVENT_BIRTH) {
		p->pos0 = 0;
	}

	if(t < 0) {
		return ACTION_ACK;
	}

	int speeduptime = creal(p->args[2]);
	if(t > speeduptime)
		p->pos0 += p->pos0/cabs(p->pos0)*(t-speeduptime)*0.01;
	else
		p->pos0 += (p->args[0]-p->pos0)*0.01;
	p->pos = global.boss->pos+p->pos0*cexp(I*0.01*t)+5*cexp(I*t*0.05+I*p->args[1]);

	Color thiscolor_additive = p->color;
	thiscolor_additive.a = 0;

	if(t > 0 && t % 5 == 0) {
		double particle_scale = min(1.0, 0.5 * p->sprite->w / 28.0);

		PARTICLE(
			.sprite = "stardust",
			.pos = p->pos,
			.color = &thiscolor_additive,
			.timeout = min(t / 6.0, 10),
			.draw_rule = ScaleFade,
			.args = { 0, 0, particle_scale * (1 + 2 * I) },
			.angle = M_PI*2*frand(),
		);
	}

	if(creal(p->args[3]) < 0) // add a way to disable the yukawa phase
		return ACTION_NONE;

	if(elly_toe_its_yukawatime(p->pos)) {
		if(!p->args[3]) {
			projectile_set_prototype(p, pp_bigball);
			p->args[3]=1;
			play_sound_ex("shot_special1", 5, false);

			PARTICLE(
				.sprite = "myon",
				.pos = p->pos,
				.color = &thiscolor_additive,
				.rule = elly_toe_fermion_yukawa_effect,
				.timeout = 60,
				.draw_rule = ScaleFade,
				.args = { add_ref(p), 0, 20 * I },
				.angle = M_PI*2*frand(),
			);
		}

		p->pos0*=1.01;
	} else if(p->args[3]) {
		projectile_set_prototype(p, pp_ball);
		p->args[3]=0;
	}

	return ACTION_NONE;
}

static int elly_toe_higgs(Projectile *p, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	if(elly_toe_its_yukawatime(p->pos)) {
		if(!p->args[3]) {
			projectile_set_prototype(p, pp_rice);
			p->args[3]=1;
		}
		p->args[0]*=1.01;
	} else if(p->args[3]) {
		projectile_set_prototype(p, pp_flea);
		p->args[3]=0;
	}

	int global_time = global.frames - global.boss->current->starttime - HIGGSTIME;
	int max_time = SYMMETRYTIME - HIGGSTIME;
	double rotation = 0.5 * M_PI * max(0, (int)global.diff - D_Normal);

	if(global_time > max_time) {
		global_time = max_time;
	}

	complex vel = p->args[0] * cexp(I*rotation*global_time/(float)max_time);
	p->pos = p->pos0 + t * vel;
	p->angle = carg(vel);

	return ACTION_NONE;
}

static complex elly_toe_laser_pos(Laser *l, float t) { // a[0]: direction, a[1]: type, a[2]: width
	int type = creal(l->args[1]+0.5);

	if(t == EVENT_BIRTH) {
		switch(type) {
		case 0:
			l->shader = r_shader_get_optional("lasers/elly_toe_fermion");
			l->color = *RGBA(0.4, 0.4, 1.0, 0.0);
			break;
		case 1:
			l->shader = r_shader_get_optional("lasers/elly_toe_photon");
			l->color = *RGBA(1.0, 0.4, 0.4, 0.0);
			break;
		case 2:
			l->shader = r_shader_get_optional("lasers/elly_toe_gluon");
			l->color = *RGBA(0.4, 1.0, 0.4, 0.0);
			break;
		case 3:
			l->shader = r_shader_get_optional("lasers/elly_toe_higgs");
			l->color = *RGBA(1.0, 0.4, 1.0, 0.0);
			break;
		default:
			log_fatal("Unknown Elly laser type.");
		}
		return 0;
	}

	double width = creal(l->args[2]);
	switch(type) {
	case 0:
		return l->pos+l->args[0]*t;
	case 1:
		return l->pos+l->args[0]*(t+width*I*sin(t/width));
	case 2:
		return l->pos+l->args[0]*(t+width*(0.6*(cos(3*t/width)-1)+I*sin(3*t/width)));
	case 3:
		return l->pos+l->args[0]*(t+floor(t/width)*width);
	}

	return 0;
}

#define LASER_EXTENT (4+1.5*global.diff-D_Normal)
#define LASER_LENGTH 60

static int elly_toe_laser_particle_rule(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		free_ref(p->args[3]);
	}

	if(t < 0) {
		return ACTION_ACK;
	}

	Laser *l = REF(p->args[3]);

	if(!l) {
		return ACTION_DESTROY;
	}

	p->pos = l->prule(l, 0);

	return ACTION_NONE;
}

static void elly_toe_laser_particle(Laser *l, complex origin) {
	Color *c = color_mul(COLOR_COPY(&l->color), &l->color);
	c->a = 0;

	PARTICLE("stardust", origin, c, elly_toe_laser_particle_rule,
		.draw_rule = ScaleFade,
		.timeout = 30,
		.args = { 0, 0, 2 * I, add_ref(l) },
		.angle = M_PI*2*frand(),
	);

	PARTICLE("stain", origin, c, elly_toe_laser_particle_rule,
		.draw_rule = ScaleFade,
		.timeout = 20,
		.args = { 0, 0, 2 * I, add_ref(l) },
		.angle = M_PI*2*frand(),
	);

	PARTICLE("smoothdot", origin, c, elly_toe_laser_particle_rule,
		.draw_rule = ScaleFade,
		.timeout = 40,
		.args = { 0, 0, 1, add_ref(l) },
		.angle = M_PI*2*frand(),
	);
}

static void elly_toe_laser_logic(Laser *l, int t) {
	if(t == l->deathtime) {
		static const int transfer[][4] = {
			{1, 0, 0, 0},
			{0,-1,-1, 3},
			{0,-1, 2, 3},
			{0, 3, 3, 1}
		};

		complex newpos = l->prule(l,t);
		if(creal(newpos) < 0 || cimag(newpos) < 0 || creal(newpos) > VIEWPORT_W || cimag(newpos) > VIEWPORT_H)
			return;

		int newtype, newtype2;
		do {
			newtype = 3.999999*frand();
			newtype2 = transfer[(int)(creal(l->args[1])+0.5)][newtype];
			// actually in this case, gluons are also always possible
			if(newtype2 == 1 && frand() > 0.5) {
				newtype = 2;
			}

			// I removed type 3 because i can’t draw dotted lines and it would be too difficult either way
		} while(newtype2 == -1 || newtype2 == 3 || newtype == 3);

		complex origin = l->prule(l,t);
		complex newdir = cexp(0.3*I);

		Laser *l1 = create_laser(origin,LASER_LENGTH,LASER_LENGTH,RGBA(1, 1, 1, 0),
			elly_toe_laser_pos,elly_toe_laser_logic,
			l->args[0]*newdir,
			newtype,
			LASER_EXTENT,
			0
		);

		Laser *l2 = create_laser(origin,LASER_LENGTH,LASER_LENGTH,RGBA(1, 1, 1, 0),
			elly_toe_laser_pos,elly_toe_laser_logic,
			l->args[0]/newdir,
			newtype2,
			LASER_EXTENT,
			0
		);

		elly_toe_laser_particle(l1, origin);
		elly_toe_laser_particle(l2, origin);
	}
	l->pos+=I*0.2;
}


// This spell contains some obscure (and some less obscure) physics references.
// However the first priority was good looks and playability, so most of the
// references are pretty bad.
//
// I am waiting for the day somebody notices this and writes a 10-page comment
// about it on reddit. ;_; So I add a disclaimer beforehand (2018). Maybe it can also
// serve as entertainment for the interested hacker stumbling across these
// lines.
//
// Disclaimer:
//
// 1. This spell is based on the Standard Model of Particle physics, which is
//    very complicated, so the picture here is always very very very dumbed down.
//
// 2. The first two phases stand for the bosons and fermions in the model. My
//    way of representing them was to choose a heavily overlapping pattern for
//    the bosons and a non-overlapping one for the fermions (nod to the Pauli
//    principle). The rest of them is not a good reference at all.
//
// 3. Especially this rgb rotating thing the fermions do has not much to do
//    with physics. Maybe it represents color charge? And the rotation
//    represents the way fermions transform strangely under rotations?
//
// 4. The symmetry thing is one fm closer to the truth. In reality the symmetry
//    that is broken in the phase we live in is continuous, but that is not easy
//    to do in danmaku, so I took a mirror symmetry and broke it.
//    Then in reality, the higgs gets a funny property that makes the other
//    things which interact with it get a mass.
//    In the spell, I represented that by adding those sectors where the fermions
//    become big (massive ;)).
//
// 5. The last part is a nod to Feynman diagrams and perturbation theory. Have
//    been planning to add those to the game for quite some time. They are one
//    of the iconic sights in modern physics and they can be used as laser danmaku.
//    The rules here are again simplified:
//
//    I only use QED and QCD (not the whole SM) and leave out ghosts and the 4
//    gluon vertex (because I can’t draw dashed lasers and the 4-vertex doesn’t
//    fit into the pattern).
//
//    Of course, drawing laser danmaku is easy. The actual calculations behind
//    the diagrams are one of the most complicated fields in Physics (fun fact:
//    they are a driving force for CAS software and high precision integration
//    methods). This whole thing is called perturbation theory and one of its
//    problems is that it doesn’t really converge, so that’s the reason behind
//    “Perturbation theory breaking down”
//
// (9). I’m only human though and this is not exactly the stuff I do, so I’m
//      not an expert and more things might be off.
//
void elly_theory(Boss *b, int time) {
	if(time == EVENT_BIRTH) {
		global.shake_view = 10;
		return;
	}

	if(time < 0) {
		GO_TO(b, ELLY_TOE_TARGET_POS, 0.1);
		return;
	}

	TIMER(&time);

	AT(0) {
		assert(cabs(b->pos - ELLY_TOE_TARGET_POS) < 1);
		b->pos = ELLY_TOE_TARGET_POS;
		global.shake_view = 0;
		elly_clap(b,50000);
	}

	int fermiontime = FERMIONTIME;
	int higgstime = HIGGSTIME;
	int symmetrytime = SYMMETRYTIME;
	int yukawatime = YUKAWATIME;
	int breaktime = BREAKTIME;

	{
		int count = 30;
		int step = 1;
		int start_time = 0;
		int end_time = fermiontime + 1000;
		int cycle_time = count * step - 1;
		int cycle_step = 200;
		int solo_cycles = (fermiontime - start_time) / cycle_step - global.diff + 1;
		int warp_cycles = (higgstime - start_time) / cycle_step - 1;

		if(time - start_time <= cycle_time) {
			--count; // get rid of the spawn safe spot on first cycle
			cycle_time -= step;
		}

		FROM_TO_INT(start_time, end_time, cycle_step, cycle_time, step) {
			play_sound("shot2");

			int pnum = _ni;
			int num_warps = 0;

			if(_i < solo_cycles) {
				num_warps = global.diff;
			} else if(_i < warp_cycles && global.diff > D_Normal) {
				num_warps = 1;
			}

			// The fact that there are 4 bullets per trail is actually one of the physics references.
			for(int i = 0; i < 4; i++) {
				pnum = (count - pnum - 1);

				complex dir = I*cexp(I*(2*M_PI/count*(pnum+0.5)));
				dir *= cexp(I*0.15*sign(creal(dir))*sin(_i));

				complex bpos = b->pos + 18 * dir * i;

				PROJECTILE("rice",
					.pos = b->pos,
					.color = boson_color(&(Color){0}, i, 0),
					.rule = elly_toe_boson,
					.args = {
						2.5*dir,
						num_warps * (1 + I),
						42*2 - step * _ni + i*I, // real: activation delay, imag: pos in trail (0 is furtherst from head)
						bpos,
					},
					.max_viewport_dist = 20,
				);
			}
		}
	}

	FROM_TO(fermiontime,yukawatime+250,2) {
		// play_loop("noise1");
		play_sound_ex("shot1", 5, false);

		complex dest = 100*cexp(I*1*_i);
		for(int clr = 0; clr < 3; clr++) {
			PROJECTILE("ball", b->pos, RGBA(clr==0, clr==1, clr==2, 0),
				.rule = elly_toe_fermion,
				.args = {
					dest,
					clr*2*M_PI/3,
					40,
				},
				.max_viewport_dist=50,
			);
		}
	}

	AT(fermiontime) {
		play_sound("charge_generic");
		global.shake_view=10;
		global.shake_view_fade=1;
	}

	AT(symmetrytime) {
		play_sound("charge_generic");
		play_sound("boom");
		stagetext_add("Symmetry broken!", VIEWPORT_W/2+I*VIEWPORT_H/4, ALIGN_CENTER, get_font("big"), RGB(1,1,1), 0,100,10,20);
		global.shake_view=10;
		global.shake_view_fade=1;

		PARTICLE(
			.sprite = "blast",
			.pos = b->pos,
			.color = RGBA_MUL_ALPHA(1.0, 0.3, 0.3, 0.5),
			.timeout = 60,
			.draw_rule = GrowFade,
			.args = { 0, 4 },
			// .flags = PFLAG_DRAWADD,
		);

		for(int i = 0; i < 10; ++i) {
			PARTICLE(
				.sprite = "stain",
				.pos = b->pos,
				.color = RGBA(0.3, 0.3, 1.0, 0.0),
				.timeout = 120 + 20 * nfrand(),
				.draw_rule = ScaleFade,
				.args = { 0, 0, 2 * (1 + 5 * I) },
				.angle = M_PI*2*frand(),
			);
		}
	}

	AT(yukawatime) {
		play_sound("charge_generic");
		stagetext_add("Coupling the Higgs!", VIEWPORT_W/2+I*VIEWPORT_H/4, ALIGN_CENTER, get_font("big"), RGB(1,1,1), 0,100,10,20);
		global.shake_view=10;
		global.shake_view_fade=1;
	}

	AT(breaktime) {
		play_sound("charge_generic");
		stagetext_add("Perturbation theory", VIEWPORT_W/2+I*VIEWPORT_H/4, ALIGN_CENTER, get_font("big"), RGB(1,1,1), 0,100,10,20);
		stagetext_add("breaking down!", VIEWPORT_W/2+I*VIEWPORT_H/4+30*I, ALIGN_CENTER, get_font("big"), RGB(1,1,1), 0,100,10,20);
		global.shake_view=10;
		global.shake_view_fade=1;
	}

	FROM_TO(higgstime,yukawatime+100,4+4*(time>symmetrytime)) {
		play_loop("shot1_loop");

		int arms;

		switch(global.diff) {
			case D_Hard:    arms = 5; break;
			case D_Lunatic: arms = 7; break;
			default:        arms = 4; break;
		}

		for(int dir = 0; dir < 2; dir++) {
			for(int arm = 0; arm < arms; arm++) {
				complex v = -2*I*cexp(I*M_PI/(arms+1)*(arm+1)+0.1*I*sin(time*0.1+arm));
				if(dir)
					v = -conj(v);
				if(time>symmetrytime) {
					int t = time-symmetrytime;
					v*=cexp(-I*0.001*t*t+0.01*frand()*dir);
				}
				PROJECTILE("flea", b->pos, RGB(dir*(time>symmetrytime),0,1),
					.rule = elly_toe_higgs,
					.args = { v },
				);
			}
		}
	}

	FROM_TO(breaktime,breaktime+10000,100) {
		play_sound_ex("laser1", 0, true);

		complex phase = cexp(2*I*M_PI*frand());
		int count = 8;
		for(int i = 0; i < count; i++) {
			create_laser(b->pos,LASER_LENGTH,LASER_LENGTH/2,RGBA(1, 1, 1, 0),
				elly_toe_laser_pos,elly_toe_laser_logic,
				2*cexp(2*I*M_PI/count*i)*phase,
				0,
				LASER_EXTENT,
				0
			);
		}
	}

	FROM_TO(breaktime+35, breaktime+10000, 14) {
		play_sound("redirect");
		// play_sound("shot_special1");

		for(int clr = 0; clr < 3; clr++) {
			PROJECTILE("soul", b->pos, RGBA(clr==0, clr==1, clr==2, 0),
				.rule = elly_toe_fermion,
				.args = {
					50*cexp(1.3*I*_i),
					clr*2*M_PI/3,
					40,
					-1,
				},
				.max_viewport_dist=50,
			);
		}
	}

	FROM_TO_SND("noise1", yukawatime - 60, breaktime + 10000, 1) {};
}

void elly_spellbg_toe(Boss *b, int t) {
	r_shader("sprite_default");

	r_draw_sprite(&(SpriteParams) {
		.pos = { VIEWPORT_W/2, VIEWPORT_H/2 },
		.scale.both = 0.75 + 0.0005 * t,
		.rotation.angle = t * 0.1 * DEG2RAD,
		.sprite = "stage6/spellbg_toe",
		.color = RGB(0.6, 0.6, 0.6),
	});

	float positions[][2] = {
		{-160,0},
		{0,0},
		{0,100},
		{0,200},
		{0,300},
	};

	int delays[] = {
		-20,
		0,
		FERMIONTIME,
		HIGGSTIME,
		YUKAWATIME,
	};

	int count = sizeof(delays)/sizeof(int);
	for(int i = 0; i < count; i++) {
		if(t<delays[i])
			break;
		
		r_color(RGBA_MUL_ALPHA(1, 1, 1, 0.5*clamp((t-delays[i])*0.1,0,1)));
		char *texname = strfmt("stage6/toelagrangian/%d",i);
		float wobble = max(0,t-BREAKTIME)*0.03;
		r_mat_push();
		r_mat_translate(VIEWPORT_W/2+positions[i][0]+cos(wobble+i)*wobble,VIEWPORT_H/2-150+positions[i][1]+sin(i+wobble)*wobble,0);
		draw_sprite_batched(0,0,texname);
		free(texname);
		r_mat_pop();
	}
	
	r_color4(1, 1, 1, 1);
	r_shader_standard();
}

#undef LASER_EXTENT
#undef YUKAWATIME
#undef FERMIONTIME
#undef HIGGSTIME
#undef SYMMETRYTIME

void elly_spellbg_classic(Boss *b, int t) {
	fill_viewport(0,0,0.7,"stage6/spellbg_classic");
	r_blend(BLEND_MOD);
	r_color4(1, 1, 1, 0);
	fill_viewport(0,-t*0.005,0.7,"stage6/spellbg_chalk");
	r_blend(BLEND_PREMUL_ALPHA);
	r_color4(1, 1, 1, 1);
}

void elly_spellbg_modern(Boss *b, int t) {
	fill_viewport(0,0,0.6,"stage6/spellbg_modern");
	r_blend(BLEND_MOD);
	r_color4(1, 1, 1, 0);
	fill_viewport(0,-t*0.005,0.7,"stage6/spellbg_chalk");
	r_blend(BLEND_PREMUL_ALPHA);
	r_color4(1, 1, 1, 1);
}

void elly_spellbg_modern_dark(Boss *b, int t) {
	elly_spellbg_modern(b, t);
	fade_out(0.75 * min(1, t / 300.0));
}


static void elly_global_rule(Boss *b, int time) {
	global.boss->glowcolor = *HSL(time/120.0, 1.0, 0.25);
	global.boss->shadowcolor = *HSLA_MUL_ALPHA((time+20)/120.0, 1.0, 0.25, 0.5);
}

Boss* stage6_spawn_elly(complex pos) {
	Boss *b = create_boss("Elly", "elly", "dialog/elly", pos);
	b->global_rule = elly_global_rule;
	return b;
}

static void elly_insert_interboss_dialog(Boss *b, int t) {
	global.dialog = stage6_dialog_pre_final();
}

static void elly_begin_toe(Boss *b, int t) {
	TIMER(&t);

	AT(1) {
		start_fall_over();
		stage_start_bgm("stage6boss_phase3");
		global.shake_view_fade = 0;
	}
}

static void elly_goto_center(Boss *b, int t) {
	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.05);
}

Boss* create_elly(void) {
	Boss *b = stage6_spawn_elly(-200.0*I);

	boss_add_attack(b, AT_Move, "Catch the Scythe", 6, 0, elly_intro, NULL);
	boss_add_attack(b, AT_Normal, "Frequency", 40, 50000, elly_frequency, NULL);
	boss_add_attack_from_info(b, &stage6_spells.scythe.occams_razor, false);
	boss_add_attack(b, AT_Normal, "Frequency2", 40, 50000, elly_frequency2, NULL);
	boss_add_attack_from_info(b, &stage6_spells.scythe.orbital_clockwork, false);
	boss_add_attack_from_info(b, &stage6_spells.scythe.wave_theory, false);
	boss_add_attack(b, AT_Move, "Paradigm Shift", 3, 0, elly_paradigm_shift, NULL);
	boss_add_attack_from_info(b, &stage6_spells.baryon.many_world_interpretation, false);
	boss_add_attack(b, AT_Normal, "Baryon", 50, 55000, elly_baryonattack, NULL);
	boss_add_attack_from_info(b, &stage6_spells.baryon.wave_particle_duality, false);
	boss_add_attack_from_info(b, &stage6_spells.baryon.spacetime_curvature, false);
	boss_add_attack(b, AT_Normal, "Baryon2", 50, 55000, elly_baryonattack2, NULL);
	boss_add_attack_from_info(b, &stage6_spells.baryon.higgs_boson_uncovered, false);
	boss_add_attack_from_info(b, &stage6_spells.extra.curvature_domination, false);
	boss_add_attack(b, AT_Move, "Explode", 4, 0, elly_baryon_explode, NULL);
	boss_add_attack(b, AT_Move, "Move to center", 1, 0, elly_goto_center, NULL);
	boss_add_attack(b, AT_Immediate, "Final dialog", 0, 0, elly_insert_interboss_dialog, NULL);
	boss_add_attack(b, AT_Move, "ToE transition", 7, 0, elly_begin_toe, NULL);
	boss_add_attack_from_info(b, &stage6_spells.final.theory_of_everything, false);
	boss_start_attack(b, b->attacks);

	return b;
}

void stage6_events(void) {
	TIMER(&global.timer);

	AT(0) {
		stage_start_bgm("stage6");
		// skip_background_anim(&stage_3d_context, stage_get(6)->procs->update, 3800, &global.timer, &global.frames);
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

	AT(4160 - FADE_TIME) {
		stage_finish(GAMEOVER_WIN);
	}
}
