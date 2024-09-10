/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "framebuffer_async_read.h"

#include "sdlgpu.h"
#include "texture.h"

typedef struct FramebufferReadRequest {
	FramebufferReadAsyncCallback callback;
	void *userdata;
	struct {
		SDL_GPUTransferBuffer *buffer;
		uint buffer_size;
		uint width;
		uint height;
		PixmapFormat format;
	} transfer;
} FramebufferReadRequest;

#define REQUESTS_IN_FLIGHT 4

typedef struct AsyncReadbackSystem {
	FramebufferReadRequest requests[REQUESTS_IN_FLIGHT];
	SDL_GPUFence *fences[REQUESTS_IN_FLIGHT];
} AsyncReadbackSystem;

static AsyncReadbackSystem *sys;

#define RQ_FENCE(rq) (sys->fences[(rq) - sys->requests])

static void init_readback_system(void) {
	if(!sys) {
		sys = ALLOC(typeof(*sys), {});
	}
}

static void sync_read_requests(bool all) {
	SDL_WaitForGPUFences(sdlgpu.device, all, sys->fences, ARRAY_SIZE(sys->fences));
}

static bool ping_read_request(FramebufferReadRequest *rq) {
	SDL_GPUFence *fence = RQ_FENCE(rq);

	if(!fence) {
		return true;
	}

	if(!SDL_QueryGPUFence(sdlgpu.device, fence)) {
		return false;
	}

	auto data_size = pixmap_data_size(rq->transfer.format, rq->transfer.width, rq->transfer.height);
	rq->callback(&(Pixmap) {
		.width = rq->transfer.width,
		.height = rq->transfer.height,
		.format = rq->transfer.format,
		.origin = PIXMAP_ORIGIN_BOTTOMLEFT,
		.data_size = data_size,
		.data.untyped = SDL_MapGPUTransferBuffer(sdlgpu.device, rq->transfer.buffer, false)
	}, rq->userdata);
	SDL_UnmapGPUTransferBuffer(sdlgpu.device, rq->transfer.buffer);

	RQ_FENCE(rq) = NULL;
	SDL_ReleaseGPUFence(sdlgpu.device, fence);

	return true;
}

static FramebufferReadRequest *acquire_request(void) {
	for(int i = 0; i < ARRAY_SIZE(sys->requests); ++i) {
		auto rq = sys->requests + i;
		if(ping_read_request(rq)) {
			assert(!RQ_FENCE(rq));
			return rq;
		}
	}

	return false;
}

void sdlgpu_framebuffer_read_async(
	Framebuffer *framebuffer,
	FramebufferAttachment attachment,
	IntRect region,
	void *userdata,
	FramebufferReadAsyncCallback callback
) {
	if(UNLIKELY(framebuffer == NULL)) {
		log_error("Swapchain readback is not supported");
		callback(NULL, userdata);
		return;
	}

	auto src = sdlgpu_framebuffer_query_attachment(framebuffer, attachment);

	if(UNLIKELY(!src.texture)) {
		log_error("No texture attached");
		callback(NULL, userdata);
		return;
	}

	init_readback_system();

	auto rq = acquire_request();
	if(!rq) {
		log_warn("Queue is full, forcing synchronization");
		sync_read_requests(false);
		rq = NOT_NULL(acquire_request());
	}

	rq->transfer.width = region.w;
	rq->transfer.height = region.h;
	rq->transfer.format = sdlgpu_texfmt_to_pixfmt(src.texture->gpu_format);

	if(!rq->transfer.format) {
		log_error("Can't download texture %s (format %u)",
			src.texture->debug_label, src.texture->gpu_format);
		callback(NULL, userdata);
		return;
	}

	auto required_buffer_size = pixmap_data_size(
		rq->transfer.format, rq->transfer.width, rq->transfer.height);

	if(rq->transfer.buffer_size < required_buffer_size) {
		SDL_ReleaseGPUTransferBuffer(sdlgpu.device, rq->transfer.buffer);
		rq->transfer.buffer = SDL_CreateGPUTransferBuffer(sdlgpu.device, &(SDL_GPUTransferBufferCreateInfo) {
			.sizeInBytes = required_buffer_size,
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
		});
		rq->transfer.buffer_size = required_buffer_size;
	}

	auto cbuf = SDL_AcquireGPUCommandBuffer(sdlgpu.device);
	auto copy_pass = SDL_BeginGPUCopyPass(cbuf);
	SDL_DownloadFromGPUTexture(copy_pass,
		&(SDL_GPUTextureRegion) {
			.texture = src.texture->gpu_texture,
			.mipLevel = src.miplevel,
			.x = region.x,
			.y = region.y,
			.w = region.w,
			.h = region.h,
			.d = 1,
		}, &(SDL_GPUTextureTransferInfo) {
			.transferBuffer = rq->transfer.buffer,
			.imageHeight = region.h,
			.imagePitch = region.w,
		});
	SDL_EndGPUCopyPass(copy_pass);
	RQ_FENCE(rq) = SDL_SubmitGPUCommandBufferAndAcquireFence(cbuf);
}

void sdlgpu_framebuffer_process_read_requests(void) {
	if(!sys) {
		return;
	}

	for(int i = 0; i < ARRAY_SIZE(sys->requests); ++i) {
		auto rq = sys->requests + i;
		ping_read_request(rq);
	}
}

void sdlgpu_framebuffer_finalize_read_requests(void) {
	if(!sys) {
		return;
	}

	sync_read_requests(true);

	for(int i = 0; i < ARRAY_SIZE(sys->requests); ++i) {
		auto rq = sys->requests + i;
		ping_read_request(rq);
		SDL_ReleaseGPUTransferBuffer(sdlgpu.device, rq->transfer.buffer);
	}

	mem_free(sys);
	sys = NULL;
}
