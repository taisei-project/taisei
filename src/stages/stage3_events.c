/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include "stage3_events.h"
#include "global.h"
#include "stage.h"
#include "enemy.h"

Dialog *stage3_dialog(void) {
	Dialog *d = create_dialog(global.plr.cha == Marisa ? "dialog/marisa" : "dialog/youmu", "masterspark");
	
	dadd_msg(d, Left, "Ugh, it's like bugs being attracted by the light...");
	dadd_msg(d, Right, "That's right! The light makes us strong!");
	dadd_msg(d, Right, "This place is full of it, so feel my tremendous power!");
	dadd_msg(d, BGM, "bgm_stage3boss");
	
	return d;
}

int stage3_enterswirl(Enemy *e, int t) {
	TIMER(&t)
	
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 1, 1, 0, 0);
		
		float r, g;
		if(frand() > 0.5) {
			r = 0.3;
			g = 1.0;
		} else {
			r = 1.0;
			g = 0.3;
		}
		
		float a; for(a = 0; a < M_PI * 2; a += 1.3 - global.diff * 0.2) {
			complex dir = sin(a) + I * cos(a);
			float spd = 1 + 0.5 * sin(10 * a);
			
			create_projectile2c(e->args[1]? "ball" : "rice", e->pos, rgb(r, g, 1.0), accelerated,
				dir * 2,
				dir * spd * -0.03
			);
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

int stage3_slavefairy(Enemy *e, int t) {
	TIMER(&t)
	
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 1, 3, 0, 0);
		return 1;
	}
	
	AT(EVENT_BIRTH) {
		e->alpha = 0;
		e->unbombable = True;
	}
	
	if(t < 120)
		GO_TO(e, e->args[0], 0.03)
	
	FROM_TO_INT(30, 120, 5 - global.diff, 1, 1) {
		float a = global.timer * 0.5;
		if(_i % 2)
			a = -a;
		complex dir = sin(a) + I * cos(a);
		
		create_projectile2c("wave", e->pos + dir * 10, (global.timer % 2)? rgb(1.0, 0.3, 0.3) : rgb(0.3, 0.3, 1.0), accelerated,
			dir * 2,
			dir * -0.035
		);
		
		if(e->args[1] && !(_i % 10 / e->args[1])) create_projectile1c("ball", e->pos + dir * 10, rgb(0.3, 0.6, 0.3), linear,
			dir * (0.3 + 0.5 + 0.5 * sin(a * 3))
		);
	}
	
	if(t >= 120)
		e->pos += 3 * e->args[2] + 2.0I;
	
	return 0;
}

int stage3_bigfairy(Enemy *e, int t) {
	TIMER(&t)
	
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 5, 5, 0, 0);
		if(e->args[0] && global.timer > 2800)
			spawn_items(e->pos, 0, 0, 1, 0);
		return 1;
	}
	
	AT(EVENT_BIRTH) {
		e->alpha = 0;
		e->unbombable = True;
	}
	
	FROM_TO(30, 600, 270) {
		create_enemy3c(e->pos, 900, Fairy, stage3_slavefairy, e->pos + 70 + 50 * I, e->args[0], +1);
		create_enemy3c(e->pos, 900, Fairy, stage3_slavefairy, e->pos - 70 + 50 * I, e->args[0], -1);
	}
	
	FROM_TO(120, 600, 270) {
		create_enemy3c(e->pos, 900, Fairy, stage3_slavefairy, e->pos + 70 - 50 * I, e->args[0], +1);
		create_enemy3c(e->pos, 900, Fairy, stage3_slavefairy, e->pos - 70 - 50 * I, e->args[0], -1);
	}
	
	AT(600)
		e->hp = 0;
	
	return 0;
}

int stage3_bitchswirl(Enemy *e, int t) {
	TIMER(&t)
	
	AT(EVENT_BIRTH) {
		e->unbombable = True;
		return 1;
	}
	
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 1, 1, 0, 0);
		return -1;
	}
	
	FROM_TO(0, 120, 20) {
		create_projectile2c("flea", e->pos, rgb(1.0, 0.5, 0.5), accelerated,
			2*cexp(I*carg(global.plr.pos - e->pos)),
			0.005*cexp(I*(M_PI*2 * frand()))
		);
	}
	
	e->pos -= 5.0I * e->args[0];
	
	return 0;
}

