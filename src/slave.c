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

void create_slave(Slave **slaves, SlaveRule logic_rule, SlaveRule draw_rule, complex pos, void *parent, complex args, ...) {
	Slave *s = (Slave *)create_element((void **)slaves, sizeof(Slave));
	
	s->birthtime = global.frames;
	s->pos = pos;
	s->pos0 = pos;
	
	s->parent = parent;
	s->logic_rule = logic_rule;
	s->draw_rule = draw_rule;
	
	va_list ap;
	int i;
	
	memset(s->args, 0, 4);
	va_start(ap, args);
	for(i = 0; i < 4 && args; i++) {
		s->args[i++] = args;
		args = va_arg(ap, complex);
	}
	va_end(ap);
}

void delete_slave(Slave **slaves, Slave* slave) {
	delete_element((void **)&slaves, slave);
}

void free_slaves(Slave **slaves) {
	delete_all_elements((void **)slaves);
}

void draw_slaves(Slave *slaves) {
	Slave *s;	
	for(s = slaves; s; s = s->next)
		s->draw_rule(s);
}

void process_slaves(Slave *slaves) {
    Slave *slave;	
	for(slave = slaves; slave; slave = slave->next)
		slave->logic_rule(slave);
}

