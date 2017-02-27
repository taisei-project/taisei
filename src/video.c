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
#include "taiseigl.h"

static bool libgl_loaded = false;

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

void video_set_viewport(void) {
	float w = video.current.width;
	float h = video.current.height;
	float r = w / h;

	if(r > VIEWPORT_ASPECT_RATIO) {
		w = h * VIEWPORT_ASPECT_RATIO;
	} else if(r < VIEWPORT_ASPECT_RATIO) {
		h = w / VIEWPORT_ASPECT_RATIO;
	}

	glClear(GL_COLOR_BUFFER_BIT);
	glViewport((video.current.width - w) / 2, (video.current.height - h) / 2, (int)w, (int)h);
}

void video_update_vsync(void) {
	if(global.frameskip || config_get_int(CONFIG_VSYNC) == 1) {
		SDL_GL_SetSwapInterval(0);
	} else {
		switch (config_get_int(CONFIG_VSYNC)) {
			case 2: // adaptive
				if(SDL_GL_SetSwapInterval(-1) < 0) {
			case 0: // on
					if(SDL_GL_SetSwapInterval(1) < 0) {
						warnx("Couldn't enable vsync: %s", SDL_GetError());
					}
				}
			break;
		}
	}
}

static void video_init_gl(void) {
	video.glcontext = SDL_GL_CreateContext(video.window);

	load_gl_functions();
	check_gl_extensions();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	init_quadvbo();

	glClear(GL_COLOR_BUFFER_BIT);
}

static void _video_setmode(int w, int h, int fs, int fallback) {
	Uint32 flags = SDL_WINDOW_OPENGL;

	if(!libgl_loaded) {
		load_gl_library();
		libgl_loaded = true;
	}

	if(fs) {
		flags |= SDL_WINDOW_FULLSCREEN;
	} else {
		flags |= SDL_WINDOW_RESIZABLE;
	}

	if(!fallback) {
		video.intended.width = w;
		video.intended.height = h;
	}

	if(video.window) {
		SDL_DestroyWindow(video.window);
		video.window = NULL;
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	video.window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, flags);

	if(video.window) {
		if(video.glcontext) {
			SDL_GL_MakeCurrent(video.window, video.glcontext);
		} else {
			video_init_gl();
		}

		if(!video.glcontext) {
			errx(-1, "video_setmode(): error creating OpenGL context: %s", SDL_GetError());
			return;
		}

		video_update_vsync();
		SDL_GL_GetDrawableSize(video.window, &video.current.width, &video.current.height);
		video.real.width = video.current.width;
		video.real.height = video.current.height;
		video_set_viewport();
		return;
	}

	if(fallback) {
		errx(-1, "video_setmode(): error opening screen: %s", SDL_GetError());
		return;
	}

	warnx("video_setmode(): setting %dx%d (%s) failed, falling back to %dx%d (windowed)", w, h, fs ? "fullscreen" : "windowed", RESX, RESY);
	_video_setmode(RESX, RESY, false, true);
}

void video_setmode(int w, int h, int fs) {
	if(w == video.current.width && h == video.current.height && fs == video_isfullscreen())
		return;

	_video_setmode(w, h, fs, false);
}

int video_isfullscreen(void) {
	return !!(SDL_GetWindowFlags(video.window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP));
}

void video_set_fullscreen(bool fullscreen) {
	video_setmode(video.intended.width, video.intended.height, fullscreen);
}

void video_toggle_fullscreen(void) {
	video_set_fullscreen(!video_isfullscreen());
}

void video_resize(int w, int h) {
	video.current.width = w;
	video.current.height = h;
	video_set_viewport();
}

static void video_cfg_fullscreen_callback(ConfigIndex idx, ConfigValue v) {
	video_set_fullscreen(config_set_int(idx, v.i));
}

static void video_cfg_vsync_callback(ConfigIndex idx, ConfigValue v) {
	config_set_int(idx, v.i);
	video_update_vsync();
}

void video_init(void) {
	int i, s;
	bool fullscreen_available = false;

	memset(&video, 0, sizeof(video));

	// First, register all resolutions that are available in fullscreen

	for(s = 0; s < SDL_GetNumVideoDisplays(); ++s) {
		for(i = 0; i < SDL_GetNumDisplayModes(s); ++i) {
			SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };

			if(SDL_GetDisplayMode(s, i, &mode) != 0) {
				warnx("SDL_GetDisplayMode failed: %s", SDL_GetError());
			} else {
				video_add_mode(mode.w, mode.h);
				fullscreen_available = true;
			}
		}
	}

	if(!fullscreen_available) {
		warnx("video_init(): no available fullscreen modes");
		config_set_int(CONFIG_FULLSCREEN, false);
	}

	// Then, add some common 4:3 modes for the windowed mode if they are not there yet.
	// This is required for some multihead setups.
	for(i = 0; common_modes[i].width; ++i)
		video_add_mode(common_modes[i].width, common_modes[i].height);

	// sort it, mainly for the options menu
	qsort(video.modes, video.mcount, sizeof(VideoMode), video_compare_modes);

	video_setmode(config_get_int(CONFIG_VID_WIDTH), config_get_int(CONFIG_VID_HEIGHT), config_get_int(CONFIG_FULLSCREEN));

	config_set_callback(CONFIG_FULLSCREEN, video_cfg_fullscreen_callback);
	config_set_callback(CONFIG_VSYNC, video_cfg_vsync_callback);
}

void video_shutdown(void) {
	SDL_DestroyWindow(video.window);
	SDL_GL_DeleteContext(video.glcontext);
	free(video.modes);
}
