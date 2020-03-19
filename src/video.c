/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include <png.h>

#include "global.h"
#include "video.h"
#include "renderer/api.h"
#include "util/pngcruft.h"
#include "util/fbmgr.h"
#include "taskmanager.h"
#include "video_postprocess.h"

typedef struct VideoModeArray {
	VideoMode *mode;
	uint mcount;
} VideoModeArray;

static struct {
	VideoModeArray fs_modes;
	VideoModeArray win_modes;
	SDL_Window *window;
	VideoMode intended;
	VideoMode current;
	VideoBackend backend;
} video;

typedef struct ScreenshotTaskData {
	char *dest_path;
	Pixmap image;
} ScreenshotTaskData;

static VideoPostProcess *v_postprocess;

VideoCapabilityState (*video_query_capability)(VideoCapability cap);

static VideoCapabilityState video_query_capability_generic(VideoCapability cap) {
	switch(cap) {
		case VIDEO_CAP_FULLSCREEN:
			return VIDEO_AVAILABLE;

		case VIDEO_CAP_EXTERNAL_RESIZE:
			return video_is_fullscreen() ? VIDEO_CURRENTLY_UNAVAILABLE : VIDEO_AVAILABLE;

		case VIDEO_CAP_CHANGE_RESOLUTION:
			if(video_is_fullscreen() && config_get_int(CONFIG_FULLSCREEN_DESKTOP)) {
				return VIDEO_CURRENTLY_UNAVAILABLE;
			} else {
				return VIDEO_AVAILABLE;
			}

		case VIDEO_CAP_VSYNC_ADAPTIVE:
			return VIDEO_AVAILABLE;
	}

	UNREACHABLE;
}

static VideoCapabilityState video_query_capability_alwaysfullscreen(VideoCapability cap) {
	switch(cap) {
		case VIDEO_CAP_FULLSCREEN:
			return VIDEO_ALWAYS_ENABLED;

		case VIDEO_CAP_EXTERNAL_RESIZE:
			return VIDEO_NEVER_AVAILABLE;

		// XXX: Might not be actually working, but let's be optimistic.
		case VIDEO_CAP_CHANGE_RESOLUTION:
			return VIDEO_AVAILABLE;

		case VIDEO_CAP_VSYNC_ADAPTIVE:
			return VIDEO_AVAILABLE;
	}

	UNREACHABLE;
}

static VideoCapabilityState video_query_capability_webcanvas(VideoCapability cap) {
	switch(cap) {
		case VIDEO_CAP_EXTERNAL_RESIZE:
			return VIDEO_NEVER_AVAILABLE;

		case VIDEO_CAP_VSYNC_ADAPTIVE:
			return VIDEO_NEVER_AVAILABLE;

		default:
			return video_query_capability_generic(cap);
	}
}

static void video_add_mode_handler(VideoModeArray *mode_array, int width, int height) {
	if(mode_array->mode) {
		for(uint i = 0; i < mode_array->mcount; ++i) {
			VideoMode *m = mode_array->mode + i;

			if(m->width == width && m->height == height) {
				return;
			}
		}
	}

	mode_array->mode = realloc(mode_array->mode, (++mode_array->mcount) * sizeof(VideoMode));

	mode_array->mode[mode_array->mcount-1].width  = width;
	mode_array->mode[mode_array->mcount-1].height = height;
}

static void video_add_mode(int width, int height, bool fullscreen) {
	if(fullscreen) {
		video_add_mode_handler(&video.fs_modes, width, height);
	} else {
		video_add_mode_handler(&video.win_modes, width, height);
	}
}

static int video_compare_modes(const void *a, const void *b) {
	VideoMode *va = (VideoMode*)a;
	VideoMode *vb = (VideoMode*)b;
	return va->width * va->height - vb->width * vb->height;
}

