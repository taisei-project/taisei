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

#include "stage.h"

#include <SDL/SDL.h>
#include "global.h"

SDL_Event event;

void stage_start() {
	init_player(&global.plr, Youmu);
}

void stage_input() {	
	while(SDL_PollEvent(&event)) {
		if(event.type == SDL_KEYDOWN) {
			switch(event.key.keysym.sym) {					
				case SDLK_LSHIFT:
					global.plr.focus = True;
					break;
				case SDLK_y:
					global.plr.fire = True;
					break;
				case SDLK_ESCAPE:
					exit(1);
					break;
				default:
					break;
			}
		} else if(event.type == SDL_KEYUP) {
			switch(event.key.keysym.sym) {
				case SDLK_LSHIFT:
					global.plr.focus = False;
					break;
				case SDLK_y:
					global.plr.fire = False;
					break;
				default:
					break;
			}
		} else if(event.type == SDL_QUIT) {
			exit(1);
		}
	}
	
	int speed = 5 - 3*global.plr.focus;
	
	Uint8 *keys = SDL_GetKeyState(NULL);
	
	global.plr.moving = False;
	
	if(keys[SDLK_LEFT]) {
		global.plr.moving = True;
		global.plr.dir = 1;
	} else if(keys[SDLK_RIGHT]) {
		global.plr.moving = True;
		global.plr.dir = 0;
	}	
	
	if(keys[SDLK_LEFT] && global.plr.x - global.plr.ani.w/2 - speed > 0)
		global.plr.x -= speed;		
	if(keys[SDLK_RIGHT] && global.plr.x + global.plr.ani.w/2 + speed < VIEWPORT_W)
		global.plr.x += speed;
	if(keys[SDLK_UP] && global.plr.y - global.plr.ani.h/2 - speed > 0)
		global.plr.y -= speed;
	if(keys[SDLK_DOWN] && global.plr.y + global.plr.ani.h/2 + speed < VIEWPORT_H)
		global.plr.y += speed;

}

void stage_draw() {
	glPushMatrix();
	glTranslatef(VIEWPORT_X,VIEWPORT_Y,0);
	
	player_draw(&global.plr);
	
	Projectile *proj = global.projs;
	while(proj != NULL) {
		draw_projectile(proj);		
		proj = proj->next;
	}

	Fairy *f = global.fairies;
	while(f != NULL) {
		draw_fairy(f);		
		f = f->next;
	}	
	
	glPopMatrix();
	
	draw_texture(SCREEN_W/2, SCREEN_H/2, &global.textures.hud);
	
	glFlush(); 
	SDL_GL_SwapBuffers();
}

void stage_logic() {
	player_logic(&global.plr);
	
	process_fairies();
	process_projectiles();
	
	global.frames++;
}

void stage_end() {
	free_projectiles();
	free_fairies();
	global.frames = 0;
}
		