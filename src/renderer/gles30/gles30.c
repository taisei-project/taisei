/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "gles30.h"
#include "../glescommon/gles.h"

static void gles30_init(void) {
	gles_init(&_r_backend_gles30, 3, 0);
}

RendererBackend _r_backend_gles30 = {
	.name = "gles30",
	.funcs = {
		.init = gles30_init,
		.texture_dump = gles_texture_dump,
		.screenshot = gles_screenshot,
	},
	.custom = &(GLBackendData) {
		.vtable = {
			.init_context = gles_init_context,
		}
	},
};
