/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "opengl.h"
#include "../api.h"
#include "resource/resource.h"
#include "resource/texture.h"

typedef struct TextureImpl {
	GLuint gl_handle;
	GLuint pbo;
	GLenum fmt_internal;
	GLenum fmt_external;
} TextureImpl;
