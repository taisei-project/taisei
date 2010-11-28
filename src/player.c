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

#include "player.h"

#include <SDL/SDL.h>
#include "projectile.h"
#include "global.h"

void init_player(Player* plr, Character cha) {
	plr->x = VIEWPORT_W/2;
	plr->y = VIEWPORT_H-20;
	
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
		glTranslatef(plr->x, plr->y, 0);
		
		if(plr->focus != 0) {
			glPushMatrix();
				glRotatef(global.frames*10, 0, 0, 1);
				glScalef(1.3, 1.3, 1);
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
		create_projectile(&_projs.youmu, plr->x-10, plr->y-20, 0, ((Color){1,1,1}), simple, 20, 0)->type = PlrProj;
		create_projectile(&_projs.youmu, plr->x+10, plr->y-20, 0, ((Color){1,1,1}), simple, 20, 0)->type = PlrProj;
		
		if(plr->power >= 2) {
			int a = 15;
			if(plr->focus > 0) a = 5;
			create_projectile(&_projs.youmu, plr->x-10, plr->y-20, -a, ((Color){1,1,1}), simple, 20, 0)->type = PlrProj;
			create_projectile(&_projs.youmu, plr->x+10, plr->y-20, a, ((Color){1,1,1}), simple, 20, 0)->type = PlrProj;
		}
	}
	
	if(plr->focus < 0 || (plr->focus > 0 && plr->focus < 30))
		plr->focus++;
}