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

Projectile *create_particle4c(char *name, complex pos, Color *clr, ProjDRule draw, ProjRule rule, complex a1, complex a2, complex a3, complex a4) {
	Projectile *p = create_projectile_p(&global.particles, prefix_get_tex(name, "part/"), pos, clr, draw, rule, a1, a2, a3, a4);
	
	p->type = Particle;
	return p;
}

Projectile *create_projectile4c(char *name, complex pos, Color *clr, ProjRule rule, complex a1, complex a2, complex a3, complex a4) {
	return create_projectile_p(&global.projs, prefix_get_tex(name, "proj/"), pos, clr, ProjDraw, rule, a1, a2, a3, a4);
}

Projectile *create_projectile_p(Projectile **dest, Texture *tex, complex pos, Color *clr,
							    ProjDRule draw, ProjRule rule, complex a1, complex a2, complex a3, complex a4) {
	Projectile *p = create_element((void **)dest, sizeof(Projectile));
	
	p->birthtime = global.frames;
	p->pos = pos;
	p->pos0 = pos;
	p->angle = M_PI/2;
	p->rule = rule;
	p->draw = draw;
	p->tex = tex;
	p->type = FairyProj;
	p->clr = clr;
	
	p->args[0] = a1;
	p->args[1] = a2;
	p->args[2] = a3;
	p->args[3] = a4;
	
	if(dest != &global.particles && clr != NULL) {
		Color *color = rgba(clr->r, clr->g, clr->b, clr->a);
		
		create_projectile_p(&global.particles, tex, pos, color, Shrink, bullet_flare_move, 16, add_ref(p), 0, 0);
	}
	return p;
}

void _delete_projectile(void **projs, void *proj) {
	Projectile *p = proj;
	p->rule(p, EVENT_DEATH);
		
	if(p->clr)
		free(p->clr);
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
		float angle = carg(global.plr.pos - p->pos) + p->angle;
		int projr = sqrt(pow(p->tex->w/4*cos(angle),2)*8/10 + pow(p->tex->h/2*sin(angle)*8/10,2));		
		
		if(cabs(global.plr.pos - p->pos) < projr + 1)
			return 1;
	} else if(p->type >= PlrProj) {
		Enemy *e = global.enemies;
		while(e != NULL) {
			if(e->hp != ENEMY_IMMUNE && cabs(e->pos - p->pos) < 15) {
				global.points += 100;
				e->hp -= p->type - PlrProj + 1;
				return 2;
			}
			e = e->next;
		}
		
		if(global.boss != NULL && cabs(global.boss->pos - p->pos) < 15) {
			global.boss->dmg+= p->type - PlrProj + 1;
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
			create_particle1c("flare", proj->pos, NULL, Fade, timeout, 30);
			create_item(proj->pos, 0, BPoint)->auto_collect = 10;
		}
			
		if(collision)
			col = collision_projectile(proj);
		
		if(col && proj->type != Particle) {
			Color *clr = NULL;
			if(proj->clr) {
				clr = malloc(sizeof(Color));
				memcpy(clr, proj->clr, sizeof(Color));
			}
			create_projectile_p(&global.particles, proj->tex, proj->pos, clr, DeathShrink, timeout_linear, 10, 5*cexp(proj->angle*I), 0, 0);
		}
				
		if(col == 1 && global.frames - abs(global.plr.recovery) >= 0)
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

int accelerated(Projectile *p, int t) {
	p->angle = carg(p->args[0]);
	p->pos = p->pos0 + p->args[0]*t + p->args[1]*t*t;
	
	return 1;
}

int asymptotic(Projectile *p, int t) { // v = a[0]*(a[1] + 1); a[1] -> 0
	p->angle = carg(p->args[0] + p->args[1]);
	
	p->args[1] *= 0.8;
	p->pos += p->args[0]*(p->args[1] + 1);
	
	return 1;
}

void _ProjDraw(Projectile *proj, int t) {
	if(proj->clr != NULL && !tconfig.intval[NO_SHADER]) {
		GLuint shader = get_shader("bullet_color");
		glUseProgramObjectARB(shader);
		
		glUniform4fv(glGetUniformLocation(shader, "color"), 1, (GLfloat *)proj->clr);
	}
	if(proj->clr != NULL && tconfig.intval[NO_SHADER])
		glColor3f(0,0,0);
	
	draw_texture_p(0,0, proj->tex);
	
	glColor3f(1,1,1);
	
	if(!tconfig.intval[NO_SHADER])
		glUseProgramObjectARB(0);
}

void ProjDraw(Projectile *proj, int t) {
	glPushMatrix();
	glTranslatef(creal(proj->pos), cimag(proj->pos), 0);
	glRotatef(proj->angle*180/M_PI+90, 0, 0, 1);

	_ProjDraw(proj, t);
	
	glPopMatrix();
}

void PartDraw(Projectile *proj, int t) {
	glPushMatrix();
	glTranslatef(creal(proj->pos), cimag(proj->pos), 0);
	glRotatef(proj->angle*180/M_PI+90, 0, 0, 1);

	if(proj->clr)
		glColor4fv((float *)proj->clr);
	
	draw_texture_p(0,0, proj->tex);
	
	glPopMatrix();	
	glColor3f(1,1,1);
}
void Blast(Projectile *p, int t) {
	if(t == 1) {
		p->args[1] = frand()*360 + frand()*I;
		p->args[2] = frand() + frand()*I;
	}
	
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	glRotatef(creal(p->args[1]), cimag(p->args[1]), creal(p->args[2]), cimag(p->args[2]));
	glScalef(t/p->args[0], t/p->args[0], 1);
	glColor4f(0.3,0.6,1,1 - t/p->args[0]);
	draw_texture_p(0,0,p->tex);
	glPopMatrix();
	
	glColor4f(1,1,1,1);
}
	
void Shrink(Projectile *p, int t) {	
	glPushMatrix();
	float s = 2.0-t/p->args[0]*2;
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	glRotatef(p->angle*180/M_PI+90, 0, 0, 1);
	glScalef(s, s, 1);
	
	_ProjDraw(p, t);
	glPopMatrix();
}

void DeathShrink(Projectile *p, int t) {
	glPushMatrix();
	float s = 2.0-t/p->args[0]*2;
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	glRotatef(p->angle*180/M_PI+90, 0, 0, 1);
	glScalef(s, 1, 1);
		
	_ProjDraw(p, t);
	glPopMatrix();
}

void GrowFade(Projectile *p, int t) {
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	glRotatef(p->angle*180/M_PI+90, 0, 0, 1);
	glScalef(t/p->args[0]*(1+p->args[1]), t/p->args[0]*(1+p->args[1]), 1);
	
	if(p->clr)
		glColor4f(p->clr->r,p->clr->g,p->clr->b,1-t/p->args[0]);
	else
		glColor4f(1,1,1,1-t/p->args[0]);
	
	draw_texture_p(0,0,p->tex);
	
	glColor4f(1,1,1,1);
	glPopMatrix();
}		

int bullet_flare_move(Projectile *p, int t) {
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
	ProjDraw(p, t);
	glColor4f(1,1,1,1);
}

int timeout(Projectile *p, int t) {
	if(t >= creal(p->args[0]))
		return ACTION_DESTROY;
	
	return 1;
}

int timeout_linear(Projectile *p, int t) {
	if(t >= creal(p->args[0]))
		return ACTION_DESTROY;
	
	p->angle = carg(p->args[1]);
	p->pos = p->pos0 + p->args[1]*t;
	
	return 1;
}