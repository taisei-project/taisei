/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include "global.h"
#include "video.h"
#include "taisei_err.h"
#include <stdlib.h>

static VideoMode common_modes[] = {
	{RESX, RESY},
	
	{640, 480},
	{800, 600},
	{1024, 768},
	{1280, 960},
	{1152, 864},
	{1400, 1050},
	{1440, 1080},
	
	{0, 0},
};

static void video_add_mode(int width, int height) {
	if(video.modes) {
		int i; for(i = 0; i < video.mcount; ++i) {
			VideoMode *m = &(video.modes[i]);
			if(m->width == width && m->height == height)
				return;
		}
	}
	
	video.modes = (VideoMode*)realloc(video.modes, (++video.mcount) * sizeof(VideoMode));
	video.modes[video.mcount-1].width  = width;
	video.modes[video.mcount-1].height = height;
}

static int video_compare_modes(const void *a, const void *b) {
	VideoMode *va = (VideoMode*)a;
	VideoMode *vb = (VideoMode*)b;
	return va->width * va->height - vb->width * vb->height;
}

static void _video_setmode(int w, int h, int fs, int fallback) {
	int flags = SDL_OPENGL;
	if(fs) flags |= SDL_FULLSCREEN;
	
	if(!fallback) {
		video.intended.width = w;
		video.intended.height = h;
	}
	
	if(display)
		SDL_FreeSurface(display);
	
	if(!(display = SDL_SetVideoMode(w, h, 32, flags))) {
		if(fallback) {
			errx(-1, "video_setmode(): error opening screen: %s", SDL_GetError());
			return;
		}
		
		warnx("video_setmode(): setting %dx%d failed, falling back to %dx%d", w, h, RESX, RESY);
		_video_setmode(RESX, RESY, fs, True);
	}
	
	const SDL_VideoInfo *info = SDL_GetVideoInfo();
	video.current.width = info->current_w;
	video.current.height = info->current_h;
	
	glViewport(0, 0, video.current.width, video.current.height);
}

void video_setmode(int w, int h, int fs) {
	_video_setmode(w, h, fs, False);
}

int video_isfullscreen(void) {
	return !!(display->flags & SDL_FULLSCREEN);
}

void video_toggle_fullscreen(void) {
	video_setmode(video.intended.width, video.intended.height, !video_isfullscreen());
}

void video_init(void) {
	SDL_Rect **modes;
	int i;
	
	memset(&video, 0, sizeof(video));
	
	// First register all resolutions that are available in fullscreen
	modes = SDL_ListModes(NULL, SDL_FULLSCREEN | SDL_HWSURFACE);
	if(!modes) {
		warnx("video_init(): no available fullscreen modes");
		tconfig.intval[FULLSCREEN] = False;
	} else if(modes == (SDL_Rect**)-1) {
		warnx("video_init(): you seem to have a weird video driver");
	} else {
		for(i = 0; modes[i]; ++i) {
			video_add_mode(modes[i]->w, modes[i]->h);
		}
	}
	
	// Then, add some common 4:3 modes for the windowed mode if they are not there yet.
	// This is required for some multihead setups.
	for(i = 0; common_modes[i].width; ++i)
		video_add_mode(common_modes[i].width, common_modes[i].height);
	
	// sort it, mainly for the options menu
	qsort(video.modes, video.mcount, sizeof(VideoMode), video_compare_modes);
	
	for(i = 0; i < video.mcount; ++i) {
		printf(" +++ %dx%d\n", video.modes[i].width, video.modes[i].height);
	}
	
	video_setmode(tconfig.intval[VID_WIDTH], tconfig.intval[VID_HEIGHT], tconfig.intval[FULLSCREEN]);
	SDL_WM_SetCaption("TaiseiProject", NULL); 
}

void video_shutdown(void) {
	SDL_FreeSurface(display);
	free(video.modes);
}
