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

ProjCache _projs;

void load_projectiles() {
	load_texture(FILE_PREFIX "gfx/projectiles/ball.png", &_projs.ball);
	load_texture(FILE_PREFIX "gfx/projectiles/rice.png", &_projs.rice);
	load_texture(FILE_PREFIX "gfx/projectiles/bigball.png", &_projs.bigball);
	load_texture(FILE_PREFIX "gfx/proyoumu.png", &_projs.youmu);
}

void create_projectile(int x, int y, int v, float angle, ProjRule rule, Texture *tex, Color clr) {
	Projectile *p, *last = global.projs;
	
	p = malloc(sizeof(Projectile));
	
	if(last != NULL) {
		while(last->next != NULL)
			last = last->next;
		last->next = p;
	} else {
		global.projs = p;
	}
	
	*p = ((Projectile){ global.frames,x,y,x,y,v,angle,rule,tex,NULL,last,clr });
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
	glPushMatrix();
	
	glTranslatef(proj->x, proj->y, 0);
	glRotatef(proj->angle, 0, 0, 1);
	
	Texture *tex = proj->tex;
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex->gltex);	
	
	float wq = ((float)tex->w/2.0)/tex->truew;
	float hq = ((float)tex->h)/tex->trueh;
	
	glBegin(GL_QUADS);
		glTexCoord2f(0,0);
		glVertex3f(-tex->w/4, -tex->h/2, 0.0f);
		
		glTexCoord2f(0,hq);
		glVertex3f(-tex->w/4, tex->h/2, 0.0f);
		
		glTexCoord2f(wq,hq);
		glVertex3f(tex->w/4, tex->h/2, 0.0f);
		
		glTexCoord2f(wq,0);
		glVertex3f(tex->w/4, -tex->h/2, 0.0f);

		glColor3fv((float *)&proj->clr);
		glTexCoord2f(wq,0);
		glVertex3f(-tex->w/4, -tex->h/2, 0.0f);
		
		glTexCoord2f(wq,hq);
		glVertex3f(-tex->w/4, tex->h/2, 0.0f);
		
		glTexCoord2f(2*wq,hq);
		glVertex3f(tex->w/4, tex->h/2, 0.0f);
		
		glTexCoord2f(2*wq,0);
		glVertex3f(tex->w/4, -tex->h/2, 0.0f);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	
	glPopMatrix();
	glColor3f(1,1,1);
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

int test_collision(Projectile *p) {
	float angle = atan((float)(global.plr.y - p->y)/(global.plr.x - p->x));
	
	int projr = sqrt(pow(p->tex->w/4*cos(angle),2)*8/10 + pow(p->tex->h/2*sin(angle)*8/10,2));
	
	if(sqrt(pow(p->x-global.plr.x,2) + pow(p->y-global.plr.y,2)) < projr+5)
		return 1;
	return 0;
}


void process_projectiles() {
	Projectile *proj = global.projs, *del = NULL;
	while(proj != NULL) {
		proj->rule(proj);
		
		if(proj->x + proj->tex->w/2 < 0 || proj->x - proj->tex->w/2 > VIEWPORT_W
			|| proj->y + proj->tex->h/2 < 0 || proj->y - proj->tex->h/2 > VIEWPORT_H)
			del = proj;
		
		if(test_collision(proj))
			game_over();
		proj = proj->next;
	}
	
	if(del)
		delete_projectile(del);
}

void simple(Projectile *p) { // sure is physics in here
	p->y = p->sy + p->v*sin((p->angle-90)/180*M_PI)*(global.frames-p->birthtime);
	p->x = p->sx + p->v*cos((p->angle-90)/180*M_PI)*(global.frames-p->birthtime);
}