int stage3_cornerfairy(Enemy *e, int t) {
	TIMER(&t)
	
	AT(EVENT_DEATH) {
		spawn_items(e->pos, 5, 5, 0, 0);
		return -1;
	}
	
	AT(EVENT_BIRTH) {
		e->alpha = 0;
		e->unbombable = True;
	}
	
	FROM_TO(0, 120, 1)
		GO_TO(e, e->args[0], 0.01)
	
	FROM_TO(120, 240, 1) {
		GO_TO(e, e->args[1], 0.025 * min((t - 120) / 42.0, 1))
		int d = 5; //(D_Lunatic - global.diff + 3);
		if(!(t % d)) {
			int i, cnt = 10 + global.diff * 1.5;
			
			for(i = 0; i < cnt; ++i) {
				float c = 0.5 + 0.5 * sin(t / 15.0);
				
				create_projectile2c(cabs(e->args[2])? "wave" : "thickrice", e->pos, cabs(e->args[2])? rgb(0.5 - c*0.2, 0.3 + c*0.7, 1.0) : rgb(1.0 - c*0.5, 0.6, 0.5 + c*0.5), asymptotic,
					//2*cexp(I*(carg(global.plr.pos - e->pos) + i)),
					(global.diff > D_Normal? 2 : 1.5)*cexp(I*((2*i*M_PI/cnt)+carg((VIEWPORT_W+I*VIEWPORT_H)/2 - e->pos))),
					1.5
				);
				
				/*
				if(global.diff > D_Normal && !(t % 5) && !e->args[2]) {
					create_projectile2c("flea", e->pos, rgb(1.0, 0.5, 0.5), asymptotic,
						2*cexp(I*(carg(global.plr.pos - e->pos) + i)),
						1.5
					);
				}
				*/
			}
		}
	}
	
	AT(260)
		e->hp = 0;
	
	return 0;
}

void stage3_mid_intro(Boss *boss, int time) {
	GO_TO(boss, VIEWPORT_W/2.0 + 100.0I, 0.03);
}

void stage3_mid_outro(Boss *boss, int time) {
	if(time == 0) {
		spawn_items(boss->pos, 10, 10, 0, 1);
		Projectile *p;
		for(p = global.projs; p; p = p->next)
			p->type = DeadProj;
	}
	
	boss->pos += pow(max(0, time)/30.0, 2) * cexp(I*(3*M_PI/2 + 0.5 * sin(time / 20.0)));
}

int stage3_mid_poison(Projectile *p, int time) {
	int result = accelerated(p, time);
	
	if(time < 0)
		return 1;
	
	if(!(time % (57 - global.diff * 3)) && p->type != DeadProj) {
		float a = p->args[2];
		float t = p->args[3] + time;
		
		create_projectile2c((frand() > 0.5)? "thickrice" : "rice", p->pos, rgb(0.3, 0.7 + 0.3 * psin(a/3.0 + t/20.0), 0.3), accelerated,
				0,
				0.005*cexp(I*(M_PI*2 * sin(a/5.0 + t/20.0)))
		);
	}
	
	return result;
}

int stage3_mid_a0_proj(Projectile *p, int time) {
	#define A0_PROJ_START 120
	#define A0_PROJ_CHARGE 20
	TIMER(&time)
	
	FROM_TO(A0_PROJ_START, A0_PROJ_START + A0_PROJ_CHARGE, 1)
		return 1;
	
	AT(A0_PROJ_START + A0_PROJ_CHARGE + 1) if(p->type != DeadProj) {
		p->args[1] = 3;
		p->args[0] = (3 + 2 * global.diff / (float)D_Lunatic) * cexp(I*carg(global.plr.pos - p->pos));
		
		int cnt = 2 + global.diff, i;
		for(i = 0; i < cnt; ++i) {
			tsrand_fill(2);
			create_particle3c("lasercurve", 0, rgb(max(0.3, 1.0 - p->clr->r), p->clr->g, p->clr->b), EnemyFlareShrink, enemy_flare, 100, cexp(I*(M_PI*anfrand(0))) * (1 + afrand(1)), add_ref(p));
			
			int jcnt = 1 + (global.diff > D_Normal), j;
			for(j = 0; j < jcnt; ++j) create_projectile1c("thickrice", p->pos, p->clr->r == 1.0? rgb(1.0, 0.3, 0.3) : rgb(0.3, 0.3, 1.0), accelerated,
				0
				-cexp(I*(i*2*M_PI/cnt + global.frames / 15.0)) * (1.0 + 0.1 * j)
			); //->draw = global.diff > D_Hard? ProjDrawAdd : ProjDraw;
		}
	}
	
	return asymptotic(p, time);
	#undef A0_PROJ_START
	#undef A0_PROJ_CHARGE
}

