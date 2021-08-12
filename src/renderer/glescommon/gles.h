/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "../glcommon/vtable.h"

void gles_init(RendererBackend *gles_backend, int major, int minor);
void gles_init_context(SDL_Window *w);
bool gles_screenshot(Pixmap *out);
bool gles_texture_dump(Texture *tex, uint mipmap, uint layer, Pixmap *dst);