void video_get_viewport_size(float *width, float *height) {
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

void video_get_viewport(FloatRect *vp) {
	video_get_viewport_size(&vp->w, &vp->h);
	vp->x = (int)((video.current.width  - vp->w) / 2);
	vp->y = (int)((video.current.height - vp->h) / 2);
}

static void video_set_viewport(void) {
	FloatRect vp;
	video_get_viewport(&vp);
	r_framebuffer_viewport_rect(NULL, vp);
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
		log_sdl_error(LOG_WARN, "SDL_GetCurrentDisplayMode");
		return;
	}

	if(video.current.width != mode.w || video.current.height != mode.h) {
		log_error("BUG: window is not actually fullscreen after modesetting. Video mode: %ix%i, window size: %ix%i",
			mode.w, mode.h, video.current.width, video.current.height);
	}
}

static void video_update_mode_settings(void) {
	SDL_ShowCursor(false);
	video_update_vsync();
	SDL_GetWindowSize(video.window, &video.current.width, &video.current.height);
	video_set_viewport();
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

static void video_new_window_internal(uint display, uint w, uint h, uint32_t flags, bool fallback) {
	if(video.window) {
		SDL_DestroyWindow(video.window);
		video.window = NULL;
	}

	char title[sizeof(WINDOW_TITLE) + strlen(TAISEI_VERSION) + 2];
	snprintf(title, sizeof(title), "%s v%s", WINDOW_TITLE, TAISEI_VERSION);
	video.window = r_create_window(
		title,
		SDL_WINDOWPOS_CENTERED_DISPLAY(display),
		SDL_WINDOWPOS_CENTERED_DISPLAY(display),
		w, h, flags | SDL_WINDOW_HIDDEN
	);

	if(video.window) {
		SDL_ShowWindow(video.window);
		SDL_SetWindowMinimumSize(video.window, SCREEN_W / 4, SCREEN_H / 4);
		video_update_mode_settings();
		return;
	}

	if(fallback) {
		log_fatal("Failed to create window with mode %ix%i (%s): %s", w, h, modeflagsstr(flags), SDL_GetError());
		return;
	}

	video_new_window_internal(display, RESX, RESY, flags & ~SDL_WINDOW_FULLSCREEN_DESKTOP, true);
}

static void video_new_window(uint display, uint w, uint h, bool fs, bool resizable) {
	uint32_t flags = 0;

	if(fs) {
		flags |= get_fullscreen_flag();
	} else if(resizable && video.backend != VIDEO_BACKEND_EMSCRIPTEN) {
		flags |= SDL_WINDOW_RESIZABLE;
	}

	video_new_window_internal(display, w, h, flags, false);
	display = video_current_display();

	log_info("Created a new window: %ix%i (%s), on display #%i %s",
		video.current.width,
		video.current.height,
		modeflagsstr(SDL_GetWindowFlags(video.window)),
		display,
		video_display_name(display)
	);

	events_pause_keyrepeat();
	SDL_RaiseWindow(video.window);
}

static bool video_set_display_mode(uint display, uint w, uint h) {
	SDL_DisplayMode closest, target = { .w = w, .h = h };

	if(!SDL_GetClosestDisplayMode(display, &target, &closest)) {
		log_error("No available display modes for %ix%i on display %i", w, h, display);
		return false;
	}

	if(closest.w != w || closest.h != h) {
		log_warn("Can't use %ix%i, closest available is %ix%i", w, h, closest.w, closest.h);
	}

	if(SDL_SetWindowDisplayMode(video.window, &closest)) {
		log_error("Failed to set display mode for %ix%i on display %i: %s", closest.w, closest.h, display, SDL_GetError());
		return false;
	}

	return true;
}

static void video_set_fullscreen_internal(bool fullscreen) {
	uint32_t flags = fullscreen ? get_fullscreen_flag() : 0;
	events_pause_keyrepeat();

	if(SDL_SetWindowFullscreen(video.window, flags) < 0) {
		log_error("Failed to switch to %s mode: %s", modeflagsstr(flags), SDL_GetError());
	}

	SDL_RaiseWindow(video.window);
}

void video_set_mode(uint display, uint w, uint h, bool fs, bool resizable) {
	video.intended.width = w;
	video.intended.height = h;

	if(display >= video_num_displays()) {
		log_warn("Display index %u is invalid, falling back to 0 (%s)", display, video_display_name(0));
		display = 0;
	}

	if(!video.window) {
		video_new_window(display, w, h, fs, resizable);
		return;
	}

	uint old_display = video_current_display();
	bool display_changed = display != old_display;
	bool size_changed = w != video.current.width || h != video.current.height;

	if(display_changed) {
		video_new_window(display, w, h, fs, resizable);
		return;
	}

	if(size_changed) {
		if(fs && !config_get_int(CONFIG_FULLSCREEN_DESKTOP)) {
			video_set_display_mode(display, w, h);
		} else if(video.backend == VIDEO_BACKEND_X11) {
			// XXX: I would like to use SDL_SetWindowSize for size changes, but apparently it's impossible to reliably detect
			//      when it fails to actually resize the window. For example, a tiling WM (awesome) may be getting in its way
			//      and we'd never know. SDL_GL_GetDrawableSize/SDL_GetWindowSize aren't helping as of SDL 2.0.5.
			//
			//      There's not much to be done about it. We're at mercy of SDL here and SDL is at mercy of the WM.
			video_new_window(display, w, h, fs, resizable);
			return;
		} else if(video.backend == VIDEO_BACKEND_EMSCRIPTEN && !fs) {
			// Needed to work around various SDL bugs and HTML/DOM quirks...
			video_new_window(display, w, h, fs, resizable);
			return;
		} else {
			SDL_SetWindowSize(video.window, w, h);
		}
	}

	video_set_fullscreen_internal(fs);
	SDL_SetWindowResizable(video.window, resizable);

	if(size_changed) {
		video_update_mode_settings();
	}
}

void video_set_fullscreen(bool fullscreen) {
	video_set_mode(
		SDL_GetWindowDisplayIndex(video.window),
		video.intended.width,
		video.intended.height,
		fullscreen,
		config_get_int(CONFIG_VID_RESIZABLE)
	);
}

void video_set_display(uint idx) {
	video_set_mode(
		idx,
		video.intended.width,
		video.intended.height,
		config_get_int(CONFIG_FULLSCREEN),
		config_get_int(CONFIG_VID_RESIZABLE)
	);
}

static void* video_screenshot_task(void *arg) {
	ScreenshotTaskData *tdata = arg;

	pixmap_convert_inplace_realloc(&tdata->image, PIXMAP_FORMAT_RGB8);
	pixmap_flip_to_origin_inplace(&tdata->image, PIXMAP_ORIGIN_BOTTOMLEFT);

	uint width = tdata->image.width;
	uint height = tdata->image.height;
	uint8_t *pixels = tdata->image.data.untyped;

	SDL_RWops *output = vfs_open(tdata->dest_path, VFS_MODE_WRITE);

	if(!output) {
		log_error("VFS error: %s", vfs_get_error());
		return NULL;
	}

	char *syspath = vfs_repr(tdata->dest_path, true);
	log_info("Saving screenshot as %s", syspath);
	free(syspath);

	const char *error = NULL;
	png_structp png_ptr;
	png_infop info_ptr;

	png_byte *volatile row_pointers[height];
	memset((void*)row_pointers, 0, sizeof(row_pointers));

	png_ptr = pngutil_create_write_struct();

	if(png_ptr == NULL) {
		error = "pngutil_create_write_struct() failed";
		goto done;
	}

	info_ptr = png_create_info_struct(png_ptr);

	if(info_ptr == NULL) {
		error = "png_create_info_struct() failed";
		goto done;
	}

	if(setjmp(png_jmpbuf(png_ptr))) {
		error = "PNG error";
		goto done;
	}

	png_set_IHDR(
		png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT
	);

	for(int y = 0; y < height; y++) {
		row_pointers[y] = png_malloc(png_ptr, width * 3);
		memcpy(row_pointers[y], pixels + width * 3 * (height - 1 - y), width * 3);
	}

	pngutil_init_rwops_write(png_ptr, output);
	png_set_rows(png_ptr, info_ptr, (void*)row_pointers);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

done:
	if(error) {
		log_error("Couldn't save screenshot: %s", error);
	}

	for(int y = 0; y < height; y++) {
		png_free(png_ptr, row_pointers[y]);
	}

	if(png_ptr != NULL) {
		png_destroy_write_struct(&png_ptr, info_ptr ? &info_ptr : NULL);
	}

	SDL_RWclose(output);
	return NULL;
}

static void video_screenshot_free_task_data(void *arg) {
	ScreenshotTaskData *tdata = arg;
	free(tdata->image.data.untyped);
	free(tdata->dest_path);
	free(tdata);
}

void video_take_screenshot(void) {
	ScreenshotTaskData tdata;
	memset(&tdata, 0, sizeof(tdata));

	if(!r_screenshot(&tdata.image)) {
		log_error("Failed to take a screenshot");
		return;
	}

	SystemTime systime;
	char timestamp[FILENAME_TIMESTAMP_MIN_BUF_SIZE];
	get_system_time(&systime);
	filename_timestamp(timestamp, sizeof(timestamp), systime);
	tdata.dest_path = strfmt("storage/screenshots/taisei_%s.png", timestamp);

	task_detach(taskmgr_global_submit((TaskParams) {
		.callback = video_screenshot_task,
		.userdata = memdup(&tdata, sizeof(tdata)),
		.userdata_free_callback = video_screenshot_free_task_data,
	}));
}

bool video_is_resizable(void) {
	return SDL_GetWindowFlags(video.window) & SDL_WINDOW_RESIZABLE;
}

bool video_is_fullscreen(void) {
	return WINFLAGS_IS_FULLSCREEN(SDL_GetWindowFlags(video.window));
}

static void video_init_sdl(void) {
	// XXX: workaround for an SDL bug: https://bugzilla.libsdl.org/show_bug.cgi?id=4127
	SDL_SetHintWithPriority(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0", SDL_HINT_OVERRIDE);

	uint num_drivers = SDL_GetNumVideoDrivers();
	const char *video_drivers[num_drivers];

	void *buf;
	SDL_RWops *out = SDL_RWAutoBuffer(&buf, 256);

	SDL_RWprintf(out, "Available video drivers:");

	for(uint i = 0; i < num_drivers; ++i) {
		video_drivers[i] = SDL_GetVideoDriver(i);
		SDL_RWprintf(out, " %s", video_drivers[i]);
	}

	SDL_WriteU8(out, 0);
	log_info("%s", (char*)buf);
	SDL_RWclose(out);

	// https://bugzilla.libsdl.org/show_bug.cgi?id=3948
	// A suboptimal X11 server may be available on top of those systems,
	// so we push X11 down in the priority list.
	const char *prefer_drivers = env_get_string_nonempty("TAISEI_PREFER_SDL_VIDEODRIVERS", "wayland,cocoa,windows,x11");
	const char *force_driver = env_get_string_nonempty("TAISEI_VIDEO_DRIVER", NULL);

	if(force_driver) {
		log_warn("TAISEI_VIDEO_DRIVER is deprecated and will be removed, use TAISEI_PREFER_SDL_VIDEODRIVERS or SDL_VIDEODRIVER instead");
	} else {
		force_driver = env_get_string_nonempty("SDL_VIDEODRIVER", NULL);
	}

	if(prefer_drivers && *prefer_drivers && !force_driver) {
		char buf[strlen(prefer_drivers) + 1];
		char *driver, *bufptr = buf;
		int drivernum = 0;
		strcpy(buf, prefer_drivers);

		while((driver = strtok_r(NULL, " :;,", &bufptr))) {
			bool skip = true;

			for(uint i = 0; i < num_drivers; ++i) {
				if(!strcmp(video_drivers[i], driver)) {
					skip = false;
					break;
				}
			}

			++drivernum;

			if(skip) {
				continue;
			}

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
	int minw, minh;
	SDL_GetWindowMinimumSize(video.window, &minw, &minh);

	if(w < minw || h < minh) {
		log_warn("Bad resize: %ix%i is too small!", w, h);
		// FIXME: the video_new_window is actually a workaround for Wayland.
		// I'm not sure if it's necessary for anything else.
		video_new_window(video_current_display(), video.intended.width, video.intended.height, false, video_is_resizable());
		return;
	}

	log_debug("%ix%i --> %ix%i", video.current.width, video.current.height, w, h);
	video.current.width = w;
	video.current.height = h;
	video_set_viewport();
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

static bool video_handle_config_event(SDL_Event *evt, void *arg) {
	ConfigValue *val = evt->user.data1;

	switch(evt->user.code) {
		case CONFIG_FULLSCREEN:
			video_set_fullscreen(val->i);
			break;

		case CONFIG_VID_DISPLAY:
			video_set_display(val->i);
			break;

		case CONFIG_VID_RESIZABLE:
			SDL_SetWindowResizable(video.window, val->i);
			break;

		case CONFIG_VSYNC:
			video_update_vsync();
			break;
	}

	return false;
}

uint video_num_displays(void) {
	int displays = SDL_GetNumVideoDisplays();

	if(displays < 1) {
		if(displays == 0) {
			log_warn("SDL_GetNumVideoDisplays() returned 0, this shouldn't happen");
		} else {
			log_sdl_error(LOG_WARN, "SDL_GetNumVideoDisplays");
		}

		displays = 1;
	}

	return displays;
}

const char *video_display_name(uint id) {
	assert(id < video_num_displays());

	const char *name = SDL_GetDisplayName(id);

	if(name == NULL) {
		log_sdl_error(LOG_WARN, "SDL_GetDisplayName");
		name = "Unknown";
	}

	return name;
}

uint video_current_display(void) {
	int display = SDL_GetWindowDisplayIndex(video.window);

	if(display < 0) {
		log_sdl_error(LOG_WARN, "SDL_GetWindowDisplayIndex");
		display = 1;
	}

	return display;
}

void video_init(void) {
	bool fullscreen_available = false;

	video_init_sdl();

	const char *driver = SDL_GetCurrentVideoDriver();
	log_info("Using driver '%s'", driver);

	video_query_capability = video_query_capability_generic;

	if(!strcmp(driver, "x11")) {
		video.backend = VIDEO_BACKEND_X11;
	} else if(!strcmp(driver, "emscripten")) {
		video.backend = VIDEO_BACKEND_EMSCRIPTEN;
		video_query_capability = video_query_capability_webcanvas;

		// We can not start in fullscreen in the browser properly, so disable it here.
		// Fullscreen is still accessible via the settings menu and the shortcut key.
		config_set_int(CONFIG_FULLSCREEN, false);
	} else if(!strcmp(driver, "KMSDRM")) {
		video.backend = VIDEO_BACKEND_KMSDRM;
		video_query_capability = video_query_capability_alwaysfullscreen;
	} else if(!strcmp(driver, "RPI")) {
		video.backend = VIDEO_BACKEND_RPI;
		video_query_capability = video_query_capability_alwaysfullscreen;
	} else if(!strcmp(driver, "Switch")) {
		video.backend = VIDEO_BACKEND_SWITCH;
		video_query_capability = video_query_capability_alwaysfullscreen;
	} else {
		video.backend = VIDEO_BACKEND_OTHER;
	}

	r_init();

	// Register all resolutions that are available in fullscreen
	for(int s = 0; s < video_num_displays(); ++s) {
		log_info("Found display #%i: %s", s, video_display_name(s));
		for(int i = 0; i < SDL_GetNumDisplayModes(s); ++i) {
			SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };

			if(SDL_GetDisplayMode(s, i, &mode) != 0) {
				log_sdl_error(LOG_WARN, "SDL_GetDisplayMode");
			} else {
				video_add_mode(mode.w, mode.h, true);
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
	VideoMode common_modes[] = {
		{RESX, RESY},
		{SCREEN_W, SCREEN_H},

		{640, 480},
		{800, 600},
		{1024, 768},
		{1280, 960},
		{1152, 864},
		{1400, 1050},
		{1440, 1080},
		{0, 0},
	};

	if(video_query_capability(VIDEO_CAP_FULLSCREEN) != VIDEO_ALWAYS_ENABLED) {
		for(int i = 0; common_modes[i].width; ++i) {
			video_add_mode(common_modes[i].width, common_modes[i].height, false);
		}
	}

	// sort it, mainly for the options menu
	qsort(video.fs_modes.mode, video.fs_modes.mcount, sizeof(VideoMode *), video_compare_modes);
	qsort(video.win_modes.mode, video.win_modes.mcount, sizeof(VideoMode *), video_compare_modes);

	video_set_mode(
		config_get_int(CONFIG_VID_DISPLAY),
		config_get_int(CONFIG_VID_WIDTH),
		config_get_int(CONFIG_VID_HEIGHT),
		config_get_int(CONFIG_FULLSCREEN),
		config_get_int(CONFIG_VID_RESIZABLE)
	);

	events_register_handler(&(EventHandler) {
		.proc = video_handle_window_event,
		.priority = EPRIO_SYSTEM,
		.event_type = SDL_WINDOWEVENT,
	});

	events_register_handler(&(EventHandler) {
		.proc = video_handle_config_event,
		.priority = EPRIO_SYSTEM,
		.event_type = MAKE_TAISEI_EVENT(TE_CONFIG_UPDATED),
	});

	log_info("Video subsystem initialized");
}

void video_post_init(void) {
	fbmgr_init();
	v_postprocess = video_postprocess_init();
	r_framebuffer(video_get_screen_framebuffer());
}

void video_shutdown(void) {
	video_postprocess_shutdown(v_postprocess);
	fbmgr_shutdown();
	events_unregister_handler(video_handle_window_event);
	events_unregister_handler(video_handle_config_event);
	SDL_DestroyWindow(video.window);
	r_shutdown();
	free(video.win_modes.mode);
	free(video.fs_modes.mode);
	SDL_VideoQuit();
}

Framebuffer *video_get_screen_framebuffer(void) {
	return video_postprocess_get_framebuffer(v_postprocess);
}

void video_swap_buffers(void) {
	Framebuffer *pp_fb = video_postprocess_render(v_postprocess);

	if(pp_fb) {
		r_flush_sprites();

		Framebuffer *prev_fb = r_framebuffer_current();
		r_framebuffer(NULL);

		r_state_push();
		r_mat_proj_push_ortho(SCREEN_W, SCREEN_H);
		r_shader_standard();
		r_color3(1, 1, 1);
		draw_framebuffer_tex(pp_fb, SCREEN_W, SCREEN_H);
		r_framebuffer_clear(pp_fb, CLEAR_ALL, RGBA(0, 0, 0, 0), 1);
		r_mat_proj_pop();
		r_state_pop();

		r_swap(video.window);
		r_framebuffer(prev_fb);
	} else {
		r_swap(video.window);
	}

	// XXX: Unfortunately, there seems to be no reliable way to sync this up with events
	config_set_int(CONFIG_FULLSCREEN, video_is_fullscreen());
}

VideoBackend video_get_backend(void) {
	return video.backend;
}

VideoMode video_get_mode(uint idx, bool fullscreen) {
	if(fullscreen) {
		assert(idx < video.fs_modes.mcount);
		return video.fs_modes.mode[idx];
	}

	assert(idx < video.win_modes.mcount);
	return video.win_modes.mode[idx];
}

uint video_get_num_modes(bool fullscreen) {
	if(fullscreen) {
		return video.fs_modes.mcount;
	}

	return video.win_modes.mcount;
}

VideoMode video_get_current_mode(void) {
	return video.current;
}
