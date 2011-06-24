/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "boss.h"
#include "global.h"
#include <string.h>
#include <stdio.h>

Boss *create_boss(char *name, char *ani, complex pos) {
	Boss *buf = malloc(sizeof(Boss));
	
	buf->name = malloc(strlen(name) + 1);
	strcpy(buf->name, name);
	buf->pos = pos;
	buf->pos0 = pos;
	buf->time0 = 0;
	
	buf->acount = 0;
	buf->attacks = NULL;
	buf->current = NULL;
	
	buf->ani = get_ani(ani);
	
	buf->anirow = 0;
	buf->dmg = 0;
	
	return buf;
}

void spell_opening(Boss *b, int time) {
	int y = VIEWPORT_H - 15;
	if(time > 40 && time <= 100)
		y -= (VIEWPORT_H-35)/60*(time-40);
	if(time > 100) {
		y = 20;
	}
	
	draw_text(AL_Right, VIEWPORT_W, y, b->current->name, _fonts.standard);
}

void draw_boss(Boss *boss) {
	draw_animation_p(creal(boss->pos), cimag(boss->pos), boss->anirow, boss->ani);
	
	if(boss->current && (boss->current->type == AT_Spellcard || boss->current->type == AT_SurvivalSpell))
		spell_opening(boss, global.frames - boss->current->starttime);
	
	draw_text(AL_Left, 10, 20, boss->name, _fonts.standard);
	
	if(boss->current) {		
		char buf[16];
		snprintf(buf, 16,  "%.2f", (boss->current->timeout - global.frames + boss->current->starttime)/(float)FPS);
		draw_text(AL_Center, VIEWPORT_W - 20, 10, buf, _fonts.standard);
	}
	
	glPushMatrix();
	glTranslatef(boss->attacks[boss->acount-1].dmglimit+10, 4,0);
		
	int i;
	for(i = boss->acount-1; i >= 0; i--) {
		if(boss->dmg > boss->attacks[i].dmglimit)
			continue;
		
		switch(boss->attacks[i].type) {
		case AT_Normal:
			glColor3f(1,1,1);
			break;
		case AT_Spellcard:
			glColor3f(1,0.8,0.8);
			break;
		case AT_SurvivalSpell:
			glColor3f(1,0.5,0.5);
		}		
		
		glBegin(GL_QUADS);
		glVertex3f(-boss->attacks[i].dmglimit, 0, 0);
		glVertex3f(-boss->attacks[i].dmglimit, 2, 0);
		glVertex3f(-boss->dmg, 2, 0);
		glVertex3f(-boss->dmg, 0, 0);
		glEnd();
	}
	glColor3f(1,1,1);
	glPopMatrix();
}

void process_boss(Boss *boss) {
	if(boss->current && boss->current->waypoints) {		
		int time = global.frames - boss->current->starttime;
			
		struct Waypoint *wps = boss->current->waypoints;
		int *i = &boss->current->wp;
		
		if(time < 0) {
			boss->pos = boss->pos0 + (wps[0].pos - boss->pos0)/ATTACK_START_DELAY * (ATTACK_START_DELAY + time);
		} else {
			int wtime = time % wps[boss->current->wpcount-1].time + 1;
			
			int next = *i + 1;
			if(next >= boss->current->wpcount)
				next = 0;
			
			boss->pos = wps[*i].pos + (wps[next].pos - wps[*i].pos)/(wps[*i].time - boss->time0) * (wtime - boss->time0);
			
			if(wtime >= wps[*i].time) {
				boss->pos0 = wps[next].pos;
				boss->time0 = wps[*i].time % wps[boss->current->wpcount-1].time;
				*i = next;
			}
			
			boss->current->rule(boss, time % wps[boss->current->wpcount-1].time);
		}	
		
		if(time > boss->current->timeout)
			boss->dmg = boss->current->dmglimit + 1;
		if(boss->dmg >= boss->current->dmglimit) {
			boss->pos0 = boss->pos;
			boss->time0 = 0;
			boss->current++;
			if(boss->current - boss->attacks < boss->acount)
				start_attack(boss, boss->current);
			else
				boss->current = NULL;
		}
	}	
}

void boss_death(Boss **boss) {
	free_boss(*boss);
	*boss = NULL;
	
	Projectile *p;
	for(p = global.projs; p; p = p->next)
		if(p->type == FairyProj)
			p->type = DeadProj;
	
	delete_lasers(&global.lasers);
}

void free_boss(Boss *boss) {
	del_ref(boss);
	
	free(boss->name);
	int i;
	for(i = 0; i < boss->acount; i++)
		free_attack(&boss->attacks[i]);
	if(boss->attacks)
		free(boss->attacks);
}

void free_attack(Attack *a) {
	free(a->name);
	free(a->waypoints);
}

void start_attack(Boss *b, Attack *a) {	
	a->starttime = global.frames + ATTACK_START_DELAY;
	a->rule(b, EVENT_BIRTH);
	if(a->type == AT_Spellcard || a->type == AT_SurvivalSpell)
		play_sound("charge_generic");
}

Attack *boss_add_attack(Boss *boss, AttackType type, char *name, float timeout, int hp, BossRule rule, BossRule draw_rule) {
	boss->attacks = realloc(boss->attacks, sizeof(Attack)*(++boss->acount));
	Attack *a = &boss->attacks[boss->acount-1];
	
// 	if(!boss->current)
		boss->current = &boss->attacks[0];
	
	a->type = type;
	a->name = malloc(strlen(name)+1);
	strcpy(a->name, name);
	a->timeout = timeout * FPS;
	
	int dmg = 0;
	if(boss->acount > 1)
		dmg = boss->attacks[boss->acount - 2].dmglimit;
	
	a->dmglimit = dmg + hp;
	a->rule = rule;
	a->draw_rule = draw_rule;
	
	a->starttime = global.frames;
	
	a->wpcount = 0;
	a->waypoints = NULL;
	a->wp = 0;

	return a;
}

void boss_add_waypoint(Attack *attack, complex pos, int time) {
	attack->waypoints = realloc(attack->waypoints, (sizeof(struct Waypoint))*(++attack->wpcount));
	struct Waypoint *w = &attack->waypoints[attack->wpcount - 1];
	w->pos = pos;
	w->time = time;
}