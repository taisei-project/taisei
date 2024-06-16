/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "opengl.h"

bool glcommon_debug_requested(void);
void glcommon_debug_enable(void);
void glcommon_set_debug_label(char *label_storage, const char *kind_name, GLenum gl_enum, GLuint gl_handle, const char *label);
void glcommon_set_debug_label_local(char *label_storage, const char *kind_name, GLuint gl_handle, const char *label);
void glcommon_set_debug_label_gl(GLenum identifier, GLuint name, const char *label);
