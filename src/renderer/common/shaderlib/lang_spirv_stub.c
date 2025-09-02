/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "lang_spirv_private.h"
#include "shaderlib.h"

#include "log.h"

void spirv_init_compiler(void) { }
void spirv_shutdown_compiler(void) { }

bool _spirv_compile(
	const ShaderSource *in,
	ShaderSource *out,
	MemArena *arena,
	const SPIRVCompileOptions *options
) {
	log_error("Compiled without SPIR-V support");
	return false;
}

bool _spirv_decompile(
	const ShaderSource *in,
	ShaderSource *out,
	MemArena *arena,
	const SPIRVDecompileOptions *options
)  {
	log_error("Compiled without SPIR-V support");
	return false;
}

ShaderReflection *_spirv_reflect(const ShaderSource *src, MemArena *arena) {
	log_error("Compiled without SPIR-V support");
	return NULL;
}
