/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "stage6_events.h"
#include "stage6.h"
#include <global.h>

Dialog *stage6_dialog(void) {
	PlayerCharacter *pc = global.plr.mode->character;
	Dialog *d = create_dialog(pc->dialog_sprite_name, "dialog/elly");

	if(pc->id == PLR_CHAR_MARISA) {
		dadd_msg(d,Left, "And so the big bad boss behind it all is…");
		dadd_msg(d,Left, "Wait, Elly?!");
		dadd_msg(d,Right, "That’s strange; I thought Kurumi was keeping people from sneaking into my laboratory.");
		dadd_msg(d,Left, "You have a lab?! Last time we fought, ya were just some random guard. You didn’t even know any science!");
		dadd_msg(d,Right, "After Yūka was defeated and left us for Gensōkyō, Kurumi and I decided we needed to do some soul searching and find another job.");
		dadd_msg(d,Right, "We stranded ourselves in the Outside World on accident, and I found my true calling as a mathematical theorist.");
		dadd_msg(d,Right, "This world I’ve built… this is a world dictated not by the madness of Gensōkyō, but one of reason, logic, and common sense!");
		dadd_msg(d,Left, "Yeah, and that’s why it broke the barrier, didn’t it? You can’t have a borin’ world like this next t’ Gensōkyō without causin’ us issues.");
		dadd_msg(d,Right, "An unenlightened fool like you wouldn’t understand… You’re even still using magic, of all things!");
		dadd_msg(d,Left, "I understand enough to know that you’re the problem. And like any good mathematician, I’m gonna solve it by blastin’ it to bits!");
		dadd_msg(d,Right, "No! You’ll never destroy my life’s work! I’ll dissect that nonsensical magic of yours, and then I’ll tear it apart!");
	} else if(pc->id == PLR_CHAR_YOUMU) {
		dadd_msg(d,Left, "Are you the one behind this new world?");
		dadd_msg(d,Right, "That is correct.");
		dadd_msg(d,Left, "Considering that scythe of yours, I’m really quite surprised that a shikigami would be behind everything.");
		dadd_msg(d,Right, "That is because you completely misunderstand. I am not a shikigami, and never was. My duty was once to protect a powerful yōkai, but when she left for Gensōkyō, I was left behind.");
		dadd_msg(d,Right, "Together with my friend, Kurumi, we left for the Outside World and were stranded there. But I gained an incredible new power once I realized my true calling as a mathematical theorist.");
		dadd_msg(d,Left, "I don’t completely follow, but if you’re a yōkai, I can’t see how you could be a scientist. It goes against your nature.");
		dadd_msg(d,Right, "An unenlightened fool such as yourself could never comprehend my transcendence.");
		dadd_msg(d,Right, "I have become more than a simple yōkai, for I alone am now capable of discovering the secrets behind the illusion of reality!");
		dadd_msg(d,Left, "Well, whatever you’re doing here is disintegrating the barrier of common sense and madness keeping Gensōkyō from collapsing into the Outside World.");
		dadd_msg(d,Left, "You’re also infringing on the Netherworld’s space and my mistress is particularly upset about that. That means I have to stop you, no matter what.");
		dadd_msg(d,Right, "Pitiful servant of the dead. You’ll never be able to stop my life’s work from being fulfilled! I’ll simply unravel that nonsense behind your half-and-half existence!");
	} else if(pc->id == PLR_CHAR_REIMU) {
		dadd_msg(d,Left, "Who are you? You’re a lot less intimidating than I expected.");
		dadd_msg(d,Right, "I knew you would come here to interrupt me.");
		dadd_msg(d,Left, "You seem to know who I am, just like that earlier yōkai did. I can’t say the same for myself.");
		dadd_msg(d,Right, "That is because we met before in another time, when you were much younger…");
		dadd_msg(d,Right, "Back then, I was a mere guard. But then my master left Kurumi and I behind. We fell into the real world instead of Gensōkyō,");
		dadd_msg(d,Right, "and I realized that logic was far superior to the chaos of fantasy.");
		dadd_msg(d,Left, "Do you really think it’s acceptable to threaten Gensōkyō just for some petty delusion of intelligence?");
		dadd_msg(d,Right, "An unenlightened fool like you could never see how your faith blinds you.");
		dadd_msg(d,Left, "You talk big but in the end you’re just a yōkai who is full of herself. It doesn’t matter how smart your world is if it threatens mine.");
		dadd_msg(d,Left, "I’ll tear everything down all the same.");
		dadd_msg(d,Right, "You’ll regret threatening my life’s work when I take your world away. Soon you will be a shrine maiden of nothing!");
	}
	dadd_msg(d, BGM, "stage6boss");
	return d;
}

