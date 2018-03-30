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

#define GL33_MAX_TEXUNITS 8

// Internal helper functions

uint gl33_active_texunit(void);
uint gl33_activate_texunit(uint unit);
void gl33_bind_pbo(GLuint pbo);

void gl33_debug_object_label(GLenum identifier, GLuint name, const char *label);

void gl33_sync_shader(void);
void gl33_sync_texunit(uint unit);
void gl33_sync_texunits(void);
void gl33_sync_render_target(void);
void gl33_sync_vertex_buffer(void);
void gl33_sync_blend_mode(void);
void gl33_sync_cull_face_mode(void);
void gl33_sync_depth_test_func(void);
void gl33_sync_capabilities(void);

void gl33_vertex_buffer_deleted(VertexBuffer *vbuf);
void gl33_texture_deleted(Texture *tex);
void gl33_render_target_deleted(RenderTarget *target);
void gl33_shader_deleted(ShaderProgram *prog);
