/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "laser.h"
#include "global.h"
#include "list.h"

Laser *create_laser_p(LaserType type, complex pos, complex dir, int time, int deathtime, Color *color, LaserRule rule, complex a0, complex a1, complex a2, complex a3) {
	Laser *l = create_element((void **)&global.lasers, sizeof(Laser));
		
	l->type = type;
	l->birthtime = global.frames;
	l->time = time;
	l->deathtime = deathtime;
	l->pos = pos;
	l->dir = dir;
	l->rule = rule;
	l->color = color;
	
	l->args[0] = a0;	
	l->args[1] = a1;
	l->args[2] = a2;
	l->args[3] = a3;
		
	return l;
}

void draw_laser_line(Laser *laser) {
	float width = cabs(laser->dir);
	if(global.frames - laser->birthtime < laser->time*3/5.0)
		width = 2;
	else if(global.frames - laser->birthtime < laser->time)
		width = 2 + (width - 2)/laser->time * (global.frames - laser->birthtime);
	else if(global.frames - laser->birthtime > laser->deathtime-20)
		width = width - width/20 * (global.frames - laser->birthtime - laser->deathtime + 20);
	
	glPushMatrix();
	glTranslatef(creal(laser->pos), cimag(laser->pos),0);
	glRotatef(carg(laser->dir)*180/M_PI,0,0,1);
		
	glBindTexture(GL_TEXTURE_2D, get_tex("part/lasercurve")->gltex);
	
	glColor4fv((float *)laser->color);
	
	glTranslatef(500, 0, 0);
	glScalef(1000,width+4,1);
	
	draw_quad();
	
	glColor3f(1,1,1);
	
	glScalef(1,0.8,1);
	
	draw_quad();
	
	glPopMatrix();
}

void draw_laser_curve(Laser *laser) {	
	Texture *tex = get_tex("part/lasercurve");
	glBindTexture(GL_TEXTURE_2D, tex->gltex);
	
	glColor4fv((float *)laser->color);
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		
	float t = global.frames - laser->birthtime - laser->time;
	if(t < 0)
		t = 0;
		
	for(; t < global.frames - laser->birthtime && t <= laser->deathtime; t += 0.5) {
		complex pos = laser->rule(laser,t);
		glPushMatrix();
		
		float t1 = t - (global.frames - laser->birthtime - laser->time/2);
		
		float tail = laser->time/1.9;

		float s = -0.75/pow(tail,2)*(t1-tail)*(t1+tail);
				
		glTranslatef(creal(pos), cimag(pos), 0);
							
		float wq = ((float)tex->w)/tex->truew;
		float hq = ((float)tex->h)/tex->trueh;			
		
		glScalef(s*tex->w*wq,s*tex->h*hq,s);			
		draw_quad();
		
		glPopMatrix();
	}
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glColor4f(1,1,1,1);
		
}

void draw_lasers() {
	Laser *laser;
	
	for(laser = global.lasers; laser; laser = laser->next) {
		glEnable(GL_TEXTURE_2D);
		if(laser->type == LT_Line)
			draw_laser_line(laser);
		else
			draw_laser_curve(laser);
		glDisable(GL_TEXTURE_2D);
	}
}

void delete_lasers() {
	delete_all_elements((void **)&global.lasers, delete_element);
}

void process_lasers() {
	Laser *laser = global.lasers, *del = NULL;
	
	while(laser != NULL) {
		int c = 0;
		if(laser->type == LT_Line) {
			if(laser->rule)
				laser->rule(laser, global.frames - laser->birthtime);
			c = collision_laser_line(laser);
		} else {
			c = collision_laser_curve(laser);
		}
		
		if(c && global.frames - laser->birthtime > laser->time)
			player_death(&global.plr);
		
		if(global.frames - laser->birthtime > laser->deathtime + laser->time) {
			del = laser;
			laser = laser->next;
			delete_element((void **)&global.lasers, del);
			if(laser == NULL) break;
		} else {
			laser = laser->next;
		}
	}
}

int collision_laser_line(Laser *l) {
	Player *plr = &global.plr;	
	float x = creal(l->pos) + creal(l->dir)/cimag(l->dir)*(cimag(plr->pos) - cimag(l->pos));
	
	if(fabs(creal(plr->pos) - x) < fabs(creal(l->dir*I)/2))
		return 1;
	else
		return 0;
}

int collision_laser_curve(Laser *l) {
	float t = global.frames - l->birthtime - l->time;
	if(t < 0)
		t = 0;
	
	for(; t < global.frames - l->birthtime && t <= l->deathtime; t+=2) {
		complex pos = l->rule(l,t);
		if(cabs(pos - global.plr.pos) < 5)
			return 1;
	}
	return 0;
}