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

void draw_fairies() {
	glEnable(GL_TEXTURE_2D);
	Texture *tex = &global.textures.fairy_circle;
	glBindTexture(GL_TEXTURE_2D, tex->gltex);
	
	Fairy *f;
	glPushMatrix();
	
	float s = sin((float)global.frames/10.f)/6 + 0.8;
	float wq = ((float)tex->w)/tex->truew;
	float hq = ((float)tex->h)/tex->trueh;
	
	glColor3f(0.2,0.7,1);
	
	for(f = global.fairies; f; f = f->next) {
		glEnable(GL_TEXTURE_2D);		
		glPushMatrix();
				
		glTranslatef(f->x,f->y,0);
		glRotatef(SDL_GetTicks()/2,0,0,1);
		glScalef(tex->w/2,tex->h/2,1);
		glScalef(s, s, s);
		
		glBegin(GL_QUADS);
			glTexCoord2f(0,0); glVertex3f(-1, -1, 0);
			glTexCoord2f(0,hq); glVertex3f(-1, 1, 0);
			glTexCoord2f(wq,hq); glVertex3f(1, 1, 0);
			glTexCoord2f(wq,0); glVertex3f(1, -1, 0);
		glEnd();	
		
		glPopMatrix();		
	}
	glPopMatrix();
	
	for(f = global.fairies; f; f = f->next) {
		glPushMatrix();
		glTranslatef(f->x,f->y,0);
				
		if(f->dir) {
			glCullFace(GL_FRONT);
			glScalef(-1,1,1);
		}
		draw_animation(0, 0, f->moving, f->ani);
		
		glCullFace(GL_BACK);
		glPopMatrix();
	}
	
	glDisable(GL_TEXTURE_2D);
}

void free_fairies() {
	delete_all_elements((void **)&global.fairies);
}

void process_fairies() {
	Fairy *fairy = global.fairies, *del = NULL;
	
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