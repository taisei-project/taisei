/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#include "renderer/api.h"

void gles30_fbcopyfallback_init(void);
void gles30_fbcopyfallback_shutdown(void);
void gles30_fbcopyfallback_framebuffer_copy(Framebuffer *dst, Framebuffer *src, BufferKindFlags flags);
