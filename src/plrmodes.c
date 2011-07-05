/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "plrmodes.h"
#include "player.h"
#include "global.h"

/* Youmu */

void YoumuOppositeMyon(Enemy *e, int t) {
	complex pos = e->pos + global.plr.pos;
	
	create_particle2c("flare", pos, NULL, Shrink, timeout, 10, -e->pos+10I);
}

int youmu_opposite_myon(Enemy *e, int t) {
	if(t == EVENT_DEATH)
		free_ref(e->args[0]);
	if(t < 0)
		return 1;
	
	Player *plr = &global.plr;
	
	if(plr->focus < 15) {
		e->args[2] = carg(plr->pos - e->pos0);
		e->pos = e->pos0 - plr->pos;
		
		if(cabs(e->pos) > 30)
			e->pos -= 5*cexp(I*carg(e->pos));
	}
	
	if(plr->fire && !(global.frames % 4))
		create_projectile1c("youmu", e->pos + plr->pos, NULL, linear, -21*cexp(I*e->args[2]))->type = PlrProj; 
	
	e->pos0 = e->pos + plr->pos;
	
	return 1;
}

int youmu_homing(Projectile *p, int t) { // a[0]: velocity, a[1]: target, a[2]: old velocity
 	p->angle = rand();
	if(t == EVENT_DEATH)
		free_ref(p->args[1]);
	
	
	complex tv = p->args[2];
	if(REF(p->args[1]) != NULL)
		tv = cexp(I*carg(((Enemy *)REF(p->args[1]))->pos - p->pos));
	
		
	p->args[2] = tv;
	
	p->pos += p->args[0]*log(t + 1) + tv*t*t/300.0;
	p->pos0 = p->pos;
	
	return 1;
}

void youmu_shot(Player *plr) {
	if(plr->fire) {
		if(!(global.frames % 4)) {
			create_projectile1c("youmu", plr->pos + 10 - I*20, NULL, linear, -20I)->type = PlrProj;
			create_projectile1c("youmu", plr->pos - 10 - I*20, NULL, linear, -20I)->type = PlrProj;
						
			if(plr->power >= 2) {
				float a = 0.20;
				if(plr->focus > 0) a = 0.06;
				create_projectile1c("youmu", plr->pos - 10 - I*20, NULL, linear, -20I*cexp(-I*a))->type = PlrProj;
				create_projectile1c("youmu", plr->pos + 10 - I*20, NULL, linear, -20I*cexp(I*a))->type = PlrProj;
			}		
		}
		
		float a = 1;
		if(plr->focus > 0)
			a = 0.4;
		if(plr->shot == YoumuHoming && !(global.frames % 7)) {
			complex ref = -1;
			if(global.boss != NULL)
				ref = add_ref(global.boss);
			else if(global.enemies != NULL)
				ref = add_ref(global.enemies);
			
			if(ref != -1)
				create_projectile2c("hghost", plr->pos, NULL, youmu_homing, a*cexp(I*rand()), ref)->type = PlrProj;
		}
	}
	
	if(plr->shot == YoumuOpposite && plr->slaves == NULL)
		create_enemy_p(&plr->slaves, plr->pos, ENEMY_IMMUNE, YoumuOppositeMyon, youmu_opposite_myon, 0, 0, 0, 0);
}

void youmu_bomb(Player *plr) {
}

void youmu_power(Player *plr, float npow) {
}

/* Marisa */

// Laser Sign
void MariLaser(Projectile *p, int t) {
	if(REF(p->args[1]) == NULL)
		return;
	
	if(cimag(p->pos) - cimag(global.plr.pos) < 90) {
		glScissor(VIEWPORT_X, SCREEN_H - VIEWPORT_Y- cimag(((Enemy *)REF(p->args[1]))->pos)+1, VIEWPORT_W+VIEWPORT_X, VIEWPORT_H);
		glEnable(GL_SCISSOR_TEST);
	}
		
	ProjDraw(p, t);
		
	glDisable(GL_SCISSOR_TEST);
}

int mari_laser(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		free_ref(p->args[1]);
		return 1;
	}
		
	if(REF(p->args[1]) == NULL)
		return ACTION_DESTROY;
	
	linear(p, t);
	
	p->pos = ((Enemy *)REF(p->args[1]))->pos + p->pos;
	
	return 1;
}

