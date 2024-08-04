/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "framebuffer.h"

#include "sdlgpu.h"

#include "util.h"

Framebuffer *sdlgpu_framebuffer_create(void) {
	auto fb = ALLOC(Framebuffer);

	for(int i = 0; i < FRAMEBUFFER_MAX_OUTPUTS; ++i) {
		fb->output_mapping[i] = FRAMEBUFFER_ATTACH_COLOR0 + i;
	}

	return fb;
}

void sdlgpu_framebuffer_attach(Framebuffer *framebuffer, Texture *tex, uint mipmap, FramebufferAttachment attachment) {
	uint idx = attachment;
	assert(idx <= ARRAY_SIZE(framebuffer->attachments));

	sdlgpu_framebuffer_flush(framebuffer, (1u << idx));

	framebuffer->attachments[idx] = (FramebufferAttachmentData) {
		.target = {
			.texture = tex,
			.mip_level = mipmap,
		},
		.load.op = SDL_GPU_LOADOP_LOAD,
	};
}

FramebufferAttachmentQueryResult sdlgpu_framebuffer_query_attachment(
	Framebuffer *framebuffer, FramebufferAttachment attachment
) {
	uint idx = attachment;
	assert(idx <= ARRAY_SIZE(framebuffer->attachments));

	auto a = &framebuffer->attachments[idx];

	return (FramebufferAttachmentQueryResult) {
		.texture = a->target.texture,
		.miplevel = a->target.mip_level,
	};
}

void sdlgpu_framebuffer_outputs(
	Framebuffer *framebuffer, FramebufferAttachment config[FRAMEBUFFER_MAX_OUTPUTS], uint8_t write_mask
) {
	for(int i = 0; i < FRAMEBUFFER_MAX_OUTPUTS; ++i) {
		if(write_mask & (1 << i)) {
			framebuffer->output_mapping[i] = config[i];

			if(config[i] != FRAMEBUFFER_ATTACH_NONE) {
				assert(config[i] >= FRAMEBUFFER_ATTACH_COLOR0);
				assert(config[i] < FRAMEBUFFER_ATTACH_COLOR0 + FRAMEBUFFER_MAX_COLOR_ATTACHMENTS);
			}
		}
	}
}

void sdlgpu_framebuffer_destroy(Framebuffer *framebuffer) {
	sdlgpu_framebuffer_flush(framebuffer, ~0);

	if(sdlgpu.st.framebuffer == framebuffer) {
		sdlgpu.st.framebuffer = NULL;
	}

	mem_free(framebuffer);
}

static void sdlgpu_framebuffer_default_clear(
	BufferKindFlags flags, const Color *colorval, float depthval
) {
	if(flags & BUFFER_DEPTH) {
		sdlgpu.st.default_framebuffer.depth.op = SDL_GPU_LOADOP_CLEAR;
		sdlgpu.st.default_framebuffer.depth.clear.depth = depthval;
	}

	if(flags & BUFFER_COLOR) {
		sdlgpu.st.default_framebuffer.color.op = SDL_GPU_LOADOP_CLEAR;
		sdlgpu.st.default_framebuffer.color.clear.color.as_taisei = *colorval;
	}
}

void sdlgpu_framebuffer_clear(
	Framebuffer *framebuffer, BufferKindFlags flags, const Color *colorval, float depthval
) {
	if(!framebuffer) {
		sdlgpu_framebuffer_default_clear(flags, colorval, depthval);
		return;
	}

	if(flags & BUFFER_DEPTH) {
		auto a = &framebuffer->attachments[FRAMEBUFFER_ATTACH_DEPTH];
		if(a->target.texture) {
			a->load.op = SDL_GPU_LOADOP_CLEAR;
			a->load.clear.depth = depthval;
		}
	}

	if(flags & BUFFER_COLOR) {
		for(int i = 0; i < FRAMEBUFFER_MAX_OUTPUTS; ++i) {
			FramebufferAttachment target = framebuffer->output_mapping[i];

			if(target != FRAMEBUFFER_ATTACH_NONE) {
				auto a = &framebuffer->attachments[target];
				a->load.op = SDL_GPU_LOADOP_CLEAR;
				a->load.clear.color.as_taisei = *colorval;
			}
		}
	}
}

void sdlgpu_framebuffer_copy(Framebuffer *dst, Framebuffer *src, BufferKindFlags flags) {
	UNREACHABLE;
}

IntExtent sdlgpu_framebuffer_get_size(Framebuffer *fb) {
	if(fb) {
		log_fatal("Not implemented");
	}

	return (IntExtent) {
		.w = sdlgpu.frame.swapchain.width,
		.h = sdlgpu.frame.swapchain.height,
	};
}

static FloatRect *sdlgpu_framebuffer_viewport_pointer(Framebuffer *fb) {
	if(fb) {
		return &fb->viewport;
	} else {
		return &sdlgpu.st.default_framebuffer.viewport;
	}
}

void sdlgpu_framebuffer_viewport(Framebuffer *fb, FloatRect vp) {
	*sdlgpu_framebuffer_viewport_pointer(fb) = vp;
}

void sdlgpu_framebuffer_viewport_current(Framebuffer *fb, FloatRect *vp) {
	*vp = *sdlgpu_framebuffer_viewport_pointer(fb);
}

void sdlgpu_framebuffer_set_debug_label(Framebuffer *fb, const char *label) {
	strlcpy(fb->debug_label, label, sizeof(fb->debug_label));
}

const char *sdlgpu_framebuffer_get_debug_label(Framebuffer* fb) {
	return fb->debug_label;
}

void sdlgpu_framebuffer_flush(Framebuffer *framebuffer, uint32_t attachment_mask) {
	UNREACHABLE;
}

static void sdlgpu_framebuffer_setup_outputs_default_framebuffer(RenderPassOutputs *outputs) {
	SDL_GpuTexture *swapchain_tex = sdlgpu_get_swapchain_texture();

	outputs->color[0] = (SDL_GpuColorAttachmentInfo) {
		.textureSlice.texture = swapchain_tex,
		.clearColor = sdlgpu.st.default_framebuffer.color.clear.color.as_sdlgpu,
		.loadOp = sdlgpu.st.default_framebuffer.color.op,
		.storeOp = swapchain_tex ? SDL_GPU_STOREOP_DONT_CARE : SDL_GPU_STOREOP_STORE,
	};

	outputs->color_formats[0] = sdlgpu.frame.swapchain.fmt;

	outputs->have_depth_stencil = false;
	outputs->num_color_attachments = swapchain_tex ? 1 : 0;

	sdlgpu.st.default_framebuffer.color.op = SDL_GPU_LOADOP_LOAD;
	sdlgpu.st.default_framebuffer.depth.op = SDL_GPU_LOADOP_LOAD;
}

void sdlgpu_framebuffer_setup_outputs(Framebuffer *framebuffer, RenderPassOutputs *outputs) {
	if(framebuffer == NULL) {
		sdlgpu_framebuffer_setup_outputs_default_framebuffer(outputs);
		return;
	}

	UNREACHABLE;
}
