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
	plr->x = 0;
	plr->y = 0;
	
	plr->focus = False;
	plr->fire = False;
	plr->moving = False;
	plr->dir = 0;
	
	plr->cha = cha;
	
	init_animation(&plr->ani, 2, 4, 5, FILE_PREFIX "gfx/youmu.png");
}

void player_draw(Player* plr) {
	glPushMatrix();
	glDisable(GL_CULL_FACE);
	
	if(plr->dir) {
		glTranslatef(plr->x,plr->y,0);
		glScalef(-1,1,1);
		glTranslatef(-plr->x,-plr->y,0);
	}
	draw_animation(plr->x, plr->y, !plr->moving, &plr->ani);
	
	glEnable(GL_CULL_FACE);
	glPopMatrix();
}

void player_logic(Player* plr) {
	if(plr->fire && !(global.frames % 4)) {
		create_projectile(plr->x-10, plr->y, 20, 0, simple, ProjBall, ((Color){1,1,1}));
		create_projectile(plr->x+10, plr->y, 20, 0, simple, ProjBall, ((Color){1,1,1}));
	}
}