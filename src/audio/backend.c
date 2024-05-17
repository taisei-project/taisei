/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "backend.h"

#include "util/env.h"

#undef A
#define A(x) extern AudioBackend _a_backend_##x;
TAISEI_BUILDCONF_AUDIO_BACKENDS
#undef A

AudioBackend *_a_backends[] = {
	#define A(x) &_a_backend_##x,
	TAISEI_BUILDCONF_AUDIO_BACKENDS
	#undef A
	NULL,
};

AudioBackend _a_backend;

static AudioBackend *_audio_find_backend(const char *name) {
	for(AudioBackend **b = _a_backends; *b; ++b) {
		if(!strcmp((*b)->name, name)) {
			return *b;
		}
	}

	log_error("Unknown audio backend '%s'", name);
	return NULL;
}

static bool _audio_backend_try_init(AudioBackend *backend) {
	_a_backend = *backend;
	log_debug("Trying %s", _a_backend.name);
	return _a_backend.funcs.init();
}

bool audio_backend_init(void) {
	const char *name = env_get("TAISEI_AUDIO_BACKEND", "");
	bool initialized = false;

	if(!*name) {
		name = TAISEI_BUILDCONF_AUDIO_DEFAULT;
	}

	AudioBackend *backend = _audio_find_backend(name);

	if(backend) {
		initialized = _audio_backend_try_init(backend);
	}

	for(AudioBackend **b = _a_backends; *b && !initialized; ++b) {
		if(*b != backend) {
			initialized = _audio_backend_try_init(*b);
		}
	}

	return initialized;
}
