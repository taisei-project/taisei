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
		create_enemy_p(&plr->slaves, plr->pos, ENEMY_IMMUNE, youmu_opposite_draw, youmu_opposite_logic, add_ref(plr), 0, 0, 0);
}

void youmu_opposite_draw(Enemy *e, int t) {
	complex pos = e->pos + ((Player *)REF(e->args[0]))->pos;
	
	create_particle2c("flare", pos, NULL, Shrink, timeout, 10, -e->pos+10I);
}

void youmu_opposite_logic(Enemy *e, int t) {
	if(t == EVENT_DEATH)
		free_ref(e->args[0]);
	if(t < 0)
		return;
	
	Player *plr = (Player *)REF(e->args[0]);
	
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

/* Marisa */

int mari_laser(Projectile *p, int t) {
	linear(p, t);
	
	Player *plr = (Player *)REF(p->args[1]);
	
	p->pos = plr->pos + p->pos - creal(p->pos0)*abs(plr->focus)/30.0;
}

void marisa_shot(Player *plr) {
	if(plr->fire) {
		if(!(global.frames % 4)) {
			create_projectile2c("marisa", +10, NULL, mari_laser, -20I, add_ref(plr))->type = PlrProj;
			create_projectile2c("marisa", -10, NULL, mari_laser, -20I, add_ref(plr))->type = PlrProj;
		}
	}
}