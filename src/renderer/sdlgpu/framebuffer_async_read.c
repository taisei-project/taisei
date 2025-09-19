/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
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
	struct {
		SDL_GPUTexture *texture;
		uint width;
		uint height;
	} rgbcvt;
} AsyncReadbackSystem;

static AsyncReadbackSystem *sys;

#define RQ_FENCE(rq) (sys->fences[(rq) - sys->requests])

static void init_readback_system(void) {
	if(!sys) {
		sys = ALLOC(typeof(*sys), {});
	}
}

static void sync_read_requests(bool all) {
	SDL_GPUFence *fences[ARRAY_SIZE(sys->fences)];
	uint num_fences = 0;

	for(uint i = 0; i < ARRAY_SIZE(fences); ++i) {
		if(sys->fences[i]) {
			fences[num_fences++] = sys->fences[i];
		}
	}

	if(num_fences == 0) {
		return;
	}

	bool ok = SDL_WaitForGPUFences(sdlgpu.device, all, fences, num_fences);
	if(UNLIKELY(!ok)) {
		log_sdl_error(LOG_ERROR, "SDL_WaitForGPUFences");
	}
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
	auto data = SDL_MapGPUTransferBuffer(sdlgpu.device, rq->transfer.buffer, false);

	if(UNLIKELY(!data)) {
		log_sdl_error(LOG_ERROR, "SDL_MapGPUTransferBuffer");
		rq->callback(NULL, rq->userdata);
	} else {
		rq->callback(&(Pixmap) {
			.width = rq->transfer.width,
			.height = rq->transfer.height,
			.format = rq->transfer.format,
			.origin = PIXMAP_ORIGIN_TOPLEFT,
			.data_size = data_size,
			.data.untyped = data,
		}, rq->userdata);
		SDL_UnmapGPUTransferBuffer(sdlgpu.device, rq->transfer.buffer);
	}

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
	// Framebuffer faux_fb;
	Texture faux_fb_tex;
	FramebufferAttachmentQueryResult src;
	bool needs_blit = false;

	if(framebuffer == NULL) {
		if(!sdlgpu.frame.faux_backbuffer.tex) {
			log_error("Swapchain readback is not supported");
			callback(NULL, userdata);
			return;
		}

		faux_fb_tex = (Texture) {
			.gpu_texture = sdlgpu.frame.faux_backbuffer.tex,
			.gpu_format = sdlgpu.frame.faux_backbuffer.fmt,
			.params = (TextureParams) {
				.class = TEXTURE_CLASS_2D,
				.width = sdlgpu.frame.faux_backbuffer.width,
				.height = sdlgpu.frame.faux_backbuffer.height,
				.layers = 1,
				.anisotropy = 1,
			},
			.debug_label = "Faux backbuffer",
		};

		if(sdlgpu.frame.faux_backbuffer.fmt == SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM) {
			needs_blit = true;
		}

		src = (FramebufferAttachmentQueryResult) { &faux_fb_tex };
	} else {
		src = sdlgpu_framebuffer_query_attachment(framebuffer, attachment);
	}

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
	rq->callback = callback;
	rq->userdata = userdata;

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
			.size = required_buffer_size,
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
		});

		if(UNLIKELY(!rq->transfer.buffer)) {
			log_sdl_error(LOG_ERROR, "SDL_CreateGPUTransferBuffer");
			callback(NULL, userdata);
			return;
		}

		rq->transfer.buffer_size = required_buffer_size;
	}

	auto cbuf = SDL_AcquireGPUCommandBuffer(sdlgpu.device);

	if(UNLIKELY(!cbuf)) {
		log_sdl_error(LOG_FATAL, "SDL_AcquireGPUCommandBuffer");
		callback(NULL, userdata);
		return;
	}

	if(needs_blit) {
		if(
			sys->rgbcvt.texture &&
			(sys->rgbcvt.width < region.w || sys->rgbcvt.height < region.h)
		) {
			log_debug("Staging RGBA texture is too small: need %ux%u, have %ux%u; recreating",
				region.w, region.h, sys->rgbcvt.width, sys->rgbcvt.height);
			SDL_ReleaseGPUTexture(sdlgpu.device, sys->rgbcvt.texture);
			sys->rgbcvt.texture = NULL;
		}

		if(!sys->rgbcvt.texture) {
			sys->rgbcvt.width = max(sys->rgbcvt.width, region.w);
			sys->rgbcvt.height = max(sys->rgbcvt.height, region.h);
			log_debug("Creating a %ux%u staging RGBA texture",
				sys->rgbcvt.width, sys->rgbcvt.height);

			sys->rgbcvt.texture = SDL_CreateGPUTexture(sdlgpu.device, &(SDL_GPUTextureCreateInfo) {
				.type = SDL_GPU_TEXTURETYPE_2D,
				.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
				.width = sys->rgbcvt.width,
				.height = sys->rgbcvt.height,
				.layer_count_or_depth = 1,
				.num_levels = 1,
				.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
			});

			if(UNLIKELY(!sys->rgbcvt.texture)) {
				log_sdl_error(LOG_ERROR, "SDL_CreateGPUTexture");
				callback(NULL, userdata);
				SDL_CancelGPUCommandBuffer(cbuf);
				return;
			}
		}

		SDL_BlitGPUTexture(cbuf, &(SDL_GPUBlitInfo) {
			.source = {
				.texture = src.texture->gpu_texture,
				.x = region.x,
				.y = region.y,
				.w = region.w,
				.h = region.h,
			},
			.destination = {
				.texture = sys->rgbcvt.texture,
				.x = 0,
				.y = 0,
				.w = region.w,
				.h = region.h,
			},
			.load_op = SDL_GPU_LOADOP_DONT_CARE,
			.filter = SDL_GPU_FILTER_NEAREST,
			.cycle = true,
		});

		src.texture->gpu_texture = sys->rgbcvt.texture;
		region.x = region.y = 0;
	}

	auto copy_pass = SDL_BeginGPUCopyPass(cbuf);
	SDL_DownloadFromGPUTexture(copy_pass,
		&(SDL_GPUTextureRegion) {
			.texture = src.texture->gpu_texture,
			.mip_level = src.miplevel,
			.x = region.x,
			.y = region.y,
			.w = region.w,
			.h = region.h,
			.d = 1,
		}, &(SDL_GPUTextureTransferInfo) {
			.transfer_buffer = rq->transfer.buffer,
			.rows_per_layer = region.h,
			.pixels_per_row = region.w,
		});
	SDL_EndGPUCopyPass(copy_pass);
	RQ_FENCE(rq) = SDL_SubmitGPUCommandBufferAndAcquireFence(cbuf);

	if(UNLIKELY(!RQ_FENCE(rq))) {
		callback(NULL, userdata);
		return;
	}
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

	if(sys->rgbcvt.texture) {
		SDL_ReleaseGPUTexture(sdlgpu.device, sys->rgbcvt.texture);
		sys->rgbcvt.texture = NULL;
	}

	mem_free(sys);
	sys = NULL;
}
