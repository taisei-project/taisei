/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "zipfile.h"
#include "log.h"
#include "util/miscmath.h"

enum {
	ZIP_MAGIC_EOCD = 0x06054b50,
	ZIP_MAGIC_CDFH = 0x02014b50,
	ZIP_MAGIC_LFH  = 0x04034b50,
};

typedef struct __attribute__((packed)) ZipEndOfCentralDirectory {
	uint32_t magic;
	uint16_t num_disk;
	uint16_t cd_start_disk;
	uint16_t num_records_on_disk;
	uint16_t num_records_total;
	uint32_t size;
	uint32_t offset;
	uint16_t comment_size;
	char comment[];
} ZipEndOfCentralDirectory;

typedef struct __attribute__((packed)) ZipCentralDirectoryFileHeader {
	uint32_t magic;
	uint16_t ver_made_by;
	uint16_t ver_needed;
	uint16_t gp_flags;
	uint16_t compression;
	uint16_t mod_time;
	uint16_t mod_date;
	uint32_t crc32;
	uint32_t comp_size;
	uint32_t uncomp_size;
	uint16_t name_len;
	uint16_t extra_len;
	uint16_t comment_len;
	uint16_t disk_start;
	uint16_t int_attrs;
	uint32_t ext_attrs;
	uint32_t lfh_offset;
	// uint8_t name[name_len];
	// uint8_t extra[extra_len];
	// uint8_t comment[comment_len];
} ZipCentralDirectoryFileHeader;

/*
 * The ONLY reason we need to parse this one is because ZIP sucks.
 * The central directory headers don't point to data offsets,
 * and this header is variable-length.
 */
typedef struct __attribute__((packed)) ZipLocalFileHeader {
	uint32_t magic;
	uint16_t ver_needed;
	uint16_t gp_flags;
	uint16_t compression;
	uint16_t mod_time;
	uint16_t mod_date;
	uint32_t crc32;
	uint32_t comp_size;
	uint32_t uncomp_size;
	uint16_t name_len;
	uint16_t extra_len;
	// uint8_t name[name_len];
	// uint8_t extra[extra_len];
	// uint8_t data[comp_size];
} ZipLocalFileHeader;

typedef struct ZipCentralDirectoryMeta {
	uint32_t offset;
	uint32_t size;
	uint16_t num_records;
} ZipCentralDirectoryMeta;

// NOTE: no-op on LE systems, swaps on BE systems
INLINE uint16_t LE16(uint16_t x) { return SDL_Swap16LE(x); }
INLINE uint32_t LE32(uint32_t x) { return SDL_Swap32LE(x); }

#define LE(x) _Generic((x), \
	uint16_t: LE16, \
	uint32_t: LE32 \
)(x)

typedef enum EOCDParseResult {
	EOCD_NOT_FOUND,
	EOCD_PARSED,
	EOCD_UNSUPPORTED,
} EOCDParseResult;

static bool block_in_range(uint32_t ofs, uint32_t size, uint32_t max_ofs) {
	return !(ofs + size < ofs || ofs + size > max_ofs);
}