void stage3_mid_a0(Boss *boss, int time) {
	int i;
	TIMER(&time)
	
	GO_TO(boss, creal(global.plr.pos) + I*cimag(boss->pos), 0.03)
	
	FROM_TO_INT(0, 90000, 60 + 10 * (D_Lunatic - global.diff), 0, 1) {
		int cnt = 30 - 4 * (D_Lunatic - global.diff);
		
		for(i = 0; i < cnt; ++i) {
			complex v = (2 - psin((max(3, global.diff+1)*2*M_PI*i/(float)cnt) + time)) * cexp(I*2*M_PI/cnt*i);
			create_projectile2c("wave", boss->pos - v * 50, _i % 2? rgb(1.0, 1.0, 0.3) : rgb(0.3, 1.0, 0.3), stage3_mid_a0_proj,
				v,
				2.0
			);
		}
	}
}

void stage3_mid_a1(Boss *boss, int time) {
	int i;
	TIMER(&time)
	
	FROM_TO(0, 120, 1)
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/2, 0.03)
	
	if(time > 120) {
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/2 + sin(time/50.0) * time/6.5 * cexp(I * M_PI_2 * time/100.0), 0.03)
		
		if(!(time % 70)) {
			for(i = 0; i < 15; ++i) {
				float a = M_PI/(5 + global.diff) * i * 2;
				create_projectile4c("wave", boss->pos, rgb(0.3, 0.3 + 0.7 * psin(a*3 + time/50.0), 0.3), stage3_mid_poison,
					0,
					0.02 * cexp(I*(a+time/10.0)),
					a,
					time
				);
			}
		}
		
		if(global.diff > D_Easy && !(time % 35)) {
			int cnt = global.diff * 3;
			for(i = 0; i < cnt; ++i) create_projectile2c("ball", boss->pos, rgb(1.0, 1.0, 0.3), asymptotic,
				(0.5 + 3 * psin(time + M_PI/3*2*i)) * cexp(I*(time / 20.0 + M_PI/cnt*i*2)),
				1.5
			);
		}
		
		if(!(time % 3)) {
			for(i = -1; i < 2; i += 2) {
				float c = psin(time/10.0);
				create_projectile1c("crystal", boss->pos, rgba(0.3 + c * 0.7, 0.6 - c * 0.3, 0.3, 0.7), linear,
					10 * cexp(I*(carg(global.plr.pos - boss->pos) + (M_PI/4.0 * i * (1-time/2500.0)) * (1 - 0.5 * psin(time/15.0))))
				);
			}
		}
	}
}

int stage3_mid_poison2(Projectile *p, int time) {
	int result = linear(p, time);
	
	if(time < 0)
		return 1;
	
	int d = 30 - global.diff * 3;
	if(!(time % d) && p->type != DeadProj) {
		float a = p->args[2];
		float t = p->args[3] + time;
		p->args[1] = !p->args[1];
		
		create_projectile2c((time % (2*d))? "thickrice" : "rice", p->pos, rgb(0.3 + 0.7 * psin(a/3.0 + t/20.0), 1.0, 0.3), accelerated,
				0,
				-0.01 * p->args[0]
		);
	}
	
	return result;
}

