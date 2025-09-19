/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "defs.h"

#include "memory/arena.h"

// sha256 hexdigest  : 64 bytes
// separator         : 1 byte
// 64-bit size (hex) : 8 bytes
// null terminator   : 1 byte
#define SHADER_CACHE_HASH_BUFSIZE 74

bool shader_cache_hash(const ShaderSource *src, const ShaderMacro *macros, size_t buf_size, char out_buf[buf_size])
	attr_nonnull(1, 4) attr_nodiscard;

bool shader_cache_get(const char *hash, const char *key, ShaderSource *entry, MemArena *arena)
	attr_nonnull(1, 2, 3) attr_nodiscard;

bool shader_cache_set(const char *hash, const char *key, const ShaderSource *src)
	attr_nonnull(1, 2, 3);
