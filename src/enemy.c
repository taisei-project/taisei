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

void create_enemy(Enemy **enemies, EnemyDrawRule draw_rule, EnemyLogicRule logic_rule,
				  complex pos, int hp, void *parent, complex args, ...) {
	Enemy *e = (Enemy *)create_element((void **)enemies, sizeof(Enemy));
	
	e->birthtime = global.frames;
	e->pos = pos;
	e->pos0 = pos;
	
	e->hp = hp;
	
	e->parent = parent;
	e->logic_rule = logic_rule;
	e->draw_rule = draw_rule;
	
	va_list ap;
	int i;
	
	memset(e->args, 0, 4);
	va_start(ap, args);
	for(i = 0; i < 4 && args; i++) {
		e->args[i++] = args;
		args = va_arg(ap, complex);
	}
	va_end(ap);
	
	e->logic_rule(e, EVENT_BIRTH);
}

void delete_enemy(Enemy **enemies, Enemy* enemy) {
	enemy->logic_rule(enemy, EVENT_DEATH);
	delete_element((void **)enemies, enemy);
}

void free_enemies(Enemy **enemies) {
	delete_all_elements((void **)enemies);
}

void draw_enemies(Enemy *enemies) {
	Enemy *e;	
		
	for(e = enemies; e; e = e->next)
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
	glColor3f(0.2,0.7,1);
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
}

void process_enemies(Enemy **enemies) {
	Enemy *enemy = *enemies, *del = NULL;
	
	while(enemy != NULL) {
		enemy->logic_rule(enemy, global.frames - enemy->birthtime);
						
		if(enemy->hp != ENEMY_IMMUNE
		&& (creal(enemy->pos) < -20 || creal(enemy->pos) > VIEWPORT_W + 20
		|| cimag(enemy->pos) < -20 || cimag(enemy->pos) > VIEWPORT_H + 20
		|| enemy->hp <= 0)) {
			del = enemy;
			enemy = enemy->next;
			delete_enemy(enemies, del);
		} else {
			enemy = enemy->next;
		}
	}
}