static EOCDParseResult try_parse_eocd(const ZipContext *ctx, uint32_t offset, ZipCentralDirectoryMeta *cd_meta) {
	auto eocd = (const ZipEndOfCentralDirectory*)((const char*)ctx->mem + offset);

	if(LE(eocd->magic) != ZIP_MAGIC_EOCD) {
		return EOCD_NOT_FOUND;
	}

	uint32_t comment_size = LE(eocd->comment_size);
	uint32_t expected_comment_size = ctx->mem_size - (offset + sizeof(*eocd));

	if(expected_comment_size != comment_size) {
		return EOCD_NOT_FOUND;
	}

	if(LE(eocd->num_disk) != 0) {
		log_error("%s: Unsupported multi-disk archive", ctx->log_prefix);
		return EOCD_UNSUPPORTED;
	}

	uint32_t num_records_total = LE(eocd->num_records_total);
	uint32_t num_records_on_disk = LE(eocd->num_records_on_disk);

	if(LE(eocd->cd_start_disk) != 0 || num_records_on_disk != num_records_total) {
		log_error("%s: Inconsistent end of central directory", ctx->log_prefix);
		return EOCD_UNSUPPORTED;
	}

	cd_meta->size = LE(eocd->size);
	cd_meta->offset = LE(eocd->offset);
	cd_meta->num_records = LE(eocd->num_records_total);

	if(
		cd_meta->size == 0xFFFFFFFFu || cd_meta->offset == 0xFFFFFFFFu ||
		num_records_total == 0xFFFFu || num_records_on_disk == 0xFFFFu
	) {
		log_error("%s: ZIP64 archive not supported", ctx->log_prefix);
		return EOCD_UNSUPPORTED;
	}

	if(
		ctx->mem_size - sizeof(*eocd) < cd_meta->offset ||
		ctx->mem_size - sizeof(*eocd) - cd_meta->offset < cd_meta->size
	) {
		log_error("%s: Invalid central directory offset and/or size", ctx->log_prefix);
		return EOCD_UNSUPPORTED;
	}

	log_debug("%s: Central directory at %d (size %d), %d records", ctx->log_prefix,
		cd_meta->offset, cd_meta->size, cd_meta->num_records);

	return EOCD_PARSED;
}

static bool find_cd(const ZipContext *ctx, ZipCentralDirectoryMeta *cd_meta) {
	uint32_t mem_size = ctx->mem_size;
	uint32_t eocd_offset = mem_size - sizeof(ZipEndOfCentralDirectory);
	uint32_t max_shift = min(eocd_offset, UINT16_MAX);
	uint32_t shift = 0;

	do {
		switch(try_parse_eocd(ctx, eocd_offset - shift, cd_meta)) {
			case EOCD_NOT_FOUND:   continue;
			case EOCD_PARSED:      return true;
			case EOCD_UNSUPPORTED: return false;
		}
	} while(shift++ < max_shift);

	log_error("%s: End of central directory record not found", ctx->log_prefix);
	return false;
}

