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
		
	for(e = enemies; e; e = e->next)
		if(e->draw_rule)
			e->draw_rule(e, global.frames - e->birthtime);
}

void Fairy(Enemy *e, int t) {
	glEnable(GL_TEXTURE_2D);
	
	float s = sin((float)(global.frames-e->birthtime)/10.f)/6 + 0.8;
	glPushMatrix();
	glTranslatef(creal(e->pos),cimag(e->pos),0);
	
	glPushMatrix();	
	glRotatef(global.frames*10,0,0,1);
	glScalef(s, s, s);
// 	glColor4f(1,1,1,0.7);
	draw_texture(0,0,"fairy_circle");
	glPopMatrix();
	
	glColor4f(1,1,1,1);
	glPushMatrix();
	if(e->dir) {
		glCullFace(GL_FRONT);
		glScalef(-1,1,1);
	}
	draw_animation(0, 0, e->moving, "fairy");
	glPopMatrix();
	
	glPopMatrix();	
	glDisable(GL_TEXTURE_2D);
	
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
		
		if(enemy->hp > ENEMY_IMMUNE && cabs(enemy->pos - global.plr.pos) < 25)
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