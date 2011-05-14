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

Projectile *create_particle(char *name, complex pos, Color *clr, ProjDRule draw, ProjRule rule, complex arg1, ...) {
	va_list ap;
	complex a[4];	
	int i;
	
	memset(a, 0, sizeof(a));
	
	va_start(ap, arg1);
	
	a[0] = arg1;
	for(i = 1; i < 4; i++) {
		a[i] = (complex)va_arg(ap, complex);
	}
	va_end(ap);
	
	return create_projectile_dv(&global.particles, name, pos, clr, draw, rule, a);
}

Projectile *create_projectile(char *name, complex pos, Color *clr, ProjRule rule, complex arg1, ...) {
	va_list ap;
	complex a[4], ar;
	int i;
	
	memset(a, 0, sizeof(a));
		
	va_start(ap, arg1);
	
	a[0] = arg1;
	for(i = 1; i < 4; i++) {
		a[i] = (complex)va_arg(ap, complex);
	}
	va_end(ap);
		
	return create_projectile_dv(&global.projs, name, pos, clr, ProjDraw, rule, a);
}

Projectile *create_projectile_dv(Projectile **dest, char *name, complex pos, Color *clr,
							    ProjDRule draw, ProjRule rule, complex *args) {
	Projectile *p = create_element((void **)dest, sizeof(Projectile));
	
	char buf[128];
	if(name[0] == '!')
		strncpy(buf, name+1, sizeof(buf));
	else {
		strcpy(buf, "proj/");
		strcat(buf, name);
	}
	
	p->birthtime = global.frames;
	p->pos = pos;
	p->pos0 = pos;
	p->angle = 0;
	p->rule = rule;
	p->draw = draw;
	p->tex = get_tex(buf);
	p->type = FairyProj;
	p->clr = clr;
	p->parent = NULL;
		
	memcpy(p->args, args, sizeof(p->args));
	
	
	if(dest != &global.particles && clr != NULL) {
		Color *color = rgba(clr->r, clr->g, clr->b, clr->a);
		
		create_particle(name, pos, color, Shrink, bullet_flare_move, 16, (complex)add_ref(p));
	}
	return p;
}

void _delete_projectile(void **projs, void *proj) {
	((Projectile *)proj)->rule(proj, EVENT_DEATH);
	
	if(((Projectile*)proj)->clr)
		free(((Projectile*)proj)->clr);	
	del_ref(proj);
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
	
	for(proj = projs; proj; proj = proj->next)	
		proj->draw(proj, global.frames - proj->birthtime);
}

void process_projectiles(Projectile **projs, char collision) {
	Projectile *proj = *projs, *del = NULL;
	
	char killed = 0;
	char col = 0;
	int action;
	while(proj != NULL) {
		action = proj->rule(proj, global.frames - proj->birthtime);
		
		if(proj->type == DeadProj && killed < 2) {
			killed++;
			action = ACTION_DESTROY;
			create_particle("!flare", proj->pos, NULL, Fade, timeout, (complex)40);
			create_item(proj->pos, 0, BPoint)->auto_collect = 10;
		}
			
		if(collision)
			col = collision_projectile(proj);
		
		if(col == 1 && (global.frames - abs(global.plr.recovery)) >= 0)
			plr_death(&global.plr);
		
		if(action == ACTION_DESTROY || col
		|| creal(proj->pos) + proj->tex->w/2 < 0 || creal(proj->pos) - proj->tex->w/2 > VIEWPORT_W
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

int linear(Projectile *p, int t) { // sure is physics in here; a[0]: velocity	
	p->angle = carg(p->args[0]);
	p->pos = p->pos0 + p->args[0]*t;
	
	return 1;
}

void ProjDraw(Projectile *proj, int t) {
	if(proj->clr != NULL) {
		GLuint shader = get_shader("bullet_color");
		glUseProgramObjectARB(shader);
		
		glUniform4fv(glGetUniformLocation(shader, "color"), 1, (GLfloat *)proj->clr);
	}
	
	glPushMatrix();
	glTranslatef(creal(proj->pos), cimag(proj->pos), 0);
	glRotatef(proj->angle*180/M_PI+90, 0, 0, 1);

	draw_texture_p(0,0, proj->tex);
	glPopMatrix();
	
	glUseProgramObjectARB(0);
}

void Shrink(Projectile *p, int t) {	
	if(p->clr != NULL) {
		GLuint shader = get_shader("bullet_color");
		glUseProgramObjectARB(shader);
		glUniform4fv(glGetUniformLocation(shader, "color"), 1, (GLfloat *)p->clr);
	}
	
	glPushMatrix();
	float s = 2.0-t/p->args[0]*2;
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	glRotatef(p->angle, 0, 0, 1);
	glScalef(s, s, 1);
	draw_texture_p(0, 0, p->tex);
	glPopMatrix();
	
	glUseProgramObjectARB(0);
}

int bullet_flare_move(Projectile *p, int t) {
	int i;

	if(t > 16 || REF(p->args[1]) == NULL) {
		free_ref(p->args[1]);
		return ACTION_DESTROY;
	}
	
	
	p->pos = ((Projectile *) REF(p->args[1]))->pos;
	p->angle = ((Projectile *) REF(p->args[1]))->angle;
		
	return 1;
}

void Fade(Projectile *p, int t) {
	glColor4f(1,1,1, 1.0 - (float)t/p->args[0]);
	draw_texture_p(creal(p->pos), cimag(p->pos), p->tex);
	glColor4f(1,1,1,1);
}

int timeout(Projectile *p, int t) {
	if(t >= creal(p->args[0]))
		return ACTION_DESTROY;
	
	return 1;
}