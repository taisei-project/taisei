/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "../api.h"
#include "opengl.h"
#include "resource/texture.h"
#include "../common/backend.h"

// Internal helper functions

uint gl33_active_texunit(void);
uint gl33_activate_texunit(uint unit);

void gl33_bind_pbo(GLuint pbo);
void gl33_bind_vao(GLuint vao);
void gl33_bind_vbo(GLuint vbo);

void gl33_sync_shader(void);
void gl33_sync_texunit(uint unit, bool prepare_rendering);
void gl33_sync_texunits(bool prepare_rendering);
void gl33_sync_framebuffer(void);
void gl33_sync_vertex_array(void);
void gl33_sync_blend_mode(void);
void gl33_sync_cull_face_mode(void);
void gl33_sync_depth_test_func(void);
void gl33_sync_capabilities(void);
void gl33_sync_vao(void);
void gl33_sync_vbo(void);

GLuint gl33_vao_current(void);
GLuint gl33_vbo_current(void);

void gl33_vertex_buffer_deleted(VertexBuffer *vbuf);
void gl33_vertex_array_deleted(VertexArray *varr);
void gl33_texture_deleted(Texture *tex);
void gl33_framebuffer_deleted(Framebuffer *fb);
void gl33_shader_deleted(ShaderProgram *prog);

extern RendererBackend _r_backend_gl33;
