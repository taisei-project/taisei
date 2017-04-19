/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#ifndef VIDEO_H
#define VIDEO_H

#define WINDOW_TITLE "TaiseiProject"
#define VIDEO_ASPECT_RATIO ((double)SCREEN_W/SCREEN_H)

#include <SDL.h>
#include <stdbool.h>

typedef struct VideoMode {
	int width;
	int height;
} VideoMode;

typedef struct {
	VideoMode *modes;
	int mcount;
	VideoMode intended;
	VideoMode current;
	VideoMode real;
	SDL_Window *window;
	SDL_GLContext *glcontext;
} Video;

extern Video video;

void video_init(void);
void video_shutdown(void);
void video_setmode(int w, int h, bool fs, bool resizable);
void video_get_viewport_size(int *width, int *height);
void video_set_viewport(void);
bool video_isfullscreen(void);
bool video_isresizable(void);
void video_set_fullscreen(bool);
void video_toggle_fullscreen(void);
void video_resize(int w, int h);
void video_update_vsync(void);
void video_take_screenshot(void);

#endif
