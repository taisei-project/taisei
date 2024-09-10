/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "lang_dxbc.h"
#include "log.h"

bool dxbc_init_compiler(void) {
	log_error("Compiled without DXBC support");
	return false;
}

void dxbc_shutdown_compiler(void) {
}

bool dxbc_compile(
	const ShaderSource *in,
	ShaderSource *out,
	MemArena *arena,
	const DXBCCompileOptions *options
) {
	log_error("Compiled without DXBC support");
	return false;
}
