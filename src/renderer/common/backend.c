/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "backend.h"

#undef R
#define R(x) extern RendererBackend _r_backend_##x;
TAISEI_BUILDCONF_RENDERER_BACKENDS
#undef R

RendererBackend *_r_backends[] = {
	#define R(x) &_r_backend_##x,
	TAISEI_BUILDCONF_RENDERER_BACKENDS
	#undef R
	NULL,
};

RendererBackend _r_backend;

static void _r_set_backend(RendererBackend *backend) {
	memcpy(&_r_backend, backend, sizeof(_r_backend));
}

static RendererBackend* _r_find_backend(const char *name) {
	for(RendererBackend **b = _r_backends; *b; ++b) {
		if(!strcmp((*b)->name, name)) {
			return *b;
		}
	}

	log_fatal("Unknown renderer backend '%s'", name);
}

void _r_backend_init(void) {
	static bool initialized;

	if(initialized) {
		return;
	}

	const char *backend = env_get("TAISEI_RENDERER", "");

	if(!*backend) {
		backend = TAISEI_BUILDCONF_RENDERER_DEFAULT;
	}

	RendererBackend *bptr = _r_find_backend(backend);
	bptr->funcs.init();
	_r_set_backend(bptr);

	initialized = true;
}

void _r_backend_inherit(RendererBackend *dest, const RendererBackend *base) {
	static const uint num_funcs = sizeof(dest->funcs)/sizeof(dest->funcs.init);
	inherit_missing_pointers(num_funcs, (void**)&dest->funcs, (void**)&base->funcs);
}
