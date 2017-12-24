/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "stage4_events.h"
#include "stage6_events.h"
#include "global.h"
#include "stage.h"
#include "enemy.h"
#include "laser.h"

void kurumi_spell_bg(Boss*, int);
void kurumi_slaveburst(Boss*, int);
void kurumi_redspike(Boss*, int);
void kurumi_aniwall(Boss*, int);
void kurumi_blowwall(Boss*, int);
void kurumi_danmaku(Boss*, int);
void kurumi_extra(Boss*, int);

Dialog *stage4_dialog(void) {
	PlayerCharacter *pc = global.plr.mode->character;
	Dialog *d = create_dialog(pc->dialog_sprite_name, "dialog/kurumi");

	if(pc->id == PLR_CHAR_MARISA) {
		dadd_msg(d,Right, "Halt, intruder!");
		dadd_msg(d,Left, "Now here’s a face I haven’t seen in a long time. What’s this fancy house all about anyways? Decided to guard Yūka again, or is it somebody new?");
		dadd_msg(d,Right, "It’s none of your business, that’s what it is. My friend is busy with some important work and she can’t be disturbed by nosy humans poking into where they don’t belong.");
		dadd_msg(d,Left, "Now that’s no way to treat an old acquaintance! I know ya from before, so how about just givin’ me a backstage pass, eh?");
		dadd_msg(d,Right, "If you know me as well as you think you do, you’d know that all you’re going to get acquainted with is the taste of your own blood!");
		dadd_msg(d,Left, "Shoot, so that’s a no to my reservation, right?");
	} else if(pc->id == PLR_CHAR_YOUMU) {
		dadd_msg(d,Right, "Halt, intruder!");
		dadd_msg(d,Left, "Oh, and who might you be?");
		dadd_msg(d,Right, "Kuru— Hey, my name isn’t important for a nosy person like you to know!");
		dadd_msg(d,Left, "I only wanted to know so I could refer to you politely. I don’t want to call you a “random somebody” in your own house.");
		dadd_msg(d,Left, "Your house is very nice, by the way. Although I still think Hakugyokurō is bigger. Have you ever been there? Lady Yuyuko loves guests.");
		dadd_msg(d,Right, "This isn’t my house, and you’re not allowed to snoop any more than you have! If you keep ignoring me, I’ll have to suck you dry right here where we stand!");
		dadd_msg(d,Left, "It’s not your house, and yet you’re telling me to leave? You sound just as presumptuous as the other vampire I’ve met.");
		dadd_msg(d,Right, "I can bet you that I’m much more frightening.");
	} else if(pc->id == PLR_CHAR_REIMU) {
		dadd_msg(d,Right, "Halt, intruder!");
		dadd_msg(d,Left, "Oh, it’s somebody new.");
		dadd_msg(d,Right, "No, definitely not! I could never forget you from all those years ago! That was the most intense battle of my life!");
		dadd_msg(d,Left, "Hmm, nope, I don’t remember fighting you. Maybe if you told me your name, I could recall faster.");
		dadd_msg(d,Right, "Unforgivable! How about I just jog your memory through terror instead?");
		dadd_msg(d,Left, "I’m in a hurry, and that also sounds unpleasant.");
		dadd_msg(d,Right, "You don’t get a choice! Prepare to have bloody nightmares for weeks, shrine maiden!");
	}

	dadd_msg(d, BGM, "stage4boss");
	return d;
}

Dialog *stage4_dialog_end(void) {
	PlayerCharacter *pc = global.plr.mode->character;
	Dialog *d = create_dialog(pc->dialog_sprite_name, NULL);


	if(pc->id == PLR_CHAR_MARISA) {
		dadd_msg(d,Left, "Seems like the mastermind’s someone she’s close to, huh? Guess I gotta get ready for another blast from the past.");
	} else if(pc->id == PLR_CHAR_YOUMU) {
		dadd_msg(d,Left, "You’re not as scary as her, or even as good of a host. Maybe you should work on your manners and buy yourself a nice mansion to lord over instead of taking someone else’s.");

	} else if(pc->id == PLR_CHAR_REIMU) {
		dadd_msg(d,Left, "See, I don’t scare easily, so that didn’t work. You should have just told me your name.");
	}
	return d;
}

int stage4_splasher(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 1, Power, 3, Bomb, 1, NULL);
		return 1;
	}

	e->moving = true;
	e->dir = creal(e->args[0]) < 0;

	FROM_TO(0, 50, 1)
		e->pos += e->args[0]*(1-t/50.0);

	FROM_TO_SND("shot1_loop", 66-6*global.diff, 150, 5-global.diff) {
		tsrand_fill(4);
		PROJECTILE(
			.texture = afrand(0) > 0.5 ? "rice" : "thickrice",
			.pos = e->pos,
			.color = rgb(0.8,0.3-0.1*afrand(1),0.5),
			.rule = accelerated,
			.args = {
				e->args[0]/2+(1-2*afrand(2))+(1-2*afrand(3))*I,
				0.02*I
			}
		);
	}

	FROM_TO(200, 300, 1)
		e->pos -= creal(e->args[0])*(t-200)/100.0;

	return 1;
}

int stage4_fodder(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Power, 1, NULL);
		return 1;
	}

	if(creal(e->args[0]) != 0)
		e->moving = true;
	e->dir = creal(e->args[0]) < 0;
	e->pos += e->args[0];

	FROM_TO(100, 200, 22-global.diff*3) {
		play_sound_ex("shot3",5,false);
		PROJECTILE("ball", e->pos, rgb(1, 0.3, 0.5), asymptotic, {
			2*cexp(I*M_PI*2*frand()),
			3
		});
	}

	return 1;
}


int stage4_partcircle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 2, Power, 1, NULL);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(30,60,1) {
		e->args[0] *= 0.9;
	}

	FROM_TO(60,76,1) {
		int i;
		for(i = 0; i < global.diff; i++) {
			play_sound("shot2");
			complex n = cexp(I*M_PI/16.0*_i + I*carg(e->args[0])-I*M_PI/4.0 + 0.01*I*i*(1-2*(creal(e->args[0]) > 0)));
			PROJECTILE("wave", e->pos + (30)*n, rgb(1-0.2*i,0.5,0.7), asymptotic, { 2*n, 2+2*i });
		}
	}

	FROM_TO(160, 200, 1)
		e->args[0] += 0.05*I;

	return 1;
}

