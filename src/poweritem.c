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

void create_poweritem(int x, int y, float acc, int angle, ItemRule rule) {
	Poweritem *p = create_element((void **)&global.poweritems, sizeof(Poweritem));
	p->x = x;
	p->y = y;
	p->sy = y;
	p->acc = acc;
	p->velo = -3;
	p->angle = angle;
	p->birthtime = global.frames;
	p->tex = &global.textures.poweritems;
	p->rule = rule;
}


void delete_poweritem(Poweritem *poweritem) {
	delete_element((void **)&global.poweritems, poweritem);
}

void draw_poweritems() {
	Poweritem *p;
	Texture *tex = &global.textures.poweritems;

	for(p = global.poweritems; p; p = p->next){
		draw_texture(p->x, p->y, &global.textures.poweritems);
	}
}

void free_poweritems() {
	delete_all_elements((void **)&global.poweritems);
}

void simpleItem(int *y,int sy, float acc, long t, float *velo) {
	if(*velo < 3 ) *velo += acc;
	*y = sy + (*velo * t);
}

void process_poweritems() {
	Poweritem *poweritem = global.poweritems, *del = NULL;
        int v;
	while(poweritem != NULL) {
		poweritem->rule(&poweritem->y, poweritem->sy, poweritem->acc, global.frames - poweritem->birthtime, &poweritem->velo);
		v = collision(poweritem);
		if(v == 1) {
			global.plr.power += 0.1;
		}
		if(v == 1 || poweritem->x < -20 || poweritem->x > VIEWPORT_W + 20 || poweritem->y < -20 || poweritem->y > VIEWPORT_H + 20) {
			del = poweritem;
			poweritem = poweritem->next;
			delete_poweritem(del);
		} else {
			poweritem = poweritem->next;
		}
	}
}

int collision(Poweritem *p) {
	float angle = atan((float)(global.plr.y - p->y)/(global.plr.x - p->x));
	
	if(sqrt(pow(p->x-global.plr.x,2) + pow(p->y-global.plr.y,2)) < 10)
		return 1;
	
	return 0;
}
