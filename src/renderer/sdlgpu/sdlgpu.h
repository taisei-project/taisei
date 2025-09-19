/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "framebuffer.h"

#include "../api.h"  // IWYU pragma: export
#include "../common/backend.h"  // IWYU pragma: export

#include "util.h"

#include <SDL3/SDL_gpu.h>

typedef uint16_t sdlgpu_id_t;

typedef enum CommandBufferID {
	CBUF_UPLOAD,
	CBUF_DRAW,

	NUM_CBUFS,
} CommandBufferID;

typedef struct SDLGPUGlobal {
	SDL_GPUDevice *device;
	SDL_Window *window;

	Texture *null_textures[NUM_TEXTURE_CLASSES];

	struct {
		union {
			struct {
				SDL_GPUCommandBuffer *upload_cbuf;
				SDL_GPUCommandBuffer *cbuf;
			};
			SDL_GPUCommandBuffer *command_buffers[NUM_CBUFS];
		};

		struct {
			SDL_GPUTexture *tex;
			SDL_GPUTextureFormat fmt;
			SDL_GPUPresentMode present_mode;
			SDL_GPUPresentMode next_present_mode;
			uint32_t width;
			uint32_t height;
		} swapchain;

		struct {
			SDL_GPUTexture *tex;
			SDL_GPUTextureFormat fmt;
			uint32_t width;
			uint32_t height;
		} faux_backbuffer;

		uint counter;
	} frame;

	struct {
		ShaderProgram *shader;
		Framebuffer *framebuffer;
		Framebuffer *prev_framebuffer;
		DefaultFramebufferState default_framebuffer;
		r_capability_bits_t caps;
		Color default_color;
		BlendMode blend;
		CullFaceMode cull;
		DepthTestFunc depth_func;
		IntRect scissor;
	} st;

	struct {
		SDL_GPURenderPass *pass;
		RenderPassOutputs outputs;
	} render_pass;

	struct {
		SDL_GPUCopyPass *for_cbuf[NUM_CBUFS];
	} copy_pass;

	struct {
		sdlgpu_id_t shader_programs;
		sdlgpu_id_t vertex_arrays;
	} ids;

	bool debug;
	StringBuffer debug_buf;
} SDLGPUGlobal;

extern SDLGPUGlobal sdlgpu;

INLINE SDL_GPUPrimitiveType sdlgpu_primitive_ts2sdl(Primitive p) {
	static const SDL_GPUPrimitiveType mapping[] = {
		[PRIM_POINTS] = SDL_GPU_PRIMITIVETYPE_POINTLIST,
		[PRIM_LINE_STRIP] = SDL_GPU_PRIMITIVETYPE_LINESTRIP,
		[PRIM_LINES] = SDL_GPU_PRIMITIVETYPE_LINELIST,
		[PRIM_TRIANGLE_STRIP] = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
		[PRIM_TRIANGLES] = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
	};

	uint idx = p;
	assume(idx < ARRAY_SIZE(mapping));
	return mapping[idx];
}

INLINE SDL_GPUCullMode sdlgpu_cullmode_ts2sdl(CullFaceMode m) {
	static const SDL_GPUCullMode mapping[] = {
		[CULL_BACK] = SDL_GPU_CULLMODE_BACK,
		[CULL_FRONT] = SDL_GPU_CULLMODE_FRONT,
		[CULL_BOTH] = SDL_GPU_CULLMODE_NONE,  // FIXME?
	};

	uint idx = m;
	assume(idx < ARRAY_SIZE(mapping));
	return mapping[idx];
}

INLINE SDL_GPUBlendOp sdlgpu_blendop_ts2sdl(BlendOp op) {
	static const SDL_GPUBlendOp mapping[] = {
		[BLENDOP_ADD] = SDL_GPU_BLENDOP_ADD,
		[BLENDOP_SUB] = SDL_GPU_BLENDOP_SUBTRACT,
		[BLENDOP_REV_SUB] = SDL_GPU_BLENDOP_REVERSE_SUBTRACT,
		[BLENDOP_MIN] = SDL_GPU_BLENDOP_MIN,
		[BLENDOP_MAX] = SDL_GPU_BLENDOP_MAX,
	};

	uint idx = op;
	assume(idx < ARRAY_SIZE(mapping));
	return mapping[idx];
}

INLINE SDL_GPUBlendFactor sdlgpu_blendfactor_ts2sdl(BlendFactor f) {
	static const SDL_GPUBlendFactor mapping[] = {
		[BLENDFACTOR_ZERO] = SDL_GPU_BLENDFACTOR_ZERO,
		[BLENDFACTOR_ONE] = SDL_GPU_BLENDFACTOR_ONE,
		[BLENDFACTOR_SRC_COLOR] = SDL_GPU_BLENDFACTOR_SRC_COLOR,
		[BLENDFACTOR_INV_SRC_COLOR] = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR,
		[BLENDFACTOR_SRC_ALPHA] = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
		[BLENDFACTOR_INV_SRC_ALPHA] = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
		[BLENDFACTOR_DST_COLOR] = SDL_GPU_BLENDFACTOR_DST_COLOR,
		[BLENDFACTOR_INV_DST_COLOR] = SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR,
		[BLENDFACTOR_DST_ALPHA] = SDL_GPU_BLENDFACTOR_DST_ALPHA,
		[BLENDFACTOR_INV_DST_ALPHA] = SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA,

		/* Not supported:
		 *  SDL_GPU_BLENDFACTOR_CONSTANT_COLOR
    	 * 	SDL_GPU_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR
    	 *  SDL_GPU_BLENDFACTOR_SRC_ALPHA_SATURATE
		 */
	};

	uint idx = f;
	assume(idx < ARRAY_SIZE(mapping));
	return mapping[idx];
}

INLINE SDL_GPUCompareOp sdlgpu_cmpop_ts2sdl(DepthTestFunc f) {
	static const SDL_GPUCompareOp mapping[] = {
		[DEPTH_ALWAYS] = SDL_GPU_COMPAREOP_ALWAYS,
		[DEPTH_EQUAL] = SDL_GPU_COMPAREOP_EQUAL,
		[DEPTH_GEQUAL] = SDL_GPU_COMPAREOP_GREATER_OR_EQUAL,
		[DEPTH_GREATER] = SDL_GPU_COMPAREOP_GREATER,
		[DEPTH_LEQUAL] = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
		[DEPTH_LESS] = SDL_GPU_COMPAREOP_LESS,
		[DEPTH_NEVER] = SDL_GPU_COMPAREOP_NEVER,
		[DEPTH_NOTEQUAL] = SDL_GPU_COMPAREOP_NOT_EQUAL,
	};

	uint idx = f;
	assume(idx < ARRAY_SIZE(mapping));
	return mapping[idx];
}

void sdlgpu_renew_swapchain_texture(void);
SDL_GPUTexture *sdlgpu_get_swapchain_texture(void);

SDL_GPURenderPass *sdlgpu_begin_or_resume_render_pass(RenderPassOutputs *outputs);
SDL_GPUCopyPass *sdlgpu_begin_or_resume_copy_pass(CommandBufferID cbuf_id);
void sdlgpu_stop_current_pass(CommandBufferID cbuf_id);

void sdlgpu_cmdbuf_debug(SDL_GPUCommandBuffer *cbuf, const char *format, ...);
