/*
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA  02110-1301, USA.
 
 ---
 Copyright (C) 2010, Lukas Weber <laochailan@web.de>
 */

#include "projectile.h"

#include <stdlib.h>
#include <math.h>
#include "global.h"

void create_projectile(int x, int y, int v, float angle, ProjRule rule, DrawRule draw, Color clr) {
	Projectile *p, *last = global.projs;
	
	p = malloc(sizeof(Projectile));
	
	if(last != NULL) {
		while(last->next != NULL)
			last = last->next;
		last->next = p;
	} else {
		global.projs = p;
	}
	
	*p = ((Projectile){ global.frames,x,y,x,y,v,angle,rule,draw,NULL,last,clr });
}

void delete_projectile(Projectile *proj) {
	if(proj->prev != NULL)
		proj->prev->next = proj->next;
	if(proj->next != NULL)
		proj->next->prev = proj->prev;	
	if(global.projs == proj)
		global.projs = proj->next;
	
	free(proj);
}

void draw_projectile(Projectile* proj) {
// 	glPushMatrix();
// 	glTranslatef(proj->x, proj->y, 0);
// 	glRotatef(proj->angle, 0, 0, 1);
// 	glTranslatef(-proj->x, -proj->y, 0);
// 	draw_texture(proj->x, proj->y, proj->tex);
// 	
// 	if(proj->overlay != NULL) {
// 		glColor3fv((float *)&proj->clr);
// 		draw_texture(proj->x, proj->y, proj->overlay);
// 		glColor3f(1,1,1);
// 	}
// 	glPopMatrix();
	proj->draw(proj->x, proj->y, proj->angle, &proj->clr);
}

void free_projectiles() {
	Projectile *proj = global.projs;
	Projectile *tmp;
	
	do {
		tmp = proj;
		delete_projectile(tmp);
	} while((proj = proj->next) != 0);
	
	global.projs = NULL;
}

void process_projectiles() {
	Projectile *proj = global.projs, *del = NULL;
	while(proj != NULL) {
		proj->rule(proj);
		
		if(proj->x < 0 || proj->x > VIEWPORT_W || proj->y < 0 || proj->y > VIEWPORT_H)
			del = proj;
		proj = proj->next;
	}
	
	if(del)
		delete_projectile(del);
}

void simple(Projectile *p) { // sure is physics in here
	p->y = p->sy + p->v*sin((p->angle-90)/180*M_PI)*(global.frames-p->birthtime);
	p->x = p->sx + p->v*cos((p->angle-90)/180*M_PI)*(global.frames-p->birthtime);
}


void ProjBall(int x, int y, float angle, Color *clr) { 
	// This would be a lot easier using opengls matrices for rotation...
	
	glColor4f(1,1,1,1);
	glBegin(GL_TRIANGLE_FAN);		
		glVertex3f(x, y, 0);
		
		float i;
		for(i = 360; i >= 0; i -= 40)			
			glVertex3f(8*cos(i/180*M_PI)+x, 8*sin(i/180*M_PI)+y, 0);
	glEnd();
	
	glBegin(GL_TRIANGLE_STRIP);
		for(i = 0; i <= 380; i+=20) {
			if(((int)i/20)&1) {
				glColor4f(1,1,1,1);
				glVertex3f(6*cos(i/180*M_PI)+x, 6*sin(i/180*M_PI)+y, 0);
				
			} else {					
				glColor3fv((float*)clr);
				glVertex3f(10*cos(i/180*M_PI)+x, 10*sin(i/180*M_PI)+y, 0);
			}
		}
	glEnd();
			
			
	glColor3fv((float*)clr);
	glBegin(GL_LINE_STRIP);
		for(i = 360; i >= 0; i -= 40)
			glVertex3f(12*cos(i/180*M_PI)+x, 12*sin(i/180*M_PI)+y, 0);
	glEnd();
	glColor4f(1,1,1,1);
}
