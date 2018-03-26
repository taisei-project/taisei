/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "util.h"
#include "hashtable.h"
#include "../api.h"
#include "../common/opengl.h"
#include "resource/resource.h"
#include "resource/shader_program.h"

struct ShaderProgram {
	GLuint gl_handle;
	Hashtable *uniforms;
	int renderctx_block_idx;
};

typedef void (*UniformSetter)(Uniform *uniform, uint count, const void *data);

struct Uniform {
	ShaderProgram *prog;
	uint location;
	UniformType type;
	void *cache;
	size_t cache_size;
	uint update_count;
};

void gl33_sync_uniforms(ShaderProgram *prog);