int stage4_cardbuster(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 1, Power, 2, NULL);
		return 1;
	}

	FROM_TO(0, 120, 1)
		e->pos += (e->args[0]-e->pos0)/120.0;

	FROM_TO(200, 300, 1)
		e->pos += (e->args[1]-e->args[0])/100.0;

	FROM_TO(400, 600, 1)
		e->pos += (e->args[2]-e->args[1])/200.0;

	int c = 40;
	complex n = cexp(I*carg(global.plr.pos - e->pos) + 4*M_PI/(c+1)*I*_i);

	FROM_TO_SND("shot1_loop", 120, 120+c*global.diff, 1) {
		if(_i&1)
			PROJECTILE("card", e->pos + 30*n, rgb(0, 0.8, 0.2), accelerated, { (0.8+0.2*global.diff)*n, 0.01*(1+0.01*_i)*n });
	}

	FROM_TO_SND("shot1_loop", 300, 320+20*global.diff, 1) {
		PROJECTILE("card", e->pos + 30*n, rgb(0, 0.7, 0.5), asymptotic, { (0.8+0.2*global.diff)*n, 0.4*I });
	}

	return 1;
}

int stage4_backfire(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 3, Power, 2, NULL);
		return 1;
	}

	FROM_TO(0,20,1)
		e->args[0] -= 0.05*I;

	FROM_TO(60,100,1)
		e->args[0] += 0.05*I;

	if(t > 100)
		e->args[0] -= 0.02*I;


	e->pos += e->args[0];

	FROM_TO(20,180+global.diff*20,2) {
		play_sound("shot2");
		complex n = cexp(I*M_PI*frand()-I*copysign(M_PI/2.0, creal(e->args[0])));
		int i;
		for(i = 0; i < global.diff; i++)
			PROJECTILE("wave", e->pos, rgb(0.2, 0.2, 1-0.2*i), asymptotic, { 2*n, 2+2*i });
	}

	return 1;
}

int stage4_bigcircle(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 1, Power, 3, NULL);

		return 1;
	}

	FROM_TO(0, 70, 1)
		e->pos += e->args[0];

	FROM_TO(200, 300, 1)
		e->pos -= e->args[0];


	FROM_TO(80,100+30*global.diff,20) {
		play_sound("shot_special1");
		int i;
		int n = 10+3*global.diff;
		for(i = 0; i < n; i++) {
			PROJECTILE(
				.texture = "bigball",
				.pos = e->pos,
				.color = rgb(0,0.8-0.4*_i,0),
				.rule = asymptotic,
				.args = {
					2*cexp(2.0*I*M_PI/n*i+I*3*_i),
					3*sin(6*M_PI/n*i)
				},
				.flags = PFLAG_DRAWADD,
			);

			if(global.diff > D_Easy) {
				PROJECTILE(
					.texture = "ball",
					.pos = e->pos,
					.color = rgb(0,0.3*_i,0.4),
					.rule = asymptotic,
					.args = {
						(1.5+global.diff*0.2)*cexp(I*3*(i+frand())),
						I*5*sin(6*M_PI/n*i)
					},
					.flags = PFLAG_DRAWADD,
				);
			}
		}
	}
	return 1;
}

int stage4_explosive(Enemy *e, int t) {
	if(t == EVENT_DEATH || (t >= 100 && global.diff >= D_Normal)) {
		int i;
		if(t == EVENT_DEATH)
			spawn_items(e->pos, Power, 1, NULL);

		int n = 10*global.diff;
		complex phase = global.plr.pos-e->pos;
		phase /= cabs(phase);

		for(i = 0; i < n; i++) {
			double angle = 2*M_PI*i/n+carg(phase);
			if(frand() < 0.5)
				angle += M_PI/n;
			PROJECTILE(
				.texture = "ball",
				.pos = e->pos,
				.color = rgb(0.1+0.4*(i&1), 0.2, 1-0.6*(i&1)),
				.rule = accelerated,
				.args = {
					1.5*(1.1+0.3*global.diff)*cexp(I*angle),
					0.001*cexp(I*angle)
				}
			);
		}

		play_sound("shot1");
		return ACTION_DESTROY;
	}

	e->pos += e->args[0];

	return 1;
}

void KurumiSlave(Enemy *e, int t, bool render) {
	if(render) {
		return;
	}

	if(!(t%2)) {
		complex offset = (frand()-0.5)*30;
		offset += (frand()-0.5)*20.0*I;
		PARTICLE(
			.texture = "lasercurve",
			.pos = offset,
			.color = rgb(0.3,0.0,0.0),
			.draw_rule = EnemyFlareShrink,
			.rule = enemy_flare,
			.args = { 50, (-50.0*I-offset)/50.0, add_ref(e) },
			.flags = PFLAG_DRAWADD,
		);
	}
}

void kurumi_intro(Boss *b, int t) {
	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.03);
}

int kurumi_burstslave(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_BIRTH)
		e->args[1] = e->args[0];
	AT(EVENT_DEATH) {
		free_ref(e->args[2]);
		return 1;
	}


	if(t == 600 || REF(e->args[2]) == NULL)
		return ACTION_DESTROY;

	e->pos += 2*e->args[1]*(sin(t/10.0)+1.5);

	FROM_TO(0, 600, 18-2*global.diff) {
		float r = cimag(e->pos)/VIEWPORT_H;
		PROJECTILE("wave", e->pos + 10.0*I*e->args[0], rgb(r,0,0), accelerated, { 2.0*I*e->args[0], -0.01*e->args[1] });
		PROJECTILE("wave", e->pos - 10.0*I*e->args[0], rgb(r,0,0), accelerated, {-2.0*I*e->args[0], -0.01*e->args[1] });
		play_sound("shot1");
	}

	FROM_TO(40, 100,1) {
		e->args[1] -= e->args[0]*0.02;
		e->args[1] *= cexp(0.02*I);
	}

	return 1;
}


void kurumi_slaveburst(Boss *b, int time) {
	int t = time % 400;
	TIMER(&t);

	if(time == EVENT_DEATH)
		killall(global.enemies);

	if(time < 0)
		return;

	AT(0) {
		int i;
		int n = 3+2*global.diff;
		for(i = 0; i < n; i++) {
			create_enemy3c(b->pos, ENEMY_IMMUNE, KurumiSlave, kurumi_burstslave, cexp(I*2*M_PI/n*i+0.2*I*time/500), 0, add_ref(b));
		}
	}
}

