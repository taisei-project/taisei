/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "lang_wgsl.h"
#include "shaderlib.h"

#include "log.h"

#include <taint.h>

bool wgsl_translate_from_spirv(
	const ShaderSource *in,
	ShaderSource *out,
	MemArena *arena,
	const WGSLTranslateOptions *options
) {
	if(in->lang.lang != SHLANG_SPIRV) {
		log_error("Source is not a SPIR-V shader");
		return false;
	}

	uint32_t *spv = CASTPTR_ASSUME_ALIGNED(in->content, uint32_t);
	uint32_t spv_size = in->content_size / sizeof(*spv);

	auto opts = taint_options_create();

	taint_options_set_bool(opts, TAINT_OPT_WGSL_ALLOW_NON_UNIFORM_DERIVATIVES,
		options->flags & WGSL_TFLAG_ALLOW_NON_UNIFORM_DERIVATIVES);
	taint_options_set_bool(opts, TAINT_OPT_WGSL_ALLOW_NON_UNIFORM_SUBGROUP_OPERATIONS,
		options->flags & WGSL_TFLAG_ALLOW_NON_UNIFORM_SUBGROUP_OPERATIONS);
	taint_options_set_bool(opts, TAINT_OPT_WGSL_DISABLE_UNREACHABLE_CODE_WARNING,
		options->flags & WGSL_TFLAG_DISABLE_UNREACHABLE_CODE_WARNING);
	taint_options_set_bool(opts, TAINT_OPT_WGSL_MINIFY,
		options->flags & WGSL_TFLAG_MINIFY);

	auto mod = taint_spirv_to_wgsl(spv_size, spv, opts);
	taint_options_destroy(opts);

	if(!mod) {
		log_error("taint_spirv_to_wgsl() failed");
		taint_module_destroy(mod);
		return false;
	}

	size_t wgsl_size;
	auto wgsl = taint_module_get_wgsl(mod, &wgsl_size);

	size_t diag_size;
	auto diag = taint_module_get_wgsl(mod, &diag_size);

	if(diag && *diag) {
		log_custom(wgsl ? LOG_WARN : LOG_ERROR, "%s", diag);
	}

	if(!wgsl) {
		if(!diag) {
			log_fatal_if_debug("Unknown error");
		}

		taint_module_destroy(mod);
		return false;
	}

	*out = (ShaderSource) {
		.lang = {
			.lang = SHLANG_WGSL,
		},
		.content = marena_memdup(arena, wgsl, wgsl_size + 1),
		.content_size = wgsl_size,
		.entrypoint = marena_strdup(arena, in->entrypoint),
		.stage = in->stage,
	};

	taint_module_destroy(mod);
	return true;
}

bool wgsl_supported(void) {
	return true;
}
