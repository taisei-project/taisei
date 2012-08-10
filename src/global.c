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

void init_global(void) {
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
	
	SDL_EnableKeyRepeat(TS_KR_DELAY, TS_KR_INTERVAL);
}

void print_state_checksum(void) {
	int plr, spos = 0, smisc = 0, sargs = 0, proj = 0;
	Player *p = &global.plr;
	Enemy *s;
	Projectile *pr;
	
	plr = creal(p->pos)+cimag(p->pos)+p->focus+p->fire+p->power+p->lifes+p->bombs+p->recovery+p->deathtime+p->continues+p->moveflags;
	
	for(s = global.plr.slaves; s; s = s->next) {
		spos += creal(s->pos + s->pos0) + cimag(s->pos + s->pos0);
		smisc += s->birthtime + s->hp + s->unbombable + s->alpha;
		sargs += cabs(s->args[0]) + cabs(s->args[1]) + cabs(s->args[2]) + cabs(s->args[3]) + s->alpha;
	}
	
	for(pr = global.projs; pr; pr = pr->next)
		proj += cabs(pr->pos + pr->pos0) + pr->birthtime + pr->angle + pr->type + cabs(pr->args[0]) + cabs(pr->args[1]) + cabs(pr->args[2]) + cabs(pr->args[3]);

	printf("[%05d] %d\t(%d\t%d\t%d)\t%d\n", global.frames, plr, spos, smisc, sargs, proj);
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

void set_ortho(void) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, SCREEN_W, SCREEN_H, 0, -100, 100);
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_DEPTH_TEST);
}

void colorfill(float r, float g, float b, float a) {
	if(a <= 0) return;
	
	glColor4f(r,g,b,a);
	
	glPushMatrix();
	glScalef(SCREEN_W,SCREEN_H,1);
	glTranslatef(0.5,0.5,0);
	
	draw_quad();
	glPopMatrix();
	
	glColor4f(1,1,1,1);	
}

void fade_out(float f) {
	colorfill(0, 0, 0, f);
}

double psin(double x) {
	return 0.5 + 0.5 * sin(x);
}

double max(double a, double b) {
	return (a > b)? a : b;
}

double min(double a, double b) {
	return (a < b)? a : b;
}

double clamp(double f, double lower, double upper) {
	if(f < lower)
		return lower;
	if(f > upper)
		return upper;
	
	return f;
}

void take_screenshot(void)
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
		free(data);
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
	} else if(event->type == SDL_QUIT) {
		exit(0);
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