int kurumi_spikeslave(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_BIRTH)
		e->args[1] = e->args[0];
	AT(EVENT_DEATH) {
		free_ref(e->args[2]);
		return 1;
	}


	if(t == 300+50*global.diff || REF(e->args[2]) == NULL)
		return ACTION_DESTROY;

	e->pos += e->args[1];
	e->args[1] *= cexp(0.01*I*e->args[0]);

	FROM_TO(0, 600, 18-2*global.diff) {
		float r = cimag(e->pos)/VIEWPORT_H;
		PROJECTILE("wave", e->pos + 10.0*I*e->args[0], rgb(r,0,0), linear, { 1.5*I*e->args[1], -0.01*e->args[0] });
		PROJECTILE("wave", e->pos - 10.0*I*e->args[0], rgb(r,0,0), linear, {-1.5*I*e->args[1], -0.01*e->args[0] });
		play_sound_ex("shot1",5,false);
	}

	return 1;
}

void kurumi_redspike(Boss *b, int time) {
	int t = time % 500;

	if(time == EVENT_DEATH)
		killall(global.enemies);

	if(time < 0)
		return;

	TIMER(&t);

	FROM_TO(0, 500, 60) {
		create_enemy3c(b->pos, ENEMY_IMMUNE, KurumiSlave, kurumi_spikeslave, 1-2*(_i&1), 0, add_ref(b));
	}

	if(global.diff < D_Hard) {
		FROM_TO(0, 500, 150-50*global.diff) {
			int i;
			int n = global.diff*8;
			for(i = 0; i < n; i++)
				PROJECTILE(
					.texture = "bigball",
					.pos = b->pos,
					.color = rgb(1.0, 0.0, 0.0),
					.rule = asymptotic,
					.args = {
						(1+0.1*(global.diff == D_Normal))*3*cexp(2.0*I*M_PI/n*i+I*carg(global.plr.pos-b->pos)),
						3
					},
					.flags = PFLAG_DRAWADD,
				);
		}
	} else {
		AT(80) {
			b->ani.stdrow = 1;
		}
		AT(499) {
			b->ani.stdrow = 0;
		}
		FROM_TO_INT(80, 500, 40,200,2+2*(global.diff == D_Hard)) {
			tsrand_fill(2);
			complex offset = 100*afrand(0)*cexp(2.0*I*M_PI*afrand(1));
			complex n = cexp(I*carg(global.plr.pos-b->pos-offset));
			PROJECTILE("rice", b->pos+offset, rgb(1,0,0), accelerated,
				.args = { -1*n, 0.05*n },
				.flags = PFLAG_DRAWADD,
			);
			play_sound_ex("shot2",5,false);
		}
	}
}

void kurumi_spell_bg(Boss *b, int time) {
	float f = 0.5+0.5*sin(time/80.0);

	glPushMatrix();
	glTranslatef(VIEWPORT_W/2, VIEWPORT_H/2,0);
	glScalef(0.6,0.6,1);
	glColor3f(f,1-f,1-f);
	draw_texture(0, 0, "stage4/kurumibg1");
	glColor3f(1,1,1);
	glPopMatrix();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	fill_screen(time/300.0, time/300.0, 0.5, "stage4/kurumibg2");

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}

void kurumi_outro(Boss *b, int time) {
	b->pos += -5-I;
}

void kurumi_global_rule(Boss *b, int time) {
	if(b->current && ATTACK_IS_SPELL(b->current->type)) {
		b->shadowcolor = rgba(0.0, 0.4, 0.5, 0.5);
	} else {
		b->shadowcolor = rgba(1.0, 0.1, 0.0, 0.5);
	}
}

Boss* stage4_spawn_kurumi(complex pos) {
	Boss* b = create_boss("Kurumi", "kurumi", "dialog/kurumi", pos);
	b->glowcolor = rgba(0.5, 0.1, 0.0, 1.0);
	b->global_rule = kurumi_global_rule;
	return b;
}

Boss* create_kurumi_mid(void) {
	Boss *b = stage4_spawn_kurumi(VIEWPORT_W/2-400.0*I);

	boss_add_attack(b, AT_Move, "Introduction", 2, 0, kurumi_intro, NULL);
	boss_add_attack_from_info(b, &stage4_spells.mid.gate_of_walachia, false);
	if(global.diff < D_Hard) {
		boss_add_attack_from_info(b, &stage4_spells.mid.dry_fountain, false);
	} else {
		boss_add_attack_from_info(b, &stage4_spells.mid.red_spike, false);
	}
	boss_add_attack(b, AT_Move, "Outro", 2, 1, kurumi_outro, NULL);
	boss_start_attack(b, b->attacks);
	return b;
}

int splitcard(Projectile *p, int t) {
	if(t == creal(p->args[2])) {
		p->color = rgb(0, color_component(p->color,CLR_B), color_component(p->color, CLR_G));
		play_sound_ex("redirect", 10, false);
	}
	if(t > creal(p->args[2])) {
		p->args[0] += 0.01*p->args[3];
	}

	return asymptotic(p, t);
}

int stage4_supercard(Enemy *e, int t) {
	int time = t % 150;

	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 2, Power, 3, NULL);
		return 1;
	}

	e->pos += e->args[0];

	FROM_TO(50, 70, 1) {
		e->args[0] *= 0.7;
	}

	if(t > 450) {
		e->pos -= I;
		return 1;
	}

	__timep = &time;

	FROM_TO(70, 70+20*global.diff, 1) {
		play_sound_ex("shot1",5,false);

		int i;
		complex n = cexp(I*carg(global.plr.pos - e->pos) + 2*M_PI/20.*I*_i);
		for(i = -1; i <= 1 && t; i++)
			PROJECTILE("card", e->pos + 30*n, rgb(0,0.4,1-_i/40.0), splitcard,
				{1*n, 0.1*_i, 100-time+70, 1.4*I*i*n}
                        );
	}

	return 1;
}

void kurumi_boss_intro(Boss *b, int t) {
	TIMER(&t);
	GO_TO(b, BOSS_DEFAULT_GO_POS, 0.015);

	AT(120)
		global.dialog = stage4_dialog();
}

static int splitcard_elly(Projectile *p, int t) {
	if(t == creal(p->args[2])) {
    		p->args[0]+=p->args[3];
		p->color = derive_color(p->color, CLRMASK_B, rgb(0,0,-color_component(p->color,CLR_B)));
		play_sound_ex("redirect", 10, false);
	}
	
	return asymptotic(p, t);
}

