/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "fairy.h"
#include "global.h"
#include "list.h"

void create_fairy(int x, int y, int v, int angle, int hp, FairyRule rule) {
	Fairy *f = create_element((void **)&global.fairies, sizeof(Fairy));
	
	f->sx = x;
	f->sy = y;
	f->x = x;
	f->y = y;
	f->v = v;
	f->angle = angle;
	f->rule = rule;
	f->birthtime = global.frames;
	f->moving = 0;
	f->hp = hp;
	f->dir = 0;
	
	f->ani = &global.textures.fairy;
}

void delete_fairy(Fairy *fairy) {
	delete_element((void **)&global.fairies, fairy);
}

void draw_fairy(Fairy *f) {
	glPushMatrix();
		glTranslatef(f->x, f->y,0);
		glPushMatrix();
			float s = sin((float)global.frames/10.0f)/4.0f+1.3f;
			glScalef(s, s, s);
			glRotatef(global.frames*10,0,0,1);
			glColor3f(0.5,0.8,1);
			draw_texture(0, 0, &global.textures.fairy_circle);
			glColor3f(1,1,1);
		glPopMatrix();
		glPushMatrix();
			glDisable(GL_CULL_FACE);
			
			if(f->dir)
				glScalef(-1,1,1);
			draw_animation(0, 0, f->moving, f->ani);
			
			glEnable(GL_CULL_FACE);
		glPopMatrix();
	glPopMatrix();
}

void free_fairies() {
	delete_all_elements((void **)&global.fairies);
}

void process_fairies() {
	Fairy *fairy = global.fairies, *del = NULL;
	
	fairy = global.fairies;
	while(fairy != NULL) {
		fairy->rule(fairy);
		
		if(fairy->x < -20 || fairy->x > VIEWPORT_W + 20 || fairy->y < -20 || fairy->y > VIEWPORT_H + 20 || fairy->hp <= 0) {
			del = fairy;
			fairy = fairy->next;
			delete_fairy(del);
		} else {
			fairy = fairy->next;
		}
	}
}