/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "../api.h"

#define RSTATE_FLAGS \
	RSTATE(CAPABILITIES) \
	RSTATE(COLOR) \
	RSTATE(BLENDMODE) \
	RSTATE(CULLMODE) \
	RSTATE(DEPTHFUNC) \
	RSTATE(SHADER) \
	RSTATE(SHADER_UNIFORMS) \
	RSTATE(RENDERTARGET) \
	RSTATE(SCISSOR) \

typedef enum RendererStateID {
	#define RSTATE(id) RSTATE_ID_##id,
	RSTATE_FLAGS
	#undef RSTATE
	NUM_RSTATE_FLAGS,
} RendererStateID;

typedef enum RendererStateFlags {
	#define RSTATE(id) RSTATE_##id = (1 << RSTATE_ID_##id),
	RSTATE_FLAGS
	#undef RSTATE
} RendererStateFlags;

typedef uint32_t r_state_bits_t;

typedef struct RendererStateRollback {
	r_state_bits_t dirty_bits;

	r_capability_bits_t capabilities;
	Color color;
	BlendMode blend_mode;
	CullFaceMode cull_mode;
	DepthTestFunc depth_func;
	ShaderProgram *shader;
	// TODO uniforms
	Framebuffer *framebuffer;
	IntRect scissor;
} RendererStateRollback;

void _r_state_touch_capabilities(void);
void _r_state_touch_color(void);
void _r_state_touch_blend_mode(void);
void _r_state_touch_cull_mode(void);
void _r_state_touch_depth_func(void);
void _r_state_touch_shader(void);
void _r_state_touch_uniform(Uniform *uniform);
void _r_state_touch_framebuffer(void);
void _r_state_touch_scissor(void);

void _r_state_init(void);
void _r_state_shutdown(void);
