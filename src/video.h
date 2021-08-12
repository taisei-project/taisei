/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "util/geometry.h"
#include "renderer/api.h"

#include <SDL.h>

#define WINDOW_TITLE "Taisei Project"
#define VIDEO_ASPECT_RATIO ((double)SCREEN_W/SCREEN_H)

enum {
	// virtual screen coordinates
	SCREEN_W = 800,
	SCREEN_H = 600,
};

#define SCREEN_SIZE { SCREEN_W, SCREEN_H }

typedef union VideoMode {
	// NOTE: These really should be floats, since this represents abstract screen coordinates, not pixels.
	// However, SDL's API expects integers everywhere, so it does not really make sense.

	struct {
		// TODO: get rid of this and just typedef to IntExtent?

		int width;
		int height;
	};

	IntExtent as_int_extent;
} VideoMode;

typedef enum VideoBackend {
	VIDEO_BACKEND_OTHER,
	VIDEO_BACKEND_X11,
	VIDEO_BACKEND_EMSCRIPTEN,
	VIDEO_BACKEND_KMSDRM,
	VIDEO_BACKEND_RPI,
	VIDEO_BACKEND_SWITCH,
} VideoBackend;

typedef enum VideoCapability {
	VIDEO_CAP_FULLSCREEN,
	VIDEO_CAP_EXTERNAL_RESIZE,
	VIDEO_CAP_CHANGE_RESOLUTION,
	VIDEO_CAP_VSYNC_ADAPTIVE,
} VideoCapability;

typedef enum VideoCapabilityState {
	VIDEO_NEVER_AVAILABLE,
	VIDEO_AVAILABLE,
	VIDEO_ALWAYS_ENABLED,
	VIDEO_CURRENTLY_UNAVAILABLE,
} VideoCapabilityState;

void video_init(void);
void video_post_init(void);
void video_shutdown(void);
void video_set_mode(uint display, uint w, uint h, bool fs, bool resizable);
void video_set_fullscreen(bool fullscreen);
void video_get_viewport(FloatRect *vp);
void video_get_viewport_size(float *width, float *height);
bool video_is_fullscreen(void);
bool video_is_resizable(void);
extern VideoCapabilityState (*video_query_capability)(VideoCapability cap);
void video_take_screenshot(void);
void video_swap_buffers(void);
uint video_num_displays(void);
uint video_current_display(void);
void video_set_display(uint idx);
const char *video_display_name(uint id) attr_returns_nonnull;
Framebuffer *video_get_screen_framebuffer(void);
VideoBackend video_get_backend(void);
VideoMode video_get_mode(uint idx, bool fullscreen);
uint video_get_num_modes(bool fullscreen);
VideoMode video_get_current_mode(void);
double video_get_scaling_factor(void);
SDL_Window *video_get_window(void);
