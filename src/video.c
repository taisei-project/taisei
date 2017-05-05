/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include <png.h>

#include "global.h"
#include "video.h"
#include "taiseigl.h"

Video video;
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

void video_get_viewport_size(int *width, int *height) {
	float w = video.current.width;
	float h = video.current.height;
	float r = w / h;

	if(r > VIDEO_ASPECT_RATIO) {
		w = h * VIDEO_ASPECT_RATIO;
	} else if(r < VIDEO_ASPECT_RATIO) {
		h = w / VIDEO_ASPECT_RATIO;
	}
	*width = w;
	*height = h;
}


void video_set_viewport(void) {
	int w, h;
	video_get_viewport_size(&w,&h);

	glClear(GL_COLOR_BUFFER_BIT);
	glViewport((video.current.width - w) / 2, (video.current.height - h) / 2, w, h);
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
						log_warn("SDL_GL_SetSwapInterval() failed: %s", SDL_GetError());
					}
				}
			break;
		}
	}
}

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
	LogLevel lvl = LOG_DEBUG;

	switch(type) {
		case GL_DEBUG_TYPE_ERROR: strtype = "error"; lvl = LOG_FATAL; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: strtype = "deprecated"; lvl = LOG_WARN; break;
		case GL_DEBUG_TYPE_PORTABILITY: strtype = "portability"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: strtype = "undefined"; break;
		case GL_DEBUG_TYPE_PERFORMANCE: strtype = "performance"; break;
		case GL_DEBUG_TYPE_OTHER: strtype = "other"; break;
	}

	switch(severity) {
		case GL_DEBUG_SEVERITY_LOW: strsev = "low"; break;
		case GL_DEBUG_SEVERITY_MEDIUM: strsev = "medium"; break;
		case GL_DEBUG_SEVERITY_HIGH: strsev = "high"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: strsev = "notify"; break;
	}

	if(type == GL_DEBUG_TYPE_OTHER && severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
		// too verbose, spits a message every time some text is drawn
		return;
	}

	log_custom(lvl, "[OpenGL debug, %s, %s] %i: %s", strtype, strsev, id, message);
}

static void APIENTRY video_gl_debug_enable(void) {
	GLuint unused = 0;
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(video_gl_debug, NULL);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unused, true);
	log_info("Enabled OpenGL debugging");
}
#endif

static void video_init_gl(void) {
	video.glcontext = SDL_GL_CreateContext(video.window);

	load_gl_functions();
	check_gl_extensions();

#ifdef DEBUG_GL
	if(glext.debug_output) {
		video_gl_debug_enable();
	} else {
		log_warn("OpenGL debugging is not supported by the implementation");
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

static void video_update_quality(void) {
	int vw, vh;
	video_get_viewport_size(&vw,&vh);
	float q = (float)vh / SCREEN_H;

	float fg = q * config_get_float(CONFIG_FG_QUALITY);
	float bg = q * config_get_float(CONFIG_BG_QUALITY);
	float text = q * config_get_float(CONFIG_TEXT_QUALITY);

	log_debug("q:%f, fg:%f, bg:%f, text:%f", q, fg, bg, text);

	reinit_fbo(&resources.fbo.bg[0], bg);
	reinit_fbo(&resources.fbo.bg[1], bg);
	reinit_fbo(&resources.fbo.fg[0], fg);
	reinit_fbo(&resources.fbo.fg[1], fg);

	reload_fonts(text);
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
			log_fatal("Error creating OpenGL context: %s", SDL_GetError());
			return;
		}

		SDL_ShowCursor(false);
		video_update_vsync();
		SDL_GL_GetDrawableSize(video.window, &video.current.width, &video.current.height);
		video.real.width = video.current.width;
		video.real.height = video.current.height;
		video_set_viewport();
		video_update_quality();
		return;
	}

	if(fallback) {
		log_fatal("Error opening screen: %s", SDL_GetError());
		return;
	}

	log_warn("Setting %dx%d (%s) failed, falling back to %dx%d (windowed)", w, h,
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
	} else if(resizable) {
		flags |= SDL_WINDOW_RESIZABLE;
	}

	_video_setmode(w, h, flags, false);

	log_info("Changed mode to %ix%i%s%s",
		video.current.width,
		video.current.height,
		video_isfullscreen() ? ", fullscreen" : ", windowed",
		video_isresizable() ? ", resizable" : ""
	);
}

