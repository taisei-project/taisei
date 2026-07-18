/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "backend.h"

#include "config.h"
#include "i18n/i18n.h"
#include "memory/scratch.h"
#include "util.h"
#include "util/choice_dialog.h"
#include "util/env.h"
#include "version.h"
#include "video.h"

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
	if(!name || !*name || !strcmp(name, "default")) {
		return &R_BACKEND(TAISEI_BUILDCONF_RENDERER_DEFAULT);
	}

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

static char *parse_renderer_string(MemArena *arena, const char *s, char **out_opts) {
	if(!s || !*s) {
		return NULL;
	}

	char *name = marena_strdup(arena, s);
	char *sep = strchr(name, ':');
	*out_opts = NULL;

	if(sep) {
		*sep = '\0';
		*out_opts = sep + 1;
	}

	return name;
}

static const char *get_config_renderer(MemArena *arena) {
	auto cfg_val = config_get_str(CONFIG_RENDERER);

#ifndef _WIN32
	return cfg_val;
#else
	// We only really need this for Windows, and show_choice_dialog() is currently only implemented for win32

	if(!env_get("TAISEI_RENDERER_NAG", true)) {
		return cfg_val;
	}

	struct {
		const char *label;
		const char *value;
	} choices[] = {
		// TODO determine which ones are actually available
		{ _("Automatic"), "default" },
		{ "Vulkan",       "sdlgpu:vulkan" },
		{ "Direct3D 12",  "sdlgpu:direct3d12" },
		{ "OpenGL 3.3",   "gl33" },
	};

	int selection = 0;
	const char *labels[countof(choices)];

	for(int i = 0; i < countof(choices); ++i) {
		if(!strcmp(cfg_val, choices[i].value)) {
			selection = i;
		}

		labels[i] = choices[i].label;
	}

	StringBuffer buf = { arena };
	strbuf_printf(&buf, "%s v%s", WINDOW_TITLE, TAISEI_VERSION);
	char *title = strbuf_commit(&buf);

	int idx = show_choice_dialog(arena, &(ChoiceDialogParams) {
		.choices = labels,
		.num_choices = countof(labels),
		.initial_selection = selection,
		.cancel_label = _("Cancel"),
		.ok_label = _("Run"),
		.window_title = title,
		.text = _(
			"Select the rendering API.\n\n"
			"Change this if the game crashes or displays incorrectly.\n"),
	});

	if(idx < 0) {
		exit(0);
	}

	assume(idx < countof(choices));

	auto val = choices[idx].value;
	config_set_str(CONFIG_RENDERER, val);

	return val;
#endif
}

void _r_backend_init(void) {
	static bool initialized;

	if(initialized) {
		return;
	}

	auto scratch = acquire_scratch_arena();

	RendererBackend *backend;
	char *opts = NULL;
	const char *name = parse_renderer_string(scratch, env_get("TAISEI_RENDERER", ""), &opts);
	bool try_fallbacks = true;

	if(name) {
		try_fallbacks = !strcmp(name, "default");
	} else {
		name = parse_renderer_string(scratch, get_config_renderer(scratch), &opts);
	}

	backend = _r_find_backend(name);

	if(!backend) {
		if(try_fallbacks) {
			log_warn("Unknown renderer backend '%s'", name);
		} else {
			log_fatal("Unknown renderer backend '%s'", name);
		}
	} else {
		if(_r_backend_try_init(backend, opts)) {
			goto done;
		}
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
