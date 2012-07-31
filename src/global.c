/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "global.h"
#include "video.h"
#include <SDL/SDL.h>
#include <time.h>
#include <stdio.h>
#include <png.h>
#include "paths/native.h"
#include "resource/resource.h"
#include "taisei_err.h"
#include "replay.h"

Global global;

void init_global() {
	memset(&global, 0, sizeof(global));	
	
	tsrand_seed_p(&global.rand_game, time(0));
	tsrand_seed_p(&global.rand_visual, time(0));
	tsrand_switch(&global.rand_visual);
	
	memset(&resources, 0, sizeof(Resources));
	load_resources();
	printf("- fonts:\n");
	init_fonts();
	
	memset(&global.replay, 0, sizeof(Replay));
	global.replaymode = REPLAY_RECORD;
}

void game_over() {
	global.game_over = GAMEOVER_DEFEAT;
	printf("Game Over!\n");
}

void frame_rate(int *lasttime) {
	int t = *lasttime + 1000.0/FPS - SDL_GetTicks();
	if(t > 0)
		SDL_Delay(t);
	
	*lasttime = SDL_GetTicks();
}

double approach(double v, double t, double d) {
	if(v < t) {
		v += d; 
		if(v > t)
			return t;
	} else if(v > t) {
		v -= d;
		if(v < t)
			return t;
	}
	
	return v;
}

void calc_fps(FPSCounter *fps) {
	if(!fps->stagebg_fps)
		fps->stagebg_fps = FPS;
	
	if(SDL_GetTicks() > fps->fpstime+1000) {
		fps->show_fps = fps->fps;
		fps->fps = 0;
		fps->fpstime = SDL_GetTicks();
	} else {
		fps->fps++;
	}
	
	if(!global.menu)
		fps->stagebg_fps = approach(fps->stagebg_fps, fps->show_fps, 0.1);
}

void set_ortho() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, SCREEN_W, SCREEN_H, 0, -100, 100);
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_DEPTH_TEST);
}

void fade_out(float f) {
	if(f == 0) return;
	
	glColor4f(0,0,0,f);
	
	glPushMatrix();
	glScalef(SCREEN_W,SCREEN_H,1);
	glTranslatef(0.5,0.5,0);
	
	draw_quad();
	glPopMatrix();
	
	glColor4f(1,1,1,1);	
}

void take_screenshot()
{
	FILE *out;
	char *data;
	char outfile[128], *outpath;
	time_t rawtime;
	struct tm * timeinfo;
	int w, h;
	
	w = video.current.width;
	h = video.current.height;
	
	data = malloc(3 * w * h);
	
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(outfile, 128, "/taisei_%Y%m%d_%H-%M-%S_%Z.png", timeinfo);
	
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
	glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);

	png_structp png_ptr;
    png_infop info_ptr;
	png_byte **row_pointers;
	int y;
	
	png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct (png_ptr);

	png_set_IHDR(png_ptr, info_ptr, w, h, 8, PNG_COLOR_TYPE_RGB,
                  PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	row_pointers = png_malloc(png_ptr, h*sizeof(png_byte *));
	
	for(y = 0; y < h; y++) {
		row_pointers[y] = png_malloc(png_ptr, 8*3*w);
		
		memcpy(row_pointers[y], data + w*3*(h-1-y), w*3);
	}
	
	png_init_io(png_ptr, out);
	png_set_rows(png_ptr, info_ptr, row_pointers);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	
	for(y = 0; y < h; y++)
		png_free(png_ptr, row_pointers[y]);
		
	png_free(png_ptr, row_pointers);
	
	png_destroy_write_struct(&png_ptr, &info_ptr);
	
	free(data);
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
			video_toggle_fullscreen();
	}
}

int strendswith(char *s, char *e) {
	int ls = strlen(s);
	int le = strlen(e);
	
	if(le > ls)
		return False;
	
	int i; for(i = 0; i < le; ++i)
		if(s[ls-i-1] != e[le-i-1])
			return False;
	
	return True;
}

char* difficulty_name(Difficulty diff) {
	switch(diff) {
		case D_Easy:	return "Easy";		break;
		case D_Normal:	return "Normal";	break;
		case D_Hard:	return "Hard";		break;
		case D_Lunatic:	return "Lunatic";	break;
		default:		return "Unknown";	break;
	}
}

void stralloc(char **dest, char *src) {
	if(*dest)
		free(*dest);
	*dest = malloc(strlen(src)+1);
	strcpy(*dest, src);
}
