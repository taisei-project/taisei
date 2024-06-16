/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "framebuffer.h"
#include "texture.h"

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

	sdlgpu_texture_incref(tex);
	sdlgpu_texture_decref(framebuffer->attachments[idx].texture);

	framebuffer->attachments[idx] = (TextureSlice) {
		.texture = tex,
		.mip_level = mipmap,
	};
}

FramebufferAttachmentQueryResult sdlgpu_framebuffer_query_attachment(
	Framebuffer *framebuffer, FramebufferAttachment attachment
) {
	uint idx = attachment;
	assert(idx <= ARRAY_SIZE(framebuffer->attachments));

	auto a = &framebuffer->attachments[idx];

	return (FramebufferAttachmentQueryResult) {
		.texture = a->texture,
		.miplevel = a->mip_level,
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

	for(uint i = 0; i < ARRAY_SIZE(framebuffer->attachments); ++i) {
		sdlgpu_texture_decref(framebuffer->attachments[i].texture);
	}

	if(sdlgpu.st.framebuffer == framebuffer) {
		sdlgpu.st.framebuffer = NULL;
	}

	if(sdlgpu.st.prev_framebuffer == framebuffer) {
		sdlgpu.st.prev_framebuffer = NULL;
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
		sdlgpu.st.default_framebuffer.color.clear.color = *colorval;
	}
}

void sdlgpu_framebuffer_taint(Framebuffer *framebuffer) {
	for(uint i = 0; i < FRAMEBUFFER_MAX_ATTACHMENTS; ++i) {
		if(framebuffer->attachments[i].texture != NULL) {
			sdlgpu_texture_taint(framebuffer->attachments[i].texture);
		}
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
		if(a->texture) {
			a->texture->load.op = SDL_GPU_LOADOP_CLEAR;
			a->texture->load.clear.depth = depthval;
		}
	}

	if(flags & BUFFER_COLOR) {
		for(int i = 0; i < FRAMEBUFFER_MAX_OUTPUTS; ++i) {
			FramebufferAttachment target = framebuffer->output_mapping[i];

			if(target != FRAMEBUFFER_ATTACH_NONE) {
				auto a = &framebuffer->attachments[target];

				if(a->texture) {
					a->texture->load.op = SDL_GPU_LOADOP_CLEAR;
					a->texture->load.clear.color = *colorval;
				}
			}
		}
	}
}

static SDL_GPUCopyPass *sdlgpu_framebuffer_copy_attachment(
	SDL_GPUCopyPass *copy_pass, Framebuffer *dst, Framebuffer *src, FramebufferAttachment attachment
) {
	auto a_dst = &dst->attachments[attachment];
	auto a_src = &src->attachments[attachment];

	if(a_dst->texture && a_src->texture) {
		copy_pass = sdlgpu_texture_copy(copy_pass, a_dst, a_src, true);
	}

	return copy_pass;
}

void sdlgpu_framebuffer_copy(Framebuffer *dst, Framebuffer *src, BufferKindFlags flags) {
	SDL_GPUCopyPass *copy_pass = sdlgpu_begin_or_resume_copy_pass(CBUF_DRAW);

	if(flags & BUFFER_COLOR) {
		for(uint i = 0; i < FRAMEBUFFER_MAX_COLOR_ATTACHMENTS; ++i) {
			copy_pass = sdlgpu_framebuffer_copy_attachment(copy_pass, dst, src, FRAMEBUFFER_ATTACH_COLOR0 + i);
		}
	}

	if(flags & BUFFER_DEPTH) {
		copy_pass = sdlgpu_framebuffer_copy_attachment(copy_pass, dst, src, FRAMEBUFFER_ATTACH_DEPTH);
	}
}

IntExtent sdlgpu_framebuffer_get_size(Framebuffer *fb) {
	if(fb) {
		for(int i = 0; i < FRAMEBUFFER_MAX_ATTACHMENTS; ++i) {
			Texture *tex = fb->attachments[i].texture;

			if(tex) {
				// Assume all attachments are the same size
				return (IntExtent) {
					.w = tex->params.width,
					.h = tex->params.height,
				};
			}
		}

		return (IntExtent) { };
	}

	return (IntExtent) {
		.w = sdlgpu.frame.swapchain.width,
		.h = sdlgpu.frame.swapchain.height,
	};
}

FloatRect *sdlgpu_framebuffer_viewport_pointer(Framebuffer *fb) {
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
	// log_warn("FLUSH %p [%s] mask=0x%04x", framebuffer, framebuffer->debug_label, attachment_mask);

	RenderPassOutputs outputs = {
		.depth_format = SDL_GPU_TEXTUREFORMAT_INVALID,
	};

	auto fb_depth_attachment = &framebuffer->attachments[FRAMEBUFFER_ATTACH_DEPTH];

	if(
		attachment_mask & (1u << FRAMEBUFFER_ATTACH_DEPTH) &&
		fb_depth_attachment->texture &&
		fb_depth_attachment->texture->load.op == SDL_GPU_LOADOP_CLEAR
	) {
		outputs.depth_stencil = (SDL_GPUDepthStencilTargetInfo) {
			.clear_depth = fb_depth_attachment->texture->load.clear.depth,
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
			.store_op = SDL_GPU_STOREOP_STORE,
			.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
			.texture = fb_depth_attachment->texture->gpu_texture,
			.cycle = true,   // FIXME add a client hint for cycling?
		};

		outputs.depth_format = fb_depth_attachment->texture->gpu_format;
		fb_depth_attachment->texture->load.op = SDL_GPU_LOADOP_LOAD;
	}

	for(uint i = FRAMEBUFFER_ATTACH_COLOR0; i < ARRAY_SIZE(framebuffer->attachments); ++i) {
		if(
			attachment_mask & (1u << i) &&
			framebuffer->attachments[i].texture &&
			framebuffer->attachments[i].texture->load.op == SDL_GPU_LOADOP_CLEAR
		) {
			outputs.color[outputs.num_color_attachments++] = (SDL_GPUColorTargetInfo) {
				.clear_color = framebuffer->attachments[i].texture->load.clear.color.sdl_fcolor,
				.load_op = SDL_GPU_LOADOP_CLEAR,
				.store_op = SDL_GPU_STOREOP_STORE,
				.texture = framebuffer->attachments[i].texture->gpu_texture,
				.mip_level = framebuffer->attachments[i].mip_level,
				.layer_or_depth_plane = 0,
				.cycle = true,   // FIXME add a client hint for cycling?
			};

			outputs.color_formats[i] = framebuffer->attachments[i].texture->gpu_format;
			framebuffer->attachments[i].texture->load.op = SDL_GPU_LOADOP_LOAD;
		}
	}

	// log_debug("Num color attachments to clear: %u", num_clear_attachments);
	// log_debug("%s the depth attachment", depth_attachment.texture ? "Clearing" : "Not clearing");

	if(outputs.num_color_attachments > 0 || outputs.depth_stencil.texture) {
		sdlgpu_begin_or_resume_render_pass(&outputs);
	}
}

static void sdlgpu_framebuffer_setup_outputs_default_framebuffer(RenderPassOutputs *outputs) {
	SDL_GPUTexture *swapchain_tex = sdlgpu.frame.swapchain.tex;

	if(sdlgpu.frame.faux_backbuffer.tex) {
		swapchain_tex = sdlgpu.frame.faux_backbuffer.tex;
		assert(sdlgpu.frame.faux_backbuffer.fmt == sdlgpu.frame.swapchain.fmt);
	}

	outputs->color[0] = (SDL_GPUColorTargetInfo) {
		.texture = swapchain_tex,
		.clear_color = sdlgpu.st.default_framebuffer.color.clear.color.sdl_fcolor,
		.load_op = sdlgpu.st.default_framebuffer.color.op,
		.store_op = swapchain_tex ? SDL_GPU_STOREOP_STORE : SDL_GPU_STOREOP_DONT_CARE,
	};

	outputs->color_formats[0] = sdlgpu.frame.swapchain.fmt;
	outputs->depth_format = SDL_GPU_TEXTUREFORMAT_INVALID;
	outputs->num_color_attachments = swapchain_tex ? 1 : 0;

	sdlgpu.st.default_framebuffer.color.op = SDL_GPU_LOADOP_LOAD;
	sdlgpu.st.default_framebuffer.depth.op = SDL_GPU_LOADOP_LOAD;
}

void sdlgpu_framebuffer_setup_outputs(Framebuffer *framebuffer, RenderPassOutputs *outputs) {
	if(framebuffer == NULL) {
		sdlgpu_framebuffer_setup_outputs_default_framebuffer(outputs);
		return;
	}

	*outputs = (RenderPassOutputs) { };

	auto fb_depth_attachment = &framebuffer->attachments[FRAMEBUFFER_ATTACH_DEPTH];

	if(fb_depth_attachment->texture) {
		outputs->depth_format = fb_depth_attachment->texture->gpu_format;
		outputs->depth_stencil = (SDL_GPUDepthStencilTargetInfo) {
			.clear_depth = fb_depth_attachment->texture->load.clear.depth,
			.load_op = fb_depth_attachment->texture->load.op,
			.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
			.store_op = SDL_GPU_STOREOP_STORE,
			.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
			.texture = fb_depth_attachment->texture->gpu_texture,
			// FIXME add a client hint for cycling?
			.cycle = fb_depth_attachment->texture->load.op == SDL_GPU_LOADOP_CLEAR,
		};

		// fb_depth_attachment->texture->is_virgin = false;
		fb_depth_attachment->texture->load.op = SDL_GPU_LOADOP_LOAD;
	} else {
		outputs->depth_format = SDL_GPU_TEXTUREFORMAT_INVALID;
	}

	for(int i = 0; i < FRAMEBUFFER_MAX_OUTPUTS; ++i) {
		// FIXME we can't have sparse attachments
		FramebufferAttachment target = framebuffer->output_mapping[i];

		if(target == FRAMEBUFFER_ATTACH_NONE) {
			continue;
		}

		auto attachment = &framebuffer->attachments[target];

		if(attachment->texture == NULL) {
			continue;
		}

		if(attachment->texture->load.op == SDL_GPU_LOADOP_LOAD) {
			sdlgpu_texture_prepare(attachment->texture);
		}

		uint a = outputs->num_color_attachments++;
		outputs->color[a] = (SDL_GPUColorTargetInfo) {
			.clear_color = attachment->texture->load.clear.color.sdl_fcolor,
			.load_op = attachment->texture->load.op,
			.store_op = SDL_GPU_STOREOP_STORE,
			.texture = attachment->texture->gpu_texture,
			.mip_level = attachment->mip_level,
			.layer_or_depth_plane = 0,
			// FIXME add a client hint for cycling?
			.cycle = attachment->texture->load.op == SDL_GPU_LOADOP_CLEAR,
		};
		outputs->color_formats[a] = attachment->texture->gpu_format;

		// attachment->texture->is_virgin = false;
		attachment->texture->load.op = SDL_GPU_LOADOP_LOAD;
	}
}