int marisa_laser_slave(Enemy *e, int t) {
	if(global.plr.fire && global.frames - global.plr.recovery >= 0) {
		if(!(global.frames % 4))
			create_projectile_p(&global.projs, get_tex("proj/marilaser"), 0, NULL, MariLaser, mari_laser, -20I, add_ref(e),0,0)->type = PlrProj;
		
		if(!(global.frames%3)) {
			float s = 0.5 + 0.3*sin(global.frames/7.0);
			create_particle2c("marilaser_part0", 0, rgb(1-s,0.5,s), PartDraw, mari_laser, -15I, add_ref(e))->type = PlrProj;
		}
		create_particle1c("lasercurve", e->pos, NULL, Fade, timeout, 4)->type = PlrProj;
	}	
	
	e->pos = global.plr.pos + (1 - abs(global.plr.focus)/30.0)*e->pos0 + (abs(global.plr.focus)/30.0)*e->args[0];
	
	return 1;
}

void MariLaserSlave(Enemy *e, int t) {
	glPushMatrix();
	glTranslatef(creal(e->pos), cimag(e->pos), -1);
	glRotatef(global.frames * 3, 0, 0, 1);
	
	draw_texture(0,0,"part/lasercurve");
		
	glPopMatrix();
}

// Laser sign bomb (implemented as Enemy)

static void draw_masterspark_ring(complex base, int t) {
	glPushMatrix();
	
	glTranslatef(creal(base), cimag(base)-t*t*0.2, 0);
		
	float f = sqrt(t/500.0)*1200;
	
	Texture *tex = get_tex("masterspark_ring");
	glScalef(f/tex->w, 1-tex->h/f,0);
	draw_texture_p(0,0,tex);
		
	glPopMatrix();
}

void MasterSpark(Enemy *e, int t) {
	glPushMatrix();
	
	float angle = 9 - t/e->args[0]*6.0, fade = 1;
	
	if(t < creal(e->args[0]/6))
		fade = t/e->args[0]*6;
	
	if(t > creal(e->args[0])/4*3)
		fade = 1-t/e->args[0]*4 + 3;
	
	glColor4f(1,0.85,1,fade);	
	glTranslatef(creal(global.plr.pos), cimag(global.plr.pos), 0);	
	glRotatef(-angle,0,0,1);
	draw_texture(0, -450, "masterspark");
	glColor4f(0.85,1,1,fade);
	glRotatef(angle*2,0,0,1);
	draw_texture(0, -450, "masterspark");
	glColor4f(1,1,1,fade);
	glRotatef(-angle,0,0,1);
	draw_texture(0, -450, "masterspark");
	glPopMatrix();
	
	glColor4f(0.9,1,1,fade*0.8);
	int i;
	for(i = 0; i < 8; i++)
		draw_masterspark_ring(global.plr.pos - 50I, t%30 + 15*i);
	
	glColor4f(1,1,1,1);
}

int master_spark(Enemy *e, int t) {
	if(t > creal(e->args[0]))
		return ACTION_DESTROY;
		
	if(global.projs && global.projs->type != PlrProj)
		global.projs->type = DeadProj;
	
	if(global.boss)
		global.boss->dmg++;
	
	Enemy *en;
	for(en = global.enemies; en; en = en->next)
		en->hp--;
	
	return 1;
}

// Generic Marisa

void marisa_shot(Player *plr) {
	if(plr->fire) {
		if(!(global.frames % 6)) {
			create_projectile1c("marisa", plr->pos + 10 - 15I, NULL, linear, -20I)->type = PlrProj;
			create_projectile1c("marisa", plr->pos - 10 - 15I, NULL, linear, -20I)->type = PlrProj;
		}
	}
	
	if(plr->shot == MarisaLaser && plr->slaves == NULL)
		create_enemy_p(&plr->slaves, -40I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, -40I,0,0,0);
}

void marisa_bomb(Player *plr) {
	switch(plr->shot) {
	case MarisaLaser:
		create_enemy_p(&plr->slaves, 0, ENEMY_BOMB, MasterSpark, master_spark, 280,0,0,0);
		break;
	default:
		break;
	}
}

void marisa_power(Player *plr, float npow) {
	Enemy *e = plr->slaves, *tmp;
	while(e != 0) {
		tmp = e;
		e = e->next;
		if(tmp->hp == ENEMY_IMMUNE)
			delete_enemy(&plr->slaves, tmp);
	}
	
	switch(plr->shot) {
	case MarisaLaser:
		if(npow < 1) {
			create_enemy_p(&plr->slaves, -40I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, -40I,0,0,0);
			return;
		}
		
		if(npow >= 1) {
			create_enemy_p(&plr->slaves, 25-5I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, 8-40I,0,0,0);
			create_enemy_p(&plr->slaves, -25-5I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, -8-40I,0,0,0);
		}
		
		if(npow >= 3) {
			create_enemy_p(&plr->slaves, 17-30I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, 4-45I,0,0,0);
			create_enemy_p(&plr->slaves, -17-30I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, -4-45I,0,0,0);
		} else if(npow >= 2) {
			create_enemy_p(&plr->slaves, -30I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, -50I,0,0,0);
		}
		break;
	default:
		break;
	}
}