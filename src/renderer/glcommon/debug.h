/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "opengl.h"

bool glcommon_debug_requested(void);
void glcommon_debug_enable(void);
void glcommon_debug_object_label(GLenum identifier, GLuint name, const char *label);
void glcommon_set_debug_label(char *label_storage, const char *kind_name, GLenum gl_enum, GLuint gl_handle, const char *label);
