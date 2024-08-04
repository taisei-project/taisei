/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "sdlgpu.h"

enum {
	PIPECACHE_KEY_TEXFMT_BITS = 6,
	PIPECACHE_FMT_MASK = (1 << PIPECACHE_KEY_TEXFMT_BITS) - 1,
};

typedef struct PipelineCacheKey {
	uint32_t blend : 28;      // enum: BlendMode
	uint32_t attachment_formats : (FRAMEBUFFER_MAX_OUTPUTS * PIPECACHE_KEY_TEXFMT_BITS);
	sdlgpu_id_t shader_program;
	sdlgpu_id_t vertex_array;
	uint8_t primitive : 4;    // enum: Primitive
	uint8_t cull_mode : 2;    // enum: CullFaceMode; 0 = disabled
	uint8_t depth_func : 7;   // enum: DepthTestFunc
	uint8_t depth_test : 1;
	uint8_t depth_write : 1;
} PipelineCacheKey;

typedef struct PipelineDescription {
	ShaderProgram *shader_program;
	VertexArray *vertex_array;
	CullFaceMode cull_mode;       // must be 0 if disabled in cap_bits
	DepthTestFunc depth_func;     // msut be DEPTH_ALWAYS if disabled in cap_bits
	BlendMode blend_mode;
	Primitive primitive;
	r_capability_bits_t cap_bits;

	uint num_outputs;
	struct {
		SDL_GpuTextureFormat format;  // must be PIPECACHE_FMT_MASK for unused outputs
	} outputs[FRAMEBUFFER_MAX_OUTPUTS];
} PipelineDescription;

void sdlgpu_pipecache_init(void);
void sdlgpu_pipecache_wipe(void);
SDL_GpuGraphicsPipeline *sdlgpu_pipecache_get(PipelineDescription *pd);
void sdlgpu_pipecache_deinit(void);