void stage3_mid_a2(Boss *boss, int time) {
	TIMER(&time)
	
	if(time < 0)
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/6, 0.05)

	FROM_TO_INT(30, 9000, 35 - (int)rint(global.diff * 1.35), 1, 1) {
		int i, cnt = 5 + global.diff;
		for(i = 0; i < cnt; ++i) {
			float a = M_PI/cnt * i * 2;
			
			create_projectile4c("soul", boss->pos, (_i % 2)? rgb(0.3, 0.3, 1.0) : rgb(1.0, 0.3, 0.3), stage3_mid_poison2,
				2 * cexp(I * (a + ((global.diff == D_Hard)? 7.2 : 7.3) * time + M_PI * (_i % 2))),
				0,
				a,
				time
			)->draw = ProjDrawAdd;
		}
	}
}

void stage3_mid_spellbg(Boss *h, int time) {
	float b = (0.3 + 0.2 * (sin(time / 50.0) * sin(time / 25.0f + M_PI)));
	float a = 1.0;
	
	if(time < 0)
		a += (time / (float)ATTACK_START_DELAY);
	float s = 0.3 + 0.7 * a;
	
	glColor4f(b*0.7, b*0.7, b*0.7, a);
	//int t = abs(time);
	
	fill_screen(-time/50.0 + 0.5, time/100.0+0.5, s, "stage3/spellbg1");
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	/*
	float ff = max(0, min(1, time * 0.01)) * 0.7;
	glColor4f(ff, ff, ff, ff);
	
	int i; for(i = 0; i < 5; ++i)
		fill_screen(sin(t / (10.0 + i * 0.5)) * 0.01, cos(t / (11.0 + i * 0.3)) * 0.01, s, "stage3/spellbg2");
	glColor4f(b*0.7, b*0.7, b*0.7, a);
	*/
	fill_screen(time/50.0 + 0.5, time/90.0+0.5, s, "stage3/spellbg1");
	fill_screen(-time/55.0 + 0.5, -time/100.0+0.5, s, "stage3/spellbg1");
	fill_screen(time/55.0 + 0.5, -time/90.0+0.5, s, "stage3/spellbg1");
	/*
	glColor4f(0.7, 0, 0, a * 0.7);
	fill_screen(time/99.0 + 0.5, -time/30.0+0.5, s*0.5, "stage3/spellbg1");
	fill_screen(time/37.0 + 0.5, -time/53.0+0.5, s*0.5, "stage3/spellbg1");
	*/
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glColor4f(1, 1, 1, 1);
}

