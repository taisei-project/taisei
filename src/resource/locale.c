/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

 #include "locale.h"

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
		uint32_t u32_array[6];
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
	void *content = NULL;
	I18nLocale *locale = NULL;

	if(!stream) {
		log_error("VFS error: %s", vfs_get_error());
		goto fail;
	}

	size_t filesize;
	content = SDL_LoadFile_IO(stream, &filesize, true);

	if(!content) {
		log_error("%s: error reading MO file: %s", st->path, SDL_GetError());
		goto fail;
	}

	MoHeader *header = content;
	bool flip_endian;

	if(header->magic == 0x950412de) {
		flip_endian = false;
	} else if(header->magic == 0xde120495) {
		flip_endian = true;
	} else {
		log_error("%s is not a MO file: magic number mismatch", st->path);
		goto fail;
	}

	flip_fields(flip_endian, header->u32_array, ARRAY_SIZE(header->u32_array));

	if(header->revision != 0) {
		log_error("%s: Unsupported MO version", st->path);
		goto fail;
	}

	if(header->original_strings_offset + sizeof(MoStringDescriptor) * header->number_of_strings > filesize
	|| header->translated_strings_offset + sizeof(MoStringDescriptor) * header->number_of_strings > filesize) {
		log_error("%s: MO file has nonsense offsets", st->path);
		goto fail;
	}

	MoStringDescriptor *original_strings = content + header->original_strings_offset;
	MoStringDescriptor *translated_strings = content + header->translated_strings_offset;

	flip_fields(flip_endian, original_strings[0].u32_array, header->number_of_strings * ARRAY_SIZE(original_strings[0].u32_array));
	flip_fields(flip_endian, translated_strings[0].u32_array, header->number_of_strings * ARRAY_SIZE(translated_strings[0].u32_array));

	int strings_offset = filesize;
	int max_offset = 0;

	for(int i = 0; i < header->number_of_strings; i++) {
		strings_offset = min(strings_offset, original_strings[i].offset);
		strings_offset = min(strings_offset, translated_strings[i].offset);
		max_offset = max(max_offset, translated_strings[i].offset + translated_strings[i].length + 1);
		max_offset = max(max_offset, original_strings[i].offset + original_strings[i].length + 1);
	}

	if(max_offset > filesize) {
		log_error("%s: MO file has nonsense string offsets", st->path);
		goto fail;
	}

	size_t strings_size = filesize - strings_offset;
	locale = ALLOC_FLEX(I18nLocale, strings_size);
	memcpy(locale->strings, content + strings_offset, strings_size);
	ht_str2str_extern_create(&locale->table);

	for(int i = 0; i < header->number_of_strings; i++) {
		MoStringDescriptor *ostr = &original_strings[i];
		MoStringDescriptor *tstr = &translated_strings[i];

		if(locale->strings[ostr->offset + ostr->length - strings_offset] != '\0'
		|| locale->strings[tstr->offset + tstr->length - strings_offset] != '\0') {
			log_error("%s: MO string missing NUL termination", st->path);
			goto fail;
		}

		const char *orig = &locale->strings[ostr->offset - strings_offset];
		const char *tran = &locale->strings[tstr->offset - strings_offset];
		ht_str2str_extern_set(&locale->table, orig, tran);
	}

	res_load_finished(st, locale);
	return;

fail:
	SDL_free(content);

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
