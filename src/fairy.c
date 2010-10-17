/*
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA  02110-1301, USA.
 
 ---
 Copyright (C) 2010, Lukas Weber <laochailan@web.de>
 */

#include "fairy.h"
#include "global.h"

void create_fairy(int x, int y, int v, int angle, int hp, FairyRule rule) {
	Fairy *fairy, *last = global.fairies;
	
	fairy = malloc(sizeof(Fairy));
	
	if(last != NULL) {
		while(last->next != NULL)
			last = last->next;
		last->next = fairy;
	} else {
		global.fairies = fairy;
	}
	
	*fairy = ((Fairy) { .prev = last,
					   .next = NULL,
					   .sx = x,
					   .sy = y,
					   .x = x,
					   .y = y,
					   .v = v,
					   .angle = angle,
					   .rule = rule,
					   .birthtime = global.frames,
					   .moving = 0,
					   .hp = hp,
					   .dir = 0 
	});
	
	init_animation(&fairy->ani, 2, 4, 2, FILE_PREFIX "gfx/fairy.png"); 
}

void delete_fairy(Fairy *fairy) {
	if(fairy->prev != NULL)
		fairy->prev->next = fairy->next;
	if(fairy->next != NULL)
		fairy->next->prev = fairy->prev;	
	if(global.fairies == fairy)
		global.fairies = fairy->next;
	
	free(fairy);
}

void draw_fairy(Fairy *f) {
	draw_animation(f->x, f->y, f->moving, &f->ani);
}

void free_fairies() {
	Fairy *fairy = global.fairies;
	Fairy *tmp;
	
	while(fairy != 0) {
		tmp = fairy;
		fairy = fairy->next;
		delete_fairy(tmp);
	}
	
	global.fairies = NULL;
}

void process_fairies() {
	Fairy *fairy = global.fairies, *del = NULL;
	while(fairy != NULL) {
		fairy->rule(fairy);
		
		if(fairy->x < 0 || fairy->x > VIEWPORT_W || fairy->y < 0 || fairy->y > VIEWPORT_H || fairy->hp <= 0)
			del = fairy;
		fairy = fairy->next;
	}
	
	if(del)
		delete_fairy(del);
}