void kurumi_breaker(Boss *b, int time) {
	int t = time % 400;
	int i;

	int c = 10+global.diff*2;
	int kt = 20;

	if(time < 0)
		return;

	GO_TO(b, VIEWPORT_W/2 + VIEWPORT_W/3*sin(time/220) + I*cimag(b->pos), 0.02);

	TIMER(&t);

	FROM_TO_SND("shot1_loop", 50, 400, 50-7*global.diff) {
		complex p = b->pos + 150*sin(_i) + 100.0*I*cos(_i);

		for(i = 0; i < c; i++) {
			complex n = cexp(2.0*I*M_PI/c*i);
			PROJECTILE("rice", p, rgb(1,0,0.5), splitcard_elly, {
				3*n,
				0,
				kt,
				1.5*cexp(I*carg(global.plr.pos - p - 2*kt*n))-2.6*n
			});

		}
	}

	FROM_TO(60, 400, 100) {
		play_sound("shot_special1");
		aniplayer_queue(&b->ani,1,0,0);
		for(i = 0; i < 20; i++) {
			PROJECTILE("bigball", b->pos, rgb(0.5,0,0.5), asymptotic,
				.args = { cexp(2.0*I*M_PI/20.0*i), 3 },
				.flags = PFLAG_DRAWADD,
			);
		}
	}

}

int aniwall_bullet(Projectile *p, int t) {
	if(t < 0)
		return 1;

	if(t > creal(p->args[1])) {
		if(global.diff > D_Normal) {
			tsrand_fill(2);
			p->args[0] += 0.1*(0.1-0.2*afrand(0) + 0.1*I-0.2*I*afrand(1))*(global.diff-2);
			p->args[0] += 0.002*cexp(I*carg(global.plr.pos - p->pos));
		}

		p->pos += p->args[0];
	}

	p->color = derive_color(p->color, CLRMASK_R, rgb(cimag(p->pos)/VIEWPORT_H, 0, 0));
	return 1;
}

int aniwall_slave(Enemy *e, int t) {
	float re, im;

	if(t < 0)
		return 1;

	if(creal(e->pos) <= 0)
		e->pos = I*cimag(e->pos);
	if(creal(e->pos) >= VIEWPORT_W)
		e->pos = VIEWPORT_W + I*cimag(e->pos);
	if(cimag(e->pos) <= 0)
		e->pos = creal(e->pos);
	if(cimag(e->pos) >= VIEWPORT_H)
		e->pos = creal(e->pos) + I*VIEWPORT_H;

	re = creal(e->pos);
	im = cimag(e->pos);

	if(cabs(e->args[1]) <= 0.1) {
		if(re == 0 || re == VIEWPORT_W) {

			e->args[1] = 1;
			e->args[2] = 10.0*I;
		}

		e->pos += e->args[0]*t;
	} else {
		if((re <= 0) + (im <= 0) + (re >= VIEWPORT_W) + (im >= VIEWPORT_H) == 2) {
			float sign = 1;
			sign *= 1-2*(re > 0);
			sign *= 1-2*(im > 0);
			sign *= 1-2*(cimag(e->args[2]) == 0);
			e->args[2] *= sign*I;
		}

		e->pos += e->args[2];


		if(!(t % 7-global.diff-2*(global.diff > D_Normal))) {
			complex v = e->args[2]/cabs(e->args[2])*I*sign(creal(e->args[0]));
			if(cimag(v) > -0.1 || global.diff >= D_Normal) {
				play_sound("shot1");
				PROJECTILE("ball", e->pos+I*v*20*nfrand(), rgb(1,0,0), aniwall_bullet, { 1*v, 40 });
			}
		}
	}

	return 1;
}

void KurumiAniWallSlave(Enemy *e, int t, bool render) {
	if(render) {
		return;
	}

	if(e->args[1]) {
		PARTICLE("lasercurve", e->pos, rgb(1,1,1), timeout, { 30 },
			.draw_rule = Fade,
			.flags = PFLAG_DRAWADD,
		);
	}
}

void kurumi_aniwall(Boss *b, int time) {
	TIMER(&time);

	AT(EVENT_DEATH) {
		killall(global.enemies);
	}

	GO_TO(b, VIEWPORT_W/2 + VIEWPORT_W/3*sin(time/200) + I*cimag(b->pos),0.03)

	if(time < 0)
		return;

	b->ani.stdrow = 1;
	AT(0) {
		play_sound("laser1");
		create_lasercurve2c(b->pos, 50, 80, rgb(1, 0.8, 0.8), las_accel, 0, 0.2*cexp(0.4*I));
		create_enemy1c(b->pos, ENEMY_IMMUNE, KurumiAniWallSlave, aniwall_slave, 0.2*cexp(0.4*I));
		create_lasercurve2c(b->pos, 50, 80, rgb(1, 0.8, 0.8), las_accel, 0, 0.2*cexp(I*M_PI - 0.4*I));
		create_enemy1c(b->pos, ENEMY_IMMUNE, KurumiAniWallSlave, aniwall_slave, 0.2*cexp(I*M_PI - 0.4*I));
	}
}

void kurumi_sbreaker(Boss *b, int time) {
	if(time < 0)
		return;

	int dur = 300+50*global.diff;
	int t = time % dur;
	int i;
	TIMER(&t);

	int c = 10+global.diff*2;
	int kt = 40;

	FROM_TO_SND("shot1_loop", 50, dur, 2+(global.diff < D_Hard)) {
		complex p = b->pos + 150*sin(_i/8.0)+100.0*I*cos(_i/15.0);

		complex n = cexp(2.0*I*M_PI/c*_i);
		PROJECTILE("rice", p, rgb(1.0, 0.0, 0.5), splitcard_elly, {
			2*n,
			0,
			kt,
			1.5*cexp(I*carg(global.plr.pos - p - 2*kt*n))-1.7*n
		});

	}

	FROM_TO(60, dur, 100) {
		play_sound("shot_special1");
		aniplayer_queue(&b->ani,1,0,0);
		for(i = 0; i < 20; i++) {
			PROJECTILE("bigball", b->pos, rgb(0.5, 0.0, 0.5), asymptotic,
				.args = { cexp(2.0*I*M_PI/20.0*i), 3 },
				.flags = PFLAG_DRAWADD,
			);
		}
	}
}