void stage3_boss_spellbg(Boss *b, int time) {
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

Boss* stage3_create_midboss(void) {
	Boss *scuttle = create_boss("Scuttle", "scuttle", VIEWPORT_W/2 - 200.0I);
	boss_add_attack(scuttle, AT_Move, "Introduction", 2, 0, stage3_mid_intro, NULL);
	boss_add_attack(scuttle, AT_Normal, "Lethal Bite", 30, 25000, stage3_mid_a0, NULL);
	boss_add_attack(scuttle, AT_Spellcard, "Venom Sign ~ Deadly Dance", 25, 25000, stage3_mid_a1, stage3_mid_spellbg);
	if(global.diff > D_Normal)
		boss_add_attack(scuttle, AT_Spellcard, "Venom Sign ~ Acid Rain", 20, 23000, stage3_mid_a2, stage3_mid_spellbg);
	boss_add_attack(scuttle, AT_Move, "Runaway", 2, 1, stage3_mid_outro, NULL);
	scuttle->zoomcolor = rgb(0.4, 0.5, 0.4);
	
	start_attack(scuttle, scuttle->attacks);
	return scuttle;
}

int stage3_boss_a1_laserbullet(Projectile *p, int time) {
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
			
			Laser *l = create_lasercurve2c(p->pos, 2 * deathtime, deathtime, rgb(1.0, 0.5, 0.5), las_accel, 0, accel);
			l->width = 15;
			create_projectile3c("ball", p->pos, rgb(1.0, 0.5, 0.5), stage3_boss_a1_laserbullet, add_ref(l), deathtime - 1, 0);
		} else {
			int cnt = max(3 + global.diff, 5), i;
			
			for(i = 0; i < cnt; ++i) {
				create_projectile2c("thickrice", p->pos, (i % 2)? rgb(1.0, 0.5, 0.5) : rgb(0.5, 0.5, 1.0), asymptotic,
					(0.1 + frand()) * cexp(I*2*i*M_PI/cnt), 3
				)->draw = ProjDrawAdd;
			}
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

void stage3_boss_a1_slave_part(Projectile *p, int t) {
	Texture *tex = p->tex;
	glBindTexture(GL_TEXTURE_2D, tex->gltex);
	float b = 1 - t / p->args[0];
	glColor4f(p->clr->r*b, p->clr->g*b, p->clr->b*b, 1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	draw_texture_p(0,0, p->tex);
	glPopMatrix();
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1,1,1,1);
}

int stage3_boss_a1_slave(Enemy *e, int time) {
	TIMER(&time)
	
	float angle = e->args[2] * (time / 70.0 + e->args[1]);
	complex dir = cexp(I*angle);
	Boss *boss = (Boss*)REF(e->args[0]);
	
	if(!boss)
		return ACTION_DESTROY;
	
	AT(EVENT_DEATH) {
		free_ref(e->args[0]);
		spawn_items(e->pos, 1, 1, 0, 0);
		return 1;
	}
	
	GO_TO(e, boss->pos + 150 * sin(time / 100.0) * dir, 0.03)
	
	if(!(time % 2)) {
		float c = 0.5 * psin(time / 25.0);
		Projectile *p = create_projectile_p(&global.projs, prefix_get_tex("lasercurve", "part/"), e->pos, rgb(1.0 - c, 0.5, 0.5 + c), stage3_boss_a1_slave_part, timeout, 120, 0, 0, 0);
		p->type = FairyProj;
	}
	
	if(!creal(e->args[3]) && !(time % 140)) {
		float dt = 70;
		
		Laser *l = create_lasercurve3c(e->pos, dt, dt, rgb(1.0, 1.0, 0.5), las_sine, 2.5*dir, M_PI/4, 0.2);
		create_lasercurve4c(e->pos, dt, dt, rgb(0.5, 1.0, 0.5), las_sine_expanding, 2.5*dir, M_PI/20, 0.2, M_PI);
		create_projectile3c("ball", e->pos, rgb(1.0, 0.5, 0.5), stage3_boss_a1_laserbullet, add_ref(l), dt-1, 1);
	}
	
	// night ignite balls
	if(creal(e->args[3]) && global.diff > D_Easy) {
		FROM_TO(300, 1000000, 180) {
			int cnt = 5, i;
			for(i = 0; i < cnt; ++i) {
				create_projectile2c("ball", e->pos, rgb(0.5, 1.0, 0.5), accelerated, 0, 0.02 * cexp(I*i*2*M_PI/cnt))->draw = ProjDrawAdd;
				if(global.diff > D_Hard)
					create_projectile2c("ball", e->pos, rgb(1.0, 1.0, 0.5), accelerated, 0, 0.01 * cexp(I*i*2*M_PI/cnt))->draw = ProjDrawAdd;
			}
		}
	}
	
	return 1;
}

void stage3_boss_a1(Boss *boss, int time) {
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
			create_enemy3c(boss->pos, ENEMY_IMMUNE, Swirl, stage3_boss_a1_slave, add_ref(boss), i*2*M_PI/cnt, j);
	}
}

int stage3_boss_a2_laserbullet(Projectile *p, int time) {
	if(time == EVENT_DEATH) {
		free_ref(p->args[0]);
		return 1;
	} else if(time < 0)
		return 1;
	
	if(time < 0)
		return 1;
	
	Laser *laser = (Laser*)REF(p->args[0]);
	
	if(!laser)
		return ACTION_DESTROY;
	
	p->pos = laser->prule(laser, time - p->args[1]);
	
	return 1;
}

void stage3_boss_a2(Boss *boss, int time) {
	TIMER(&time)
	
	float dfactor = global.diff / (float)D_Lunatic;
	int i, j;
	
	AT(EVENT_DEATH) {
		killall(global.enemies);
		return;
	}
	
	if(time < 0) {
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/3, 0.05)
		return;
	}
	
	AT(0) for(j = -1; j < 2; j += 2) for(i = 0; i < 7; ++i)
		create_enemy4c(boss->pos, ENEMY_IMMUNE, Swirl, stage3_boss_a1_slave, add_ref(boss), i*2*M_PI/7, j, 1);
	
	FROM_TO_INT(0, 1000000, 180, 120, 10) {
		float dt = 200;
		float lt = 100 * dfactor;
		
		float a = _ni*M_PI/2.5 + _i + time;
		
		//float b = 0.45;
		//float clr = psin(time / 30.0) * (1.0 - b);
		
		//create_lasercurve2c(boss->pos, dt*2, dt, rgb(0.5, 1.0, 0.5), las_accel, 0, 0.1 * cexp(I*a));
		
		float b = 0.4;
		float c = 0.3;
		
		Laser *l1 = create_lasercurve3c(boss->pos, lt,			dt, rgb(b, b, 1.0), las_sine_expanding, 2 * cexp(I*a), M_PI/4, 0.05);
		Laser *l2 = create_lasercurve3c(boss->pos, lt * 1.5,	dt, rgb(1.0, b, b), las_sine_expanding, 2 * cexp(I*a), M_PI/4, 0.05 - 0.002 * min(global.diff, D_Hard));
		Laser *l3 = create_lasercurve3c(boss->pos, lt,			dt, rgb(b, b, 1.0), las_sine_expanding, 2 * cexp(I*a), M_PI/4, 0.05 - 0.004 * min(global.diff, D_Hard));
		
		l2->width = 15;
		
		int i; for(i = 0; i < 5 + 15 * dfactor; ++i) {
			create_projectile2c("plainball",	boss->pos, rgb(c, c, 1.0), stage3_boss_a2_laserbullet, add_ref(l1), i)->draw = ProjDrawAdd;
			create_projectile2c("bigball",		boss->pos, rgb(1.0, c, c), stage3_boss_a2_laserbullet, add_ref(l2), i)->draw = ProjDrawAdd;
			create_projectile2c("plainball",	boss->pos, rgb(c, c, 1.0), stage3_boss_a2_laserbullet, add_ref(l3), i)->draw = ProjDrawAdd;
		}
	}
}

