/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "locale.h"

#include "memory/scratch.h"
#include "util.h"
#include "util/io.h"
#include "util/miscmath.h"

#define LOCALE_PATH_PREFIX "res/locale/"
#define LOCALE_PATH_SUFFIX "/LC_MESSAGES/taisei.mo"

#define HT_SUFFIX                      str2str_extern
#define HT_KEY_TYPE                    const char*
#define HT_VALUE_TYPE                  const char*
#define HT_FUNC_KEYS_EQUAL(key1, key2) (!strcmp(key1, key2))
#define HT_FUNC_HASH_KEY(key)          htutil_hashfunc_string(key)
#define HT_FUNC_COPY_KEY(dst, src)     (*(dst) = (src))
#define HT_KEY_FMT                     "s"
#define HT_KEY_PRINTABLE(key)          (key)
#define HT_VALUE_FMT                   "s"
#define HT_VALUE_PRINTABLE(val)        (val)
#define HT_DECL
#define HT_IMPL
#include "hashtable_incproxy.inc.h"

struct I18nLocale {
	ht_str2str_extern_t table;
	char strings[];
};

typedef struct MoHeader {
	union {
		struct {
			uint32_t magic;
			uint32_t revision;
			uint32_t number_of_strings;
			uint32_t original_strings_offset;
			uint32_t translated_strings_offset;
		};
		uint32_t u32_array[5];
	};
} MoHeader;

typedef struct MoStringDescriptor {
	union {
		struct {
			uint32_t length;
			uint32_t offset;
		};
		uint32_t u32_array[2];
	};
} MoStringDescriptor;

static char *locale_path(const char *name) {
	return try_path(LOCALE_PATH_PREFIX, name, LOCALE_PATH_SUFFIX);
}

static bool locale_check_path(const char *path) {
	return strstartswith(path, LOCALE_PATH_PREFIX) && strendswith(path, LOCALE_PATH_SUFFIX);
}

static void flip_fields(bool flip_endian, uint32_t *ptr, int n) {
	if(flip_endian) {
		for(int i = 0; i < n; i++) {
			ptr[i] = SDL_Swap32(ptr[i]);
		}
	}
}

static void locale_unload(void *vlocale) {
	I18nLocale *locale = vlocale;
	ht_str2str_extern_destroy(&locale->table);
	mem_free(locale);
}

