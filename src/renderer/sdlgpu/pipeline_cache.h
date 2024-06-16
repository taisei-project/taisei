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
	PIPECACHE_KEY_TEXFMT_BITS = 7,
	PIPECACHE_FMT_MASK = (1 << PIPECACHE_KEY_TEXFMT_BITS) - 1,
};

typedef struct PipelineCacheKey {
	uint32_t blend : 28;      // enum: BlendMode
	uint8_t cull_mode : 2;    // enum: CullFaceMode; 0 = disabled
	uint8_t depth_test : 1;
	uint8_t depth_write : 1;
	uint8_t front_face_ccw : 1;
	uint8_t primitive : 3;    // enum: Primitive
	uint32_t attachment_formats : (FRAMEBUFFER_MAX_OUTPUTS * PIPECACHE_KEY_TEXFMT_BITS);
	uint32_t depth_format : PIPECACHE_KEY_TEXFMT_BITS;
	uint8_t depth_func : 3;   // enum: DepthTestFunc
	sdlgpu_id_t shader_program;
	sdlgpu_id_t vertex_array;
} PipelineCacheKey;

typedef struct PipelineDescription {
	ShaderProgram *shader_program;
	VertexArray *vertex_array;
	CullFaceMode cull_mode;       // must be 0 if disabled in cap_bits
	DepthTestFunc depth_func;     // msut be DEPTH_ALWAYS if disabled in cap_bits
	BlendMode blend_mode;
	Primitive primitive;
	r_capability_bits_t cap_bits;
	SDL_GPUFrontFace front_face;

	uint num_outputs;
	struct {
		SDL_GPUTextureFormat format;
	} outputs[FRAMEBUFFER_MAX_OUTPUTS];
	SDL_GPUTextureFormat depth_format;
} PipelineDescription;

void sdlgpu_pipecache_init(void);
void sdlgpu_pipecache_wipe(void);
SDL_GPUGraphicsPipeline *sdlgpu_pipecache_get(PipelineDescription *pd);
void sdlgpu_pipecache_deinit(void);
void sdlgpu_pipecache_unref_shader_program(sdlgpu_id_t shader_id);
void sdlgpu_pipecache_unref_vertex_array(sdlgpu_id_t va_id);
