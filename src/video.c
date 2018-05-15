/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <png.h>

#include "global.h"
#include "video.h"
#include "renderer/api.h"
#include "util/pngcruft.h"

Video video;

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
	r_viewport((video.current.width - w) / 2, (video.current.height - h) / 2, w, h);
}

static void video_update_vsync(void) {
	if(global.frameskip || config_get_int(CONFIG_VSYNC) == 0) {
		r_vsync(VSYNC_NONE);
	} else {
		switch(config_get_int(CONFIG_VSYNC)) {
			case 1:  r_vsync(VSYNC_NORMAL);   break;
			default: r_vsync(VSYNC_ADAPTIVE); break;
		}
	}
}

static void video_update_quality(void) {
	int vw, vh;
	video_get_viewport_size(&vw, &vh);
	float q = (float)vh / SCREEN_H;
	video.quality_factor = q;

	float fg = q * config_get_float(CONFIG_FG_QUALITY);
	float bg = q * config_get_float(CONFIG_BG_QUALITY);
	float text = q * config_get_float(CONFIG_TEXT_QUALITY);

	log_debug("q:%f, fg:%f, bg:%f, text:%f", q, fg, bg, text);

	init_fbo_pair(&resources.fbo_pairs.bg, bg, TEX_TYPE_RGB);
	init_fbo_pair(&resources.fbo_pairs.fg, fg, TEX_TYPE_RGB);
	init_fbo_pair(&resources.fbo_pairs.rgba, fg, TEX_TYPE_RGBA);

	reload_fonts(text);
}

static uint32_t get_fullscreen_flag(void) {
	if(config_get_int(CONFIG_FULLSCREEN_DESKTOP)) {
		return SDL_WINDOW_FULLSCREEN_DESKTOP;
	} else {
		return SDL_WINDOW_FULLSCREEN;
	}
}

static void video_check_fullscreen_sanity(void) {
	SDL_DisplayMode mode;

	if(SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(video.window), &mode)) {
		log_warn("SDL_GetCurrentDisplayMode failed: %s", SDL_GetError());
		return;
	}

	if(video.current.width != mode.w || video.current.height != mode.h) {
		log_warn("BUG: window is not actually fullscreen after modesetting. Video mode: %ix%i, window size: %ix%i",
			mode.w, mode.h, video.current.width, video.current.height);
	}
}

static void video_update_mode_settings(void) {
	SDL_ShowCursor(false);
	video_update_vsync();
	SDL_GetWindowSize(video.window, &video.current.width, &video.current.height);
	video.real.width = video.current.width;
	video.real.height = video.current.height;
	video_set_viewport();
	video_update_quality();
	events_emit(TE_VIDEO_MODE_CHANGED, 0, NULL, NULL);

	if(video_is_fullscreen() && !config_get_int(CONFIG_FULLSCREEN_DESKTOP)) {
		video_check_fullscreen_sanity();
	}
}

static const char* modeflagsstr(uint32_t flags) {
	if(WINFLAGS_IS_FAKE_FULLSCREEN(flags)) {
		return "fake fullscreen";
	} else if(WINFLAGS_IS_REAL_FULLSCREEN(flags)) {
		return "true fullscreen";
	} else if(flags & SDL_WINDOW_RESIZABLE) {
		return "windowed, resizable";
	} else {
		return "windowed";
	}
}

static void video_new_window_internal(int w, int h, uint32_t flags, bool fallback) {
	if(video.window) {
		SDL_DestroyWindow(video.window);
		video.window = NULL;
	}

	char title[sizeof(WINDOW_TITLE) + strlen(TAISEI_VERSION) + 2];
	snprintf(title, sizeof(title), "%s v%s", WINDOW_TITLE, TAISEI_VERSION);
	video.window = r_create_window(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, flags);

	if(video.window) {
		video_update_mode_settings();
		return;
	}

	if(fallback) {
		log_fatal("Failed to create window with mode %ix%i (%s): %s", w, h, modeflagsstr(flags), SDL_GetError());
		return;
	}

	video_new_window_internal(RESX, RESY, flags & ~SDL_WINDOW_FULLSCREEN_DESKTOP, true);
}

static void video_new_window(int w, int h, bool fs, bool resizable) {
	uint32_t flags = 0;

	if(fs) {
		flags |= get_fullscreen_flag();
	} else if(resizable) {
		flags |= SDL_WINDOW_RESIZABLE;
	}

	video_new_window_internal(w, h, flags, false);

	log_info("Created a new window: %ix%i (%s)",
		video.current.width,
		video.current.height,
		modeflagsstr(SDL_GetWindowFlags(video.window))
	);

	events_pause_keyrepeat();
	SDL_RaiseWindow(video.window);
}

