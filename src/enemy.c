/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "enemy.h"

#include <stdlib.h>
#include <math.h>
#include "global.h"
#include "projectile.h"
#include "list.h"

void create_enemy_p(Enemy **enemies, complex pos, int hp, EnemyDrawRule draw_rule, EnemyLogicRule logic_rule,
				  complex a1, complex a2, complex a3, complex a4) {
	Enemy *e = (Enemy *)create_element((void **)enemies, sizeof(Enemy));
	e->moving = 0;
	e->dir = 0;
	
	e->birthtime = global.frames;
	e->pos = pos;
	e->pos0 = pos;
	
	e->hp = hp;
	e->alpha = 1.0;
	
	e->logic_rule = logic_rule;
	e->draw_rule = draw_rule;
		
	e->args[0] = a1;
	e->args[1] = a2;
	e->args[2] = a3;
	e->args[3] = a4;
	
	e->logic_rule(e, EVENT_BIRTH);
}

void _delete_enemy(void **enemies, void* enemy) {
	Enemy *e = (Enemy *)enemy;
	
	if(e->hp <= 0 && e->hp > ENEMY_IMMUNE) {
		int i;
		for(i = 0; i < 10; i++)
			create_particle2c("flare", e->pos, NULL, Fade, timeout_linear, 10, (3+frand()*10)*cexp(I*frand()*2*M_PI));
		create_particle1c("blast", e->pos, NULL, Blast, timeout, 20);
		create_particle1c("blast", e->pos, NULL, Blast, timeout, 20);
		create_particle2c("blast", e->pos, NULL, GrowFade, timeout, 15,0);
		e->logic_rule(enemy, EVENT_DEATH);
	}
	del_ref(enemy);
	
	delete_element((void **)enemies, enemy);
}

void delete_enemy(Enemy **enemies, Enemy* enemy) {
	_delete_enemy((void**) enemies, enemy);
}

void delete_enemies(Enemy **enemies) {
	delete_all_elements((void **)enemies, _delete_enemy);
}

void draw_enemies(Enemy *enemies) {
	Enemy *e;
	int reset = False;
		
	for(e = enemies; e; e = e->next) {
		if(e->draw_rule) {
			if(e->alpha < 1) {
				e->alpha += 1 / 60.0;
				if(e->alpha > 1)
					e->alpha = 1;
				
				glColor4f(1,1,1,e->alpha);
				reset = True;
			}
			
			e->draw_rule(e, global.frames - e->birthtime);
			if(reset)
				glColor4f(1,1,1,1);
		}
	}
}

int enemy_flare(Projectile *p, int t) { // a[0] timeout, a[1] velocity, a[2] ref to enemy
	if(t >= creal(p->args[0]) || REF(p->args[2]) == NULL) {
		return ACTION_DESTROY;
	} if(t == EVENT_DEATH) {
		free_ref(p->args[2]);
		return 1;
	} else if(t < 0) {
		return 1;
	}
	
	p->pos += p->args[1];
	
	return 1;
}

void EnemyFlareShrink(Projectile *p, int t) {
	Enemy *e = (Enemy *)REF(p->args[2]);
	if(e == NULL)
		return;
	
	glPushMatrix();
	float s = 2.0-t/p->args[0]*2;
	
	if(e->pos + p->pos)
		glTranslatef(creal(e->pos + p->pos), cimag(e->pos + p->pos), 0);
	
	if(p->angle != M_PI*0.5)
		glRotatef(p->angle*180/M_PI+90, 0, 0, 1);
	
	if(s != 1)
		glScalef(s, s, 1);
	
	if(p->clr)
		glColor4fv((float *)p->clr);
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	draw_texture_p(0, 0, p->tex);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glPopMatrix();
	
	if(p->clr)
		glColor3f(1,1,1);
}

void BigFairy(Enemy *e, int t) {
	if(!(t % 5)) {
		complex offset = (frand()-0.5)*30 + (frand()-0.5)*20I;
		create_particle3c("lasercurve", offset, rgb(0,0.2,0.3), EnemyFlareShrink, enemy_flare, 50, (-50I-offset)/50.0, add_ref(e));
	}
	
	glPushMatrix();
	glTranslatef(creal(e->pos), cimag(e->pos), 0);
	
	float s = sin((float)(global.frames-e->birthtime)/10.f)/6 + 0.8;
	
	glPushMatrix();	
	glRotatef(global.frames*10,0,0,1);
	glScalef(s, s, s);
	draw_texture(0,0,"fairy_circle");
	glPopMatrix();
	
	if(e->dir) {
		glCullFace(GL_FRONT);
		glScalef(-1,1,1);
	}
	draw_animation(0, 0, e->moving, "bigfairy");
	glPopMatrix();
	
	if(e->dir)
		glCullFace(GL_BACK);
}

void Fairy(Enemy *e, int t) {
	
	float s = sin((float)(global.frames-e->birthtime)/10.f)/6 + 0.8;
	glPushMatrix();
	glTranslatef(creal(e->pos),cimag(e->pos),0);
	
	glPushMatrix();	
	glRotatef(global.frames*10,0,0,1);
	glScalef(s, s, s);
	draw_texture(0,0,"fairy_circle");
	glPopMatrix();
	
	glPushMatrix();
	if(e->dir) {
		glCullFace(GL_FRONT);
		glScalef(-1,1,1);
	}
	draw_animation(0, 0, e->moving, "fairy");
	glPopMatrix();
	
	glPopMatrix();
	
	if(e->dir) {
		glCullFace(GL_BACK);
	}
}

void Swirl(Enemy *e, int t) {
	glPushMatrix();
	glTranslatef(creal(e->pos), cimag(e->pos),0);
	glRotatef(t*15,0,0,1);
	draw_texture(0,0, "swirl");
	glPopMatrix();
}

void process_enemies(Enemy **enemies) {
	Enemy *enemy = *enemies, *del = NULL;
	
	while(enemy != NULL) {
		int action = enemy->logic_rule(enemy, global.frames - enemy->birthtime);
		
		if(enemy->hp > ENEMY_IMMUNE && cabs(enemy->pos - global.plr.pos) < 7)
			plr_death(&global.plr);
		
		if((enemy->hp > ENEMY_IMMUNE
		&& (creal(enemy->pos) < -20 || creal(enemy->pos) > VIEWPORT_W + 20
		|| cimag(enemy->pos) < -20 || cimag(enemy->pos) > VIEWPORT_H + 20
		|| enemy->hp <= 0)) || action == ACTION_DESTROY) {
			del = enemy;
			enemy = enemy->next;
			delete_enemy(enemies, del);
		} else {
			enemy = enemy->next;
		}		
	}
}
