/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "video.h"

#include "dynarray.h"
#include "events.h"
#include "global.h"
#include "renderer/api.h"
#include "stagedraw.h"
#include "taskmanager.h"
#include "util/env.h"
#include "util/fbmgr.h"
#include "util/graphics.h"
#include "version.h"
#include "video_postprocess.h"

typedef DYNAMIC_ARRAY(VideoMode) VideoModeArray;

typedef enum FramedumpSource {
	FRAMEDUMP_SRC_SCREEN,
	FRAMEDUMP_SRC_VIEWPORT,
} FramedumpSource;

static struct {
	VideoModeArray fs_modes;
	VideoModeArray win_modes;
	SDL_Window *window;
	VideoPostProcess *postprocess;
	VideoMode intended;
	VideoMode current;
	VideoBackend backend;
	double scaling_factor;

	struct {
		char *name_prefix;
		size_t name_prefix_len;
		size_t frame_count;
		int compression;
		FramedumpSource source;
	} framedump;

	SDL_DisplayID *displays;
	int num_displays;
} video;

#define FRAMEDUMP_FILENAME_SUFFIX ".png"
#define FRAMEDUMP_FILENAME_NUM_DIGITS 8
#define FRAMEDUMP_FILENAME_EXTRA_BUFSIZE \
	(FRAMEDUMP_FILENAME_NUM_DIGITS + sizeof(FRAMEDUMP_FILENAME_SUFFIX))
#define FRAMEDUMP_FILENAME_FORMAT "%08u" FRAMEDUMP_FILENAME_SUFFIX

VideoCapabilityState (*video_query_capability)(VideoCapability cap);

typedef struct ScreenshotTaskData {
	char *dest_path;     // NULL if in framedump mode
	Pixmap image;
	uint32_t frame_num;  // for framedump mode only
} ScreenshotTaskData;

#define VIDEO_MIN_SIZE_FACTOR 0.8
#define VIDEO_MIN_WIDTH       (int)(SCREEN_W * VIDEO_MIN_SIZE_FACTOR)
#define VIDEO_MIN_HEIGHT      (int)(SCREEN_H * VIDEO_MIN_SIZE_FACTOR)

/*
 * BEGIN Conversion between screen-space and pixel-space coordinates (for high-DPI mode)
 * TODO: figure out how to round these correctly
 */

attr_unused static inline int coords_val_screen_to_pixels(int screen_coord) {
	return round(screen_coord * video.scaling_factor);
}

attr_unused static inline int coords_val_pixels_to_screen(int pixel_coord) {
	return round(pixel_coord / video.scaling_factor);
}

attr_unused static inline IntOffset coords_ofs_screen_to_pixels(IntOffset screen_ofs) {
	IntOffset pixel_ofs;
	pixel_ofs.x = coords_val_screen_to_pixels(screen_ofs.x);
	pixel_ofs.y = coords_val_screen_to_pixels(screen_ofs.y);
	return pixel_ofs;
}

attr_unused static inline IntOffset coords_ofs_pixels_to_screen(IntOffset pixel_ofs) {
	IntOffset screen_ofs;
	screen_ofs.x = coords_val_pixels_to_screen(pixel_ofs.x);
	screen_ofs.y = coords_val_pixels_to_screen(pixel_ofs.y);
	return screen_ofs;
}

attr_unused static inline IntExtent coords_ext_screen_to_pixels(IntExtent screen_ext) {
	IntExtent pixel_ext;
	pixel_ext.w = coords_val_screen_to_pixels(screen_ext.w);
	pixel_ext.h = coords_val_screen_to_pixels(screen_ext.h);
	return pixel_ext;
}

attr_unused static inline IntExtent coords_ext_pixels_to_screen(IntExtent pixel_ext) {
	IntExtent screen_ext;
	screen_ext.w = coords_val_pixels_to_screen(pixel_ext.w);
	screen_ext.h = coords_val_pixels_to_screen(pixel_ext.h);
	return screen_ext;
}

attr_unused static inline IntRect coords_rect_screen_to_pixels(IntRect screen_rect) {
	IntRect pixel_rect;
	pixel_rect.extent = coords_ext_screen_to_pixels(screen_rect.extent);
	pixel_rect.offset = coords_ofs_screen_to_pixels(screen_rect.offset);
	return pixel_rect;
}