int blowwall_slave(Enemy *e, int t) {
	float re, im;

	if(t < 0)
		return 1;

	e->pos += e->args[0]*t;

	if(creal(e->pos) <= 0)
		e->pos = I*cimag(e->pos);
	if(creal(e->pos) >= VIEWPORT_W)
		e->pos = VIEWPORT_W + I*cimag(e->pos);
	if(cimag(e->pos) <= 0)
		e->pos = creal(e->pos);
	if(cimag(e->pos) >= VIEWPORT_H)
		e->pos = creal(e->pos) + I*VIEWPORT_H;

	re = creal(e->pos);
	im = cimag(e->pos);

	if(re <= 0 || im <= 0 || re >= VIEWPORT_W || im >= VIEWPORT_H) {
		int i, c;
		float f;
		char *type;

		c = 20 + global.diff*40;

		for(i = 0; i < c; i++) {
			f = frand();

			if(f < 0.3)
				type = "soul";
			else if(f < 0.6)
				type = "bigball";
			else
				type = "plainball";

			PROJECTILE(type, e->pos, rgb(1.0, 0.1, 0.1), asymptotic,
				.args = { (1+3*f)*cexp(2.0*I*M_PI*frand()), 4 },
				.flags = PFLAG_DRAWADD
			);
		}

		play_sound("shot_special1");
		return ACTION_DESTROY;
	}

	return 1;
}

static void bwlaser(Boss *b, float arg, int slave) {
	create_lasercurve2c(b->pos, 50, 100, rgb(1, 0.5+0.3*slave, 0.5+0.3*slave), las_accel, 0, (0.1+0.1*slave)*cexp(I*arg));

	if(slave) {
		play_sound("laser1");
		create_enemy1c(b->pos, ENEMY_IMMUNE, NULL, blowwall_slave, 0.2*cexp(I*arg));
	} else {
		// FIXME: needs a better sound
		play_sound("shot2");
		play_sound("shot_special1");
	}
}

void kurumi_blowwall(Boss *b, int time) {
	int t = time % 600;
	TIMER(&t);

	if(time == EVENT_DEATH)
		killall(global.enemies);

	if(time < 0) {
		GO_TO(b, BOSS_DEFAULT_GO_POS, 0.04)
		return;
	}

	b->ani.stdrow=1;
	AT(50)
		bwlaser(b, 0.4, 1);

	AT(100)
		bwlaser(b, M_PI-0.4, 1);

	FROM_TO(200, 300, 50)
		bwlaser(b, -M_PI*frand(), 1);

	FROM_TO(300, 500, 10)
		bwlaser(b, M_PI/10*_i, 0);

}

static int kdanmaku_proj(Projectile *p, int t) {
	int time = creal(p->args[0]);
	if(t == time) {
		p->color=rgb(0.6,0.3,1.0);
		p->tex=get_tex("proj/flea");
		p->args[1] = -I;
	}
	if(t > time)
		p->args[1] -= 0.01*I;
	p->pos += p->args[1];
	p->angle = carg(p->args[1]);
	return 1;
}

int kdanmaku_slave(Enemy *e, int t) {
	float re;

	if(t < 0)
		return 1;

	if(!e->args[1])
		e->pos += e->args[0]*t;
	else
		e->pos += 5.0*I;

	if(creal(e->pos) <= 0)
		e->pos = I*cimag(e->pos);
	if(creal(e->pos) >= VIEWPORT_W)
		e->pos = VIEWPORT_W + I*cimag(e->pos);

	re = creal(e->pos);

	if(re <= 0 || re >= VIEWPORT_W)
		e->args[1] = 1;

	if(cimag(e->pos) >= VIEWPORT_H)
		return ACTION_DESTROY;

	if(e->args[2] && e->args[1]) {
		int i, n = 2+4*global.diff;
		float speed = 1+0.1*(global.diff>D_Normal);

		for(i = 0; i < n; i++) {
			complex p = VIEWPORT_W/(float)n*(i+frand()) + I*cimag(e->pos);
			if(cabs(p-global.plr.pos) > 60) {
				PROJECTILE("thickrice", p, rgb(1, 0.5, 0.5), kdanmaku_proj,
					.args = { 600, speed*0.5*cexp(2.0*I*M_PI*sin(245*t+i*i*3501)) },
					.flags = PFLAG_DRAWADD,
				);
			}
		}
		play_sound_ex("redirect", 3, false);
	}

	return 1;
}

void kurumi_danmaku(Boss *b, int time) {
	int t = time % 600;
	TIMER(&t);

	if(time == EVENT_DEATH)
		killall(global.enemies);
	if(time < 0)
		return;

	AT(50) {
		play_sound("laser1");
		create_lasercurve2c(b->pos, 50, 100, rgb(1, 0.8, 0.8), las_accel, 0, 0.2*cexp(I*carg(-b->pos)));
		create_lasercurve2c(b->pos, 50, 100, rgb(1, 0.8, 0.8), las_accel, 0, 0.2*cexp(I*carg(VIEWPORT_W-b->pos)));
		create_enemy3c(b->pos, ENEMY_IMMUNE, KurumiAniWallSlave, kdanmaku_slave, 0.2*cexp(I*carg(-b->pos)), 0, 1);
		create_enemy3c(b->pos, ENEMY_IMMUNE, KurumiAniWallSlave, kdanmaku_slave, 0.2*cexp(I*carg(VIEWPORT_W-b->pos)), 0, 0);
	}
}

void kurumi_extra_shield_pos(Enemy *e, int time) {
	double dst = 75 + 100 * max((60 - time) / 60.0, 0);
	double spd = cimag(e->args[0]) * min(time / 120.0, 1);
	e->args[0] += spd;
	e->pos = global.boss->pos + dst * cexp(I*creal(e->args[0]));
}

bool kurumi_extra_shield_expire(Enemy *e, int time) {
	if(time > creal(e->args[1])) {
		e->hp = 0;
		return true;
	}

	return false;
}

int kurumi_extra_dead_shield_proj(Projectile *p, int time) {
	if(time < 0) {
		return 1;
	}

	p->color = mix_colors(
		rgb(0.2, 0.1, 0.5),
		rgb(2.0, 0.0, 0.0),
	min(time / 60.0f, 1.0f));

	return asymptotic(p, time);
}

int kurumi_extra_dead_shield(Enemy *e, int time) {
	if(time < 0) {
		return 1;
	}

	if(!(time % 6)) {
		// complex dir = cexp(I*(M_PI * 0.5 * nfrand() + carg(global.plr.pos - e->pos)));
		// complex dir = cexp(I*(carg(global.plr.pos - e->pos)));
		complex dir = cexp(I*creal(e->args[0]));
		PROJECTILE("rice", e->pos, 0, kurumi_extra_dead_shield_proj, { 2*dir, 10 });
		play_sound("shot1");
	}

	time += cimag(e->args[1]);

	kurumi_extra_shield_pos(e, time);

	if(kurumi_extra_shield_expire(e, time)) {
		int cnt = 10;
		for(int i = 0; i < cnt; ++i) {
			complex dir = cexp(I*M_PI*2*i/(double)cnt);
			tsrand_fill(2);
			PROJECTILE("ball", e->pos, 0, kurumi_extra_dead_shield_proj,
				.args = { 1.5 * (1 + afrand(0)) * dir, 4 + anfrand(1) },
				.flags = PFLAG_DRAWADD,
			);
		}

		// FIXME: needs a more powerful 'explosion' sound
		play_sound("shot_special1");
		play_sound("enemy_death");
	}

	return 1;
}

