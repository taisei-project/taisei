/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "plrmodes.h"
#include "player.h"
#include "global.h"
#include "stage.h"

/* Youmu */

// Haunting Sign

int youmu_homing(Projectile *p, int t) { // a[0]: velocity, a[1]: target
	if(t == EVENT_DEATH) {
		return 1;
	}
		
	Enemy *target;
	
	if((target = REF(p->args[1]))) {
		p->args[0] += 0.2*cexp(I*carg(target->pos - p->pos));
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

void Slice(Projectile *p, int t) {
	if(t < creal(p->args[0])/20.0)
		p->args[1] += 1;

	if(t > creal(p->args[0])-10) {
		p->args[1] += 3;
		p->args[2] += 1;
	}
	
	float f = p->args[1]/p->args[0]*20.0;
	
	glColor4f(1,1,1,1.0 - p->args[2]/p->args[0]*20.0);
	
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos),0);
	glRotatef(p->angle,0,0,1);
	glScalef(f,1,1);
	draw_texture(0,0,"part/youmu_slice");
	
	glPopMatrix();
	
	glColor4f(1,1,1,1);
}

void YoumuSlash(Enemy *e, int t) {
	fade_out(10.0/t+sin(t/10.0)*0.1);
	
}

int spin(Projectile *p, int t) {
	int i = timeout_linear(p, t);
	
	if(t < 0)
		return 1;	
		
	p->args[3] += 0.06;
	p->angle = p->args[3];
		
	return i;
}

int youmu_slash(Enemy *e, int t) {
	if(t > creal(e->args[0]))
		return ACTION_DESTROY;
	if(t < 0)
		return 1;
	
	TIMER(&t);
	
	AT(0)
		global.plr.pos = VIEWPORT_W/5.0 + (VIEWPORT_H - 100)*I;
	
	FROM_TO(8,20,1)
		global.plr.pos = VIEWPORT_W + (VIEWPORT_H - 100)*I - exp(-_i/8.0+log(4*VIEWPORT_W/5.0));
		
	FROM_TO(30, 60, 10) {
		tsrand_fill(3);
		create_particle1c("youmu_slice", VIEWPORT_W/2.0 - 150 + 100*_i + VIEWPORT_H/2.0*I - 10-10I + 20*afrand(0)+20I*afrand(1), NULL, Slice, timeout, 200)->angle = -10.0+20.0*afrand(2);
	}
	
	FROM_TO(40,200,1)
		if(frand() > 0.7) {
			tsrand_fill(6);
			create_particle2c("blast", VIEWPORT_W*afrand(0) + (VIEWPORT_H+50)*I, rgb(afrand(1),afrand(2),afrand(3)), Shrink, timeout_linear, 80, 3*(1-2.0*afrand(4))-14I+afrand(5)*2I);
		}
		
	int tpar = 30;
	if(t < 30)
		tpar = t;
	
	if(t < creal(e->args[0])-60 && frand() > 0.2) {
		tsrand_fill(3);
		create_particle2c("smoke", VIEWPORT_W*afrand(0) + (VIEWPORT_H+100)*I, rgba(0.4,0.4,0.4,afrand(1)*0.2 - 0.2 + 0.6*(tpar/30.0)), PartDraw, spin, 300, -7I+afrand(2)*1I);		
	}
	return 1;
}

// Opposite Sign

void YoumuOppositeMyon(Enemy *e, int t) {
	complex pos = e->pos;
	
	create_particle2c("flare", pos, NULL, Shrink, timeout, 10, -e->pos+10I);
}

