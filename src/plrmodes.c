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
	if(t < 0)
		return 1;
	
	Player *plr = &global.plr;
	
	if(plr->focus < 15) {
		e->args[2] = carg(plr->pos - e->pos0);
		e->pos = e->pos0 - plr->pos;
		
		if(cabs(e->pos) > 30)
			e->pos -= 5*cexp(I*carg(e->pos));
	}
	
	if(plr->fire && !(global.frames % 6))
		create_projectile1c("youmu", e->pos + plr->pos, NULL, linear, -21*cexp(I*e->args[2]))->type = PlrProj; 
	
	e->pos0 = e->pos + plr->pos;
	
	return 1;
}

int youmu_homing(Projectile *p, int t) { // a[0]: velocity, a[1]: target
	if(t == EVENT_DEATH) {
		return 1;
	}
		
	Enemy *target;
	
	if((target = REF(p->args[1]))) {
		p->args[0] += 0.15*cexp(I*carg(target->pos - p->pos));
	} else {
		free_ref(p->args[1]);
		
		if(global.boss)
			p->args[1] = add_ref(global.boss);
		else if(global.enemies)
			p->args[1] = add_ref(global.enemies);
		else
			p->args[1] = add_ref(NULL);
		
	}
	
	p->angle = carg(p->args[0]);
	
	p->pos += p->args[0];
		
	return 1;
}

void youmu_shot(Player *plr) {
	if(plr->fire) {
		if(!(global.frames % 6)) {
			create_projectile1c("youmu", plr->pos + 10 - I*20, NULL, linear, -20I)->type = PlrProj;
			create_projectile1c("youmu", plr->pos - 10 - I*20, NULL, linear, -20I)->type = PlrProj;		
		}
		
		if(plr->shot == YoumuHoming) {
			if(plr->focus && !(global.frames % 12-(int)plr->power)*1.5) {
				int ref = -1;
				if(global.boss != NULL)
					ref = add_ref(global.boss);
				else if(global.enemies != NULL)
					ref = add_ref(global.enemies);
				if(ref == -1)
					ref = add_ref(NULL);
				create_projectile2c("hghost", plr->pos, NULL, youmu_homing, 5*cexp(I*sin(global.frames/10.0))/I, ref)->type = PlrProj;
				create_projectile2c("hghost", plr->pos, NULL, youmu_homing, 5*cexp(I*-sin(global.frames/10.0))/I, ref)->type = PlrProj;
			}
			
			if(!plr->focus && !(global.frames % 8-(int)plr->power)) {
				float arg = -M_PI/2;
				if(global.enemies)
					arg = carg(global.enemies->pos - plr->pos);
				if(global.boss)
					arg = carg(global.boss->pos - plr->pos);
								
				create_projectile2c("hghost", plr->pos, NULL, accelerated, 10*cexp((arg-0.2)*I), 0.4*cexp((arg-0.2)*I))->type = PlrProj;
				create_projectile2c("hghost", plr->pos, NULL, accelerated, 10*cexp(arg*I), 0.4*cexp(arg*I))->type = PlrProj;
				create_projectile2c("hghost", plr->pos, NULL, accelerated, 10*cexp((arg+0.2)*I), 0.4*cexp((arg+0.2)*I))->type = PlrProj;
			}
		}
	}
	
	if(plr->shot == YoumuOpposite && plr->slaves == NULL)
		create_enemy_p(&plr->slaves, plr->pos, ENEMY_IMMUNE, YoumuOppositeMyon, youmu_opposite_myon, 0, 0, 0, 0);
}


int petal_gravity(Projectile *p, int t) {
	if(t < 0)
		return 1;
	
	complex d = VIEWPORT_W/2.0 + VIEWPORT_H/2.0*I - p->pos;
	
	if(t < 230)
		p->args[0] += 5000/pow(cabs(d),2)*cexp(I*carg(d));
	p->pos += p->args[0];
	
	return 1;
}

void Slice(Projectile *p, int t) {
	if(t < creal(p->args[0])/20.0)
		p->args[1] += 1;

	if(t > creal(p->args[0])/20.0*19)
		p->args[1] -= 1;
	
	float f = p->args[1]/p->args[0]*20.0;
	
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos),0);
	glScalef(f,1,1);
	draw_texture(0,0,"part/youmu_slice");
	
	glPopMatrix();
}

