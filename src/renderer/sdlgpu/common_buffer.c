/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "common_buffer.h"

#include "sdlgpu.h"

static void sdlgpu_buffer_update_debug_label(CommonBuffer *cbuf) {
	if(cbuf->gpubuf) {
		SDL_SetGPUBufferName(sdlgpu.device, cbuf->gpubuf, cbuf->debug_label);
	}
}

void sdlgpu_buffer_set_debug_label(CommonBuffer *cbuf, const char *label) {
	strlcpy(cbuf->debug_label, label, sizeof(cbuf->debug_label));
	sdlgpu_buffer_update_debug_label(cbuf);
}

CommonBuffer *sdlgpu_buffer_create(SDL_GPUBufferUsageFlags usage, size_t alloc_size) {
	CommonBuffer *cbuf = mem_alloc(alloc_size);
	cachedbuf_init(&cbuf->cachedbuf);
	cachedbuf_resize(&cbuf->cachedbuf, alloc_size);
	cbuf->usage = usage;
	return cbuf;
}

void sdlgpu_buffer_destroy(CommonBuffer *cbuf) {
	SDL_ReleaseGPUTransferBuffer(sdlgpu.device, cbuf->transferbuf);
	SDL_ReleaseGPUBuffer(sdlgpu.device, cbuf->gpubuf);
	cachedbuf_deinit(&cbuf->cachedbuf);
	mem_free(cbuf);
}

void sdlgpu_buffer_invalidate(CommonBuffer *cbuf) {
	// FIXME do we need to do anything here?
	cbuf->cachedbuf.stream_offset = 0;
}

void sdlgpu_buffer_resize(CommonBuffer *cbuf, size_t new_size) {
	assert(cbuf->cachedbuf.cache != NULL);
	cachedbuf_resize(&cbuf->cachedbuf, new_size);
}

static bool sdlgpu_buffer_resize_gpubuf(CommonBuffer *cbuf) {
	if(cbuf->commited_size == cbuf->cachedbuf.size) {
		assert(cbuf->gpubuf != NULL);
		return false;
	}

	if(cbuf->gpubuf) {
		SDL_ReleaseGPUBuffer(sdlgpu.device, cbuf->gpubuf);
	}

	if(cbuf->transferbuf) {
		SDL_ReleaseGPUTransferBuffer(sdlgpu.device, cbuf->transferbuf);
	}

	cbuf->gpubuf = SDL_CreateGPUBuffer(sdlgpu.device, &(SDL_GPUBufferCreateInfo) {
		.usage = cbuf->usage,
		.size = cbuf->cachedbuf.size,
	});

	if(UNLIKELY(!cbuf->gpubuf)) {
		log_sdl_error(LOG_FATAL, "SDL_CreateGPUBuffer");
		UNREACHABLE;
	}

	cbuf->transferbuf = SDL_CreateGPUTransferBuffer(sdlgpu.device, &(SDL_GPUTransferBufferCreateInfo) {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = cbuf->cachedbuf.size,
	});

	if(UNLIKELY(!cbuf->transferbuf)) {
		log_sdl_error(LOG_FATAL, "SDL_CreateGPUTransferBuffer");
		UNREACHABLE;
	}

	cbuf->commited_size = cbuf->cachedbuf.size;

	sdlgpu_buffer_update_debug_label(cbuf);

	return true;
}

SDL_IOStream *sdlgpu_buffer_get_stream(CommonBuffer *cbuf) {
	return cbuf->cachedbuf.stream;
}

void sdlgpu_buffer_flush(CommonBuffer *cbuf) {
	CachedBufferUpdate update = cachedbuf_flush(&cbuf->cachedbuf);

	if(sdlgpu_buffer_resize_gpubuf(cbuf)) {
		// TODO: instead of reuploading the whole buffer, track the part that was ever updated
		update.data = cbuf->cachedbuf.cache;
		update.size = cbuf->cachedbuf.size;
		update.offset = 0;
	}

	if(!update.size) {
		return;
	}

	uint8_t *mapped = SDL_MapGPUTransferBuffer(sdlgpu.device, cbuf->transferbuf, true);

	if(UNLIKELY(!mapped)) {
		log_sdl_error(LOG_FATAL_IF_DEBUG, "SDL_MapGPUTransferBuffer");
		return;
	}

	memcpy(mapped + update.offset, update.data, update.size);
	SDL_UnmapGPUTransferBuffer(sdlgpu.device, cbuf->transferbuf);

	auto copypass = sdlgpu_begin_or_resume_copy_pass(CBUF_UPLOAD);

	SDL_UploadToGPUBuffer(copypass, &(SDL_GPUTransferBufferLocation) {
		.transfer_buffer = cbuf->transferbuf,
		.offset = update.offset,
	}, &(SDL_GPUBufferRegion) {
		.buffer = cbuf->gpubuf,
		.offset = update.offset,
		.size = update.size,
	}, true);
}
