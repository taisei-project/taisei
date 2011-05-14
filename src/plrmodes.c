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
	
	create_particle("!flare", pos, NULL, Shrink, timeout, (complex)10, -e->pos+10I, 1);
}

void youmu_opposite_logic(Enemy *e, int t) {
	if(t < 0)
		return;
	
	Player *plr = (Player *)e->parent;
	
	if(plr->focus < 15) {
		e->args[2] = carg(plr->pos - e->pos0);
		e->pos = e->pos0 - plr->pos;
		
		if(cabs(e->pos) > 30)
			e->pos -= 5*cexp(I*carg(e->pos));
	}
	
	if(plr->fire && !(global.frames % 4))
		create_projectile("youmu", e->pos + plr->pos, NULL, linear, -20*cexp(I*e->args[2]))->type = PlrProj; 
	
	e->pos0 = e->pos + plr->pos;	
}

int youmu_homing(Projectile *p, int t) { // a[0]: velocity, a[1]: target, a[2]: old velocity
//  	p->angle = t/30.0;
	if(t == EVENT_DEATH) {
		free_ref(p->args[1]);
	}
	
	complex tv = p->args[2];
	if(REF(p->args[1]) != NULL)
		tv = cexp(I*carg(((Enemy *)REF(p->args[1]))->pos - p->pos));
	
		
	p->args[2] = tv;
	
	p->pos += p->args[0]*log(t + 1) + tv*t*t/300.0;
	p->pos0 = p->pos;
	
	return 1;
}