int youmu_opposite_myon(Enemy *e, int t) {
	if(t == EVENT_BIRTH)
		e->pos = e->pos0 + global.plr.pos;
	if(t < 0)
		return 1;
	
	Player *plr = &global.plr;
	float arg = carg(e->pos0);
	float rad = cabs(e->pos0);
	
	if(plr->focus < 1 && plr->moveflags && !creal(e->args[0]))
		arg -= (carg(e->pos0)-carg(e->pos-plr->pos))*2;
	
	if(global.frames - plr->prevmovetime <= 30 && global.frames == plr->movetime) {
		int new = plr->curmove;
		int old = plr->prevmove;
		
		if(new == MOVEFLAG_UP && old == MOVEFLAG_DOWN) {
			arg = -M_PI/2;
			e->args[0] = plr->movetime;
		} else if(new == MOVEFLAG_DOWN && old == MOVEFLAG_UP) {
			arg = -3*M_PI/2;
			e->args[0] = plr->movetime;
		} else if(new == MOVEFLAG_LEFT && old == MOVEFLAG_RIGHT) {
			arg = M_PI;
			e->args[0] = plr->movetime;
		} else if(new == MOVEFLAG_RIGHT && old == MOVEFLAG_LEFT) {
			arg = 0;
			e->args[0] = plr->movetime;
		}
	}
	
	if(creal(e->args[0]) && plr->movetime != creal(e->args[0]))
		e->args[0] = 0;
	
	e->pos0 = rad * cexp(I*arg);
	complex target = plr->pos + e->pos0;
	e->pos += cexp(I*carg(target - e->pos)) * min(10, 0.07 * cabs(target - e->pos));
	
	if(plr->fire && !(global.frames % 6) && global.plr.deathtime >= -1) {
		int a = 20;
		
		if(plr->power >= 3) {
			a = 13;
			create_projectile1c("hghost", e->pos, NULL, linear, -21*cexp(I*(carg(-e->pos0)+0.1)))->type = PlrProj+45;
			create_projectile1c("hghost", e->pos, NULL, linear, -21*cexp(I*(carg(-e->pos0)-0.1)))->type = PlrProj+45;
		}
		
		create_projectile1c("hghost", e->pos, NULL, linear, -21*cexp(I*carg(-e->pos0)))->type = PlrProj+(60+a*(int)plr->power);
	}
	
	return 1;
}

int youmu_split(Enemy *e, int t) {
	if(t < 0)
		return 1;
	
	if(t > creal(e->args[0]))
		return ACTION_DESTROY;
	
	TIMER(&t);
	
	FROM_TO(30,260,1) {
		tsrand_fill(2);
		create_particle2c("smoke", VIEWPORT_W/2 + VIEWPORT_H/2*I, rgba(0.4,0.4,0.4,afrand(0)*0.2+0.4), PartDraw, spin, 300, 6*cexp(I*afrand(1)*2*M_PI));
	}
	
	FROM_TO(100,220,10) {
		tsrand_fill(3);
		create_particle1c("youmu_slice", VIEWPORT_W/2.0 + VIEWPORT_H/2.0*I - 200-200I + 400*afrand(0)+400I*afrand(1), NULL, Slice, timeout, 100-_i)->angle = 360.0*afrand(2);
	}
	
	float talt = atan((t-e->args[0]/2)/30.0)*10+atan(-e->args[0]/2);
	global.plr.pos = VIEWPORT_W/2.0 + (VIEWPORT_H-80)*I + VIEWPORT_W/3.0*sin(talt);
	
	return 1;
}

// Youmu Generic

void youmu_shot(Player *plr) {
	if(plr->fire) {

		if(!(global.frames % 4))
			play_sound("generic_shot");
		
		if(!(global.frames % 6)) {
			create_projectile1c("youmu", plr->pos + 10 - I*20, NULL, linear, -20I)->type = PlrProj+120;
			create_projectile1c("youmu", plr->pos - 10 - I*20, NULL, linear, -20I)->type = PlrProj+120;		
		}
		
		if(plr->shot == YoumuHoming) {
			if(plr->focus && !(global.frames % 45)) {
				int ref = -1;
				if(global.boss != NULL)
					ref = add_ref(global.boss);
				else if(global.enemies != NULL)
					ref = add_ref(global.enemies);
				if(ref == -1)
					ref = add_ref(NULL);
				
				create_projectile2c("youhoming", plr->pos, NULL, youmu_homing, -3I, ref)->type = PlrProj+(450+225*((int)plr->power));
			}
			
			if(!plr->focus && !(global.frames % (int)round(8-plr->power*1.3))) {												
				create_projectile2c("hghost", plr->pos, NULL, accelerated, 2-10I, -0.4I)->type = PlrProj+27;
				create_projectile2c("hghost", plr->pos, NULL, accelerated, -10I, -0.4I)->type = PlrProj+27;
				create_projectile2c("hghost", plr->pos, NULL, accelerated, -2-10I, -0.4I)->type = PlrProj+27;
			}
		}
	}
	
	if(plr->shot == YoumuOpposite && plr->slaves == NULL)
		create_enemy_p(&plr->slaves, 40I, ENEMY_IMMUNE, YoumuOppositeMyon, youmu_opposite_myon, 0, 0, 0, 0);
}

