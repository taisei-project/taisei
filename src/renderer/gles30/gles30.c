/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "gles30.h"
#include "../glescommon/gles.h"
#include "../glescommon/texture.h"

static void gles30_init(void) {
	gles_init(&_r_backend_gles30, 3, 0);
}

RendererBackend _r_backend_gles30 = {
	.name = "gles30",
	.funcs = {
		.init = gles30_init,
		.screenshot = gles_screenshot,
	},
	.custom = &(GLBackendData) {
		.vtable = {
			.texture_type_info = gles_texture_type_info,
			.init_context = gles_init_context,
		}
	},
};