bool zipfile_parse(const ZipContext *ctx, ZipParseCallback callback, void *userdata) {
	ZipCentralDirectoryMeta cd_meta = {};

	if(ctx->mem_size < sizeof(ZipEndOfCentralDirectory)) {
		log_error("%s: File is too small", ctx->log_prefix);
		return false;
	}

	if(ctx->mem_size > INT32_MAX) {
		log_error("%s: File is too large", ctx->log_prefix);
		return false;
	}

	if(!find_cd(ctx, &cd_meta)) {
		return false;
	}

	const char *mem = ctx->mem;
	size_t mem_size = ctx->mem_size;

	uint32_t ofs = cd_meta.offset;
	uint32_t max_ofs = ofs + cd_meta.size;

	if(
		max_ofs <= ofs ||
		cd_meta.size < sizeof(ZipCentralDirectoryFileHeader) ||
		max_ofs > mem_size
	) {
		log_error("%s: Invalid central directory size", ctx->log_prefix);
		return false;
	}

	if(cd_meta.num_records > cd_meta.size / sizeof(ZipCentralDirectoryFileHeader)) {
		log_error("%s: Central directory record count too large", ctx->log_prefix);
		return false;
	}

	for(uint32_t i = 0; i < cd_meta.num_records; ++i) {
		alignas(alignof(uint32_t)) ZipCentralDirectoryFileHeader cdfh;

		if(!block_in_range(ofs, sizeof(cdfh), max_ofs)) {
			log_error("%s: Central directory file header %d is out of bounds", ctx->log_prefix, i);
			return false;
		}

		memcpy(&cdfh, mem + ofs, sizeof(cdfh));

		if(LE(cdfh.magic) != ZIP_MAGIC_CDFH) {
			log_error("%s: Invalid central directory file header magic number at record %d", ctx->log_prefix, i);
			return false;
		}

		uint32_t end_of_cdfh = ofs + sizeof(cdfh);

		uint16_t name_len = LE(cdfh.name_len);
		uint16_t extra_len = LE(cdfh.extra_len);
		uint16_t comment_len = LE(cdfh.comment_len);

		uint32_t var_len = (uint32_t)name_len + (uint32_t)extra_len + (uint32_t)comment_len;

		if(!block_in_range(end_of_cdfh, var_len, max_ofs)) {
			log_error("%s: Central directory file header %d is out of bounds", ctx->log_prefix, i);
			return false;
		}

		uint32_t name_ofs = end_of_cdfh;
		ofs = end_of_cdfh + var_len;

		if(LE(cdfh.disk_start) != 0) {
			log_warn("%s: File `%.*s` ignored: Multi-disk is not supported",
				ctx->log_prefix, (int)name_len, mem + name_ofs);
			continue;
		}

		if(LE(cdfh.gp_flags) & 1) {
			log_warn("%s: File `%.*s` ignored: Encryption is not supported", ctx->log_prefix, (int)name_len, mem + name_ofs);
			continue;
		}

		if(
			LE(cdfh.comp_size) == 0xFFFFFFFFu ||
			LE(cdfh.uncomp_size) == 0xFFFFFFFFu ||
			LE(cdfh.lfh_offset) == 0xFFFFFFFFu
		) {
			log_warn("%s: File `%.*s` ignored: ZIP64 is not supported", ctx->log_prefix, (int)name_len, mem + name_ofs);
			continue;
		}

		ZipEntry e = {
			.name = mem + name_ofs,
			.name_len = name_len,
			.compression = LE(cdfh.compression),
			.comp_size = LE(cdfh.comp_size),
			.uncomp_size = LE(cdfh.uncomp_size),
			.lfh_offset = LE(cdfh.lfh_offset),
			.is_dir = name_len > 0 && mem[name_ofs + name_len - 1] == '/',
		};

		if(e.is_dir && (e.comp_size || e.uncomp_size)) {
			log_warn("%s: Directory %.*s contains data",
				ctx->log_prefix, (int)name_len, mem + name_ofs);
		}

		log_debug("%s: [%d] %.*s", ctx->log_prefix, i, (int)name_len, mem + name_ofs);

		if(!callback(userdata, &e, i, cd_meta.num_records)) {
			return false;
		}
	}

	return true;
}

static uint32_t zipfile_entry_error(
	const ZipContext *ctx, const ZipEntry *entry, const char *msg
) {
	log_error("%s: %.*s: %s", ctx->log_prefix, (int)entry->name_len, entry->name, msg);
	return ZIP_INVALID_OFFSET;
}

uint32_t zipfile_get_entry_data_offset(const ZipContext *ctx, const ZipEntry *entry) {
	const char *mem = ctx->mem;
	size_t mem_size = ctx->mem_size;

	alignas(alignof(uint32_t)) ZipLocalFileHeader lfh;

	if(!block_in_range(entry->lfh_offset, sizeof(lfh), mem_size)) {
		return zipfile_entry_error(ctx, entry, "Local file header is out of bounds");
	}

	memcpy(&lfh, mem + entry->lfh_offset, sizeof(lfh));

	if(LE(lfh.magic) != ZIP_MAGIC_LFH) {
		return zipfile_entry_error(ctx, entry, "Invalid local file header magic number");
	}

	uint32_t end_of_lfh = entry->lfh_offset + sizeof(lfh);
	uint32_t var_len = (uint32_t)LE(lfh.extra_len) + (uint32_t)LE(lfh.name_len);
	uint32_t data_ofs = end_of_lfh + var_len;

	if(!block_in_range(data_ofs, entry->comp_size, mem_size)) {
		return zipfile_entry_error(ctx, entry, "Data is out of bounds");
	}

	return data_ofs;
}