void stage3_boss_a3(Boss *boss, int time) {
	TIMER(&time)
	
	AT(EVENT_DEATH) {
		return;
	}
	
	if(time < 0) {
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/2, 0.05)
		return;
	}
	
	int d = 4 - min(global.diff, D_Hard);
	FROM_TO_INT(0, 1000000, 100, 60, d) {
		float a = _ni*2*M_PI/(15.0/d) + _i + time*2;
		float dt = 150;
		float lt = 100;
		
		float b = 0.5;
		
		Color *c1;
		Color *c2;
		
		if(_i % 2) {
			c1 = rgb(1.0, 1.0, b);
			c2 = rgb(b, 1.0, b);
		} else {
			c1 = rgb(1.0, b, b);
			c2 = rgb(b, b, 1.0);
		}
		
		create_lasercurve3c(boss->pos, lt,			dt, c1, las_sine, 3 * cexp(I*a), M_PI/4, 0.05);
		create_lasercurve4c(boss->pos, lt,			dt, c2, las_sine, 3 * cexp(I*a), M_PI/4, 0.05, M_PI);
		
		if(global.diff > D_Hard)
			create_lasercurve2c(boss->pos, lt,		dt, rgb(b, 1.0, 1.0), las_accel, 0, 0.1 * cexp(I*(a + M_PI)));
	}
	
	int cnt = 35;
	FROM_TO_INT(120, 1000000, 60, cnt*2, 1) {
		create_projectile2c("wave", boss->pos, (_ni % 2)? rgb(1.0, 0.5, 0.5) : rgb(0.5, 0.5, 1.0), (_ni % 2)? asymptotic : linear,
			cexp(I*2*_ni*M_PI/cnt), 10
		)->draw = ProjDrawAdd;
	}
}

