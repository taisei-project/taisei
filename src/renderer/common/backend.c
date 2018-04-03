/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

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

	const char *backend = getenv("TAISEI_RENDERER");

	if(!backend || !*backend) {
		backend = TAISEI_BUILDCONF_RENDERER_DEFAULT;
	}

	_r_set_backend(_r_find_backend(backend));
	_r_backend.funcs.init();

	initialized = true;
}
