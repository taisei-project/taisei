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
#include "../common/backend.h"

typedef struct GLTextureTypeInfo {
	GLuint internal_fmt;
	GLuint external_fmt;
	GLuint component_type;
	size_t pixel_size;
} GLTextureTypeInfo;

typedef struct GLVTable {
	GLTextureTypeInfo* (*texture_type_info)(TextureType type);
	void (*init_context)(SDL_Window *window);
} GLVTable;

typedef struct GLBackendData {
	GLVTable vtable;
} GLBackendData;

#define GLVT_OF(backend) (((GLBackendData*)backend.custom)->vtable)
#define GLVT GLVT_OF(_r_backend)
