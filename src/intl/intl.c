/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "hashtable.h"
#include "intl.h"
#include "util.h"
#include "log.h"
#include "util/miscmath.h"
#include "vfs/public.h"
#include <SDL3/SDL_iostream.h>

typedef struct Translation {
	char *text;
	char *plural;
} Translation;

#define HT_SUFFIX                      str2trans
#define HT_KEY_TYPE                    const char*
#define HT_VALUE_TYPE                  Translation
#define HT_FUNC_KEYS_EQUAL(key1, key2) (!strcmp(key1, key2))
#define HT_FUNC_HASH_KEY(key)          htutil_hashfunc_string(key)
#define HT_FUNC_COPY_KEY(dst, src)     (*(dst) = (src))
#define HT_KEY_FMT                     "s"
#define HT_KEY_PRINTABLE(key)          (key)
#define HT_VALUE_FMT                   "s"
#define HT_VALUE_PRINTABLE(val)        (none)
#define HT_KEY_CONST
#define HT_DECL
#define HT_IMPL
#include "hashtable_incproxy.inc.h"

IntlTextDomain *_intl_current_textdomain = NULL;

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

static void flip_fields(bool flip_endian, uint32_t *ptr, int n) {
	if(flip_endian) {
		for(int i = 0; i < n; i++) {
			ptr[n] = SDL_Swap32(ptr[n]);
		}
	}
}

IntlTextDomain *intl_read_mo(const char* path) {
	SDL_IOStream *stream = vfs_open(path, VFS_MODE_READ);
	if(!stream) {
		log_error("VFS error: %s", vfs_get_error());
		return NULL;
	}

	size_t filesize;
	void *content = SDL_LoadFile_IO(stream, &filesize, true);
	if(!content) {
		log_error("%s: error reading MO file: %s", path, SDL_GetError());
		return NULL;
	}

	MoHeader *header = content;
	bool flip_endian;
	if(header->magic == 0x950412de) {
		flip_endian = false;
	} else if(header->magic == 0x12de9504) {
		flip_endian = true;
	} else {
		log_error("%s is not a MO file: magic number mismatch", path);
		SDL_free(content);
		return NULL;
	}

	flip_fields(flip_endian, header->u32_array, ARRAY_SIZE(header->u32_array));

	if(header->revision != 0) {
		log_error("%s: Unsupported MO version", path);
		SDL_free(content);
		return NULL;
	}

	if(header->original_strings_offset + sizeof(MoStringDescriptor) * header->number_of_strings > filesize
	|| header->translated_strings_offset + sizeof(MoStringDescriptor) * header->number_of_strings > filesize) {
		log_error("%s: MO file has nonsense offsets", path);
		SDL_free(content);
		return NULL;
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
		log_error("%s: MO file has nonsense string offsets", path);
		SDL_free(content);
		return NULL;
	}

	IntlTextDomain *domain = ALLOC(IntlTextDomain);
	domain->strings = memdup(content + strings_offset, filesize - strings_offset);
	domain->table = ht_str2trans_new();

	for(int i = 0; i < header->number_of_strings; i++) {
		Translation t;
		MoStringDescriptor *ostr = &original_strings[i];
		MoStringDescriptor *tstr = &translated_strings[i];

		if(domain->strings[ostr->offset + ostr->length - strings_offset] != '\0'
		|| domain->strings[tstr->offset + tstr->length - strings_offset] != '\0') {
			log_error("%s: MO string missing NUL termination", path);
			intl_textdomain_destroy(domain);
			SDL_free(content);
			mem_free(domain);
			return NULL;
		}
		int textlen = strlen(&domain->strings[translated_strings[i].offset-strings_offset]);
		if(textlen != translated_strings[i].length) {
			t.plural = &domain->strings[tstr->offset - strings_offset];
			t.text = t.plural + textlen + 1;
		} else {
			t.text = &domain->strings[tstr->offset - strings_offset];
		}

		ht_str2trans_set(domain->table, &domain->strings[ostr->offset - strings_offset], t);
	}

	SDL_free(content);
	return domain;
}

void intl_textdomain_destroy(IntlTextDomain *domain) {
	ht_str2trans_destroy(domain->table);
	mem_free(domain->table);
	mem_free(domain->strings);
}

void intl_set_textdomain(IntlTextDomain *domain) {
	_intl_current_textdomain = domain;
}

const char *_intl_gettext_prehashed(IntlTextDomain *domain, const char *msgid, hash_t hash) {
	if(!domain) {
		return msgid;
	}

	Translation result;
	bool exists = ht_str2trans_lookup_prehashed(domain->table, msgid, hash, &result);
	return exists ? result.text : msgid;
}

const char *_intl_ngettext_prehashed(IntlTextDomain *domain, const char *msgid1, hash_t hash1, const char *msgid2, unsigned long int n) {
	const char *fallback = n == 1 ? msgid1 : msgid2;
	if(!domain) {
		return fallback;
	}

	Translation result;
	bool exists = ht_str2trans_lookup_prehashed(domain->table, msgid1, hash1, &result);
	if(!exists) {
		return fallback;
	}

	if(!result.plural) {
		log_error("i18n: plural original string '%s':'%s' has singular-only translation '%s'", msgid1, msgid2, result.text);
		return fallback;
	}

	return n == 1 ? result.text : result.plural;
}
