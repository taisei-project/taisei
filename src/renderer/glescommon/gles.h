/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "../glcommon/vtable.h"

void gles_init(RendererBackend *gles_backend, int major, int minor);
void gles_init_context(SDL_Window *w);
bool gles_screenshot(Pixmap *out);