attr_unused static inline IntRect coords_rect_pixels_to_screen(IntRect pixel_rect) {
	IntRect screen_rect;
	screen_rect.extent = coords_ext_pixels_to_screen(pixel_rect.extent);
	screen_rect.offset = coords_ofs_pixels_to_screen(pixel_rect.offset);
	return screen_rect;
}

/*
 * END Conversion between screen-space and pixel-space coordinates (for high-DPI mode)
 */

static VideoCapabilityState video_query_capability_generic(VideoCapability cap) {
	switch(cap) {
		case VIDEO_CAP_FULLSCREEN:
			return VIDEO_AVAILABLE;

		case VIDEO_CAP_EXTERNAL_RESIZE:
			return video_is_fullscreen() ? VIDEO_CURRENTLY_UNAVAILABLE : VIDEO_AVAILABLE;

		case VIDEO_CAP_CHANGE_RESOLUTION:
			return video_is_fullscreen() ? VIDEO_CURRENTLY_UNAVAILABLE : VIDEO_AVAILABLE;

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

static VideoCapabilityState video_query_capability_switch(VideoCapability cap) {
	switch(cap) {
		// We want the window to be resizable and resized internally by SDL
		// when the Switch gets docked/undocked
		case VIDEO_CAP_FULLSCREEN:
			return VIDEO_NEVER_AVAILABLE;

		case VIDEO_CAP_EXTERNAL_RESIZE:
			return VIDEO_AVAILABLE;

		case VIDEO_CAP_CHANGE_RESOLUTION:
			return VIDEO_NEVER_AVAILABLE;

		case VIDEO_CAP_VSYNC_ADAPTIVE:
			return VIDEO_NEVER_AVAILABLE;
	}

	UNREACHABLE;
}

static VideoCapabilityState video_query_capability_webcanvas(VideoCapability cap) {
	switch(cap) {
		case VIDEO_CAP_EXTERNAL_RESIZE:
			return VIDEO_ALWAYS_ENABLED;

		case VIDEO_CAP_FULLSCREEN:
			return VIDEO_NEVER_AVAILABLE;

		case VIDEO_CAP_CHANGE_RESOLUTION:
			return VIDEO_NEVER_AVAILABLE;

		case VIDEO_CAP_VSYNC_ADAPTIVE:
			return VIDEO_NEVER_AVAILABLE;

		default:
			return video_query_capability_generic(cap);
	}
}

static VideoCapabilityState (*video_query_capability_kiosk_fallback)(VideoCapability cap);

static VideoCapabilityState video_query_capability_kiosk(VideoCapability cap) {
	switch(cap) {
		case VIDEO_CAP_FULLSCREEN:
			return VIDEO_ALWAYS_ENABLED;

		case VIDEO_CAP_CHANGE_RESOLUTION:
			return VIDEO_NEVER_AVAILABLE;

		case VIDEO_CAP_EXTERNAL_RESIZE:
			return VIDEO_NEVER_AVAILABLE;

		default:
			return video_query_capability_kiosk_fallback(cap);
	}
}

static void video_add_mode(VideoModeArray *mode_array, IntExtent mode_screen, IntExtent min_screen, IntExtent max_screen, const char *mode_type) {
	if(
		(mode_screen.w > max_screen.w && max_screen.w > 0) ||
		(mode_screen.h > max_screen.h && max_screen.h > 0)
	) {
		log_debug("Mode %ix%i rejected: > %ix%i", mode_screen.w, mode_screen.h, max_screen.w, max_screen.h);
		return;
	}

	if(
		mode_screen.w < min_screen.w ||
		mode_screen.h < min_screen.h
	) {
		log_debug("Mode %ix%i rejected: < %ix%i", mode_screen.w, mode_screen.h, min_screen.w, min_screen.h);
		return;
	}

	for(uint i = 0; i < mode_array->num_elements; ++i) {
		VideoMode *m = mode_array->data + i;

		if(m->width == mode_screen.w && m->height == mode_screen.h) {
			log_debug("Mode %ix%i rejected: already registered", mode_screen.w, mode_screen.h);
			return;
		}
	}

	dynarray_append(mode_array, { .as_int_extent = mode_screen });
	log_debug("Add %s mode: %ix%i", mode_type, mode_screen.w, mode_screen.h);
}

static void video_add_mode_dpi_aware(VideoModeArray *mode_array, IntExtent mode_pix, IntExtent min_screen, IntExtent max_screen, const char *mode_type) {
	IntExtent mode_screen = coords_ext_pixels_to_screen(mode_pix);

	// Yes, we add both. The pixel-space size is interpreted as screen-space.
	// Anything too large to fit on the screen is rejected.
	video_add_mode(mode_array, mode_screen, min_screen, max_screen, mode_type);
	video_add_mode(mode_array, mode_pix, min_screen, max_screen, mode_type);
}

static void video_add_mode_fullscreen(IntExtent mode_pix, IntExtent min_screen, IntExtent max_screen) {
	video_add_mode_dpi_aware(&video.fs_modes, mode_pix, min_screen, max_screen, "fullscreen");
}

static void video_add_mode_windowed(IntExtent mode_pix, IntExtent min_screen, IntExtent max_screen) {
	video_add_mode_dpi_aware(&video.win_modes, mode_pix, min_screen, max_screen, "windowed");
}

static int video_compare_modes(const void *a, const void *b) {
	const VideoMode *va = a;
	const VideoMode *vb = b;
	return va->width * va->height - vb->width * vb->height;
}

static IntExtent video_get_screen_framebuffer_size(void) {
	IntExtent s;
	SDL_GetWindowSizeInPixels(video.window, &s.w, &s.h);
	return s;
}

static FloatExtent video_get_viewport_size_for_framebuffer(IntExtent framebuffer_size) {
	float w = framebuffer_size.w;
	float h = framebuffer_size.h;
	float r = w / h;

	if(r > VIDEO_ASPECT_RATIO) {
		w = h * VIDEO_ASPECT_RATIO;
	} else if(r < VIDEO_ASPECT_RATIO) {
		h = w / VIDEO_ASPECT_RATIO;
	}

	return (FloatExtent) { w, h };
}

static void video_get_viewport_for_framebuffer(FloatRect *vp, IntExtent framebuffer_size) {
	vp->extent = video_get_viewport_size_for_framebuffer(framebuffer_size);
	vp->x = (int)((framebuffer_size.w - vp->w) * 0.5);
	vp->y = (int)((framebuffer_size.h - vp->h) * 0.5);
}

void video_get_viewport_size(float *width, float *height) {
	IntExtent fb = video_get_screen_framebuffer_size();
	FloatExtent vp = video_get_viewport_size_for_framebuffer(fb);

	*width = vp.w;
	*height = vp.h;
}

void video_get_viewport(FloatRect *vp) {
	IntExtent fb = video_get_screen_framebuffer_size();
	video_get_viewport_for_framebuffer(vp, fb);
	log_debug("current w/h: %dx%d", video.current.width, video.current.height);
	log_debug("viewport x/y: %fx%f", vp->x, vp->y);
	log_debug("viewport w/h: %fx%f", vp->w, vp->h);
}

static void video_set_viewport(void) {
	FloatRect vp;
	video_get_viewport(&vp);
	r_framebuffer_viewport_rect(NULL, vp);
}

static IntExtent round_viewport_size(FloatExtent vp) {
	return (IntExtent) { round(vp.w), round(vp.h) };
}

static void video_update_mode_lists(void) {
	video.fs_modes.num_elements = 0;
	video.win_modes.num_elements = 0;

	dynarray_ensure_capacity(&video.fs_modes, 16);
	dynarray_ensure_capacity(&video.win_modes, 16);

	bool fullscreen_available = false;
	bool has_windowed_modes = (video_query_capability(VIDEO_CAP_FULLSCREEN) != VIDEO_ALWAYS_ENABLED);
	FloatExtent largest_fullscreen_viewport = { 0, 0 };

	IntExtent screenspace_min_size = { VIDEO_MIN_WIDTH, VIDEO_MIN_HEIGHT };
	screenspace_min_size = coords_ext_pixels_to_screen(screenspace_min_size);

	// Register all resolutions that are available in fullscreen and their corresponding windowed modes.
	for(int s = 0; s < video_num_displays(); ++s) {
		SDL_DisplayID display = video.displays[s];
		log_info("Found display #%i: %s", s, video_display_name(s));

		const SDL_DisplayMode *desktop_mode;
		IntExtent screenspace_max_size = { 0 };

		if(!(desktop_mode = SDL_GetDesktopDisplayMode(display))) {
			log_sdl_error(LOG_WARN, "SDL_GetDesktopDisplayMode");
		} else {
			log_debug("Desktop mode: %ix%i@%gHz, scale: %g",
				desktop_mode->w, desktop_mode->h,
				desktop_mode->refresh_rate, desktop_mode->pixel_density
			);
			screenspace_max_size.w = desktop_mode->w;
			screenspace_max_size.h = desktop_mode->h;
		}

		int mcount;
		SDL_DisplayMode **modes = SDL_GetFullscreenDisplayModes(display, &mcount);

		for(int i = 0; i < mcount; ++i) {
			const SDL_DisplayMode *mode = modes[i];

			log_debug("Display mode #%i: %ix%i@%gHz; scale = %g", i,
				mode->w, mode->h, mode->refresh_rate, mode->pixel_density);

			video_add_mode_fullscreen((IntExtent) { mode->w, mode->h }, screenspace_min_size, screenspace_max_size);
			fullscreen_available = true;

			if(has_windowed_modes) {
				FloatExtent vp = video_get_viewport_size_for_framebuffer(
					(IntExtent) { mode->w, mode->h });
				video_add_mode_windowed(
					round_viewport_size(vp), screenspace_min_size, screenspace_max_size);

				// the ratio is always constant, so we need to check only 1 dimension
				if(vp.w > largest_fullscreen_viewport.w) {
					largest_fullscreen_viewport = vp;
				}
			}
		}

		SDL_free(modes);
	}

	if(has_windowed_modes) {
		// Insert some more windowed modes derived from our "ideal" resolution.
		// This is the resolution that the assets are optimized for.
		float ideal_factor = 2;
		FloatExtent ideal_resolution = { SCREEN_W * ideal_factor, SCREEN_H * ideal_factor };

		if(largest_fullscreen_viewport.w == 0) {
			// no way to determine the upper bound; guess it
			largest_fullscreen_viewport = ideal_resolution;
		}

		IntExtent screenspace_max_size = coords_ext_pixels_to_screen(round_viewport_size(largest_fullscreen_viewport));

		float scaling_factor = 0.5;
		float scaling_factor_step = 0.25;

		while(ideal_resolution.w * scaling_factor <= largest_fullscreen_viewport.w) {
			FloatExtent vp = {
				ideal_resolution.w * scaling_factor,
				ideal_resolution.h * scaling_factor,
			};
			IntExtent pix_vp = round_viewport_size(vp);
			video_add_mode_windowed(pix_vp, screenspace_min_size, screenspace_max_size);
			scaling_factor += scaling_factor_step;
		}

		// Finally add the worst size we will tolerate, in case we haven't already.
		video_add_mode_windowed((IntExtent) { VIDEO_MIN_WIDTH, VIDEO_MIN_HEIGHT }, screenspace_min_size, screenspace_max_size);
	}

	dynarray_compact(&video.fs_modes);
	dynarray_compact(&video.win_modes);

	dynarray_qsort(&video.fs_modes, video_compare_modes);
	dynarray_qsort(&video.win_modes, video_compare_modes);

	if(!fullscreen_available) {
		log_warn("No available fullscreen modes");
		config_set_int(CONFIG_FULLSCREEN, false);
	}
}

static void video_update_scaling_factor(void) {
	IntExtent main_fb = video_get_screen_framebuffer_size();
	assert(main_fb.w > 0);
	double scaling_factor = (double)main_fb.w / video.current.width;

	if(scaling_factor != video.scaling_factor) {
		log_debug("Scaling factor updated: %f --> %f", video.scaling_factor, scaling_factor);
		video.scaling_factor = scaling_factor;
		video_update_mode_lists();

		IntExtent min_size = coords_ext_pixels_to_screen((IntExtent) { VIDEO_MIN_WIDTH, VIDEO_MIN_HEIGHT });
		SDL_SetWindowMinimumSize(video.window, min_size.w, min_size.h);
	}
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

static void video_update_mode_settings(void) {
	if(video_is_fullscreen()) {
		SDL_HideCursor();
	} else {
		SDL_ShowCursor();
	}

	SDL_GetWindowSize(video.window, &video.current.width, &video.current.height);

	video_update_vsync();
	video_update_scaling_factor();
	video_set_viewport();

	events_emit(TE_VIDEO_MODE_CHANGED, 0, NULL, NULL);
}

static const char *modeflagsstr(uint32_t flags) {
	if(flags & SDL_WINDOW_FULLSCREEN) {
		return "fullscreen";
	} else if(flags & SDL_WINDOW_RESIZABLE) {
		return "windowed, resizable";
	} else {
		return "windowed";
	}
}

static void video_new_window_internal(uint display, uint w, uint h, uint32_t flags, bool fallback) {
	if(video.window) {
		r_unclaim_window(video.window);
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

		if(video.scaling_factor != 0) {
			IntExtent min_size = coords_ext_pixels_to_screen((IntExtent) { VIDEO_MIN_WIDTH, VIDEO_MIN_HEIGHT });
			SDL_SetWindowMinimumSize(video.window, min_size.w, min_size.h);
		}

		video_update_mode_settings();
		return;
	}

	if(fallback) {
		log_fatal("Failed to create window with mode %ix%i (%s): %s", w, h, modeflagsstr(flags), SDL_GetError());
		return;
	}

	video_new_window_internal(display, RESX, RESY, flags & ~SDL_WINDOW_FULLSCREEN, true);
}

static bool restrict_to_capability(bool enabled, VideoCapability cap) {
	VideoCapabilityState capval = video_query_capability(cap);

	switch(capval) {
		case VIDEO_ALWAYS_ENABLED:
			return true;

		case VIDEO_NEVER_AVAILABLE:
			return false;

		default:
			return enabled;
	}
}

static void video_new_window(uint display, uint w, uint h, bool fs, bool resizable) {
	uint32_t flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;

	fs = restrict_to_capability(fs, VIDEO_CAP_FULLSCREEN);
	resizable = restrict_to_capability(resizable, VIDEO_CAP_EXTERNAL_RESIZE);

	if(fs) {
		flags |= SDL_WINDOW_FULLSCREEN;
	} else if(resizable) {
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

static void video_set_fullscreen_internal(bool fullscreen) {
	uint32_t flags = fullscreen ? SDL_WINDOW_FULLSCREEN : 0;
	events_pause_keyrepeat();

	if(!SDL_SetWindowFullscreen(video.window, flags)) {
		log_error("Failed to switch to %s mode: %s", modeflagsstr(flags), SDL_GetError());
	}

	SDL_RaiseWindow(video.window);
}

INLINE bool should_recreate_on_size_change(void) {
	bool defaultval = (
		/* Resize failures are impossible to detect under some WMs */
		video.backend == VIDEO_BACKEND_X11 ||
		/* Needed to work around various SDL bugs and/or HTML/DOM quirks */
		// video.backend == VIDEO_BACKEND_EMSCRIPTEN ||
	0);

	return env_get("TAISEI_VIDEO_RECREATE_ON_RESIZE", defaultval);
}

INLINE bool should_recreate_on_fullscreen_change(void) {
	bool defaultval = (
		/* FIXME Do we need this? */
		video.backend == VIDEO_BACKEND_X11 ||
	0);

	return env_get("TAISEI_VIDEO_RECREATE_ON_FULLSCREEN", defaultval);
}


void video_set_mode(uint display, uint w, uint h, bool fs, bool resizable) {
	fs = restrict_to_capability(fs, VIDEO_CAP_FULLSCREEN);
	resizable = restrict_to_capability(resizable, VIDEO_CAP_EXTERNAL_RESIZE);

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

	if(
		!restrict_to_capability(true, VIDEO_CAP_CHANGE_RESOLUTION) &&
		video.current.width > 0 && video.current.height > 0
	) {
		w = video.current.width;
		h = video.current.height;
	}

	bool display_changed = display != video_current_display();
	bool size_changed = w != video.current.width || h != video.current.height;
	bool fullscreen_changed = video_is_fullscreen() != fs;

	if(display_changed) {
		video_new_window(display, w, h, fs, resizable);
		return;
	}

	if(fullscreen_changed && should_recreate_on_fullscreen_change()) {
		video_new_window(display, w, h, fs, resizable);
		return;
	}

	if(size_changed && !fs) {
		if(!fullscreen_changed && should_recreate_on_size_change()) {
			video_new_window(display, w, h, fs, resizable);
			return;
		}

		SDL_SetWindowSize(video.window, w, h);
		SDL_SetWindowPosition(
			video.window,
			SDL_WINDOWPOS_CENTERED_DISPLAY(display),
			SDL_WINDOWPOS_CENTERED_DISPLAY(display)
		);
	}

	video_set_fullscreen_internal(fs);
	SDL_SetWindowResizable(video.window, resizable);

	if(size_changed) {
		video_update_mode_settings();
	}
}

SDL_Window *video_get_window(void) {
	return video.window;
}

void video_set_fullscreen(bool fullscreen) {
	if(video_query_capability(VIDEO_CAP_FULLSCREEN) != VIDEO_AVAILABLE) {
		return;
	}

	video_set_mode(video_current_display(),
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

static void *video_screenshot_task(void *arg) {
	ScreenshotTaskData *tdata = arg;
	pixmap_convert_inplace_realloc(&tdata->image, PIXMAP_FORMAT_RGB8);

	PixmapPNGSaveOptions opts = PIXMAP_DEFAULT_PNG_SAVE_OPTIONS;

	if(tdata->dest_path) {
		opts.zlib_compression_level = 9;

		bool ok = pixmap_save_file(tdata->dest_path, &tdata->image, &opts.base);

		if(LIKELY(ok)) {
			char *syspath = vfs_repr(tdata->dest_path, true);
			log_info("Saved screenshot as %s", syspath);
			mem_free(syspath);
		}
	} else {  // framedump mode
		char buf[video.framedump.name_prefix_len + FRAMEDUMP_FILENAME_EXTRA_BUFSIZE];
		snprintf(buf, sizeof(buf), "%s" FRAMEDUMP_FILENAME_FORMAT, video.framedump.name_prefix, tdata->frame_num);

		SDL_IOStream *stream = SDL_IOFromFile(buf, "wb");

		if(UNLIKELY(!stream)) {
			log_sdl_error(LOG_ERROR, "SDL_RWFromFile");
			return NULL;
		}

		opts.zlib_compression_level = video.framedump.compression;
		bool ok = pixmap_save_stream(stream, &tdata->image, &opts.base);
		SDL_CloseIO(stream);

		if(LIKELY(ok)) {
			log_debug("Frame dump: %s", buf);
		} else {
			log_error("Frame dump failed: %s", buf);
		}
	}

	return NULL;
}

static void video_screenshot_free_task_data(void *arg) {
	ScreenshotTaskData *tdata = arg;
	mem_free(tdata->image.data.untyped);
	mem_free(tdata->dest_path);
	mem_free(tdata);
}

static void video_take_screenshot_callback(const Pixmap *px, void *userdata) {
	if(!px) {
		log_error("Failed to capture image");
		return;
	}

	ScreenshotTaskData *tdata = userdata;
	// NOTE: could convert formats here to avoid a double copy,
	// but it's faster to do it on the task thread.
	pixmap_copy_alloc(px, &tdata->image);

	task_detach(taskmgr_global_submit((TaskParams) {
		.callback = video_screenshot_task,
		.userdata = userdata,
		.userdata_free_callback = video_screenshot_free_task_data,
	}));
}

void video_take_screenshot(bool viewport_only) {
	auto tdata = ALLOC(ScreenshotTaskData);

	SystemTime systime;
	char timestamp[FILENAME_TIMESTAMP_MIN_BUF_SIZE];
	get_system_time(&systime);
	filename_timestamp(timestamp, sizeof(timestamp), systime);
	tdata->dest_path = strfmt("storage/screenshots/taisei_%s.png", timestamp);

	Framebuffer *fb = NULL;
	FramebufferAttachment attachment = FRAMEBUFFER_ATTACH_NONE;

	if(viewport_only && stage_draw_is_initialized()) {
		fb = stage_get_fbpair(FBPAIR_FG)->front;
		attachment = FRAMEBUFFER_ATTACH_COLOR0;
	}

	r_framebuffer_read_viewport_async(
		fb, attachment, tdata, video_take_screenshot_callback);
}

static void video_take_framedump(void) {
	Framebuffer *fb = NULL;
	FramebufferAttachment attachment = FRAMEBUFFER_ATTACH_NONE;

	if(video.framedump.source == FRAMEDUMP_SRC_VIEWPORT) {
		if(!stage_draw_is_initialized()) {
			return;
		}

		fb = stage_get_fbpair(FBPAIR_FG)->front;
		attachment = FRAMEBUFFER_ATTACH_COLOR0;
	}

	auto tdata = ALLOC(ScreenshotTaskData);
	tdata->frame_num = video.framedump.frame_count++;

	r_framebuffer_read_viewport_async(
		fb, attachment, tdata, video_take_screenshot_callback);
}

bool video_is_resizable(void) {
	return SDL_GetWindowFlags(video.window) & SDL_WINDOW_RESIZABLE;
}

bool video_is_fullscreen(void) {
	return SDL_GetWindowFlags(video.window) & SDL_WINDOW_FULLSCREEN;
}

static void video_init_sdl(void) {
	// XXX: workaround for an SDL bug: https://bugzilla.libsdl.org/show_bug.cgi?id=4127
	// TODO: test if still needed for SDL3 (but in any case it won't hurt)
	SDL_SetHintWithPriority(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0", SDL_HINT_OVERRIDE);

	if(!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		log_sdl_error(LOG_FATAL, "SDL_InitSubSystem");
	}
}

static void video_update_displays(void) {
	SDL_free(video.displays);

	if(!(video.displays = SDL_GetDisplays(&video.num_displays))) {
		log_sdl_error(LOG_ERROR, "SDL_GetDisplays");
		video.num_displays = 0;
	}
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

static bool video_handle_event(SDL_Event *event, void *arg) {
	switch(event->type) {
		case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
			log_debug("SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: %ix%i", event->window.data1, event->window.data2);
			video_update_mode_settings();
			break;

		case SDL_EVENT_WINDOW_FOCUS_LOST:
			if(config_get_int(CONFIG_FOCUS_LOSS_PAUSE)) {
				events_emit(TE_GAME_PAUSE, 0, NULL, NULL);
			}
			break;

		case SDL_EVENT_DISPLAY_ADDED:
		case SDL_EVENT_DISPLAY_REMOVED:
			video_update_displays();
			break;

		default:
			if(event->type == MAKE_TAISEI_EVENT(TE_CONFIG_UPDATED)) {
				return video_handle_config_event(event, arg);
			}
	}

	return false;
}

uint video_num_displays(void) {
	return video.num_displays;
}

const char *video_display_name(uint id) {
	const char *name = SDL_GetDisplayName(video.displays[id]);

	if(name == NULL) {
		log_sdl_error(LOG_WARN, "SDL_GetDisplayName");
		return "Unknown";
	}

	return name;
}

uint video_current_display(void) {
	SDL_DisplayID display = SDL_GetDisplayForWindow(video.window);

	if(!display) {
		log_sdl_error(LOG_WARN, "SDL_GetDisplayForWindow");
		return 0;  // FIXME return error?
	}

	for(int i = 0; i < video.num_displays; ++i) {
		if(video.displays[i] == display) {
			return i;
		}
	}

	assert(0 && "Display index for window not found");
	return 0;
}

static void video_init_framedump(void) {
	const char *framedump_dir = env_get("TAISEI_FRAMEDUMP", NULL);
	const char *framedump_src = env_get("TAISEI_FRAMEDUMP_SOURCE", "screen");

	if(framedump_dir == NULL) {
		return;
	}

	if(!strcasecmp(framedump_src, "screen")) {
		video.framedump.source = FRAMEDUMP_SRC_SCREEN;
	} else if(!strcasecmp(framedump_src, "viewport")) {
		video.framedump.source = FRAMEDUMP_SRC_VIEWPORT;
	} else {
		log_warn("Unknown source '%s'; assuming 'screen'", framedump_src);
		video.framedump.source = FRAMEDUMP_SRC_SCREEN;
	}

	video.framedump.compression = env_get("TAISEI_FRAMEDUMP_COMPRESSION", 1);

	video.framedump.name_prefix_len = strlen(framedump_dir);
	video.framedump.name_prefix = mem_alloc(
		video.framedump.name_prefix_len + FRAMEDUMP_FILENAME_EXTRA_BUFSIZE);
	memcpy(video.framedump.name_prefix, framedump_dir, video.framedump.name_prefix_len);
}

void video_init(const VideoInitParams *params) {
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
		video_query_capability = video_query_capability_switch;
	} else {
		video.backend = VIDEO_BACKEND_OTHER;
	}

	if(global.is_kiosk_mode) {
		video_query_capability_kiosk_fallback = video_query_capability;
		video_query_capability = video_query_capability_kiosk;
	}

	video.scaling_factor = 0;

	r_init();

	int w, h;
	bool fullscreen;

	if(params->width > 0 || params->height > 0) {
		w = params->width;
		h = params->height;
		fullscreen = false;

		if(w <= 0) {
			w = rint(h * VIDEO_ASPECT_RATIO);
		} else if(h <= 0) {
			h = rint(w / VIDEO_ASPECT_RATIO);
		}
	} else {
		w = config_get_int(CONFIG_VID_WIDTH);
		h = config_get_int(CONFIG_VID_HEIGHT);
		fullscreen = config_get_int(CONFIG_FULLSCREEN);
	}

	video_update_displays();

	video_set_mode(
		config_get_int(CONFIG_VID_DISPLAY),
		w, h, fullscreen,
		config_get_int(CONFIG_VID_RESIZABLE)
	);

	video_update_scaling_factor();

	events_register_handler(&(EventHandler) {
		.proc = video_handle_event,
		.priority = EPRIO_SYSTEM,
	});

	video_init_framedump();
	log_info("Video subsystem initialized");
}

void video_post_init(void) {
	fbmgr_init();
	video.postprocess = video_postprocess_init();
	r_framebuffer(video_get_screen_framebuffer());
}

void video_shutdown(void) {
	video_postprocess_shutdown(video.postprocess);
	fbmgr_shutdown();
	events_unregister_handler(video_handle_event);
	r_unclaim_window(video.window);
	SDL_DestroyWindow(video.window);
	r_shutdown();
	SDL_free(video.displays);
	dynarray_free_data(&video.win_modes);
	dynarray_free_data(&video.fs_modes);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	mem_free(video.framedump.name_prefix);
}

Framebuffer *video_get_screen_framebuffer(void) {
	return video_postprocess_get_framebuffer(video.postprocess);
}

void video_swap_buffers(void) {
	Framebuffer *pp_fb = video_postprocess_render(video.postprocess);

	if(pp_fb) {
		r_flush_sprites();

		Framebuffer *prev_fb = r_framebuffer_current();
		r_framebuffer(NULL);

		r_state_push();
		r_mat_proj_push_ortho(SCREEN_W, SCREEN_H);
		r_shader_standard();
		r_color3(1, 1, 1);
		draw_framebuffer_tex(pp_fb, SCREEN_W, SCREEN_H);
		r_framebuffer_clear(pp_fb, BUFFER_ALL, RGBA(0, 0, 0, 0), 1);
		r_mat_proj_pop();
		r_state_pop();

		r_swap(video.window);
		r_framebuffer(prev_fb);
	} else {
		r_swap(video.window);
	}

	if(video.framedump.name_prefix) {
		video_take_framedump();
	}

	// XXX: Unfortunately, there seems to be no reliable way to sync this up with events
	config_set_int(CONFIG_FULLSCREEN, video_is_fullscreen());
}

VideoBackend video_get_backend(void) {
	return video.backend;
}

static VideoModeArray *get_vidmode_array(bool fullscreen) {
	return fullscreen ? &video.fs_modes : &video.win_modes;
}

bool video_get_mode(uint idx, bool fullscreen, VideoMode *out_mode) {
	VideoModeArray *a = get_vidmode_array(fullscreen);

	if(idx >= a->num_elements) {
		return false;
	}

	*out_mode = dynarray_get(a, idx);
	return true;
}

uint video_get_num_modes(bool fullscreen) {
	return get_vidmode_array(fullscreen)->num_elements;
}

VideoMode video_get_current_mode(void) {
	return video.current;
}

double video_get_scaling_factor(void) {
	return video.scaling_factor;
}
