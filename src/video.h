/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef VIDEO_H
#define VIDEO_H

#define WINDOW_TITLE "TaiseiProject"
#define VIDEO_ASPECT_RATIO ((double)SCREEN_W/SCREEN_H)

#include <SDL.h>
#include <stdbool.h>

#define WINFLAGS_IS_FULLSCREEN(f)       ((f) & SDL_WINDOW_FULLSCREEN_DESKTOP)
#define WINFLAGS_IS_FAKE_FULLSCREEN(f)  ((f) & SDL_WINDOW_FULLSCREEN_DESKTOP == SDL_WINDOW_FULLSCREEN_DESKTOP)
#define WINFLAGS_IS_REAL_FULLSCREEN(f)  ((f) & SDL_WINDOW_FULLSCREEN_DESKTOP == SDL_WINDOW_FULLSCREEN)

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
void video_set_mode(int w, int h, bool fs, bool resizable);
void video_get_viewport_size(int *width, int *height);
void video_set_viewport(void);
bool video_isfullscreen(void);
bool video_isresizable(void);
bool video_can_change_resolution(void);
void video_set_fullscreen(bool);
void video_toggle_fullscreen(void);
void video_resize(int w, int h);
void video_update_vsync(void);
void video_take_screenshot(void);

#endif
