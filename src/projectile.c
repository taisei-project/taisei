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

ProjCache _projs;

void load_projectiles() {
	load_texture(FILE_PREFIX "gfx/projectiles/ball.png", &_projs.ball);
	load_texture(FILE_PREFIX "gfx/projectiles/rice.png", &_projs.rice);
	load_texture(FILE_PREFIX "gfx/projectiles/bigball.png", &_projs.bigball);
	load_texture(FILE_PREFIX "gfx/proyoumu.png", &_projs.youmu);
}

Projectile *create_projectile(Texture *tex, int x, int y, int angle, Color clr,
							  ProjRule rule, float args, ...) {
	Projectile *p = create_element((void **)&global.projs, sizeof(Projectile));
	
	p->birthtime = SDL_GetTicks();
	p->x = x;
	p->y = y;
	p->sx = x;
	p->sy = y;
	p->angle = angle;
	p->rule = rule;
	p->tex = tex;
	p->type = FairyProj;
	p->clr = clr;
	
	va_list ap;
	int i;
	
	va_start(ap, args);
	for(i = 0; i < 4 && (args != 0); i++ && (args = va_arg(ap, double)))
		p->args[i] = args;
	va_end(ap);
	
	return p;
}

void delete_projectile(Projectile *proj) {
	delete_element((void **)&global.projs, proj);
}

void free_projectiles() {
	delete_all_elements((void **)&global.projs);
}

int test_collision(Projectile *p) {	
	if(p->type == FairyProj) {
		float angle = atan((float)(global.plr.y - p->y)/(global.plr.x - p->x));
		
		int projr = sqrt(pow(p->tex->w/4*cos(angle),2)*8/10 + pow(p->tex->h/2*sin(angle)*8/10,2));
		if(sqrt(pow(p->x-global.plr.x,2) + pow(p->y-global.plr.y,2)) < projr+1)
			// most magic line in the game.
			// i tried to get some touhou feel.
			// +/- 9 didn't really work so i used +1
			return 1;
	} else {
		Fairy *f = global.fairies;
		while(f != NULL) {
			float angle = atan((float)(f->y - p->y)/(f->x - p->x));
			
			int projr = sqrt(pow(p->tex->w/4*cos(angle),2)*8/10 + pow(p->tex->h/2*sin(angle)*8/10,2));			
			if(sqrt(pow(p->x-f->x,2) + pow(p->y-f->y,2)) < projr+10) {
				f->hp--;
                if(global.plr.power < 6) global.plr.power += 0.1;
				return 2;
			}
			f = f->next;
		}
	}
	return 0;
}

void draw_projectiles() {
	Projectile *proj = global.projs;
	int size = 3;
	int i = 0, i1;
	Texture **texs = calloc(size, sizeof(Texture *));
	
	while(proj != NULL) {
		texs[i] = proj->tex;
		for(i1 = 0; i1 < i; i1++)
			if(proj->tex == texs[i1])
				goto next0;
		
		if(i >= size)
			texs = realloc(texs, (size++)*sizeof(Texture *));
		i++;
next0:
		proj = proj->next;
	}
	
	glEnable(GL_TEXTURE_2D);	
	for(i1 = 0; i1 < i; i1++) {
		Texture *tex = texs[i1];
				
		glBindTexture(GL_TEXTURE_2D, tex->gltex);	
		
		float wq = ((float)tex->w/2.0)/tex->truew;
		float hq = ((float)tex->h)/tex->trueh;
		
		for(proj = global.projs; proj; proj = proj->next) {
			if(proj->tex != tex)
				continue;
			glPushMatrix();
			
			glTranslatef(proj->x, proj->y, 0);
			glRotatef(proj->angle, 0, 0, 1);
			glScalef(tex->w/4.0, tex->h/2.0,0);
			
			glBegin(GL_QUADS);
				glTexCoord2f(0,0); glVertex2f(-1, -1);
				glTexCoord2f(0,hq); glVertex2f(-1, 1);
				glTexCoord2f(wq,hq); glVertex2f(1, 1);
				glTexCoord2f(wq,0);	glVertex2f(1, -1);
			glEnd();
			
			glColor3fv((float *)&proj->clr);
			
			glBegin(GL_QUADS);
				glTexCoord2f(wq,0); glVertex2f(-1, -1);
				glTexCoord2f(wq,hq); glVertex2f(-1, 1);
				glTexCoord2f(2*wq,hq); glVertex2f(1, 1);
				glTexCoord2f(2*wq,0); glVertex2f(1, -1);
			glEnd();
			
			glPopMatrix();
			glColor3f(1,1,1);		
		}
	}
	glDisable(GL_TEXTURE_2D);
}

void process_projectiles() {
	Projectile *proj = global.projs, *del = NULL;
	while(proj != NULL) {
		proj->rule(&proj->x, &proj->y, proj->angle, proj->sx, proj->sy,
				   (SDL_GetTicks() - proj->birthtime)/16, proj->args);
		
		int v = test_collision(proj);
		if(v == 1)
			game_over();
		
		if(v || proj->x + proj->tex->w/2 < 0 || proj->x - proj->tex->w/2 > VIEWPORT_W
			 || proj->y + proj->tex->h/2 < 0 || proj->y - proj->tex->h/2 > VIEWPORT_H) {
			del = proj;
			proj = proj->next;
			delete_projectile(del);
			if(proj == NULL) break;
		} else {
			proj = proj->next;
		}
	}
}

void simple(float *x, float *y, int angle, int sx, int sy, int time, float* a) { // sure is physics in here; a[0]: velocity

	*y = sy + a[0]*sin((float)(angle-90)/180*M_PI)*time;
	*x = sx + a[0]*cos((float)(angle-90)/180*M_PI)*time;
}
