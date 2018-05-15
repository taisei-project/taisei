/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "kvparser.h"
#include "log.h"
#include "stringops.h"
#include "io.h"
#include "vfs/public.h"

bool parse_keyvalue_stream_cb(SDL_RWops *strm, KVCallback callback, void *data) {
	static const char separator[] = "= ";

	char buffer[256];
	int lineno = 0;
	int errors = 0;

	loopstart: while(SDL_RWgets(strm, buffer, sizeof(buffer))) {
		char *ptr = buffer;
		char *sep, *key, *val;

		++lineno;

		while(isspace(*ptr)) {
			if(!*(++ptr)) {
				// blank line
				goto loopstart;
			}
		}

		if(*ptr == '#') {
			// comment
			continue;
		}

		sep = strstr(ptr, separator);

		if(!sep) {
			++errors;
			log_warn("Syntax error on line %i: missing separator", lineno);
			continue;
		}

		// split it up
		*sep = 0;
		key = ptr;
		val = sep + sizeof(separator) - 1;

		// the separator may be preceeded by any kind of whitespace, so strip it from the key
		while(isspace(*(ptr = strchr(key, 0) - 1))) {
			*ptr = 0;
		}

		// strip any kind of line endings from the value
		while(strchr("\r\n", *(ptr = strchr(val, 0) - 1))) {
			*ptr = 0;
		}

		if(!callback(key, val, data)) {
			++errors;
			continue;
		}
	}

	return !errors;
}

bool parse_keyvalue_file_cb(const char *filename, KVCallback callback, void *data) {
	SDL_RWops *strm = vfs_open(filename, VFS_MODE_READ);

	if(!strm) {
		log_warn("VFS error: %s", vfs_get_error());
		return false;
	}

	bool status = parse_keyvalue_stream_cb(strm, callback, data);
	SDL_RWclose(strm);
	return status;
}

static bool kvcallback_hashtable(const char *key, const char *val, void *data) {
	Hashtable *ht = data;
	hashtable_set_string(ht, key, strdup((void*)val));
	return true;
}

Hashtable* parse_keyvalue_stream(SDL_RWops *strm) {
	Hashtable *ht = hashtable_new_stringkeys();

	if(!parse_keyvalue_stream_cb(strm, kvcallback_hashtable, ht)) {
		hashtable_free(ht);
		ht = NULL;
	}

	return ht;
}

Hashtable* parse_keyvalue_file(const char *filename) {
	Hashtable *ht = hashtable_new_stringkeys();

	if(!parse_keyvalue_file_cb(filename, kvcallback_hashtable, ht)) {
		hashtable_free(ht);
		ht = NULL;
	}

	return ht;
}

static bool kvcallback_spec(const char *key, const char *val, void *data) {
	KVSpec *spec = data;
	for(KVSpec *s = spec; s->name; ++s) {
		if(!strcmp(key, s->name)) {
			if(s->out_str) {
				stralloc(s->out_str, val);
			}

			if(s->out_int) {
				*s->out_int = strtol(val, NULL, 10);
			}

			if(s->out_float) {
				*s->out_float = strtod(val, NULL);
			}

			if(s->out_double) {
				*s->out_double = strtod(val, NULL);
			}

			return true;
		}
	}

	log_warn("Unknown key '%s' with value '%s' ignored", key, val);
	return true;
}

bool parse_keyvalue_stream_with_spec(SDL_RWops *strm, KVSpec *spec) {
	return parse_keyvalue_stream_cb(strm, kvcallback_spec, spec);
}

bool parse_keyvalue_file_with_spec(const char *filename, KVSpec *spec) {
	return parse_keyvalue_file_cb(filename, kvcallback_spec, spec);
}
