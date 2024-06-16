/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "sdlgpu.h"

#include "../common/backend.h"

#include <SDL3/SDL.h>

static struct {
	SDL_GpuDevice *device;
	SDL_Window *window;
} R;

static void sdlgpu_init(void) {
	R.device = SDL_GpuCreateDevice(SDL_GPU_BACKEND_ALL, true, false);

	if(!R.device) {
		log_sdl_error(LOG_FATAL, "SDL_GpuCreateDevice");
	}
}

static void sdlgpu_post_init(void) {

}

static void sdlgpu_shutdown(void) {
	SDL_GpuUnclaimWindow(R.device, R.window);
	SDL_GpuDestroyDevice(R.device);
	R.device = NULL;
}

static SDL_Window *sdlgpu_create_window(const char *title, int x, int y, int w, int h, uint32_t flags) {
	if(R.window) {
		SDL_GpuUnclaimWindow(R.device, R.window);  // FIXME
	}

	R.window = SDL_CreateWindow(title, w, h, flags);

	if(R.window && !SDL_GpuClaimWindow(
		R.device, R.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_MAILBOX
	)) {
		log_sdl_error(LOG_FATAL, "SDL_GpuClaimWindow");
	}

	return R.window;
}

RendererBackend _r_backend_sdlgpu = {
	.name = "sdlgpu",
	.funcs = {
		.init = sdlgpu_init,
		.post_init = sdlgpu_post_init,
		.shutdown = sdlgpu_shutdown,
		.create_window = sdlgpu_create_window,
	}
};
