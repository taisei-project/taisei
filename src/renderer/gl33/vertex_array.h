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

struct VertexArrayImpl {
	GLuint gl_handle;
	VertexBuffer **attachments;
	uint num_attachments;
	VertexAttribFormat *attribute_layout;
	uint num_attributes;
};