int kurumi_extra_shield(Enemy *e, int time) {
	if(time == EVENT_DEATH) {
		if(global.boss && !global.game_over && !boss_is_dying(global.boss) && e->args[2] == 0) {
			create_enemy2c(e->pos, ENEMY_IMMUNE, KurumiSlave, kurumi_extra_dead_shield, e->args[0], e->args[1]);
		}
		return 1;
	}

	if(time < 0) {
		return 1;
	}

	e->args[1] = creal(e->args[1]) + time*I;

	kurumi_extra_shield_pos(e, time);
	kurumi_extra_shield_expire(e, time);

	return 1;
}

int kurumi_extra_bigfairy1(Enemy *e, int time) {
	if(time < 0) {
		return 1;
	}
	TIMER(&time);

	int escapetime = 400+4000*(global.diff == D_Lunatic);
	if(time < escapetime) {
		GO_TO(e, e->args[0], 0.02);
	} else  {
		GO_TO(e, e->args[0]-I*VIEWPORT_H,0.01)
	}

	FROM_TO(50,escapetime,60) {
		int count = 5;
		complex phase = cexp(I*2*M_PI*frand());
		for(int i = 0; i < count; i++) {
			complex arg = cexp(I*2*M_PI*i/count);
			if(global.diff == D_Lunatic)
				arg *= phase;
			create_lasercurve2c(e->pos, 20, 200, rgb(1,0.3,0.7), las_accel, arg, 0.1*arg);
			PROJECTILE("bullet", e->pos, rgb(1.0, 0.3, 0.7), accelerated, { arg, 0.1*arg });
		}

		play_sound("laser1");
	}

	return 1;
}

