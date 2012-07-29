/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#ifndef VIDEO_H
#define VIDEO_H

typedef struct VideoMode {
	unsigned int width;
	unsigned int height;
} VideoMode;

typedef struct {
	VideoMode *modes;
	int mcount;
	VideoMode intended;
	VideoMode current;
} Video;

Video video;
SDL_Surface *display;

void video_init(void);
void video_shutdown(void);
void video_setmode(int w, int h, int fs);
int video_isfullscreen(void);
void video_toggle_fullscreen(void);

#endif
