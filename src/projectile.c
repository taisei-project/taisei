/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "projectile.h"

#include <stdlib.h>
#include <math.h>
#include "global.h"
#include "list.h"

Projectile *create_projectile(char *name, complex pos, Color clr,
							  ProjRule rule, complex args, ...) {
	Projectile *p = create_element((void **)&global.projs, sizeof(Projectile));
	
	char buf[128];
	strcpy(buf, "proj/");
	strcat(buf, name);
	
	p->birthtime = global.frames;
	p->pos = pos;
	p->pos0 = pos;
	p->angle = 0;
	p->rule = rule;
	p->tex = get_tex(buf);
	p->type = FairyProj;
	p->clr = clr;
	
	va_list ap;
	int i;
	
	va_start(ap, args);
	for(i = 0; i < 4 && args; i++) {
		p->args[i++] = args;
		args = va_arg(ap, complex);
	}
	va_end(ap);
	
	return p;
}

void delete_projectile(Projectile *proj) {
	delete_element((void **)&global.projs, proj);
}

void free_projectiles() {
	delete_all_elements((void **)&global.projs);
}

int test_collision(Projectile *p) {	
	if(p->type == FairyProj) {
		float angle = carg(global.plr.pos - p->pos);
		int projr = sqrt(pow(p->tex->w/4*cos(angle),2)*8/10 + pow(p->tex->h/2*sin(angle)*8/10,2));		
		
		if(cabs(global.plr.pos - p->pos) < projr + 1)
			return 1;
	} else {
		Enemy *e = global.enemies;
		while(e != NULL) {
			if(e->hp != ENEMY_IMMUNE && cabs(e->pos - p->pos) < 15) {
				global.points += 100;
				e->hp--;
				play_sound("hit");
				return 2;
			}
			e = e->next;
		}
		
		if(global.boss != NULL && cabs(global.boss->pos - p->pos) < 15) {
			global.boss->dmg++;
			play_sound("hit");
			return 2;
		}
	}
	return 0;
}

void draw_projectiles() {
	glEnable(GL_TEXTURE_2D);	
	
	Projectile *proj;
	Texture *tex;
	float wq, hq;
	
	for(proj = global.projs; proj; proj = proj->next) {
		tex = proj->tex;
		
		glBindTexture(GL_TEXTURE_2D, tex->gltex);
	
		wq = ((float)tex->w/2.0)/tex->truew;
		hq = ((float)tex->h)/tex->trueh;
		
		glPushMatrix();
		glTranslatef(creal(proj->pos), cimag(proj->pos), 0);
		glRotatef(proj->angle*180/M_PI+90, 0, 0, 1);
		glScalef(tex->w/4.0, tex->h/2.0,0);
		
		glBegin(GL_QUADS);
			glTexCoord2f(0,0); glVertex2f(-1, -1);
			glTexCoord2f(0,hq); glVertex2f(-1, 1);
			glTexCoord2f(wq,hq); glVertex2f(1, 1);
			glTexCoord2f(wq,0);	glVertex2f(1, -1);
		glEnd();
		
		glColor3fv((float *)&proj->clr);
		
		glBegin(GL_QUADS);
			glTexCoord2f(wq,0); glVertex2f(-1, -1);
			glTexCoord2f(wq,hq); glVertex2f(-1, 1);
			glTexCoord2f(2*wq,hq); glVertex2f(1, 1);
			glTexCoord2f(2*wq,0); glVertex2f(1, -1);
		glEnd();
		
		glPopMatrix();
		glColor3f(1,1,1);		
	}
	
	
	glDisable(GL_TEXTURE_2D);
}

void process_projectiles() {
	Projectile *proj = global.projs, *del = NULL;
	while(proj != NULL) {
		proj->rule(&proj->pos, proj->pos0, &proj->angle, global.frames - proj->birthtime, proj->args);
		
		int v = test_collision(proj);
		if(v == 1 && (global.frames - abs(global.plr.recovery)) >= 0)
			plr_death(&global.plr);
		
		if(v || creal(proj->pos) + proj->tex->w/2 < 0 || creal(proj->pos) - proj->tex->w/2 > VIEWPORT_W
			 || cimag(proj->pos) + proj->tex->h/2 < 0 || cimag(proj->pos) - proj->tex->h/2 > VIEWPORT_H) {
			del = proj;
			proj = proj->next;
			delete_projectile(del);
			if(proj == NULL) break;
		} else {
			proj = proj->next;
		}
	}
}

void linear(complex *pos, complex pos0, float *angle, int time, complex* a) { // sure is physics in here; a[0]: velocity	
	*angle = carg(a[0]);
	*pos = pos0 + a[0]*time;
}