void kurumi_extra_drainer_draw(Projectile *p, int time) {
	complex org = p->pos;
	complex targ = p->args[1];
	double a = 0.5 * creal(p->args[2]);

	ColorTransform ct;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	static_clrtransform_particle(rgba(1.0, 0.5, 0.5, a), &ct);
	recolor_apply_transform(&ct);
	loop_tex_line_p(org, targ, 16, time * 0.01, p->tex);

	static_clrtransform_particle(rgba(0.4, 0.2, 0.2, a), &ct);
	recolor_apply_transform(&ct);
	loop_tex_line_p(org, targ, 18, time * 0.0043, p->tex);

	static_clrtransform_particle(rgba(0.5, 0.2, 0.5, a), &ct);
	recolor_apply_transform(&ct);
	loop_tex_line_p(org, targ, 24, time * 0.003, p->tex);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int kurumi_extra_drainer(Projectile *p, int time) {
	if(time == EVENT_DEATH) {
		free_ref(p->args[0]);
		return 1;
	}

	if(time < 0) {
		return 1;
	}

	Enemy *e = REF(p->args[0]);
	p->pos = global.boss->pos;

	if(e) {
		p->args[1] = e->pos;
		p->args[2] = approach(p->args[2], 1, 0.5);

		if(time > 40) {
			// TODO: maybe add a special sound for this?

			int drain = clamp(4, 0, e->hp);
			e->hp -= drain;
			global.boss->current->hp = min(global.boss->current->maxhp, global.boss->current->hp + drain * 2);
		}
	} else {
		p->args[2] = approach(p->args[2], 0, 0.5);
		if(!creal(p->args[2])) {
			return ACTION_DESTROY;
		}
	}

	return 1;
}

void kurumi_extra_create_drainer(Enemy *e) {
	PROJECTILE(
		.texture_ptr = get_tex("part/sinewave"),
		.pos = e->pos,
		.rule = kurumi_extra_drainer,
		.draw_rule = kurumi_extra_drainer_draw,
		.args = { add_ref(e) },
		.type = FakeProj,
		.color_transform_rule = proj_clrtransform_particle,
	);
}

void kurumi_extra_swirl_visual(Enemy *e, int time, bool render) {
	if(!render) {
		Swirl(e, time, render);
		return;
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
	Swirl(e, time, render);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void kurumi_extra_fairy_visual(Enemy *e, int time, bool render) {
	if(!render) {
		Fairy(e, time, render);
		return;
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glUseProgram(get_shader("negative")->prog);
	Fairy(e, time, render);
	glUseProgram(0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void kurumi_extra_bigfairy_visual(Enemy *e, int time, bool render) {
	if(!render) {
		BigFairy(e, time, render);
		return;
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glUseProgram(get_shader("negative")->prog);
	BigFairy(e, time, render);
	glUseProgram(0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int kurumi_extra_fairy(Enemy *e, int t) {
	TIMER(&t);
	AT(EVENT_DEATH) {
		spawn_items(e->pos, Point, 1, NULL);
		return 1;
	}

	if(e->hp == ENEMY_IMMUNE && t > 50)
		e->hp = 500;

	if(creal(e->args[0]-e->pos) != 0)
		e->moving = true;
	e->dir = creal(e->args[0]-e->pos) < 0;

	FROM_TO(0,90,1) {
		GO_TO(e,e->args[0],0.1)
	}

	/*FROM_TO(100, 200, 22-global.diff*3) {
		PROJECTILE("ball", e->pos, rgb(1, 0.3, 0.5), asymptotic, 2*cexp(I*M_PI*2*frand()), 3);
	}*/

	int attacktime = creal(e->args[1]);
	int flytime = cimag(e->args[1]);
	FROM_TO_SND("shot1_loop", attacktime-20,attacktime+20,20) {
		complex vel = cexp(I*frand()*2*M_PI)*(2+0.1*(global.diff-D_Easy));
		if(e->args[2] == 0) { // attack type
			int corners = 5;
			double len = 50;
			int count = 5;
			for(int i = 0; i < corners; i++) {
				for(int j = 0; j < count; j++) {
					complex pos = len/2/tan(2*M_PI/corners)*I+(j/(double)count-0.5)*len;
					pos *= cexp(I*2*M_PI/corners*i);
					PROJECTILE("flea", e->pos+pos, rgb(1, 0.3, 0.5), linear, { vel+0.1*I*pos/cabs(pos) });
				}
			}
		} else {
			int count = 30;
			double rad = 20;
			for(int j = 0; j < count; j++) {
				double x = (j/(double)count-0.5)*2*M_PI;
				complex pos = 0.5*cos(x)+sin(2*x) + (0.5*sin(x)+cos(2*x))*I;
				pos*=vel/cabs(vel);
				PROJECTILE("flea", e->pos+rad*pos, rgb(0.5, 0.3, 1), linear, { vel+0.1*pos });
			}
		}
	}
	AT(attacktime) {
		e->args[0] = global.plr.pos-e->pos;
		kurumi_extra_create_drainer(e);
		play_sound("redirect");
	}
	FROM_TO(attacktime,attacktime+flytime,1) {
		e->pos += e->args[0]/flytime;
	}

	FROM_TO(attacktime,attacktime+flytime,20-global.diff*3) {
		if(global.diff>D_Easy) {
			PROJECTILE("ball", e->pos, rgba(0.1+0.07*_i, 0.3, 1-0.05*_i,0.8), timeout, { 50 }, .flags = PFLAG_DRAWADD);
		}
	}
	if(t > attacktime + flytime + 20 && global.boss) {
		GO_TO(e,global.boss->pos,0.04)
	}

	return 1;
}

void kurumi_extra(Boss *b, int time) {
	int length = 400;
	int t = time % length;
	int direction = (time/length)%2;

	int castlimit = b->current->maxhp * 0.05;
	int shieldlimit = b->current->maxhp * 0.1;

	TIMER(&t);

	if(time == EVENT_DEATH) {
		killall(global.enemies);
		return;
	}

	if(time < 120)
		GO_TO(b, VIEWPORT_W * 0.5 + VIEWPORT_H * 0.28 * I, 0.01)

	if(t == 0 && b->current->hp >= shieldlimit) {
		int cnt = 12;
		for(int i = 0; i < cnt; ++i) {
			double a = M_PI * 2 * i / (double)cnt;
			int hp = 500;
			create_enemy2c(b->pos, hp, kurumi_extra_swirl_visual, kurumi_extra_shield, a + 0.05*I, 800);
			create_enemy2c(b->pos, hp, kurumi_extra_swirl_visual, kurumi_extra_shield, a - 0.05*I, 800);
		}
	}

	AT(90) {
		int cnt = 20;
		for(int i = 0; i < cnt; i++) {
			b->current->hp -= 600;
			if(b->current->hp < castlimit)
				b->current->hp = castlimit;
			tsrand_fill(2);
			complex pos = VIEWPORT_W/2*afrand(0)+I*afrand(1)*VIEWPORT_H*2/3;
			if(direction)
				pos = VIEWPORT_W-creal(pos)+I*cimag(pos);
			// immune so they don’t get killed while they are still offscreen.
			create_enemy3c(pos-300*(1-2*direction),ENEMY_IMMUNE,kurumi_extra_fairy_visual,kurumi_extra_fairy,pos,100+20*i+100*(1.1-0.05*global.diff)*I,direction);
		}

		// XXX: maybe add a special sound for this?
		play_sound("shot_special1");
	}

	complex sidepos = VIEWPORT_W * (0.5+0.3*(1-2*direction)) + VIEWPORT_H * 0.28 * I;
	FROM_TO(90,120,1) {
		GO_TO(b, sidepos,0.1)
	}

	FROM_TO(190,200,1) {
		GO_TO(b, sidepos+30*I,0.1)
	}
	b->ani.stdrow=0;
	FROM_TO(190,300,1) {
		b->ani.stdrow=1;
	}

	FROM_TO(300,400,1) {
		GO_TO(b,VIEWPORT_W * 0.5 + VIEWPORT_H * 0.28 * I,0.1)
	}

	if(global.diff >= D_Hard) {
		AT(300) {
			double ofs = VIEWPORT_W * 0.5;
			complex pos = 0.5 * VIEWPORT_W + I * (VIEWPORT_H - 100);
			complex targ = 0.5 *VIEWPORT_W + VIEWPORT_H * 0.3 * I;
			create_enemy1c(pos + ofs, 3300, kurumi_extra_bigfairy_visual, kurumi_extra_bigfairy1, targ + 0.8*ofs);
			create_enemy1c(pos - ofs, 3300, kurumi_extra_bigfairy_visual, kurumi_extra_bigfairy1, targ - 0.8*ofs);
		}
	}
	if((t == length-20 && global.diff == D_Easy)|| b->current->hp < shieldlimit) {
		for(Enemy *e = global.enemies; e; e = e->next) {
			if(e->logic_rule == kurumi_extra_shield) {
				e->args[2] = 1; // discharge extra shield
				e->hp = 0;
				continue;
			}
		}
	}
}


Boss *create_kurumi(void) {
	Boss* b = stage4_spawn_kurumi(-400.0*I);

	boss_add_attack(b, AT_Move, "Introduction", 4, 0, kurumi_boss_intro, NULL);
	boss_add_attack(b, AT_Normal, "Sin Breaker", 25, 33000, kurumi_sbreaker, NULL);
	if(global.diff < D_Hard) {
		boss_add_attack_from_info(b, &stage4_spells.boss.animate_wall, false);
	} else {
		boss_add_attack_from_info(b, &stage4_spells.boss.demon_wall, false);
	}
	boss_add_attack(b, AT_Normal, "Cold Breaker", 25, 36000, kurumi_breaker, NULL);
	boss_add_attack_from_info(b, &stage4_spells.boss.blow_the_walls, false);
	boss_add_attack_from_info(b, &stage4_spells.boss.bloody_danmaku, false);
	boss_add_attack_from_info(b, &stage4_spells.extra.vlads_army, false);
	boss_start_attack(b, b->attacks);

	return b;
}

static int scythe_post_mid(Enemy *e, int t) {
	TIMER(&t);

	int fleetime = creal(e->args[3]);

	if(t == EVENT_DEATH) {
		if(fleetime >= 300) {
			spawn_items(e->pos, Life, 1, NULL);
		}

		return 1;
	}

	if(t < 0) {
		return 1;
	}

	AT(fleetime) {
		return ACTION_DESTROY;
	}

	double scale = min(1.0, t / 60.0) * (1.0 - clamp((t - (fleetime - 60)) / 60.0, 0.0, 1.0));
	double alpha = scale * scale;
	double spin = (0.2 + 0.2 * (1.0 - alpha)) * 1.5;

	complex opos = VIEWPORT_W/2+160*I;
	double targ = (t-300) * (0.5 + psin(t/300.0));
	double w = min(0.15, 0.0001*targ);

	complex pofs = 150*cos(w*targ+M_PI/2.0) + I*80*sin(2*w*targ);
	pofs += ((VIEWPORT_W/2+VIEWPORT_H/2*I - opos) * (global.diff - D_Easy)) / (D_Lunatic - D_Easy);

	e->pos = opos + pofs * (1.0 - clamp((t - (fleetime - 120)) / 60.0, 0.0, 1.0)) * smooth(smooth(scale));
	e->args[2] = 0.5 * scale + (1.0 - alpha) * I;
	e->args[1] = creal(e->args[1]) + spin * I;

	FROM_TO(90, fleetime - 120, 1) {
		complex shotorg = e->pos+80*cexp(I*creal(e->args[1]));
		complex shotdir = cexp(I*creal(e->args[1]));

		struct projentry { char *proj; char *snd; } projs[] = {
			{ "ball",		"shot1"},
			{ "bigball",	"shot1"},
			{ "soul",		"shot_special1"},
			{ "bigball",	"shot1"},
		};

		struct projentry *pe = &projs[_i % (sizeof(projs)/sizeof(struct projentry))];

		double ca = creal(e->args[1]) + _i/60.0;
		Color c = rgb(cos(ca), sin(ca), cos(ca+2.1));

		play_sound_ex(pe->snd, 3, true);
		PROJECTILE(pe->proj, shotorg, c, asymptotic, {
			(1.2-0.1*global.diff)*shotdir,
			5 * sin(t/150.0)
		});

	}

	FROM_TO(fleetime - 120, fleetime, 1) {
		stage_clear_hazards(false);
	}

	scythe_common(e, t);
	return 1;
}

void stage4_skip(int t);

void stage4_events(void) {
	TIMER(&global.timer);

	AT(0) {
		stage_start_bgm("stage4");
		stage4_skip(getenvint("STAGE4_TEST", 0));
	}

	AT(70) {
		create_enemy1c(VIEWPORT_H/4*3*I, 3000, BigFairy, stage4_splasher, 3-4.0*I);
		create_enemy1c(VIEWPORT_W + VIEWPORT_H/4*3*I, 3000, BigFairy, stage4_splasher, -3-4.0*I);
	}

	FROM_TO(300, 450, 20) {
		create_enemy1c(VIEWPORT_W*frand(), 200, Fairy, stage4_fodder, 3.0*I);
	}

	FROM_TO(500, 550, 10) {
		int d = _i&1;
		create_enemy1c(VIEWPORT_W*d, 1000, Fairy, stage4_partcircle, 2*cexp(I*M_PI/2.0*(0.2+0.6*frand()+d)));
	}

	FROM_TO(600, 1400, 100) {
		create_enemy3c(VIEWPORT_W/4.0 + VIEWPORT_W/2.0*(_i&1), 3000, BigFairy, stage4_cardbuster, VIEWPORT_W/6.0 + VIEWPORT_W/3.0*2*(_i&1)+100.0*I,
					VIEWPORT_W/4.0 + VIEWPORT_W/2.0*((_i+1)&1)+300.0*I, VIEWPORT_W/2.0+VIEWPORT_H*I+200.0*I);
	}

	AT(1800) {
		create_enemy1c(VIEWPORT_H/2.0*I, 2000, Swirl, stage4_backfire, 0.3);
		create_enemy1c(VIEWPORT_W+VIEWPORT_H/2.0*I, 2000, Swirl, stage4_backfire, -0.5);
	}

	FROM_TO(2000, 2600, 20)
		create_enemy1c(300.0*I*frand(), 200, Fairy, stage4_fodder, 2);

	FROM_TO(2000, 2400, 200)
		create_enemy1c(VIEWPORT_W/2+200-400*frand(), 1000, BigFairy, stage4_bigcircle, 2.0*I);

	FROM_TO(2600, 3000, 10)
		create_enemy1c(20.0*I+VIEWPORT_H/3*I*frand()+VIEWPORT_W, 100, Swirl, stage4_explosive, -3);

	AT(3200)
		global.boss = create_kurumi_mid();

	int midboss_time = STAGE4_MIDBOSS_MUSIC_TIME;

	AT(3201) {
		if(global.boss) {
			global.timer += min(midboss_time, global.frames - global.boss->birthtime) - 1;
		}
	}

	FROM_TO(3201, 3201 + midboss_time - 1, 1) {
		if(!global.enemies) {
			create_enemy4c(VIEWPORT_W/2+160.0*I, ENEMY_IMMUNE, Scythe, scythe_post_mid, 0, 1+0.2*I, 0+1*I, 3201 + midboss_time - global.timer);
		}
	}

	FROM_TO(3201 + midboss_time, 3601 + midboss_time, 10)
		create_enemy1c(VIEWPORT_W*(_i&1)+VIEWPORT_H/2*I-300.0*I*frand(), 200, Fairy, stage4_fodder, 2-4*(_i&1)+1.0*I);

	FROM_TO(3350 + midboss_time, 4000 + midboss_time, 100)
		create_enemy3c(VIEWPORT_W/4.0 + VIEWPORT_W/2.0*(_i&1), 3000, BigFairy, stage4_cardbuster, VIEWPORT_W/2.+VIEWPORT_W*0.4*(1-2*(_i&1))+100.0*I,
					VIEWPORT_W/4.0+VIEWPORT_W/2.0*((_i+1)&1)+300.0*I, VIEWPORT_W/2.0-200.0*I);

	AT(3800 + midboss_time)
		create_enemy1c(VIEWPORT_W/2, 9000, BigFairy, stage4_supercard, 4.0*I);

	FROM_TO(4300 + midboss_time, 4600 + midboss_time, 95-10*global.diff)
		create_enemy1c(VIEWPORT_W*(_i&1)+100*I, 200, Swirl, stage4_backfire, frand()*(1-2*(_i&1)));

	FROM_TO(4800 + midboss_time, 5200 + midboss_time, 10)
		create_enemy1c(20.0*I+I*VIEWPORT_H/3*frand()+VIEWPORT_W*(_i&1), 100, Swirl, stage4_explosive, (1-2*(_i&1))*3+I);

	AT(5300 + midboss_time)
		global.boss = create_kurumi();

	AT(5400 + midboss_time)
		global.dialog = stage4_dialog_end();

	AT(5550 + midboss_time - FADE_TIME) {
		stage_finish(GAMEOVER_WIN);
	}
}