int stage3_boss_prea1_slave(Enemy *e, int time) {
	TIMER(&time)
	
	int level = e->args[3];
	float angle = e->args[2] * (time / 70.0 + e->args[1]);
	complex dir = cexp(I*angle);
	Boss *boss = (Boss*)REF(e->args[0]);
	
	if(!boss)
		return ACTION_DESTROY;
	
	AT(EVENT_DEATH) {
		free_ref(e->args[0]);
		spawn_items(e->pos, 1, 1, 0, 0);
		return 1;
	}
	
	if(time < 0)
		return 1;
	
	GO_TO(e, boss->pos + (100 + 20 * e->args[2] * sin(time / 100.0)) * dir, 0.03)
	
	int d = 10 - global.diff;
	if(level > 2)
		d += 4;
	
	if(!(time % d)) {
		create_projectile1c("rice", e->pos, rgb(1.0, 0.5, 0.2), linear, 3 * cexp(I*carg(boss->pos - e->pos)));
		if(!(time % (d*2)) || level > 1)
			create_projectile1c("thickrice", e->pos, rgb(1.0, 1.0, 0.2), linear, 2.5 * cexp(I*carg(boss->pos - e->pos)));
		if(level > 2)
			create_projectile1c("wave", e->pos, rgb(0.5, 0.2 + 0.8 * psin(time / 25.0), 1.0), linear, 2 * cexp(I*carg(boss->pos - e->pos)));
	}
	
	return 1;
}

