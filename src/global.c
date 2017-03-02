/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "global.h"
#include "video.h"
#include <time.h>
#include <stdio.h>
#include <png.h>
#include "paths/native.h"
#include "resource/resource.h"
#include "taisei_err.h"
#include "replay.h"

Global global;

int getenvint(const char *v) {
	char *e = getenv(v);

	if(e) {
		return atoi(e);
	}

	return 0;
}

void init_global(void) {
	memset(&global, 0, sizeof(global));

	tsrand_init(&global.rand_game, time(0));
	tsrand_init(&global.rand_visual, time(0));

	tsrand_switch(&global.rand_visual);

	memset(&resources, 0, sizeof(Resources));

	memset(&global.replay, 0, sizeof(Replay));
	global.replaymode = REPLAY_RECORD;

	if(global.frameskip = getenvint("TAISEI_SANIC")) {
		if(global.frameskip < 0) {
			global.frameskip = INT_MAX;
		}

		warnx("FPS limiter disabled by environment. Gotta go fast! (frameskip = %i)", global.frameskip);
	}
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
	if(global.frameskip) {
		return;
	}

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
	int w, h, rw, rh;

	w = video.current.width;
	h = video.current.height;

	rw = video.real.width;
	rh = video.real.height;

	data = malloc(3 * rw * rh);

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(outfile, 128, "/taisei_%Y%m%d_%H-%M-%S_%Z.png", timeinfo);

	outpath = malloc(strlen(outfile) + strlen(get_screenshots_path()) + 1);
	strcpy(outpath, get_screenshots_path());
	strcat(outpath, outfile);

	printf("Saving screenshot as %s\n", outpath);
	out = fopen(outpath, "wb");
	free(outpath);

	if(!out) {
		perror("fopen");
		free(data);
		return;
	}

	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, rw, rh, GL_RGB, GL_UNSIGNED_BYTE, data);

	png_structp png_ptr;
    png_infop info_ptr;
	png_byte **row_pointers;

	png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct (png_ptr);

	png_set_IHDR(png_ptr, info_ptr, w, h, 8, PNG_COLOR_TYPE_RGB,
                  PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	row_pointers = png_malloc(png_ptr, h*sizeof(png_byte *));

	for(int y = 0; y < h; y++) {
		row_pointers[y] = png_malloc(png_ptr, 8*3*w);

		memcpy(row_pointers[y], data + rw*3*(h-1-y), w*3);
	}

	png_init_io(png_ptr, out);
	png_set_rows(png_ptr, info_ptr, row_pointers);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	for(int y = 0; y < h; y++)
		png_free(png_ptr, row_pointers[y]);

	png_free(png_ptr, row_pointers);

	png_destroy_write_struct(&png_ptr, &info_ptr);

	free(data);
	fclose(out);
}

bool strendswith(const char *s, const char *e) {
	int ls = strlen(s);
	int le = strlen(e);

	if(le > ls)
		return false;

	int i; for(i = 0; i < le; ++i)
		if(s[ls-i-1] != e[le-i-1])
			return false;

	return true;
}

bool strstartswith(const char *s, const char *p) {
	int ls = strlen(s);
	int lp = strlen(p);

	if(ls < lp)
		return false;

	return !strncmp(s, p, lp);
}

bool strendswith_any(const char *s, const char **earray) {
	for(const char **e = earray; *e; ++e) {
		if(strendswith(s, *e)) {
			return true;
		}
	}

	return false;
}

void stralloc(char **dest, const char *src) {
	free(*dest);

	if(src) {
		*dest = malloc(strlen(src)+1);
		strcpy(*dest, src);
	} else {
		*dest = NULL;
	}
}

char* strjoin(const char *first, ...) {
	va_list args;
	size_t size = strlen(first) + 1;
	char *str = malloc(size);

	strcpy(str, first);
	va_start(args, first);

	for(;;) {
		char *next = va_arg(args, char*);

		if(!next) {
			break;
		}

		size += strlen(next);
		str = realloc(str, size);
		strcat(str, next);
	}

	va_end(args);
	return str;
}

char* read_all(const char *filename, int *outsize) {
	char *text;
	size_t size;

	FILE *file = fopen(filename, "r");
	if(file == NULL)
		errx(-1, "Error opening '%s'", filename);

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);

	text = malloc(size+1);
	fread(text, size, 1, file);
	text[size] = 0;

	fclose(file);

	if(outsize) {
		*outsize = size;
	}

	return text;
}

char* copy_segment(const char *text, const char *delim, int *size) {
	char *seg, *beg, *end;

	beg = strstr(text, delim);
	if(!beg)
		return NULL;
	beg += strlen(delim);

	end = strstr(beg, "%%");
	if(!end)
		return NULL;

	*size = end-beg;
	seg = malloc(*size+1);
	strlcpy(seg, beg, *size+1);

	return seg;
}
// Inputdevice-agnostic method of checking whether a game control is pressed.
// ALWAYS use this instead of SDL_GetKeyState if you need it.
bool gamekeypressed(KeyIndex key) {
	return SDL_GetKeyboardState(NULL)[config_get_int(KEYIDX_TO_CFGIDX(key))] || gamepad_gamekeypressed(key);
}
