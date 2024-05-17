/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "../api.h"
#include "../common/backend.h"
#include "common_buffer.h"
#include "opengl.h"
#include "resource/texture.h"

typedef struct TextureUnit TextureUnit;

typedef enum BufferBindingIndex {
	GL33_BUFFER_BINDING_ARRAY,
	GL33_BUFFER_BINDING_COPY_WRITE,
	GL33_BUFFER_BINDING_PIXEL_UNPACK,
	GL33_BUFFER_BINDING_PIXEL_PACK,

	GL33_NUM_BUFFER_BINDINGS,

	GL33_BUFFER_BINDING_ELEMENT_ARRAY,  // super special case!
} BufferBindingIndex;

// Internal helper functions

GLenum gl33_prim_to_gl_prim(Primitive prim);
void gl33_begin_draw(VertexArray *varr, void **state);
void gl33_end_draw(void *state);

// Allocates an OpenGL texturing unit for `texture`.
// `texture->binding_unit` is set to point to the gl33 TextureUnit struct.
// The texture will become bound to the unit on next state sync.
// To force a sync, call gl33_sync_texunit(texture->binding_unit, ...) next.
//
// `texture` may be NULL, in which case a unit with empty binding will be choosen.
//
// If `lock_target` is not 0, the unit becomes "locked", so that subsequent calls to
// gl33_bind_texture can not clobber it. The lock remains active until gl33_sync_texunit
// is called with prepare_rendering=true, which normally happens before every draw call.
//
// The non-0 value of `lock_target` must be a valid texture target, like TEXTURE_2D.
// If `texture` is not NULL, then `lock_target` must be equal to `texture->binding_target`.
// Distinct values are useful with NULL textures: gl33_bind_texture will always distinct samplers
// for distinct lock_target values.
//
// If `preferred_unit` is >= 0, it suggests which texturing unit to choose. This is a mere hint,
// it is not guaranteed to be honored.
//
// Returns the numeric ID of the choosen texturing unit.
uint gl33_bind_texture(Texture *texture, GLuint lock_target, int preferred_unit);

void gl33_bind_vao(GLuint vao);
void gl33_sync_vao(void);
GLuint gl33_vao_current(void);

void gl33_bind_buffer(BufferBindingIndex bindidx, GLuint gl_handle);
void gl33_sync_buffer(BufferBindingIndex bindidx);
GLuint gl33_buffer_current(BufferBindingIndex bindidx);
GLenum gl33_bindidx_to_glenum(BufferBindingIndex bindidx);

void gl33_set_clear_color(const Color *color);
void gl33_set_clear_depth(float depth);

void gl33_sync_shader(void);
void gl33_sync_texunit(TextureUnit *unit, bool prepare_rendering, bool ensure_active) attr_nonnull(1);
void gl33_sync_texunits(bool prepare_rendering);
void gl33_sync_framebuffer(void);
void gl33_sync_blend_mode(void);
void gl33_sync_cull_face_mode(void);
void gl33_sync_depth_test_func(void);
void gl33_sync_capabilities(void);
void gl33_sync_scissor(void);

void gl33_buffer_deleted(CommonBuffer *cbuf);
void gl33_vertex_buffer_deleted(VertexBuffer *vbuf);
void gl33_vertex_array_deleted(VertexArray *varr);
void gl33_texture_deleted(Texture *tex);
void gl33_framebuffer_deleted(Framebuffer *fb);
void gl33_shader_deleted(ShaderProgram *prog);

void gl33_texture_pointer_renamed(Texture *pold, Texture *pnew);

extern RendererBackend _r_backend_gl33;
