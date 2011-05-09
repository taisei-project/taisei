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

Color *rgba(float r, float g, float b, float a) {
	Color *clr = malloc(sizeof(Color));
	clr->r = r;
	clr->g = g;
	clr->b = b;
	clr->a = a;
	
	return clr;
}

inline Color *rgb(float r, float g, float b) {
	return rgba(r, g, b, 1.0);
}

Projectile *create_projectile_d(Projectile **dest, char *name, complex pos, Color *clr,
							    ProjRule rule, complex args, ...) {
	Projectile *p = create_element((void **)dest, sizeof(Projectile));
	
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
	p->parent = NULL;
	
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

void _delete_projectile(void **projs, void *proj) {
	free(((Projectile*)proj)->clr);	
	delete_element(projs, proj);
}

void delete_projectile(Projectile **projs, Projectile *proj) {
	_delete_projectile((void **)projs, proj);
}

void delete_projectiles(Projectile **projs) {
	delete_all_elements((void **)projs, _delete_projectile);
}

int collision_projectile(Projectile *p) {	
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
				return 2;
			}
			e = e->next;
		}
		
		if(global.boss != NULL && cabs(global.boss->pos - p->pos) < 15) {
			global.boss->dmg++;
			return 2;
		}
	}
	return 0;
}

void draw_projectiles(Projectile *projs) {	
	Projectile *proj;
	
	GLuint shader = get_shader("bullet_color");
	glUseProgramObjectARB(shader);
	
	for(proj = projs; proj; proj = proj->next) {				
		if(proj->clr == NULL)
			glUseProgramObjectARB(0);
		
		glUniform4fv(glGetUniformLocation(shader, "color"), 1, (GLfloat *)proj->clr);
				
		glPushMatrix();
		glTranslatef(creal(proj->pos), cimag(proj->pos), 0);
		glRotatef(proj->angle*180/M_PI+90, 0, 0, 1);

		draw_texture_p(0,0, proj->tex);
		glPopMatrix();
				
		if(proj->clr == NULL)
			glUseProgramObjectARB(shader);
	}
	glUseProgramObjectARB(0);
}

void process_projectiles(Projectile **projs, short collision) {
	Projectile *proj = *projs, *del = NULL;
	while(proj != NULL) {
		proj->rule(proj, global.frames - proj->birthtime);
		
		int v = 0;
		if(collision)
			v = collision_projectile(proj);
		
		if(v == 1 && (global.frames - abs(global.plr.recovery)) >= 0)
			plr_death(&global.plr);
		
		if(v || creal(proj->pos) + proj->tex->w/2 < 0 || creal(proj->pos) - proj->tex->w/2 > VIEWPORT_W
			 || cimag(proj->pos) + proj->tex->h/2 < 0 || cimag(proj->pos) - proj->tex->h/2 > VIEWPORT_H) {
			del = proj;
			proj = proj->next;
			delete_projectile(projs, del);
			if(proj == NULL) break;
		} else {
			proj = proj->next;
		}
	}
}

void linear(Projectile *p, int t) { // sure is physics in here; a[0]: velocity	
	p->angle = carg(p->args[0]);
	p->pos = p->pos0 + p->args[0]*t;
}
