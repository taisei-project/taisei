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
		create_projectile("youmu", e->pos + plr->pos, ((Color){1,1,1}), linear, -20*cexp(I*e->args[1]))->type = PlrProj; 
	
	e->pos0 = e->pos + plr->pos;	
}