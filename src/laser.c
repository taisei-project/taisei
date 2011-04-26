/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "laser.h"
#include "global.h"
#include "list.h"

Laser *create_laser(LaserType type, complex pos, complex pos0, int time, int deathtime, Color *color, LaserRule rule, complex args, ...) {
	Laser *l = create_element((void **)&global.lasers, sizeof(Laser));
		
	l->type = type;
	l->birthtime = global.frames;
	l->time = time;
	l->deathtime = deathtime;
	l->pos = pos;
	l->pos0 = pos0;
	l->rule = rule;
	l->color = color;
	
	va_list ap;
	int i;
	
	va_start(ap, args);
	for(i = 0; i < 4 && args; i++) {
		l->args[i++] = args;
		args = va_arg(ap, complex);
	}
	va_end(ap);
	
// 	play_sound("laser1");
	
	return l;
}

void draw_laser_line(Laser *laser) {
	float width = cabs(laser->pos0);
	if(global.frames - laser->birthtime < laser->time*3/5.0)
		width = 2;
	else if(global.frames - laser->birthtime < laser->time)
		width = 2 + (width - 2)/laser->time * (global.frames - laser->birthtime);
	else if(global.frames - laser->birthtime > laser->deathtime-20)
		width = width - width/20 * (global.frames - laser->birthtime - laser->deathtime + 20);
	
	glPushMatrix();
	glTranslatef(creal(laser->pos), cimag(laser->pos),0);
	glRotatef(carg(laser->pos0)*180/M_PI,0,0,1);
		
	glBindTexture(GL_TEXTURE_2D, get_tex("lasercurve")->gltex);
	
	glColor4fv((float *)&laser->color);
	
	glBegin(GL_QUADS);
		glTexCoord2i(0,0);
		glVertex3f(0, width/2.0+2, 0);
		glTexCoord2i(1,0);
		glVertex3f(1000, width/2.0+2, 0);
		glTexCoord2i(1,1);
		glVertex3f(1000, -width/2.0-2, 0);
		glTexCoord2i(0,1);
		glVertex3f(0, -width/2.0-2, 0);
	glEnd();
	
	glColor3f(1,1,1);
	
	glBegin(GL_QUADS);
		glTexCoord2i(0,0);
		glVertex3f(0, width/2.0, 0);
		glTexCoord2i(1,0);
		glVertex3f(1000, width/2.0, 0);
		glTexCoord2i(1,1);
		glVertex3f(1000, -width/2.0, 0);
		glTexCoord2i(0,1);
		glVertex3f(0, -width/2.0, 0);
	glEnd();
	glPopMatrix();
}

void draw_laser_curve(Laser *laser) {
	int i;
	
	Texture *tex = get_tex("lasercurve");
	glBindTexture(GL_TEXTURE_2D, tex->gltex);
	
	glColor4fv((float *)laser->color);
	for(i = 0; i < 2; i++) {
		float t = global.frames - laser->birthtime - laser->time;
		if(t < 0)
			t = 0;
		
		for(t; t < global.frames - laser->birthtime && t <= laser->deathtime; t+=0.5) {
			complex pos = laser->rule(laser,t);
			glPushMatrix();
			
			float t1 = t - (global.frames - laser->birthtime - laser->time/2);
			
			float tail = laser->time/1.9 + (1-i)*3;

			float s = -0.75/pow(tail,2)*(t1-tail)*(t1+tail);
			s += 0.2*(1-i);
			
			if(i)
				glColor4f(1,1,1,1.0/(laser->time)*(t1+laser->time/2));
			
			glTranslatef(creal(pos), cimag(pos), 0);
			glScalef(s,s,s);
						
			float wq = ((float)tex->w)/tex->truew;
			float hq = ((float)tex->h)/tex->trueh;			
			
			glScalef(tex->w/2,tex->h/2,1);
			
			glBegin(GL_QUADS);
				glTexCoord2f(0,0); glVertex3f(-1, -1, 0);
				glTexCoord2f(0,hq); glVertex3f(-1, 1, 0);
				glTexCoord2f(wq,hq); glVertex3f(1, 1, 0);
				glTexCoord2f(wq,0); glVertex3f(1, -1, 0);
			glEnd();
			glPopMatrix();
		}
		glColor4f(1,1,1,1);
	}	
}

void draw_lasers() {
	Laser *laser;
	
	for(laser = global.lasers; laser; laser = laser->next) {
		glEnable(GL_TEXTURE_2D);
		if(laser->type == LaserLine)
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
		if(laser->type == LaserLine)
			c = collision_laser_line(laser);
		else
			c = collision_laser_curve(laser);
		
		if(c && global.frames - laser->birthtime > laser->time)
			plr_death(&global.plr);
		
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
	float x = creal(l->pos) + creal(l->pos0)/cimag(l->pos0)*(cimag(plr->pos) - cimag(l->pos));
	
	if(fabs(creal(plr->pos) - x) < fabs(creal(l->pos0*I)/2))
		return 1;
	else
		return 0;
}

int collision_laser_curve(Laser *l) {
	float t = global.frames - l->birthtime - l->time;
	if(t < 0)
		t = 0;
	
	for(t; t < global.frames - l->birthtime && t <= l->deathtime; t+=2) {
		complex pos = l->rule(l,t);
		if(cabs(pos - global.plr.pos) < 5)
			return 1;
	}
	return 0;
}