void video_take_screenshot(void) {
	SDL_RWops *out;
	char *data;
	char outfile[128], *outpath;
	time_t rawtime;
	struct tm * timeinfo;
	int w, h, rw, rh;

	w = video.current.width;
	h = video.current.height;

	rw = video.real.width;
	rh = video.real.height;

	rw = max(rw, w);
	rh = max(rh, h);

	data = malloc(3 * rw * rh);

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(outfile, 128, "/taisei_%Y%m%d_%H-%M-%S_%Z.png", timeinfo);

	outpath = strjoin(get_screenshots_path(), outfile, NULL);
	log_info("Saving screenshot as %s", outpath);
	out = SDL_RWFromFile(outpath, "w");
	free(outpath);

	if(!out) {
		log_warn("SDL_RWFromFile() failed: %s", SDL_GetError());
		free(data);
		return;
	}

	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, rw, rh, GL_RGB, GL_UNSIGNED_BYTE, data);

	png_structp png_ptr;
    png_infop info_ptr;
	png_byte **row_pointers;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_setup_error_handlers(png_ptr);
	info_ptr = png_create_info_struct(png_ptr);

	png_set_IHDR(png_ptr, info_ptr, w, h, 8, PNG_COLOR_TYPE_RGB,
                  PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	row_pointers = png_malloc(png_ptr, h*sizeof(png_byte *));

	for(int y = 0; y < h; y++) {
		row_pointers[y] = png_malloc(png_ptr, 8*3*w);

		memcpy(row_pointers[y], data + rw*3*(h-1-y), w*3);
	}

	png_init_rwops_write(png_ptr, out);
	png_set_rows(png_ptr, info_ptr, row_pointers);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	for(int y = 0; y < h; y++)
		png_free(png_ptr, row_pointers[y]);

	png_free(png_ptr, row_pointers);

	png_destroy_write_struct(&png_ptr, &info_ptr);

	free(data);
	SDL_RWclose(out);
}

bool video_isresizable(void) {
	return SDL_GetWindowFlags(video.window) & SDL_WINDOW_RESIZABLE;
}

bool video_isfullscreen(void) {
	return SDL_GetWindowFlags(video.window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void video_set_fullscreen(bool fullscreen) {
	video_setmode(video.intended.width, video.intended.height, fullscreen, config_get_int(CONFIG_VID_RESIZABLE));
}

void video_toggle_fullscreen(void) {
	video_set_fullscreen(!video_isfullscreen());
}

void video_resize(int w, int h) {
	video.current.width = w;
	video.current.height = h;
	video_set_viewport();
	video_update_quality();
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

static void video_quality_callback(ConfigIndex idx, ConfigValue v) {
	config_set_float(idx, v.f);
	video_update_quality();
}

static void video_init_sdl(void) {
	char *force_driver = getenv("TAISEI_VIDEO_DRIVER");

	if(force_driver && *force_driver) {
		log_debug("Driver '%s' forced by environment", force_driver);

		if(!strcasecmp(force_driver, "sdldefault")) {
			force_driver = NULL;
		}

		if(SDL_VideoInit(force_driver)) {
			log_fatal("SDL_VideoInit() failed: %s", SDL_GetError());
		}

		return;
	}

	// Initialize the SDL video subsystem with the correct driver
	// On Wayland/Mir systems, an X compatibility layer will likely be present
	// SDL will prioritize it by default, and we don't want that

	int drivers = SDL_GetNumVideoDrivers();
	bool initialized = false;

	for(int i = 0; i < drivers; ++i) {
		const char *driver = SDL_GetVideoDriver(i);
		if(!strcmp(driver, "x11") || !strcmp(driver, "dummy")) {
			continue;
		}

		if(SDL_VideoInit(driver)) {
			log_debug("Driver '%s' failed: %s", driver, SDL_GetError());
		} else {
			initialized = true;
			break;
		}
	}

	if(!initialized) {
		log_debug("Falling back to the default driver");

		if(SDL_VideoInit(NULL)) {
			log_fatal("SDL_VideoInit() failed: %s", SDL_GetError());
		}
	}
}

void video_init(void) {
	bool fullscreen_available = false;

	memset(&video, 0, sizeof(video));
	memset(&resources.fbo, 0, sizeof(resources.fbo));

	video_init_sdl();
	log_info("Using driver '%s'", SDL_GetCurrentVideoDriver());

	// Register all resolutions that are available in fullscreen

	for(int s = 0; s < SDL_GetNumVideoDisplays(); ++s) {
		for(int i = 0; i < SDL_GetNumDisplayModes(s); ++i) {
			SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };

			if(SDL_GetDisplayMode(s, i, &mode) != 0) {
				log_warn("SDL_GetDisplayMode() failed: %s", SDL_GetError());
			} else {
				video_add_mode(mode.w, mode.h);
				fullscreen_available = true;
			}
		}
	}

	if(!fullscreen_available) {
		log_warn("No available fullscreen modes");
		config_set_int(CONFIG_FULLSCREEN, false);
	}

	// Then, add some common 4:3 modes for the windowed mode if they are not there yet.
	// This is required for some multihead setups.
	for(int i = 0; common_modes[i].width; ++i) {
		video_add_mode(common_modes[i].width, common_modes[i].height);
	}

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
	config_set_callback(CONFIG_FG_QUALITY, video_quality_callback);
	config_set_callback(CONFIG_BG_QUALITY, video_quality_callback);
	config_set_callback(CONFIG_TEXT_QUALITY, video_quality_callback);

	log_info("Video subsystem initialized");
}

void video_shutdown(void) {
	SDL_DestroyWindow(video.window);
	SDL_GL_DeleteContext(video.glcontext);
	unload_gl_library();
	free(video.modes);
	SDL_VideoQuit();
}
