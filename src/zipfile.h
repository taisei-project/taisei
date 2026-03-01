/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

typedef enum ZipCompression {
	ZIP_COMPRESSION_NONE = 0,
	ZIP_COMPRESSION_DEFLATE = 8,
	ZIP_COMPRESSION_ZSTD = 93,
	ZIP_COMPRESSION_ZSTD_DEPRECATED = 20,
} ZipCompression;

typedef struct ZipEntry {
	const char *name;  // WARNING NOT NULL-TERMINATED
	uint16_t name_len;
	ZipCompression compression;
	uint32_t comp_size;
	uint32_t uncomp_size;
	uint32_t lfh_offset;
	bool is_dir;
} ZipEntry;

#define ZIP_INVALID_OFFSET ((uint32_t)-1)

typedef struct ZipContext {
	const char *log_prefix;
	const void *mem;
	size_t mem_size;
} ZipContext;

typedef bool (*ZipParseCallback)(void *userdata, const ZipEntry *entry, uint32_t entry_idx, uint32_t num_entries);

bool zipfile_parse(const ZipContext *ctx, ZipParseCallback callback, void *userdata)
	attr_nonnull(1, 2) attr_nodiscard;

uint32_t zipfile_get_entry_data_offset(const ZipContext *ctx, const ZipEntry *entry)
	attr_nonnull_all attr_nodiscard;