void stage3_boss_pre_common(Boss *boss, int time, int level) {
	TIMER(&time)
	int i, j, cnt = 3 + global.diff;
	
	AT(0) for(j = -1; j < 2; j += 2) for(i = 0; i < cnt; ++i)
		create_enemy4c(boss->pos, ENEMY_IMMUNE, Swirl, stage3_boss_prea1_slave, add_ref(boss), i*2*M_PI/cnt, j, level);
		
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

void stage3_boss_prea1(Boss *boss, int time) {
	stage3_boss_pre_common(boss, time, 1);
}

void stage3_boss_prea2(Boss *boss, int time) {
	stage3_boss_pre_common(boss, time, 2);
}

void stage3_boss_prea3(Boss *boss, int time) {
	stage3_boss_pre_common(boss, time, 3);
}

void stage3_boss_intro(Boss *boss, int time) {
	if(time == 110)
		global.dialog = stage3_dialog();
	
	GO_TO(boss, VIEWPORT_W/2.0 + 100.0I, 0.03);
}

Boss* stage3_create_boss(void) {
	Boss *wriggle = create_boss("Wriggle EX", "wriggleex", VIEWPORT_W/2 - 200.0I);
	boss_add_attack(wriggle, AT_Move, "Introduction", 2, 0, stage3_boss_intro, NULL);
	boss_add_attack(wriggle, AT_Normal, "", 20, 15000, stage3_boss_prea1, NULL);
	boss_add_attack(wriggle, AT_Spellcard, "Firefly Sign ~ Moonlight Rocket", 30, 20000, stage3_boss_a1, stage3_boss_spellbg);
	boss_add_attack(wriggle, AT_Normal, "", 20, 15000, stage3_boss_prea2, NULL);
	boss_add_attack(wriggle, AT_Spellcard, "Light Source ~ Wriggle Night Ignite", 25, 40000, stage3_boss_a2, stage3_boss_spellbg);
	boss_add_attack(wriggle, AT_Normal, "", 20, 15000, stage3_boss_prea3, NULL);
	boss_add_attack(wriggle, AT_Spellcard, "Bug Sign ~ Phosphaenus Hemipterus", 35, 30000, stage3_boss_a3, stage3_boss_spellbg);
	
	start_attack(wriggle, wriggle->attacks);
	return wriggle;
}

void stage3_events(void) {
	TIMER(&global.timer);
	
// 	AT(0)
// 		global.timer = 4300;
	
	FROM_TO(160, 300, 10) {
		tsrand_fill(3);
		create_enemy1c(VIEWPORT_W/2 + 20 * anfrand(0) + (VIEWPORT_H/4 + 20 * anfrand(1))*I, 200, Swirl, stage3_enterswirl, I * 3 + anfrand(2) * 3);
	}
	
	AT(400) {
		create_enemy1c(VIEWPORT_W/2 + (VIEWPORT_H/3)*I, 10000, BigFairy, stage3_bigfairy, 0);
	}
	
	FROM_TO(1100, 1300, 10) {
		create_enemy1c(20 + (VIEWPORT_H+20)*I, 50, Swirl, stage3_bitchswirl, 1);
		create_enemy1c(VIEWPORT_W-20 + (VIEWPORT_H+20)*I, 50, Swirl, stage3_bitchswirl, 1);
	}
	
	AT(1600) {
		create_enemy1c(VIEWPORT_W/2 + (VIEWPORT_H/3)*I, 10000, BigFairy, stage3_bigfairy, 1);
	}
	
	//FROM_TO(2400, 2620, 130) {
	AT(2400) {
		double offs = -50;
		
		complex p1 = 0+0.0I;
		complex p2 = VIEWPORT_W+0.0I;
		complex p3 = VIEWPORT_W + VIEWPORT_H*I;
		complex p4 = 0+VIEWPORT_H*I;
		
		create_enemy2c(p1, 500, Fairy, stage3_cornerfairy, p1 - offs - offs*I, p2 + offs - offs*I);
		create_enemy2c(p2, 500, Fairy, stage3_cornerfairy, p2 + offs - offs*I, p3 + offs + offs*I);
		create_enemy2c(p3, 500, Fairy, stage3_cornerfairy, p3 + offs + offs*I, p4 - offs + offs*I);
		create_enemy2c(p4, 500, Fairy, stage3_cornerfairy, p4 - offs + offs*I, p1 - offs - offs*I);
	}
	
	AT(2800)
		global.boss = stage3_create_midboss();
	
	FROM_TO(2801, 3000, 10) {
		tsrand_fill(3);
		create_enemy2c(VIEWPORT_W/2 + 20 * anfrand(0) + (VIEWPORT_H/4 + 20 * anfrand(1))*I, 200, Swirl, stage3_enterswirl, I * 3 + anfrand(2) * 3, 1);
	}
	
	AT(3000) {
		create_enemy1c(VIEWPORT_W - VIEWPORT_W/3 + (VIEWPORT_H/5)*I, 10000, BigFairy, stage3_bigfairy, 2);
	}
	
	FROM_TO(3000, 3100, 20) {
		create_enemy2c(VIEWPORT_W-20 + (VIEWPORT_H+20)*I, 50, Swirl, stage3_bitchswirl, 1, 1);
	}
	
	AT(3500) {
		create_enemy1c(VIEWPORT_W/3 + (VIEWPORT_H/5)*I, 10000, BigFairy, stage3_bigfairy, 2);
	}
	
	FROM_TO(3500, 3600, 20) {
		create_enemy2c(20 + (VIEWPORT_H+20)*I, 50, Swirl, stage3_bitchswirl, 1, 1);
	}
	
	//FROM_TO(4330, 4460, 130) {
	AT(4330) {
		double offs = -50;
		
		complex p1 = 0+0.0I;
		complex p2 = VIEWPORT_W+0.0I;
		complex p3 = VIEWPORT_W + VIEWPORT_H*I;
		complex p4 = 0+VIEWPORT_H*I;
		
		create_enemy3c(p1, 500, Fairy, stage3_cornerfairy, p1 - offs - offs*I, p2 + offs - offs*I, 1);
		create_enemy3c(p2, 500, Fairy, stage3_cornerfairy, p2 + offs - offs*I, p3 + offs + offs*I, 1);
		create_enemy3c(p3, 500, Fairy, stage3_cornerfairy, p3 + offs + offs*I, p4 - offs + offs*I, 1);
		create_enemy3c(p4, 500, Fairy, stage3_cornerfairy, p4 - offs + offs*I, p1 - offs - offs*I, 1);
	}
	
	FROM_TO(4760, 4940, 10) {
		create_enemy2c(VIEWPORT_W-20 - 20.0I, 50, Swirl, stage3_bitchswirl, -0.5, 1);
		create_enemy2c(20 + -20.0I, 50, Swirl, stage3_bitchswirl, -0.5, 1);
	}
	
	AT(5300)
		global.boss = stage3_create_boss();

}
