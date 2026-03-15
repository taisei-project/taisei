/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "backend.h"

#include "memory/scratch.h"
#include "util.h"
#include "util/env.h"

#define R_BACKEND_(x) (_r_backend_##x)
#define R_BACKEND(x) MACROHAX_EXPAND(MACROHAX_DEFER(R_BACKEND_)(x))

#undef R
#define R(x) extern RendererBackend R_BACKEND(x);
TAISEI_BUILDCONF_RENDERER_BACKENDS
#undef R

RendererBackend *_r_backends[] = {
	#define R(x) &R_BACKEND(x),
	TAISEI_BUILDCONF_RENDERER_BACKENDS
	#undef R
	NULL,
};

RendererBackend *_r_fallback_backends[] = {
	#define R(x) &R_BACKEND(x),
	TAISEI_BUILDCONF_RENDERER_FALLBACK_BACKENDS
	#undef R
	NULL,
};

RendererBackend _r_backend;

static void _r_set_backend(RendererBackend *backend) {
	memcpy(&_r_backend, backend, sizeof(_r_backend));
}

static RendererBackend *_r_find_backend(const char *name) {
	for(RendererBackend **b = _r_backends; *b; ++b) {
		if(!strcmp((*b)->name, name)) {
			return *b;
		}
	}

	return NULL;
}

static bool _r_backend_try_init(RendererBackend *backend, char *opts) {
	if(opts && !*opts) {
		opts = NULL;
	}

	if(opts) {
		log_info("Initializing renderer backend %s with options: %s", backend->name, opts);
	} else {
		log_info("Initializing renderer backend %s", backend->name);
	}

	if(!backend->funcs.init(backend, opts)) {
		backend->funcs.shutdown();
		log_error("Failed to initialize renderer backend %s", backend->name);
		return false;
	}

	_r_set_backend(backend);
	log_info("Renderer backend %s initialized", backend->name);
	return true;
}

void _r_backend_init(void) {
	static bool initialized;

	if(initialized) {
		return;
	}

	auto scratch = acquire_scratch_arena();

	RendererBackend *backend;
	char *name = marena_strdup(scratch, env_get("TAISEI_RENDERER", ""));
	char *opts = NULL;
	bool try_fallbacks = true;

	if(*name) {
		char *sep = strchr(name, ':');
		if(sep) {
			*sep = '\0';
			opts = sep + 1;
		}

		try_fallbacks = false;
		backend = _r_find_backend(name);
		if(!backend) {
			log_fatal("Unknown renderer backend '%s'", name);
		}
	} else {
		backend = &R_BACKEND(TAISEI_BUILDCONF_RENDERER_DEFAULT);
	}

	if(_r_backend_try_init(backend, opts)) {
		goto done;
	}

	if(try_fallbacks) {
		for(uint i = 0; i < ARRAY_SIZE(_r_fallback_backends); ++i) {
			auto b = _r_fallback_backends[i];
			if(b != backend && _r_backend_try_init(b, NULL)) {
				goto done;
			}
		}
	}

	log_fatal("Could not initialize the rendering subsystem");
	UNREACHABLE;

done:
	initialized = true;
	release_scratch_arena(scratch);
}

void _r_backend_inherit(RendererBackend *dest, const RendererBackend *base) {
	static const uint num_funcs = sizeof(dest->funcs)/sizeof(dest->funcs.init);
	inherit_missing_pointers(num_funcs, (void**)&dest->funcs, (void**)&base->funcs);
}
