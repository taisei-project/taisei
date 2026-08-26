/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "lang_wgsl.h"
#include "log.h"

bool wgsl_translate_from_spirv(
	const ShaderSource *in,
	ShaderSource *out,
	MemArena *arena
) {
	log_error("Compiled without WGSL support");
	return false;
}

bool wgsl_supported(void) {
	return false;
}
