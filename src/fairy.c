/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "fairy.h"
#include "global.h"
#include "list.h"

void create_fairy(complex pos, int hp, FairyRule rule, complex args, ...) {
	Fairy *f = create_element((void **)&global.fairies, sizeof(Fairy));
	
	f->pos0 = pos;
	f->pos = pos;
	f->rule = rule;
	f->birthtime = global.frames;
	f->moving = 0;
	f->hp = hp;
	f->dir = 0;
	
	f->ani = &global.textures.fairy;
	
	va_list ap;
	int i;
	
	va_start(ap, args);
	for(i = 0; i < 4 && args; i++) {
		f->args[i] = args;
		args = va_arg(ap, complex);
	}
	va_end(ap);
}

void delete_fairy(Fairy *fairy) {
	if(global.plr.power < 6 && fairy->hp <= 0)
		create_poweritem(fairy->pos, 6-15*I);
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
				
		glTranslatef(creal(f->pos),cimag(f->pos),0);
		glRotatef(global.frames*10,0,0,1);
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
		glTranslatef(creal(f->pos),cimag(f->pos),0);
				
		if(f->dir) {
			glCullFace(GL_FRONT);
			glScalef(-1,1,1);
		}
		draw_animation(0, 0, f->moving, f->ani);
		
		glCullFace(GL_BACK);
		glPopMatrix();
	}
	
	glColor3f(1,1,1);
	glDisable(GL_TEXTURE_2D);
}

void free_fairies() {
	delete_all_elements((void **)&global.fairies);
}

void process_fairies() {
	Fairy *fairy = global.fairies, *del = NULL;
	
	while(fairy != NULL) {
		fairy->rule(fairy);
		
		if(creal(fairy->pos) < -20 || creal(fairy->pos) > VIEWPORT_W + 20
			|| cimag(fairy->pos) < -20 || cimag(fairy->pos) > VIEWPORT_H + 20 || fairy->hp <= 0) {
			del = fairy;
			fairy = fairy->next;
			delete_fairy(del);
		} else {
			fairy = fairy->next;
		}
	}
}