/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "projectile.h"

#include <stdlib.h>
#include "global.h"
#include "list.h"
#include "vbo.h"

Projectile *create_particle4c(char *name, complex pos, Color clr, ProjDRule draw, ProjRule rule, complex a1, complex a2, complex a3, complex a4) {
	Projectile *p = create_projectile_p(&global.particles, prefix_get_tex(name, "part/"), pos, clr, draw, rule, a1, a2, a3, a4);

	p->type = Particle;
	return p;
}

Projectile *create_projectile4c(char *name, complex pos, Color clr, ProjRule rule, complex a1, complex a2, complex a3, complex a4) {
	return create_projectile_p(&global.projs, prefix_get_tex(name, "proj/"), pos, clr, ProjDraw, rule, a1, a2, a3, a4);
}

Projectile *create_projectile_p(Projectile **dest, Texture *tex, complex pos, Color clr,
							    ProjDRule draw, ProjRule rule, complex a1, complex a2, complex a3, complex a4) {
	Projectile *p, *e, **d;

	for(e = *dest; e && e->next; e = e->next)
		if(e->prev && tex->w*tex->h > e->tex->w*e->tex->h)
			break;

	if(e == NULL)
		d = dest;
	else
		d = &e;

	p = create_element((void **)d, sizeof(Projectile));

	p->birthtime = global.frames;
	p->pos = pos;
	p->pos0 = pos;
	p->angle = M_PI/2;
	p->rule = rule;
	p->draw = draw;
	p->tex = tex;
	p->type = FairyProj;
	p->clr = clr;
	p->grazed = 0;

	p->args[0] = a1;
	p->args[1] = a2;
	p->args[2] = a3;
	p->args[3] = a4;

	return p;
}

