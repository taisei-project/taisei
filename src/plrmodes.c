/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "plrmodes.h"
#include "player.h"
#include "global.h"

void youmu_opposite_draw(Enemy *e, int t) {
	complex pos = e->pos + ((Player *)e->parent)->pos;
	draw_texture(creal(pos), cimag(pos), "items/power");
}

void youmu_opposite_logic(Enemy *e, int t) {
	if(t < 0)
		return;
	
	Player *plr = (Player *)e->parent;
	
	if(plr->focus < 15) {
		e->args[1] = carg(plr->pos - e->pos0);
		e->pos = e->pos0 - plr->pos;
		
		if(cabs(e->pos) > 30)
			e->pos -= 5*cexp(I*carg(e->pos));
	}
	
	if(plr->fire && !(global.frames % 4))
		create_projectile("youmu", e->pos + plr->pos, NULL, linear, -20*cexp(I*e->args[1]))->type = PlrProj; 
	
	e->pos0 = e->pos + plr->pos;	
}

void youmu_homing(complex *pos, complex *pos0, float *angle, int t, complex* a) { // a[0]: velocity
 	*angle = t/30.0;
	
	complex tv = 0;
	
	if(global.enemies != NULL)
		tv = cexp(I*carg(global.enemies[0].pos - *pos));;
	if(global.boss != NULL)
		tv = cexp(I*carg(global.boss->pos - *pos));;

	if(t > 150 || tv == 0)
		tv = - ((rand()%3)-1)/2.0 - 0.5I;
	
	complex s0 = 0;

	*pos += a[0]*log(t + 1) + tv*t/15.0;
	*pos0 = *pos;
	
}