static void locale_load(ResourceLoadState *st) {
	SDL_IOStream *stream = vfs_open(st->path, VFS_MODE_READ);
	I18nLocale *locale = NULL;
	MemArena *scratch = acquire_scratch_arena();

	if(!stream) {
		log_error("VFS error: %s", vfs_get_error());
		goto fail;
	}

	MoHeader header;
	int64_t file_size  = SDL_GetIOSize(stream);
	int64_t read_size = SDL_ReadIO(stream, &header, sizeof(header));
	SDL_IOStatus iostat = SDL_GetIOStatus(stream);

	if(UNLIKELY(file_size < 0)) {
		// NOTE: shouldn't actually happen with our vfs implementation
		log_sdl_error(LOG_ERROR, "SDL_GetIOSize");
		goto fail;
	}

	if(UNLIKELY(iostat == SDL_IO_STATUS_EOF || read_size < sizeof(header))) {
		log_error("%s: Truncated MO header", st->path);
		goto fail;
	} else if(UNLIKELY(iostat != SDL_IO_STATUS_READY)) {
		log_error("%s: Error reading MO header: %s", st->path, SDL_GetError());
		goto fail;
	}

	bool flip_endian;

	if(header.magic == 0x950412de) {
		flip_endian = false;
	} else if(header.magic == 0xde120495) {
		flip_endian = true;
	} else {
		log_error("%s is not an MO file: magic number mismatch", st->path);
		goto fail;
	}

	flip_fields(flip_endian, header.u32_array, ARRAY_SIZE(header.u32_array));

	if(header.revision != 0) {
		log_error("%s: Unsupported MO version", st->path);
		goto fail;
	}

	if(header.number_of_strings > UINT32_MAX / sizeof(MoStringDescriptor) / 2) {
		log_error("%s: Too many strings in MO file", st->path);
		goto fail;
	}

	if(header.original_strings_offset + sizeof(MoStringDescriptor) * header.number_of_strings > file_size
	|| header.translated_strings_offset + sizeof(MoStringDescriptor) * header.number_of_strings > file_size
	|| header.original_strings_offset < sizeof(MoHeader)) {
		log_error("%s: MO file has nonsense string table offsets", st->path);
		goto fail;
	}

	size_t string_table_size = sizeof(MoStringDescriptor) * header.number_of_strings;

	if(header.original_strings_offset + string_table_size != header.translated_strings_offset) {
		log_error("%s: MO string tables are not contiguous", st->path);
		goto fail;
	}

	if(SDL_SeekIO(stream, header.original_strings_offset, SDL_IO_SEEK_SET) < 0) {
		log_error("%s: Failed to seek to MO string tables offset", st->path);
		goto fail;
	}

	auto strings_descs = ARENA_ALLOC_ARRAY(scratch, header.number_of_strings * 2, MoStringDescriptor);
	read_size = SDL_ReadIO(stream, strings_descs, string_table_size * 2);
	iostat = SDL_GetIOStatus(stream);

	if(UNLIKELY(iostat == SDL_IO_STATUS_EOF || read_size < string_table_size * 2)) {
		log_error("%s: Truncated MO string tables", st->path);
		goto fail;
	} else if(UNLIKELY(iostat != SDL_IO_STATUS_READY)) {
		log_error("%s: Error reading MO string tables: %s", st->path, SDL_GetError());
		goto fail;
	}

	MoStringDescriptor *orig_string_descs = strings_descs;
	MoStringDescriptor *trans_string_descs = orig_string_descs + header.number_of_strings;

	flip_fields(flip_endian, strings_descs->u32_array, (string_table_size * 2) / sizeof(uint32_t));

	int64_t min_strings_offset = header.translated_strings_offset + string_table_size;
	int64_t max_strings_offset = 0;
	int64_t strings_offset = file_size;

	for(uint32_t i = 0; i < header.number_of_strings; i++) {
		strings_offset = min(strings_offset, orig_string_descs[i].offset);
		strings_offset = min(strings_offset, trans_string_descs[i].offset);
		max_strings_offset = max(max_strings_offset, trans_string_descs[i].offset + trans_string_descs[i].length + 1);
		max_strings_offset = max(max_strings_offset, orig_string_descs[i].offset + orig_string_descs[i].length + 1);
	}

	if(max_strings_offset > file_size || strings_offset < min_strings_offset) {
		log_error("%s: MO file has nonsense string offsets", st->path);
		goto fail;
	}

	if(SDL_SeekIO(stream, strings_offset, SDL_IO_SEEK_SET) < 0) {
		log_error("%s: Failed to seek to MO strings offset", st->path);
		goto fail;
	}

	size_t strings_size = max_strings_offset - strings_offset;
	locale = ALLOC_FLEX(I18nLocale, strings_size);

	read_size = SDL_ReadIO(stream, locale->strings, strings_size);
	iostat = SDL_GetIOStatus(stream);

	if(UNLIKELY(iostat == SDL_IO_STATUS_EOF || read_size < strings_size)) {
		log_error("%s: Truncated MO strings", st->path);
		goto fail;
	} else if(UNLIKELY(iostat != SDL_IO_STATUS_READY)) {
		log_error("%s: Error reading MO strings: %s", st->path, SDL_GetError());
		goto fail;
	}

	ht_str2str_extern_create(&locale->table);

	for(uint32_t i = 0; i < header.number_of_strings; i++) {
		MoStringDescriptor *ostr = &orig_string_descs[i];
		MoStringDescriptor *tstr = &trans_string_descs[i];

		if(locale->strings[ostr->offset + ostr->length - strings_offset] != '\0'
		|| locale->strings[tstr->offset + tstr->length - strings_offset] != '\0') {
			log_error("%s: MO string missing NUL termination", st->path);
			goto fail;
		}

		const char *orig = &locale->strings[ostr->offset - strings_offset];
		const char *tran = &locale->strings[tstr->offset - strings_offset];

		ht_str2str_extern_set(&locale->table, orig, tran);
	}

	release_scratch_arena(scratch);
	SDL_CloseIO(stream);
	res_load_finished(st, locale);
	return;

fail:
	release_scratch_arena(scratch);
	SDL_CloseIO(stream);

	if(locale) {
		locale_unload(locale);
	}

	res_load_failed(st);
}

ResourceHandler locale_res_handler = {
	.type = RES_LOCALE,
	.typename = "locale",
	.subdir = LOCALE_PATH_PREFIX,

	.procs = {
		.find = locale_path,
		.check = locale_check_path,
		.load = locale_load,
		.unload = locale_unload,
	},
};

const char *i18n_locale_get_translation_prehashed(I18nLocale *locale, const char *source, hash_t source_hash) {
	return ht_str2str_extern_get_prehashed(&locale->table, source, source_hash, source);
}

static bool locale_search_filter(const char *path) {
	size_t pathlen = strlen(path);
	char mopath[sizeof(LOCALE_PATH_PREFIX) + sizeof(LOCALE_PATH_SUFFIX) + pathlen];
	char *p = mopath;
	memcpy(p, LOCALE_PATH_PREFIX, sizeof(LOCALE_PATH_PREFIX) - 1);
	p += sizeof(LOCALE_PATH_PREFIX) - 1;
	memcpy(p, path, pathlen);
	p += pathlen;
	memcpy(p, LOCALE_PATH_SUFFIX, sizeof(LOCALE_PATH_SUFFIX));

	VFSInfo i = vfs_query(mopath);
	return i.exists && !i.is_dir && !i.error;
}

char **i18n_find_locales(size_t *num_results) {
	char **result = vfs_dir_list_sorted(LOCALE_PATH_PREFIX, num_results, vfs_dir_list_order_ascending, locale_search_filter);

	if(!result) {
		log_error("VFS error: %s", vfs_get_error());
		*num_results = 0;
	}

	return result;
}
