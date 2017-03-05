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
#ifdef DEBUG
	#define DEBUG_GL
#endif

#ifdef DEBUG_GL
static void APIENTRY video_gl_debug(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar *message,
	const void *arg
) {
	char *strtype = "unknown";
	char *strsev = "unknown";

	switch(type) {
		case GL_DEBUG_TYPE_ERROR: strtype = "error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: strtype = "deprecated"; break;
		case GL_DEBUG_TYPE_PORTABILITY: strtype = "portability"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: strtype = "undefined"; break;
		case GL_DEBUG_TYPE_PERFORMANCE: strtype = "performance"; break;
		case GL_DEBUG_TYPE_OTHER: strtype = "other"; break;
	}

	switch(severity) {
		case GL_DEBUG_SEVERITY_LOW: strsev = "low"; break;
		case GL_DEBUG_SEVERITY_MEDIUM: strsev = "medium"; break;
		case GL_DEBUG_SEVERITY_HIGH: strsev = "high"; break;
	}

	warnx("[OpenGL debug, %s, %s] %i: %s", strtype, strsev, id, message);
}

static void APIENTRY video_gl_debug_enable(void) {
	GLuint unused = 0;
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(video_gl_debug, NULL);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unused, true);
}
#endif

static void video_init_gl(void) {
	video.glcontext = SDL_GL_CreateContext(video.window);

	load_gl_functions();
	check_gl_extensions();

#ifdef DEBUG_GL
	if(glext.version.major >= 4 && glext.version.minor >= 3) {
		video_gl_debug_enable();
	} else {
		warnx("Can't enable debugging, OpenGL context is too old (>=4.3 required, %i.%i current)",
				glext.version.major, glext.version.minor);
	}
#endif

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	init_quadvbo();

	glClear(GL_COLOR_BUFFER_BIT);
}

static void _video_setmode(int w, int h, uint32_t flags, bool fallback) {
	if(!libgl_loaded) {
		load_gl_library();
		libgl_loaded = true;
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

#ifdef DEBUG_GL
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

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

		SDL_ShowCursor(false);
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

	warnx("video_setmode(): setting %dx%d (%s) failed, falling back to %dx%d (windowed)", w, h,
			(flags & SDL_WINDOW_FULLSCREEN) ? "fullscreen" : "windowed", RESX, RESY);
	_video_setmode(RESX, RESY, flags & ~SDL_WINDOW_FULLSCREEN, true);
}

void video_setmode(int w, int h, bool fs, bool resizable) {
	if(	w == video.current.width &&
		h == video.current.height &&
		fs == video_isfullscreen() &&
		resizable == video_isresizable()
	) return;

	uint32_t flags = SDL_WINDOW_OPENGL;

	if(fs) {
		flags |= SDL_WINDOW_FULLSCREEN;
	} else if(flags & SDL_WINDOW_RESIZABLE) {
		flags |= SDL_WINDOW_RESIZABLE;
	}

	_video_setmode(w, h, flags, false);
}

bool video_isresizable(void) {
	return SDL_GetWindowFlags(video.window) & SDL_WINDOW_RESIZABLE;
}

bool video_isfullscreen(void) {
	return SDL_GetWindowFlags(video.window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void video_set_fullscreen(bool fullscreen) {
	video_setmode(video.intended.width, video.intended.height, fullscreen, video_isresizable());
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
	video.intended.width = config_get_int(CONFIG_VID_WIDTH);
	video.intended.height = config_get_int(CONFIG_VID_HEIGHT);
	video_set_fullscreen(config_set_int(idx, v.i));
}

static void video_cfg_vsync_callback(ConfigIndex idx, ConfigValue v) {
	config_set_int(idx, v.i);
	video_update_vsync();
}

static void video_cfg_resizable_callback(ConfigIndex idx, ConfigValue v) {
	SDL_SetWindowResizable(video.window, config_set_int(idx, v.i));
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

	video_setmode(
		config_get_int(CONFIG_VID_WIDTH),
		config_get_int(CONFIG_VID_HEIGHT),
		config_get_int(CONFIG_FULLSCREEN),
		config_get_int(CONFIG_VID_RESIZABLE)
	);

	config_set_callback(CONFIG_FULLSCREEN, video_cfg_fullscreen_callback);
	config_set_callback(CONFIG_VSYNC, video_cfg_vsync_callback);
	config_set_callback(CONFIG_VID_RESIZABLE, video_cfg_resizable_callback);
}

void video_shutdown(void) {
	SDL_DestroyWindow(video.window);
	SDL_GL_DeleteContext(video.glcontext);
	unload_gl_library();
	free(video.modes);
}