void youmu_bomb(Player *plr) {
	switch(plr->shot) {
		case YoumuOpposite:			
			create_enemy_p(&plr->slaves, 40I, ENEMY_BOMB, NULL, youmu_split, 280,0,0,0);
			
			break;
		case YoumuHoming:
			create_enemy_p(&plr->slaves, 40I, ENEMY_BOMB, YoumuSlash, youmu_slash, 280,0,0,0);
			break;
	}
}

void youmu_power(Player *plr, float npow) {
	if(plr->shot == YoumuOpposite && plr->slaves == NULL)
		create_enemy_p(&plr->slaves, 40I, ENEMY_IMMUNE, YoumuOppositeMyon, youmu_opposite_myon, 0, 0, 0, 0);
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
	
	float angle = creal(p->args[2]);
	float factor = (1-abs(global.plr.focus)/30.0) * !!angle;
	complex dir = -cexp(I*((angle+0.025*sin(global.frames/50.0)*(angle > 0? 1 : -1))*factor + M_PI/2));
	p->args[0] = 20*dir;
	linear(p, t);
	
	p->pos = ((Enemy *)REF(p->args[1]))->pos + p->pos;
	
	return 1;
}

int marisa_laser_slave(Enemy *e, int t) {
	if(global.plr.fire && global.frames - global.plr.recovery >= 0 && global.plr.deathtime >= -1) {
		if(!(global.frames % 4))
			create_projectile_p(&global.projs, get_tex("proj/marilaser"), 0, NULL, MariLaser, mari_laser, 0, add_ref(e),e->args[2],0)->type = PlrProj+e->args[1]*4;
		
		if(!(global.frames % 3)) {
			float s = 0.5 + 0.3*sin(global.frames/7.0);
			create_particle3c("marilaser_part0", 0, rgb(1-s,0.5,s), PartDraw, mari_laser, 0, add_ref(e), e->args[2]);
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
	if(t == EVENT_BIRTH) {
		global.shake_view = 8;
		return 1;
	}
	
	if(t > creal(e->args[0])) {
		global.shake_view = 0;
		return ACTION_DESTROY;
	}
		
	return 1;
}

// Star Sign

void MariStar(Projectile *p, int t) {
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	glRotatef(t*10, 0, 0, 1);
	draw_texture_p(0, 0, p->tex);
	glPopMatrix();
}

void MariStarBomb(Projectile *p, int t) {
	MariStar(p, t);
	
	create_particle1c("maristar_orbit", p->pos, NULL, GrowFade, timeout, 40);
}

int marisa_star_slave(Enemy *e, int t) {
	if(global.plr.fire && global.frames - global.plr.recovery >= 0 && global.plr.deathtime >= -1) {
		if(!(global.frames % 20))
			create_projectile_p(&global.projs, get_tex("proj/maristar"), e->pos, NULL, MariStar, accelerated, e->args[1], e->args[2], 0, 0)->type = PlrProj+e->args[3]*20;
	}
	
	e->pos = global.plr.pos + (1 - abs(global.plr.focus)/30.0)*e->pos0 + (abs(global.plr.focus)/30.0)*e->args[0];
	
	return 1;
}

int marisa_star_orbit(Projectile *p, int t) { // a[0]: x' a[1]: x''
	if(t == 0)
		p->pos0 = global.plr.pos;
	if(t < 0)
		return 1;
	
	if(t > 400)
		return ACTION_DESTROY;
	
	float r = cabs(p->pos0 - p->pos);
	
	p->args[1] = (1e5-t*t)*cexp(I*carg(p->pos0 - p->pos))/(r*r);
	p->args[0] += p->args[1]*0.2;
	p->pos += p->args[0];
	
	return 1;
}
	

// Generic Marisa

void marisa_shot(Player *plr) {
	if(plr->fire) {
		if(!(global.frames % 4))
			play_sound("generic_shot");
		
		if(!(global.frames % 6)) {
			create_projectile1c("marisa", plr->pos + 10 - 15I, NULL, linear, -20I)->type = PlrProj+150;
			create_projectile1c("marisa", plr->pos - 10 - 15I, NULL, linear, -20I)->type = PlrProj+150;
		}
	}
}

void marisa_bomb(Player *plr) {
	int i;
	float r, phi;
	switch(plr->shot) {
	case MarisaLaser:
		play_sound("masterspark");
		create_enemy_p(&plr->slaves, 40I, ENEMY_BOMB, MasterSpark, master_spark, 280,0,0,0);
		break;
	case MarisaStar:
		for(i = 0; i < 20; i++) {
			r = frand()*50 + 100;
			phi = frand()*2*M_PI;		
			create_particle1c("maristar_orbit", plr->pos + r*cexp(I*phi), NULL, MariStarBomb, marisa_star_orbit, I*r*cexp(I*(phi+frand()*0.5))/10);
		}		
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
			create_enemy_p(&plr->slaves, -40I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, -40I, 5, 0, 0);
		
		if(npow >= 2) {
			create_enemy_p(&plr->slaves, 25-5I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, 8-40I, 5,    M_PI/30, 0);
			create_enemy_p(&plr->slaves, -25-5I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, -8-40I, 5, -M_PI/30, 0);
		}
		
		if((int)npow == 3)
			create_enemy_p(&plr->slaves, -30I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, -50I, 5, 0, 0);
		
		if(npow >= 4) {
			create_enemy_p(&plr->slaves, 17-30I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, 4-45I, 5, 0, 0);
			create_enemy_p(&plr->slaves, -17-30I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, -4-45I, 5, 0, 0);
		}
		break;
	case MarisaStar:
		if((int)npow == 1)
			create_enemy_p(&plr->slaves, 40I, ENEMY_IMMUNE, MariLaserSlave, marisa_star_slave, -30I, -2I, -0.1I, 5);
		
		if(npow >= 2) {
			create_enemy_p(&plr->slaves, 30I+15, ENEMY_IMMUNE, MariLaserSlave, marisa_star_slave, -30I+10, -2I, -0.1I, 5);
			create_enemy_p(&plr->slaves, 30I-15, ENEMY_IMMUNE, MariLaserSlave, marisa_star_slave, -30I-10, -2I, -0.1I, 5);
		}
		
		if((int)npow == 3)
			create_enemy_p(&plr->slaves, -30I, ENEMY_IMMUNE, MariLaserSlave, marisa_star_slave, -30I, -2I, -0.1I, 5);
		if(npow >= 4) {
			create_enemy_p(&plr->slaves, 30, ENEMY_IMMUNE, MariLaserSlave, marisa_star_slave, 25, -1-2I, -0.1I, 5);
			create_enemy_p(&plr->slaves, -30, ENEMY_IMMUNE, MariLaserSlave, marisa_star_slave, -25, 1-2I, -0.1I, 5);
		}
		break;
	}
}

int plrmode_repr(char *out, size_t outsize, Character pchar, ShotMode pshot) {
	char *plr, sht;
	
	switch(pchar) {
		case Marisa		:	plr = "marisa"	;	break;
		case Youmu		:	plr = "youmu" 	;	break;
		default			:	plr = "wtf"		;	break;
	}
	
	switch(pshot) {
		case MarisaLaser:	sht = 'A'		;	break;
		case MarisaStar :	sht = 'B'		;	break;
	}
	
	return snprintf(out, outsize, "%s%c", plr, sht);
}
