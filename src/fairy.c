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
#include "list.h"

void create_fairy(int x, int y, int v, int angle, int hp, FairyRule rule) {
	Fairy *fairy = create_element((void **)&global.fairies, sizeof(Fairy));
	
	*fairy = ((Fairy) {
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
	
	fairy->ani = &global.textures.fairy;
}

void delete_fairy(Fairy *fairy) {
	delete_element((void **)&global.fairies, fairy);
}

void draw_fairy(Fairy *f) {
	glPushMatrix();
		float s = sin((float)global.frames/10.0f)/4.0f+1.3f;
		glTranslatef(f->x, f->y,0);
		glScalef(s, s, s);
		glRotatef(global.frames*10,0,0,1);
		glColor3f(0.2,0.5,1);
		draw_texture(0, 0, &global.textures.fairy_circle);
		glColor3f(1,1,1);
	glPopMatrix();
	draw_animation(f->x, f->y, f->moving, f->ani);
}

void free_fairies() {
	delete_all_elements((void **)&global.fairies);
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