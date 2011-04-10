/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage.h"

#include <SDL/SDL.h>
#include "global.h"
#include "font.h"

SDL_Event event;

void stage_start() {
	init_player(&global.plr, Youmu);
}

void stage_input() {	
	while(SDL_PollEvent(&event)) {
		if(event.type == SDL_KEYDOWN) {
			switch(event.key.keysym.sym) {					
				case SDLK_LSHIFT:
					global.plr.focus = 1;
					break;
				case SDLK_y:
					global.plr.fire = True;
					break;
				case SDLK_x:
					plr_bomb(&global.plr);
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
					global.plr.focus = -30; // that's for the transparency timer
					break;
				case SDLK_y:
					global.plr.fire = False;
					break;
				default:
					break;
			}
		} else if(event.type == SDL_QUIT) {
			global.game_over = 1;
		}
	}
	
	float speed = 0.01*VIEWPORT_W/((global.plr.focus > 0)+1);	
	
	Uint8 *keys = SDL_GetKeyState(NULL);
	
	global.plr.moving = False;
	
	if(keys[SDLK_LEFT] && !keys[SDLK_RIGHT]) {
		global.plr.moving = True;
		global.plr.dir = 1;
	} else if(keys[SDLK_RIGHT] && !keys[SDLK_LEFT]) {
		global.plr.moving = True;
		global.plr.dir = 0;
	}	
	
	if(keys[SDLK_LEFT] && creal(global.plr.pos) - global.plr.ani->w/2 - speed > 0)
		global.plr.pos -= speed;		
	if(keys[SDLK_RIGHT] && creal(global.plr.pos) + global.plr.ani->w/2 + speed < VIEWPORT_W)
		global.plr.pos += speed;
	if(keys[SDLK_UP] && cimag(global.plr.pos) - global.plr.ani->h/2 - speed > 0)
		global.plr.pos -= I*speed;
	if(keys[SDLK_DOWN] && cimag(global.plr.pos) + global.plr.ani->h/2 + speed < VIEWPORT_H)
		global.plr.pos += I*speed;

}

void stage_draw() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, SCREEN_W, SCREEN_H, 0, -10, 10);
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_DEPTH_TEST);
	
	glPushMatrix();
	glTranslatef(VIEWPORT_X,VIEWPORT_Y,0);
	player_draw(&global.plr);

	draw_projectiles();
	draw_fairies();
	draw_poweritems();
	draw_slaves();
	
	if(global.boss)
		draw_boss(global.boss);
	
	glPopMatrix();
	draw_texture(SCREEN_W/2, SCREEN_H/2, "hud");
	
	char buf[16];
	int i;
	
	glPushMatrix();
	glTranslatef(615,0,0);
	
// 	glColor3f(1,0,0);
	for(i = 0; i < global.plr.lifes; i++)
	  draw_texture(16*i,167, "star");
// 	glColor3f(0,1,0);
	for(i = 0; i < global.plr.bombs; i++)
	  draw_texture(16*i,200, "star");
	glColor3f(1,1,1);
	
	sprintf(buf, "%.2f", global.plr.power);
	draw_text(buf, 10, 236, _fonts.biolinum);
	
	sprintf(buf, "%i", global.points);
	draw_text(buf, 13, 49, _fonts.biolinum);
	
	glPopMatrix();
}

void stage_logic() {
	player_logic(&global.plr);
	
	process_fairies();
	process_projectiles();
	process_poweritems();
	process_slaves();
	
	if(global.boss) {
		process_boss(global.boss);
		if(global.boss->dmg > global.boss->attacks[global.boss->acount-1].dmglimit) {
			free_boss(global.boss);
			global.boss = NULL;
		}
	}
	global.frames++;
		
	if(SDL_GetTicks() > global.fpstime+1000) {
		fprintf(stderr, "FPS: %d\n", global.fps);
		global.fps = 0;
		global.fpstime = SDL_GetTicks();
	} else {
		global.fps++;
	}
}

void stage_end() {
	free_projectiles();
	free_fairies();
	free_poweritems();
	free_slaves();
	global.frames = 0;
}
		