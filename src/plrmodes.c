/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "plrmodes.h"
#include "player.h"
#include "global.h"

void youmu_opposite_draw(Slave *s) {
	complex pos = s->pos + ((Player *)s->parent)->pos;
	draw_texture(creal(pos), cimag(pos), "items/power");
}

void youmu_opposite_logic(Slave *slave) {
	Player *plr = (Player *)slave->parent;
	
	if(plr->focus < 15) {
		slave->args[1] = carg(plr->pos - slave->pos0);
		slave->pos = slave->pos0 - plr->pos;
		
		if(cabs(slave->pos) > 50)
			slave->pos -= 5*cexp(I*carg(slave->pos));
	}
	
	if(plr->fire && !(global.frames % 4))
		create_projectile("youmu", slave->pos + plr->pos, ((Color){1,1,1}), linear, -20*cexp(I*slave->args[1]))->type = PlrProj; 
	
	slave->pos0 = slave->pos + plr->pos;	
}