static bool video_set_display_mode(int w, int h) {
	SDL_DisplayMode closest, target = { .w = w, .h = h };
	int display = SDL_GetWindowDisplayIndex(video.window);

	if(!SDL_GetClosestDisplayMode(display, &target, &closest)) {
		log_warn("No available display modes for %ix%i on display %i", w, h, display);
		return false;
	}

	if(closest.w != w || closest.h != h) {
		log_warn("Can't use %ix%i, closest available is %ix%i", w, h, closest.w, closest.h);
	}

	if(SDL_SetWindowDisplayMode(video.window, &closest)) {
		log_warn("Failed to set display mode for %ix%i on display %i: %s", closest.w, closest.h, display, SDL_GetError());
		return false;
	}

	return true;
}

static void video_set_fullscreen(bool fullscreen) {
	uint32_t flags = fullscreen ? get_fullscreen_flag() : 0;
	events_pause_keyrepeat();

	if(SDL_SetWindowFullscreen(video.window, flags) < 0) {
		log_warn("Failed to switch to %s mode: %s", modeflagsstr(flags), SDL_GetError());
	}

	SDL_RaiseWindow(video.window);
}

void video_set_mode(int w, int h, bool fs, bool resizable) {
	video.intended.width = w;
	video.intended.height = h;

	if(!video.window) {
		video_new_window(w, h, fs, resizable);
		return;
	}

	if(w != video.current.width || h != video.current.height) {
		if(fs && !config_get_int(CONFIG_FULLSCREEN_DESKTOP)) {
			video_set_display_mode(w, h);
			video_set_fullscreen(fs);
			video_update_mode_settings();
		} else {
			// XXX: I would like to use SDL_SetWindowSize for size changes, but apparently it's impossible to reliably detect
			//      when it fails to actually resize the window. For example, a tiling WM (awesome) may be getting in its way
			//      and we'd never know. SDL_GL_GetDrawableSize/SDL_GetWindowSize aren't helping as of SDL 2.0.5.
			//
			//      There's not much to be done about it. We're at mercy of SDL here and SDL is at mercy of the WM.
			video_new_window(w, h, fs, resizable);
			return;
		}
	}

	video_set_fullscreen(fs);
	SDL_SetWindowResizable(video.window, resizable);
}

void video_take_screenshot(void) {
	SDL_RWops *out;
	char outfile[128], *outpath, *syspath;
	time_t rawtime;
	struct tm * timeinfo;
	uint width, height;
	uint8_t *pixels = r_screenshot(&width, &height);

	if(!pixels) {
		log_warn("Failed to take a screenshot");
		return;
	}

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(outfile, 128, "taisei_%Y%m%d_%H-%M-%S%z.png", timeinfo);

	outpath = strjoin("storage/screenshots/", outfile, NULL);
	syspath = vfs_repr(outpath, true);
	log_info("Saving screenshot as %s", syspath);
	free(syspath);

	out = vfs_open(outpath, VFS_MODE_WRITE);
	free(outpath);

	if(!out) {
		log_warn("VFS error: %s", vfs_get_error());
		free(pixels);
		return;
	}

	png_structp png_ptr;
	png_infop info_ptr;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_setup_error_handlers(png_ptr);
	info_ptr = png_create_info_struct(png_ptr);

	png_set_IHDR(
		png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT
	);

	png_byte *row_pointers[height];

	for(int y = 0; y < height; y++) {
		row_pointers[y] = png_malloc(png_ptr, width * 3);
		memcpy(row_pointers[y], pixels + width * 3 * (height - 1 - y), width * 3);
	}

	png_init_rwops_write(png_ptr, out);
	png_set_rows(png_ptr, info_ptr, row_pointers);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	for(int y = 0; y < height; y++) {
		png_free(png_ptr, row_pointers[y]);
	}

	png_destroy_write_struct(&png_ptr, &info_ptr);

	free(pixels);
	SDL_RWclose(out);
}

bool video_is_resizable(void) {
	return SDL_GetWindowFlags(video.window) & SDL_WINDOW_RESIZABLE;
}

bool video_is_fullscreen(void) {
	return WINFLAGS_IS_FULLSCREEN(SDL_GetWindowFlags(video.window));
}

