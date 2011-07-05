/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "global.h"
#include <SDL/SDL.h>
#include <time.h>
#include <stdio.h>
#include "paths/native.h"
#include "resource/resource.h"
#include "taisei_err.h"

Global global;

void init_global() {
	memset(&global, 0, sizeof(global));	
	srand(time(0));
	
	memset(&resources, 0, sizeof(Resources));
	load_resources();
	printf("- fonts:\n");
	init_fonts();
}

void game_over() {
	global.game_over = GAMEOVER_DEFEAT;
	printf("Game Over!\n");
}

void frame_rate(int *lasttime) {
	int t = *lasttime + 1000/FPS - SDL_GetTicks();
	if(t > 0)
		SDL_Delay(t);
	
	*lasttime = SDL_GetTicks();
}

void calc_fps(FPSCounter *fps) {
	if(SDL_GetTicks() > fps->fpstime+1000) {
		fps->show_fps = fps->fps;
		fps->fps = 0;
		fps->fpstime = SDL_GetTicks();
	} else {
		fps->fps++;
	}
}

void set_ortho() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, SCREEN_W, SCREEN_H, 0, -100, 100);
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_DEPTH_TEST);
}

void fade_out(float f) {
	glColor4f(0,0,0,f);
			
	glBegin(GL_QUADS);
	glVertex3f(0,0,0);
	glVertex3f(0,SCREEN_H,0);
	glVertex3f(SCREEN_W,SCREEN_H,0);
	glVertex3f(SCREEN_W,0,0);
	glEnd();
	
	glColor4f(1,1,1,1);	
}

inline double frand() {
	return rand()/(double)RAND_MAX;
}

extern SDL_Surface *display;

void toggle_fullscreen()
{
	int newflags = display->flags;
	newflags ^= SDL_FULLSCREEN;
	tconfig.intval[FULLSCREEN] = newflags & SDL_FULLSCREEN;
	
	SDL_FreeSurface(display);
	if((display = SDL_SetVideoMode(SCREEN_W, SCREEN_H, 32, newflags)) == NULL)
		errx(-1, "Error opening screen: %s", SDL_GetError());
}

void take_screenshot()
{
	FILE *out;
	char data[3 * SCREEN_W * SCREEN_H];
	short head[] = {0, 2, 0, 0, 0, 0, SCREEN_W, SCREEN_H, 24};
	char outfile[128], *outpath;
	time_t rawtime;
	struct tm * timeinfo;
	
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(outfile, 128, "/taisei_%Y%m%d_%H-%M-%S_%Z.tga", timeinfo);
	
	outpath = malloc(strlen(outfile) + strlen(get_screenshots_path()) + 1);
	strcpy(outpath, get_screenshots_path());
	strcat(outpath, outfile);
	
	printf("Saving screenshot as %s\n", outpath);
	out = fopen(outpath, "w");
	free(outpath);
	
	if(!out)
	{
		perror("fopen");
		return;
	}
	
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, SCREEN_W, SCREEN_H, GL_BGR, GL_UNSIGNED_BYTE, data);
	fwrite(&head, sizeof(head), 1, out);
	fwrite(data, 3 * SCREEN_W * SCREEN_H, 1, out);
	
	fclose(out);
}

void global_processevent(SDL_Event *event)
{
	int sym = event->key.keysym.sym;
	Uint8 *keys;
	
	keys = SDL_GetKeyState(NULL);
	
	if(event->type == SDL_KEYDOWN)
	{
		if(sym == tconfig.intval[KEY_SCREENSHOT])
			take_screenshot();
		if((sym == SDLK_RETURN && (keys[SDLK_LALT] || keys[SDLK_RALT])) || sym == tconfig.intval[KEY_FULLSCREEN])
			toggle_fullscreen();
	}
}
