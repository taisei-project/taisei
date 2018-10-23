/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "gles.h"
#include "texture.h"
#include "../common/backend.h"
#include "../gl33/gl33.h"

void gles_init(RendererBackend *gles_backend, int major, int minor) {
	_r_backend_inherit(gles_backend, &_r_backend_gl33);
	glcommon_setup_attributes(SDL_GL_CONTEXT_PROFILE_ES, major, minor, 0);
	glcommon_load_library();
}

void gles_init_context(SDL_Window *w) {
	GLVT_OF(_r_backend_gl33).init_context(w);
	gles_init_texformats_table();
}

bool gles_screenshot(Pixmap *out) {
	IntRect vp;
	r_framebuffer_viewport_current(NULL, &vp);
	out->width = vp.w;
	out->height = vp.h;
	out->format = PIXMAP_FORMAT_RGBA8;
	out->origin = PIXMAP_ORIGIN_BOTTOMLEFT;
	out->data.untyped = pixmap_alloc_buffer_for_copy(out);
	glReadPixels(vp.x, vp.y, vp.w, vp.h, GL_RGBA, GL_UNSIGNED_BYTE, out->data.untyped);
	return true;
}
