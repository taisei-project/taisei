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
		create_enemy_p(&plr->slaves, plr->pos, ENEMY_IMMUNE, youmu_opposite_draw, youmu_opposite_logic, 0, 0, 0, 0);
}

void youmu_opposite_draw(Enemy *e, int t) {
	complex pos = e->pos + global.plr.pos;
	
	create_particle2c("flare", pos, NULL, Shrink, timeout, 10, -e->pos+10I);
}

void youmu_opposite_logic(Enemy *e, int t) {
	if(t == EVENT_DEATH)
		free_ref(e->args[0]);
	if(t < 0)
		return;
	
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

void youmu_power(Player *plr, float npow) {
}

/* Marisa */

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

void marisa_laser_slave(Enemy *e, int t) {
	if(global.plr.fire) {
		if(!(global.frames % 4))
			create_projectile_p(&global.projs, get_tex("proj/marilaser"), 0, NULL, MariLaser, mari_laser, -20I, add_ref(e),0,0)->type = PlrProj;
		
		if(!(global.frames%3)) {
			float s = 0.5 + 0.3*sin(global.frames/7.0);
			create_particle2c("marilaser_part0", 0, rgb(1-s,0.5,s), PartDraw, mari_laser, -15I, add_ref(e))->type = PlrProj;
		}
		create_particle1c("lasercurve", e->pos, NULL, Fade, timeout, 4)->type = PlrProj;
	}	
	
	e->pos = global.plr.pos + (1 - abs(global.plr.focus)/30.0)*e->pos0 + (abs(global.plr.focus)/30.0)*e->args[0];
	
}

void MariLaserSlave(Enemy *e, int t) {
	glPushMatrix();
	glTranslatef(creal(e->pos), cimag(e->pos), -1);
	glRotatef(global.frames * 3, 0, 0, 1);
	
	draw_texture(0,0,"part/lasercurve");
		
	glPopMatrix();
}

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

void marisa_power(Player *plr, float npow) {
	delete_enemies(&plr->slaves);
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
}