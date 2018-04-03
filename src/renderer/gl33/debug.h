/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "opengl.h"

bool gl33_debug_requested(void);
void gl33_debug_enable(void);
void gl33_debug_object_label(GLenum identifier, GLuint name, const char *label);
