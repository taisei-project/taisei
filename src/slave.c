/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "slave.h"

#include <stdlib.h>
#include <math.h>
#include "global.h"
#include "projectile.h"
#include "list.h"

void create_slave(complex pos, complex pos0) {
	Slave *s = (Slave *)create_element((void **)&global.slaves, sizeof(Slave));
	
	s->birthtime = global.frames;
	s->pos = pos0;
	s->pos0 = global.plr.pos + pos0;
	s->angle = 0;	
}

void delete_slave(Slave *slave) {
	delete_element((void **)&global.slaves, slave);
}

void free_slaves() {
	delete_all_elements((void **)&global.slaves);
}

void draw_slaves() {
	Slave *s;
	Texture *tex = get_tex("items/power");
	for(s = global.slaves; s; s = s->next)
		draw_texture_p(creal(s->pos + global.plr.pos), cimag(s->pos + global.plr.pos), tex);
}

void process_slaves() {
    Slave *slave;	
	for(slave = global.slaves; slave; slave = slave->next) {
		if(global.plr.focus < 15) {
			slave->angle = carg(global.plr.pos - slave->pos0);
			slave->pos = slave->pos0 - global.plr.pos;
	
			if(cabs(slave->pos) > 50)
				slave->pos -= 5*cexp(I*carg(slave->pos));
		}
		
		if(global.plr.fire && !(global.frames % 4))
			create_projectile("youmu", slave->pos + global.plr.pos, ((Color){1,1,1}), linear, -20*cexp(I*slave->angle))->type = PlrProj; 
		
		slave->pos0 = slave->pos + global.plr.pos;
	}
}