void youmu_bomb(Player *plr) {
	int i;
	switch(plr->shot) {
		case YoumuOpposite:
			for(i = 0; i < 40; i++) {
				complex pos = VIEWPORT_W*frand() + VIEWPORT_H*frand()*I;
				complex d = VIEWPORT_W/2.0+VIEWPORT_H/2.0*I - pos;
				create_particle4c("petal", pos, rgba(1,1,1,0.5) , Petal, petal_gravity, 5*I*cexp(I*carg(d)), 0, frand() + frand()*I, frand() + 360I*frand());
			}
			break;
		case YoumuHoming:
			petal_explosion(40, VIEWPORT_W/2.0 + VIEWPORT_H/2.0*I);
			plr->pos = VIEWPORT_W/2.0 + (VIEWPORT_H - 80)*I;
			for(i = 0; i < 5; i++)			
				create_particle1c("youmu_slice", VIEWPORT_W/2.0 - 160 + 80*i + VIEWPORT_H/2.0*I, NULL, Slice, timeout, 200+i*10);
			break;
	}
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
		
		if(!(global.frames % 3)) {
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

static void draw_masterspark_ring(complex base, int t, float fade) {
	glPushMatrix();
	
	glTranslatef(creal(base), cimag(base)-t*t*0.4, 0);
		
	float f = sqrt(t/500.0)*1200;
	
	glColor4f(1,1,1,fade*20.0/t);
	
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
	
// 	glColor4f(0.9,1,1,fade*0.8);
	int i;
	for(i = 0; i < 8; i++)
		draw_masterspark_ring(global.plr.pos - 50I, t%20 + 10*i, fade);
	
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

// Star Sign

void MariStar(Projectile *p, int t) {
	glColor4f(1,1,0.7,0.8);
	draw_texture_p(creal(p->pos), cimag(p->pos), p->tex);
	create_particle1c("flare", p->pos, NULL, Shrink, timeout, 10);
	glColor4f(1,1,1,1);
}

int marisa_star_slave(Enemy *e, int t) {
	if(global.plr.fire && global.frames - global.plr.recovery >= 0) {
		if(!(global.frames % 20))
			create_projectile_p(&global.projs, get_tex("proj/maristar"), e->pos, NULL, MariStar, accelerated, e->args[1], e->args[2], 0, 0)->type = PlrProj+4;
	}
	
	e->pos = global.plr.pos + (1 - abs(global.plr.focus)/30.0)*e->pos0 + (abs(global.plr.focus)/30.0)*e->args[0];
	
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
}

void marisa_bomb(Player *plr) {
	switch(plr->shot) {
	case MarisaLaser:
		play_sound("masterspark");
		create_enemy_p(&plr->slaves, 40I, ENEMY_BOMB, MasterSpark, master_spark, 280,0,0,0);
		break;
	default:
		break;
	}
}

void marisa_power(Player *plr, float npow) {
	Enemy *e = plr->slaves, *tmp;
	
	if((int)plr->power == (int)npow)
		return;
	
	while(e != 0) {
		tmp = e;
		e = e->next;
		if(tmp->hp == ENEMY_IMMUNE)
			delete_enemy(&plr->slaves, tmp);
	}
	
	switch(plr->shot) {
	case MarisaLaser:
		if((int)npow == 1)
			create_enemy_p(&plr->slaves, -40I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, -40I,0,0,0);
		
		if(npow >= 2) {
			create_enemy_p(&plr->slaves, 25-5I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, 8-40I,0,0,0);
			create_enemy_p(&plr->slaves, -25-5I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, -8-40I,0,0,0);
		}
		
		if((int)npow == 3)
			create_enemy_p(&plr->slaves, -30I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, -50I,0,0,0);
		
		if(npow >= 4) {
			create_enemy_p(&plr->slaves, 17-30I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, 4-45I,0,0,0);
			create_enemy_p(&plr->slaves, -17-30I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, -4-45I,0,0,0);
		}
		break;
	case MarisaStar:
		if((int)npow == 1)
			create_enemy_p(&plr->slaves, 40I, ENEMY_IMMUNE, MariLaserSlave, marisa_star_slave, -30I, 3I, -0.1I,0);
		
		if(npow >= 2) {
			create_enemy_p(&plr->slaves, 30I+15, ENEMY_IMMUNE, MariLaserSlave, marisa_star_slave, -30I+10, 3I, -0.2I,0);
			create_enemy_p(&plr->slaves, 30I-15, ENEMY_IMMUNE, MariLaserSlave, marisa_star_slave, -30I-10, 3I, -0.2I,0);
		}
		
		if((int)npow == 3)
			create_enemy_p(&plr->slaves, -30I, ENEMY_IMMUNE, MariLaserSlave, marisa_star_slave, -30I, 3I, -0.1I,0);
		if(npow >= 4) {
			create_enemy_p(&plr->slaves, 30, ENEMY_IMMUNE, MariLaserSlave, marisa_star_slave, 25, 2+3I, -0.03-0.1I,0);
			create_enemy_p(&plr->slaves, -30, ENEMY_IMMUNE, MariLaserSlave, marisa_star_slave, -25, -2+3I, 0.03-0.1I,0);
		}
		break;
	}
}