void _delete_projectile(void **projs, void *proj) {
	Projectile *p = proj;
	p->rule(p, EVENT_DEATH);

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
		double angle = carg(global.plr.pos - p->pos) + p->angle;
		double projr = sqrt(pow(p->tex->w/4*cos(angle),2)*5/10.0 + pow(p->tex->h/2*sin(angle)*5/10.0,2));
		double grazer = max(p->tex->w, p->tex->h);
		double dst = cabs(global.plr.pos - p->pos);
		grazer = (0.9 * sqrt(grazer) + 0.1 * grazer) * 6;

		if(dst < projr + 1)
			return 1;

		if(!p->grazed && dst < grazer && global.frames - abs(global.plr.recovery) > 0) {
			p->grazed = true;
			player_graze(&global.plr, p->pos - grazer * 0.3 * cexp(I*carg(p->pos - global.plr.pos)), 50);
		}
	} else if(p->type >= PlrProj) {
		Enemy *e = global.enemies;
		int damage = p->type - PlrProj;

		while(e != NULL) {
			if(e->hp != ENEMY_IMMUNE && cabs(e->pos - p->pos) < 15) {
				global.plr.points += damage * 0.5;
				e->hp -= damage;
				return 2;
			}
			e = e->next;
		}

		if(global.boss && cabs(global.boss->pos - p->pos) < 42
		&& global.boss->current->type != AT_Move && global.boss->current->type != AT_SurvivalSpell && global.boss->current->starttime < global.frames) {
			global.plr.points += damage * 0.2;
			global.boss->dmg += damage;
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

void process_projectiles(Projectile **projs, bool collision) {
	Projectile *proj = *projs, *del = NULL;

	char killed = 0;
	char col = 0;
	int action;
	while(proj != NULL) {
		action = proj->rule(proj, global.frames - proj->birthtime);

		if(proj->type == DeadProj && killed < 5) {
			killed++;
			action = ACTION_DESTROY;
			create_particle1c("flare", proj->pos, 0, Fade, timeout, 30);
			create_item(proj->pos, 0, BPoint)->auto_collect = 10;
		}

		if(collision)
			col = collision_projectile(proj);

		if(col && proj->type != Particle) {
			Projectile *p = create_projectile_p(&global.particles, proj->tex, proj->pos, proj->clr, DeathShrink, timeout_linear, 10, 5*cexp(proj->angle*I), 0, 0);

			if(proj->type > PlrProj) {
				p->type = PlrProj;
			}
		}

		if(col == 1 && global.frames - abs(global.plr.recovery) >= 0)
				player_death(&global.plr);

		int e = 0;
		if(proj->type == Particle) {
			e = 300;
		}

		if(action == ACTION_DESTROY || col
		|| creal(proj->pos) + proj->tex->w/2 + e < 0 || creal(proj->pos) - proj->tex->w/2 - e > VIEWPORT_W
		|| cimag(proj->pos) + proj->tex->h/2 + e < 0 || cimag(proj->pos) - proj->tex->h/2 - e > VIEWPORT_H) {
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
	if(t < 0)
		return 1;
	p->angle = carg(p->args[0]);
	p->pos = p->pos0 + p->args[0]*t;

	return 1;
}

int accelerated(Projectile *p, int t) {
	if(t < 0)
		return 1;
	p->angle = carg(p->args[0]);

	p->pos += p->args[0];
	p->args[0] += p->args[1];

	return 1;
}

int asymptotic(Projectile *p, int t) { // v = a[0]*(a[1] + 1); a[1] -> 0
	if(t < 0)
		return 1;
	p->angle = carg(p->args[0]);

	p->args[1] *= 0.8;
	p->pos += p->args[0]*(p->args[1] + 1);

	return 1;
}

void _ProjDraw(Projectile *proj, int t) {
	if(proj->clr) {
		static GLfloat clr[4];
		Shader *shader = get_shader("bullet_color");
		glUseProgram(shader->prog);
		parse_color_array(proj->clr, clr);
		glUniform4fv(uniloc(shader, "color"), 1, clr);
	}

	draw_texture_p(0,0, proj->tex);

	if(proj->clr) {
		glUseProgram(0);
	}
}

void ProjDraw(Projectile *proj, int t) {
	glPushMatrix();
	glTranslatef(creal(proj->pos), cimag(proj->pos), 0);
	glRotatef(proj->angle*180/M_PI+90, 0, 0, 1);

	if(t < 16 && proj->type < PlrProj && proj->type != Particle) {
		float s = 2.0-t/16.0;
		if(s != 1)
			glScalef(s,s,1);
	}

	_ProjDraw(proj, t);

	glPopMatrix();
}

void ProjDrawAdd(Projectile *proj, int t) {
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	ProjDraw(proj, t);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}

void ProjDrawSub(Projectile *proj, int t) {
	glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	ProjDraw(proj, t);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
}

void PartDraw(Projectile *proj, int t) {
	glPushMatrix();
	glTranslatef(creal(proj->pos), cimag(proj->pos), 0);
	glRotatef(proj->angle*180/M_PI+90, 0, 0, 1);

	if(proj->clr) {
		parse_color_call(proj->clr, glColor4f);
	}

	draw_texture_p(0,0, proj->tex);

	glPopMatrix();

	if(proj->clr)
		glColor3f(1,1,1);
}

void Blast(Projectile *p, int t) {
	if(t == 1) {
		p->args[1] = frand()*360 + frand()*I;
		p->args[2] = frand() + frand()*I;
	}

	glPushMatrix();
	if(p->pos)
		glTranslatef(creal(p->pos), cimag(p->pos), 0);
	glRotatef(creal(p->args[1]), cimag(p->args[1]), creal(p->args[2]), cimag(p->args[2]));
	if(t != p->args[0])
		glScalef(t/p->args[0], t/p->args[0], 1);
	glColor4f(0.3,0.6,1,1 - t/p->args[0]);
	draw_texture_p(0,0,p->tex);
	glPopMatrix();

	glColor4f(1,1,1,1);
}

void Shrink(Projectile *p, int t) {
	glPushMatrix();
	float s = 2.0-t/p->args[0]*2;

	if(p->pos)
		glTranslatef(creal(p->pos), cimag(p->pos), 0);

	if(p->angle != M_PI*0.5)
		glRotatef(p->angle*180/M_PI+90, 0, 0, 1);
	if(s != 1)
		glScalef(s, s, 1);

	_ProjDraw(p, t);
	glPopMatrix();
}

void ShrinkAdd(Projectile *p, int t) {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	Shrink(p, t);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void DeathShrink(Projectile *p, int t) {
	glPushMatrix();
	float s = 2.0-t/p->args[0]*2;
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	glRotatef(p->angle*180/M_PI+90, 0, 0, 1);

	if(s != 1)
		glScalef(s, 1, 1);

	_ProjDraw(p, t);
	glPopMatrix();
}

void GrowFadeAdd(Projectile *p, int t) {
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	GrowFade(p, t);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}

void GrowFade(Projectile *p, int t) {
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	glRotatef(p->angle*180/M_PI+90, 0, 0, 1);

	float s = t/p->args[0]*(1+p->args[1]);
	if(s != 1)
		glScalef(s, s, 1);

	if(p->clr)
		parse_color_call(derive_color(p->clr, CLRMASK_A, rgba(0,0,0,1-t/p->args[0])), glColor4f);
	else if(t/p->args[0] != 0)
		glColor4f(1,1,1,1-t/p->args[0]);

	draw_texture_p(0,0,p->tex);

	glColor4f(1,1,1,1);
	glPopMatrix();
}

void Fade(Projectile *p, int t) {
	if(t/creal(p->args[0]) != 0)
		glColor4f(1,1,1, 1.0 - (float)t/p->args[0]);
	ProjDraw(p, t);

	if(t/creal(p->args[0]) != 0)
		glColor4f(1,1,1,1);
}

void FadeAdd(Projectile *p, int t) {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	parse_color_call(derive_color(p->clr, CLRMASK_A, rgba(0,0,0, 1.0 - (float)t/p->args[0])), glColor4f);
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	glRotatef(180/M_PI*p->angle+90, 0, 0, 1);

	draw_texture_p(0,0, p->tex);
	glPopMatrix();
	glColor4f(1,1,1,1);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int timeout(Projectile *p, int t) {
	if(t >= creal(p->args[0]))
		return ACTION_DESTROY;

	return 1;
}

int timeout_linear(Projectile *p, int t) {
	if(t >= creal(p->args[0]))
		return ACTION_DESTROY;
	if(t < 0)
		return 1;

	p->angle = carg(p->args[1]);
	p->pos = p->pos0 + p->args[1]*t;

	return 1;
}

void Petal(Projectile *p, int t) {
	float x = creal(p->args[2]);
	float y = cimag(p->args[2]);
	float z = creal(p->args[3]);

	glDisable(GL_CULL_FACE);

	glPushMatrix();
	if(p->pos)
		glTranslatef(creal(p->pos), cimag(p->pos),0);
	glRotatef(t*4.0 + cimag(p->args[3]), x, y, z);

	if(p->clr) {
		parse_color_call(p->clr, glColor4f);
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	draw_texture_p(0,0, p->tex);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if(p->clr)
		glColor4f(1,1,1,1);
	glPopMatrix();

	glEnable(GL_CULL_FACE);
}

void petal_explosion(int n, complex pos) {
	int i;
	for(i = 0; i < n; i++) {
		tsrand_fill(8);
		create_particle4c("petal", pos, rgba(0.6,1-afrand(0)*0.4,0.5,1-0.5*afrand(1)), Petal, asymptotic, (3+5*afrand(2))*cexp(I*M_PI*2*afrand(3)), 5, afrand(4) + afrand(5)*I, afrand(6) + 360.0*I*afrand(7));
	}
}
