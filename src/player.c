/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "player.h"

#include <SDL/SDL.h>
#include "projectile.h"
#include "global.h"

void init_player(Player* plr, Character cha) {
	plr->pos = VIEWPORT_W/2 + I*(VIEWPORT_H-20);
	
	plr->focus = False;
	plr->fire = False;
	plr->moving = False;
	plr->dir = 0;
    plr->power = 0;
	
	plr->cha = cha;
	
	init_animation(&plr->ani, 2, 4, 5, FILE_PREFIX "gfx/youmu.png");
}

void player_draw(Player* plr) {		
	glPushMatrix();
		glTranslatef(creal(plr->pos), cimag(plr->pos), 0);
		
		if(plr->focus != 0) {
			glPushMatrix();
				glRotatef(global.frames*10, 0, 0, 1);
				glScalef(1, 1, 1);
				glColor4f(1,1,1,0.2);
				draw_texture(0, 0, &global.textures.fairy_circle);
				glColor4f(1,1,1,1);
			glPopMatrix();
		}
					
		glDisable(GL_CULL_FACE);
		if(plr->dir) {
			glPushMatrix();
			glScalef(-1,1,1);
		}
		
		draw_animation(0, 0, !plr->moving, &plr->ani);
		
		if(plr->dir)
			glPopMatrix();
		
		glEnable(GL_CULL_FACE);
		
		if(plr->focus != 0) {
			glPushMatrix();
				glColor4f(1,1,1,fabs((float)plr->focus/30.0f));
				glRotatef(global.frames, 0, 0, -1);
				draw_texture(0, 0, &global.textures.focus);
				glColor4f(1,1,1,1);
			glPopMatrix();
		}
		
	glPopMatrix();
}

void player_logic(Player* plr) {
	if(plr->fire && !(global.frames % 4)) {
		create_projectile(&_projs.youmu, plr->pos + 10 - I*20, ((Color){1,1,1}), linear, -20I)->type = PlrProj;
		create_projectile(&_projs.youmu, plr->pos - 10 - I*20, ((Color){1,1,1}), linear, -20I)->type = PlrProj;
		
		if(plr->power >= 2) {
			float a = 0.20;
			if(plr->focus > 0) a = 0.06;
			create_projectile(&_projs.youmu, plr->pos + 10 - I*20, ((Color){1,1,1}), linear, I*-20*cexp(-I*a))->type = PlrProj;
			create_projectile(&_projs.youmu, plr->pos - 10 - I*20, ((Color){1,1,1}), linear, I*-20*cexp(I*a))->type = PlrProj;
		}
	}
	
	if(plr->focus < 0 || (plr->focus > 0 && plr->focus < 30))
		plr->focus++;
}