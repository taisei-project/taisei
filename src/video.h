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
#define VIEWPORT_ASPECT_RATIO (4.0f/3.0f)

typedef struct VideoMode {
	int width;
	int height;
} VideoMode;

typedef struct {
	VideoMode *modes;
	int mcount;
	VideoMode intended;
	VideoMode current;
    SDL_Window *window;
    SDL_GLContext *glcontext;
} Video;

Video video;

void video_init(void);
void video_shutdown(void);
void video_setmode(int w, int h, int fs);
void video_set_viewport(void);
int video_isfullscreen(void);
void video_toggle_fullscreen(void);
void video_resize(int w, int h);

#endif