bool video_can_change_resolution(void) {
	return !video_is_fullscreen() || !config_get_int(CONFIG_FULLSCREEN_DESKTOP);
}

static void video_cfg_fullscreen_callback(ConfigIndex idx, ConfigValue v) {
	video_set_mode(
		config_get_int(CONFIG_VID_WIDTH),
		config_get_int(CONFIG_VID_HEIGHT),
		config_set_int(idx, v.i),
		config_get_int(CONFIG_VID_RESIZABLE)
	);
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
	// XXX: workaround for an SDL bug: https://bugzilla.libsdl.org/show_bug.cgi?id=4127
	SDL_SetHintWithPriority(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0", SDL_HINT_OVERRIDE);

	uint num_drivers = SDL_GetNumVideoDrivers();
	void *buf;
	SDL_RWops *out = SDL_RWAutoBuffer(&buf, 256);

	SDL_RWprintf(out, "Available video drivers:");

	for(uint i = 0; i < num_drivers; ++i) {
		SDL_RWprintf(out, " %s", SDL_GetVideoDriver(i));
	}

	SDL_WriteU8(out, 0);
	log_info("%s", (char*)buf);
	SDL_RWclose(out);

	const char *prefer_drivers = env_get("TAISEI_PREFER_SDL_VIDEODRIVERS", "");
	const char *force_driver = env_get("TAISEI_VIDEO_DRIVER", "");

	if(*force_driver) {
		log_warn("TAISEI_VIDEO_DRIVER is deprecated and will be removed, use TAISEI_PREFER_SDL_VIDEODRIVERS or SDL_VIDEODRIVER instead");
	} else {
		force_driver = env_get("SDL_VIDEODRIVER", "");
	}

	if(!(prefer_drivers && *prefer_drivers)) {
		// https://bugzilla.libsdl.org/show_bug.cgi?id=3948
		// A suboptimal X11 server may be available on top of those systems,
		// so we push X11 down in the priority list.
		prefer_drivers = "wayland,mir,cocoa,windows,x11";
	}

	if(prefer_drivers && *prefer_drivers && !*force_driver) {
		char buf[strlen(prefer_drivers) + 1];
		char *driver, *bufptr = buf;
		int drivernum = 0;
		strcpy(buf, prefer_drivers);

		while((driver = strtok_r(NULL, " :;,", &bufptr))) {
			++drivernum;
			log_info("Trying preferred driver #%i: %s", drivernum, driver);

			if(SDL_VideoInit(driver)) {
				log_info("Driver '%s' failed: %s", driver, SDL_GetError());
			} else {
				return;
			}
		}

		if(drivernum) {
			log_info("All preferred drivers failed, falling back to SDL default");
		}
	}

	if(SDL_VideoInit(force_driver)) {
		log_fatal("SDL_VideoInit() failed: %s", SDL_GetError());
	}
}

static void video_handle_resize(int w, int h) {
	video.current.width = w;
	video.current.height = h;
	video_set_viewport();
	video_update_quality();
	events_emit(TE_VIDEO_MODE_CHANGED, 0, NULL, NULL);
}

static bool video_handle_window_event(SDL_Event *event, void *arg) {
	switch(event->window.event) {
		case SDL_WINDOWEVENT_RESIZED:
			video_handle_resize(event->window.data1, event->window.data2);
			break;

		case SDL_WINDOWEVENT_FOCUS_LOST:
			if(config_get_int(CONFIG_FOCUS_LOSS_PAUSE)) {
				events_emit(TE_GAME_PAUSE, 0, NULL, NULL);
			}
			break;
	}

	return true;
}

void video_init(void) {
	bool fullscreen_available = false;

	memset(&video, 0, sizeof(video));
	memset(&resources.fbo_pairs, 0, sizeof(resources.fbo_pairs));

	video_init_sdl();
	log_info("Using driver '%s'", SDL_GetCurrentVideoDriver());

	r_init();

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

	video_set_mode(
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

	EventHandler h = {
		.proc = video_handle_window_event,
		.priority = EPRIO_SYSTEM,
		.event_type = SDL_WINDOWEVENT,
	};

	events_register_handler(&h);
	log_info("Video subsystem initialized");
}

void video_shutdown(void) {
	SDL_DestroyWindow(video.window);
	r_shutdown();
	free(video.modes);
	SDL_VideoQuit();
	events_unregister_handler(video_handle_window_event);
}

void video_swap_buffers(void) {
	r_target(NULL);
	r_swap(video.window);
	r_clear(CLEAR_COLOR);
}