static Dialog *stage6_interboss_dialog(void) {
	PlayerCharacter *pc = global.plr.mode->character;
	Dialog *d = create_dialog(pc->dialog_sprite_name, "dialog/elly");

	dadd_msg(d, Right, "You’ve gotten this far… I can’t believe it! But that will not matter once I show you the truth of this world, and every world.");
	dadd_msg(d, Right, "Space, time, dimensions… it all becomes clear when you understand The Theory of Everything!");
	dadd_msg(d, Right, "Prepare to see the barrier destroyed!");
	
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

	FROM_TO_SND("shot1_loop",100, 180+40*global.diff, 3) {
		int i;
		for(i = 0; i < 6; i++) {
			complex n = sin(_i*0.2)*cexp(I*0.3*(i/2-1))*(1-2*(i&1));
			PROJECTILE("wave", e->pos + 120*n, rgb(1.0, 0.2-0.01*_i, 0.0), linear, {
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
	AT(EVENT_DEATH) {
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
				.texture = (i%2 == 0) ? "rice" : "flea",
				.pos = e->pos+5*(i/2)*e->args[1],
				.color = rgb(0.1*cabs(e->args[2]), 0.5, 1),
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
		PROJECTILE(
			.texture = "rice",
			.pos = e->pos + 40*cexp(I*0.6*_i+I*carg(e->args[0])),
			.color = rgb(1-psin(_i), 0.3, psin(_i)),
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

	if(t > 300) {
		return ACTION_DESTROY;
	}

	e->pos += (6-global.diff-0.005*I*t)*e->args[0];

	n = cexp(cimag(e->args[1])*I*t);
	FROM_TO_SND("shot1_loop",0,300,1) {}
	PROJECTILE(
		.texture = "bigball",
		.pos = e->pos + 80*n,
		.color = rgb(0.2, 0.5-0.5*cimag(n), 0.5+0.5*creal(n)),
		.rule = wait_proj,
		.args = {
			global.diff*cexp(0.6*I)*n,
			100
		},
		.flags = PFLAG_DRAWADD,
	);

	if(global.diff > D_Normal && t&1) {
		PROJECTILE("ball", e->pos + 80*n, rgb(0, 0.2, 0.5), accelerated,
			.args = {
				n,
				0.01*global.diff*cexp(I*carg(global.plr.pos - e->pos - 80*n))
			},
			.flags = PFLAG_DRAWADD,
		);
	}

	scythe_common(e, t);
	return 1;
}

void Scythe(Enemy *e, int t, bool render) {
	if(render) {
		return;
	}

	PARTICLE(
		.texture_ptr = get_tex("stage6/scythe"),
		.pos = e->pos+I*6*sin(global.frames/25.0),
		.draw_rule = ScaleFade,
		.rule = timeout,
		.args = { 8, e->args[2] },
		.angle = creal(e->args[1]) - M_PI/2,
	);

	PARTICLE(
		.texture = "lasercurve",
		.pos = e->pos+100*creal(e->args[2])*frand()*cexp(2.0*I*M_PI*frand()),
		.color = rgb(1.0, 0.1, 1.0),
		.draw_rule = GrowFade,
		.rule = timeout_linear,
		.args = { 60, -I+1 },
		.flags = PFLAG_DRAWADD,
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

	FROM_TO_SND("shot1_loop",40, 3000, 1) {
		float w = min(0.15, 0.0001*(t-40));
		e->pos = VIEWPORT_W/2 + 200.0*I + 200*cos(w*(t-40)+M_PI/2.0) + I*80*sin(creal(e->args[0])*w*(t-40));

		PROJECTILE(
			.texture = "ball",
			.pos = e->pos+80*cexp(I*creal(e->args[1])),
			.color = rgb(cos(creal(e->args[1])), sin(creal(e->args[1])), cos(creal(e->args[1])+2.1)),
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

	GO_TO(e, BOSS_DEFAULT_GO_POS, 0.02);
	e->args[2] = max(0.6, creal(e->args[2])-0.01*t);
	e->args[1] += (0.19-creal(e->args[1]))*0.05;
	e->args[1] = creal(e->args[1]) + I*0.9*cimag(e->args[1]);

	scythe_common(e, t);
	return 1;
}

void elly_frequency(Boss *b, int t) {
	TIMER(&t);
	b->ani.stdrow=1;
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

static void elly_clap(Boss *b, int claptime) {
	aniplayer_queue_pro(&b->ani,2,0,3,claptime,5);
	aniplayer_queue_pro(&b->ani,2,3,5,0,5);
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
			if(p->type == EnemyProj && cabs(p->pos-e->pos) < 50 && cabs(global.plr.pos-e->pos) > 50 && p->args[2] == 0) {
				e->args[3] += 1;
				//p->args[0] /= 2;
				play_sound_ex("redirect",4,false);
				if(global.diff > D_Normal && (int)(creal(e->args[3])+0.5) % (15-5*(global.diff == D_Lunatic)) == 0) {
					p->args[0] = cexp(I*f);
					p->color = rgb(1,0,0.5);
					p->tex = get_tex("proj/bullet");
					p->args[1] = 0.005*I;
				} else {
					p->args[0] = 2*cexp(I*2*M_PI*frand());
					p->color = rgba(frand(), 0, 1, 0.8);
				}
				p->args[2] = 1;
			}
		}
	}

	FROM_TO(100, 10000, 5-global.diff/2) {
		if(cabs(global.plr.pos-e->pos) > 50)
			PROJECTILE("rice", e->pos, rgb(0.3, 1, 0.8), linear, { I });
	}

	scythe_common(e, t);
	return 1;
}

void elly_newton(Boss *b, int t) {
	TIMER(&t);

	AT(0) {
		global.enemies->birthtime = global.frames;
		global.enemies->logic_rule = scythe_newton;
		elly_clap(b,100);
	}

	AT(EVENT_DEATH) {
		global.enemies->birthtime = global.frames;
		global.enemies->logic_rule = scythe_reset;
	}

	FROM_TO(0, 100000, 20+10*(global.diff>D_Normal)) {
		float a = 2.7*_i+carg(global.plr.pos-b->pos);
		int x, y;
		float w = min(D_Hard,global.diff)/2.0+1.5;

		play_sound("shot_special1");
		for(x = -w; x <= w; x++) {
			for(y = -w; y <= w; y++) {
				PROJECTILE("ball", b->pos+(x+I*y)*(18)*cexp(I*a), rgb(0, 0.5, 1), linear, { 2*cexp(I*a) });
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

		play_sound("redirect");
		if(tier <= 1+(global.diff>D_Hard) && cimag(p->args[1])*(tier+1) < n) {
			PROJECTILE(
				.texture = "flea",
				.pos = p->pos,
				.color = rgb(0.3 + 0.3 * tier, 0.6 - 0.3 * tier, 1.0),
				.rule = kepler_bullet,
				.args = {
					cabs(p->args[0])*phase,
					tier+1,
					add_ref(p)
				}
			);
		}
	}
	return 1;
}


void elly_kepler(Boss *b, int t) {
	TIMER(&t);

	AT(0) {
		global.enemies->birthtime = global.frames;
		global.enemies->logic_rule = scythe_kepler;

		PROJECTILE("soul", b->pos, rgb(0.3,0.8,1), kepler_bullet, { 50+10*global.diff });
		elly_clap(b,20);
	}

	AT(EVENT_DEATH) {
		global.enemies->birthtime = global.frames;
		global.enemies->logic_rule = scythe_reset;
	}

	FROM_TO(0, 100000, 20) {
		int c = 2;
		play_sound("shot_special1");
		for(int i = 0; i < c; i++) {
			complex n = cexp(I*2*M_PI/c*i+I*0.6*_i);
			PROJECTILE("soul", b->pos, rgb(0.3,0.8,1), kepler_bullet, {
				50*n,
				0,
				(1.4+0.1*global.diff)*n
			});
		}
	}
}

void elly_frequency2(Boss *b, int t) {
	TIMER(&t);
	b->ani.stdrow=1;
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

	FROM_TO_SND("shot1_loop",0, 2000, 3-global.diff/2) {
		complex n = sin(t*0.12*global.diff)*cexp(t*0.02*I*global.diff);
		PROJECTILE("plainball", b->pos+80*n, rgb(0,0,0.7), asymptotic, { 2*n/cabs(n), 3 });
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

	AT(250) {
		elly_clap(b,50);
    		play_sound("laser1");

	}
	FROM_TO(40, 159, 5) {
		create_laser(b->pos, 200, 10000, rgb(0,0.2,1), maxwell_laser, maxwell_laser_logic, cexp(2.0*I*M_PI/24*_i)*VIEWPORT_H*0.005, 200+15.0*I, 0, 0);
	}

}

void Baryon(Enemy *e, int t, bool render) {
	Enemy *n;
	float alpha = 1 - 0.8/(exp((cabs(global.plr.pos-e->pos)-100)/10)+1);

	if(!render) {
		if(!(t % 10) && global.boss && cabs(e->pos - global.boss->pos) > 2) {
			PARTICLE(
				.texture = "stain",
				.pos = e->pos+10*frand()*cexp(2.0*I*M_PI*frand()),
				.color = rgb(0, 1*alpha, 0.7*alpha),
				.draw_rule = Fade,
				.rule = timeout,
				.args = { 50 },
				.angle = 2*M_PI*frand(),
				.flags = PFLAG_DRAWADD,
			);
		}

		return;
	}

	glColor4f(1.0,1.0,1.0,alpha);
	draw_texture(creal(e->pos), cimag(e->pos), "stage6/baryon");
	glColor4f(1.0,1.0,1.0,1.0);

	n = REF(e->args[1]);
	if(!n)
		return;

	glBindTexture(GL_TEXTURE_2D, get_tex("stage6/baryon_connector")->gltex);
	glPushMatrix();
	glTranslatef(creal(e->pos+n->pos)/2.0, cimag(e->pos+n->pos)/2.0, 0);
	glRotatef(180/M_PI*carg(e->pos-n->pos), 0, 0, 1);
	glScalef(cabs(e->pos-n->pos)-70, 20, 1);

	draw_quad();
	glPopMatrix();
}

void BaryonCenter(Enemy *e, int t, bool render) {
	Enemy *l[2];
	int i;

	if(!render) {
		complex p = e->pos+40*frand()*cexp(2.0*I*M_PI*frand());

		PARTICLE("flare", p, rgb(0.0, 1.0, 1.0),
			.draw_rule = GrowFade,
			.rule = timeout_linear,
			.args = { 50, 1-I },
			.flags = PFLAG_DRAWADD,
		);

		PARTICLE("stain", p, rgb(0.0, 1.0, 0.2),
			.draw_rule = Fade,
			.rule = timeout,
			.args = { 50 },
			.angle = 2*M_PI*frand(),
			.flags = PFLAG_DRAWADD,
		);

		return;
	}

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

	glBindTexture(GL_TEXTURE_2D, get_tex("stage6/baryon_connector")->gltex);
	for(i = 0; i < 2; i++) {
		glPushMatrix();
		glTranslatef(creal(e->pos+l[i]->pos)/2.0, cimag(e->pos+l[i]->pos)/2.0, 0);
		glRotatef(180/M_PI*carg(e->pos-l[i]->pos), 0, 0, 1);
		glScalef(cabs(e->pos-l[i]->pos)-70, 20, 1);
		draw_quad();
		glPopMatrix();

	}
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

void elly_unbound(Boss *b, int t) {
	if(global.stage->type == STAGE_SPELL) {
		t += 100;
		GO_TO(b, BOSS_DEFAULT_GO_POS, 0.1)
	}

	TIMER(&t);

	AT(0) {
		global.enemies->birthtime = global.frames;
		global.enemies->logic_rule = scythe_explode;
		elly_clap(b,150);
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
		if(e->visual_rule == Baryon) {
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

		play_sound("shot_special1");
		play_sound_delayed("redirect",4,true,60);
		for(i = 0; i < c; i++) {
			complex n = cexp(2.0*I*_i+I*M_PI/2+I*creal(e->args[2]));
			for(j = 0; j < 3; j++) {
				PROJECTILE(.texture = "plainball",
					.pos = e->pos + 60*cexp(2.0*I*M_PI/c*i),
					.color = rgb(j == 0, j == 1, j == 2),
					.rule = eigenstate_proj,
					.args = {
						1*n,
						1,
						60,
						0.6*I*n*(j-1)*cexp(0.4*I-0.1*I*global.diff)
					},
					.flags = PFLAG_DRAWADD,
				);
			}
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

	b->ani.stdrow=1;
	AT(0)
		set_baryon_rule(baryon_eigenstate);
	AT(EVENT_DEATH)
		set_baryon_rule(baryon_reset);
}

int broglie_particle(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		free_ref(p->args[0]);
		return 1;
	}

	/*
	if(t == EVENT_BIRTH) {
		// hidden and no collision detection until scattertime
		p->type = FakeProj;
		p->draw = ProjNoDraw;
	}
	*/

	if(t < 0) {
		return 1;
	}

	int scattertime = creal(p->args[1]);

	if(t == scattertime)
		play_sound_ex("redirect",5,false);
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

			double angle_ampl = creal(p->args[3]);
			double angle_freq = cimag(p->args[3]);

			p->angle += angle_ampl * sin(t * angle_freq) * cos(2 * t * angle_freq);
			p->args[2] = -cabs(p->args[2]) * cexp(I*p->angle);
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
		l->color = hsl(hue, 1.0, 0.5);
	}

	if(t < 0) {
		return;
	}

	int dt = l->timespan * l->speed;
	float charge = min(1, pow((double)t / dt, 4));
	l->color = hsl(hue, 1.0, 0.5 + 0.2 * charge);
	l->width_exponent = 1.0 - 0.5 * charge;
}

int broglie_charge(Projectile *p, int t) {
	if(t < 0) {
		return 1;
	}

	if(t == 1)
		play_sound_ex("shot3",10,false);

	int firetime = creal(p->args[1]);

	if(t == firetime) {
		play_sound_ex("laser1",10,true);
		int attack_num = creal(p->args[2]);
		double hue = creal(p->args[3]);

		p->pos -= p->args[0] * 15;
		complex aim = cexp(I*p->angle);

		double s_ampl = 30 + 2 * attack_num;
		double s_freq = 0.10 + 0.01 * attack_num;

		for(int lnum = 0; lnum < 2; ++lnum) {
			Laser *l = create_lasercurve4c(p->pos, 75, 100, 0, las_sine,
				5*aim, s_ampl, s_freq, lnum * M_PI +
				I*(hue + lnum * (M_PI/12)/(M_PI/2)));

			l->width = 20;
			l->lrule = broglie_laser_logic;

			int pnum = 0;
			double inc = pow(1.10 - 0.10 * (global.diff - D_Easy), 2);

			for(double ofs = 0; ofs < l->deathtime; ofs += inc, ++pnum) {
				bool fast = global.diff == D_Easy || pnum & 1;

				PROJECTILE(
					.texture = fast ? "thickrice" : "rice",
					.pos = l->prule(l, ofs),
					.color = hsl(hue + lnum * (M_PI/12)/(M_PI/2), 1.0, 0.5),
					.rule = broglie_particle,
					.args = {
						add_ref(l),
						I*ofs + l->timespan + ofs - 10,
						fast ? 2.0 : 1.5,
						(1 + 2 * ((global.diff - 1) / (double)(D_Lunatic - 1))) * M_PI/11 + s_freq*10*I
					},
					.draw_rule = ProjNoDraw,
					.flags = PFLAG_DRAWADD,
					.type = FakeProj,
				);
			}
		}

		return ACTION_DESTROY;
	} else {
		float f = pow(clamp((120 - (firetime - t)) / 90.0, 0, 1), 8);
		complex o = p->pos - p->args[0] * 15;
		p->args[0] *= cexp(I*M_PI*0.2*f);
		p->pos = o + p->args[0] * 15;
	}

	return 1;
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

	for(Enemy *en = global.enemies; en; en = en->next) {
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
			.texture = "ball",
			.pos = e->pos + 15*n,
			.color = hsl(hue, 1.0, 0.55),
			.rule = broglie_charge,
			.args = {
				n,
				(fire_delay - step * _i),
				attack_num,
				hue
			},
			.flags = PFLAG_DRAWADD,
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

	if(t < 0) {
		return;
	}

	AT(0) {
		set_baryon_rule(baryon_broglie);
	}

	AT(EVENT_DEATH) {
		set_baryon_rule(baryon_reset);
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
			.texture = "ball",
			.pos = e->pos+40*cexp(I*a),
			.color = rgb(cos(ca), sin(ca), cos(ca+2.1)),
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

	AT(EVENT_DEATH) {
		Enemy *e = (Enemy*)REF(p->args[1]);
		play_sound_ex("shot3",8, false);

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
				PROJECTILE(
					.texture = "ball",
					.pos = 15*n,
					.color = rgb(0.0, 0.5, 0.1),
					.rule = ricci_proj2,
					.args = {
						-n,
						add_ref(e),
					},
					.flags = PFLAG_DRAWADD,
				);
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
		if(e->visual_rule != Baryon) {
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

	float a = 0.5 + 0.5 * max(0,tanh((time-80)/100.))*clamp(influence,0.2,1);
	a *= min(1, t / 20.0f);

	p->color = derive_color(p->color, CLRMASK_B|CLRMASK_A,
		rgba(0, 0, cabs(shift)/20.0, a)
	);

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

	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.03);

	FROM_TO(30, 100000, interval) {
		for(int i = 0; i < c; i++) {
			int w = VIEWPORT_W;
			complex pos = 0.5 * w/(float)c + fmod(w/(float)c*(i+0.5*_i),w) + (VIEWPORT_H+10)*I;

			PROJECTILE("ball", pos, rgba(0.5, 0.0, 0,0), ricci_proj, { -v*I },
				.flags = PFLAG_DRAWADD | PFLAG_NOSPAWNZOOM | PFLAG_NOGRAZE,
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
	b->ani.stdrow=1;
	TIMER(&t);
	AT(0)
		set_baryon_rule(baryon_nattack);
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
					PROJECTILE("bigball", b->pos, rgb(0,0.2,0.9), asymptotic, { n, 2 * j });
				}
			}
		} else {
			int x, y;
			int w = 1+(global.diff > D_Normal);
			complex n = cexp(I*carg(global.plr.pos-b->pos));

			for(x = -w; x <= w; x++)
				for(y = -w; y <= w; y++)
					PROJECTILE("bigball", b->pos+25*(x+I*y)*n, rgb(0,0.2,0.9), asymptotic, { n, 3 });
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
			create_lasercurve2c(pos, 70+20*global.diff, 300, rgb(1, 1, 1), las_accel, v, 0.02*frand()*copysign(1,creal(v)))->width=15;

			PROJECTILE("soul",    pos, rgb(0.4, 0.0, 1.0), linear,
				.args = { (1+2.5*afrand(0))*cexp(2.0*I*M_PI*afrand(1)) },
				.flags = PFLAG_DRAWADD
			);
			PROJECTILE("bigball", pos, rgb(1.0, 0.0, 0.4), linear,
				.args = { (1+2.5*afrand(2))*cexp(2.0*I*M_PI*afrand(3)) },
				.flags = PFLAG_DRAWADD
			);
		}
	}

	FROM_TO(0, 100000,7-global.diff) {
		play_sound_ex("shot2",10,false);
		PROJECTILE("ball", b->pos, rgb(0, 0.4,1), asymptotic,
			.args = { cexp(2.0*I*_i), 3 },
			.flags = PFLAG_DRAWADD,
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
		petal_explosion(35, e->pos);
		play_sound("bossdeath");
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
	play_loop("shot1_loop");

	if(t % (2+(global.diff < D_Hard)) == 0) {
		tsrand_fill(2);
		complex pos = VIEWPORT_W*afrand(0)+I*VIEWPORT_H*afrand(1);
		if(cabs(pos - global.plr.pos) > 50) {
			tsrand_fill(2);
			float speed = 0.5/(1+(global.diff < D_Hard));

			PROJECTILE(
				.texture = "flea",
				.pos = pos,
				.color = rgb(0.1*afrand(0), 0.6,1),
				.rule = curvature_bullet,
				.args = {
					speed*cexp(2*M_PI*I*afrand(1)),
					add_ref(e)
				}
			);
		}
	}
	if(global.diff > D_Easy && t % (60-8*global.diff) == 0) {
			tsrand_fill(2);
			complex pos = VIEWPORT_W*afrand(0)+I*VIEWPORT_H*afrand(1);

			if(cabs(pos - global.plr.pos) > 100) {
				PROJECTILE("ball", pos, rgb(1, 0.4,1), linear,
					.args = { cexp(I*carg(global.plr.pos-pos)) },
					.flags = PFLAG_DRAWADD,
				);
			}
	}

	if(global.diff >= D_Hard && !(t%20)) {
		play_sound_ex("shot2",10,false);
		Projectile *p =PROJECTILE(
			.texture = "bigball",
			.pos = global.boss->pos,
			.color = rgb(0.5, 0.4,1),
			.rule = linear,
			.args = { 4*I*cexp(I*M_PI*2/5*saw(t/100.)) },
			.flags = PFLAG_DRAWADD,
		);

		if(global.diff == D_Lunatic) {
			PROJECTILE(
				.texture = "plainball",
				.pos = global.boss->pos,
				.color = rgb(0.2, 0.4,1),
				.rule = curvature_orbiter,
				.args = {
					40*cexp(I*t/400),
					add_ref(p)
				},
				.flags = PFLAG_DRAWADD,
			);
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

		p->color = rgb(cos(p->angle), sin(p->angle), cos(p->angle+2.1));
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
	FROM_TO_SND("shot1_loop",0, 300, 4) {
		PROJECTILE(
			.texture = "ball",
			.pos = e->pos + 80*n,
			.color = rgb(carg(n), 1-carg(n), 1/carg(n)),
			.rule = wait_proj,
			.args = {
				(0.6+0.3*global.diff)*cexp(0.6*I)*n, 100
			},
			.flags = PFLAG_DRAWADD,
		);
	}

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

	AT(0)
		elly_clap(b,500);

	FROM_TO(0, 500, 10)
		for(i = 0; i < 3; i++) {
			tsrand_fill(5);
			PARTICLE(
				.texture = "stain",
				.pos = b->pos+80*afrand(0)*cexp(2.0*I*M_PI*afrand(1)),
				.color = rgb(1 - afrand(2), 0.8, 0.5),
				.draw_rule = Fade,
				.rule = timeout,
				.args = {60, 1+3*afrand(3) },
				.angle = 2*M_PI*afrand(4),
				.flags = PFLAG_DRAWADD,
			);
		}

	FROM_TO(20, 70, 30-global.diff) {
		int c = 20+2*global.diff;
		play_sound("shot_special1");
		for(i = 0; i < c; i++) {
			complex n = cexp(2.0*I*M_PI/c*i);
			PROJECTILE(
				.texture = "soul",
				.pos = b->pos,
				.color = rgb(0.2, 0, 0.9),
				.rule = accelerated,
				.args = {
					n,
					(0.01+0.002*global.diff)*n+0.01*I*n*(1-2*(_i&1))
				},
				.flags = PFLAG_DRAWADD,
			);
		}
	}

	FROM_TO(120, 240, 10-global.diff) {
		play_sound("shot1");
		int x, y;
		int w = 2;
		complex n = cexp(0.7*I*_i+0.2*I*frand());
		for(x = -w; x <= w; x++) {
			for(y = -w; y <= w; y++) {
				PROJECTILE(
					.texture = "ball",
					.pos = b->pos + (15+5*global.diff)*(x+I*y)*n,
					.color = rgb(0, 0.5, 1),
					.rule = accelerated,
					.args = { 2*n, 0.026*n }
				);
			}
		}
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

void elly_spellbg_modern_dark(Boss *b, int t) {
	elly_spellbg_modern(b, t);
	fade_out(0.75 * min(1, t / 300.0));
}

static void elly_global_rule(Boss *b, int time) {
	global.boss->glowcolor = hsla(time/120.0, 1.0, 0.25, 0.5);
	global.boss->shadowcolor = hsla((time+20)/120.0, 1.0, 0.25, 0.5);
}

Boss* stage6_spawn_elly(complex pos) {
	Boss *b = create_boss("Elly", "elly", "dialog/elly", pos);
	b->global_rule = elly_global_rule;
	return b;
}

static void elly_insert_interboss_dialog(Boss *b, int t) {
	if(t == 0) {
		global.dialog = stage6_interboss_dialog();
	}
}

Boss* create_elly(void) {
	Boss *b = stage6_spawn_elly(-200.0*I);

	boss_add_attack(b, AT_Move, "Catch the Scythe", 6, 0, elly_intro, NULL);
	boss_add_attack(b, AT_Normal, "Frequency", 40, 50000, elly_frequency, NULL);
	boss_add_attack_from_info(b, &stage6_spells.scythe.occams_razor, false);
	boss_add_attack(b, AT_Normal, "Frequency2", 40, 50000, elly_frequency2, NULL);
	boss_add_attack_from_info(b, &stage6_spells.scythe.orbital_clockwork, false);
	boss_add_attack_from_info(b, &stage6_spells.scythe.wave_theory, false);
	boss_add_attack(b, AT_Move, "Unbound", 3, 10, elly_unbound, NULL);
	boss_add_attack_from_info(b, &stage6_spells.baryon.many_world_interpretation, false);
	boss_add_attack(b, AT_Normal, "Baryon", 40, 50000, elly_baryonattack, NULL);
	boss_add_attack_from_info(b, &stage6_spells.baryon.wave_particle_duality, false);
	boss_add_attack_from_info(b, &stage6_spells.baryon.spacetime_curvature, false);
	boss_add_attack(b, AT_Normal, "Baryon", 40, 50000, elly_baryonattack2, NULL);
	boss_add_attack_from_info(b, &stage6_spells.baryon.higgs_boson_uncovered, false);
	boss_add_attack(b, AT_Move, "", 0, 0, elly_insert_interboss_dialog, NULL);
	boss_add_attack(b, AT_Move, "Explode", 6, 10, elly_baryon_explode, NULL);
	boss_add_attack_from_info(b, &stage6_spells.extra.curvature_domination, false);
	boss_add_attack_from_info(b, &stage6_spells.final.theory_of_everything, false);
	boss_start_attack(b, b->attacks);

	return b;
}

void stage6_events(void) {
	TIMER(&global.timer);

	AT(0) {
		stage_start_bgm("stage6");
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
