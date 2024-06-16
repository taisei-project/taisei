/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "lang_spirv.h"

bool _spirv_compile(const ShaderSource *in, ShaderSource *out, const SPIRVCompileOptions *options) attr_nonnull(1, 2, 3);
bool _spirv_decompile(const ShaderSource *in, ShaderSource *out, const SPIRVDecompileOptions *options) attr_nonnull(1, 2, 3);
