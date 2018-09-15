/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "../common/shader_spirv.h"
#include "util.h"

bool spirv_compile(const ShaderSource *in, ShaderSource *out, const SPIRVCompileOptions *options) {
	log_warn("Compiled without SPIR-V support");
	return false;
}

bool spirv_decompile(const ShaderSource *in, ShaderSource *out, const SPIRVDecompileOptions *options) {
	log_warn("Compiled without SPIR-V support");
	return false;
}

bool spirv_transpile(const ShaderSource *in, ShaderSource *out, const SPIRVTranspileOptions *options) {
	log_warn("Compiled without SPIR-V support");
	return false;
}
