/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Code by Juergen Kieslich
 */

#include "poweritem.h"
#include "global.h"
#include "list.h"

void create_poweritem(complex pos, complex v) {
	Poweritem *p = create_element((void **)&global.poweritems, sizeof(Poweritem));
	p->pos = pos;
	p->pos0 = pos;
	p->v = v;
	p->birthtime = global.frames;
}


void delete_poweritem(Poweritem *poweritem) {
	delete_element((void **)&global.poweritems, poweritem);
}

void draw_poweritems() {
	Poweritem *p;
	for(p = global.poweritems; p; p = p->next)
		draw_texture(creal(p->pos), cimag(p->pos), &global.textures.poweritems);
}

void free_poweritems() {
	delete_all_elements((void **)&global.poweritems);
}

void move_poweritem(Poweritem *p) {
	int t = global.frames - p->birthtime;
	int lim = 3;
	// v(t) = creal(p->v)/(t/15 + 1) + I*((cimag(p->v)+lim)/(t + 1) + lim)
	// calculus power!
	
	p->pos = p->pos0 + creal(p->v)*log(t/5.0 + 1)*5 + I*((cimag(p->v)+lim)*log(t/5.0+1)*5 + lim*t);
}

void process_poweritems() {
	Poweritem *poweritem = global.poweritems, *del = NULL;
    int v;
	while(poweritem != NULL) {
		move_poweritem(poweritem);
		
		v = collision(poweritem);
		if(v == 1) {
			global.plr.power += 0.1;
		}
		if(v == 1 || creal(poweritem->pos) < -5 || creal(poweritem->pos) > VIEWPORT_W + 5
			|| cimag(poweritem->pos) < -20 || cimag(poweritem->pos) > VIEWPORT_H + 20) {
			del = poweritem;
			poweritem = poweritem->next;
			delete_poweritem(del);
		} else {
			poweritem = poweritem->next;
		}
	}
}

int collision(Poweritem *p) {
	if(cabs(global.plr.pos - p->pos) < 15)
		return 1;
	
	return 0;
}
