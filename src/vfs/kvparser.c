/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "kvparser.h"

#include "public.h"

#include "log.h"

static SDL_IOStream *kvparser_open(const char *filename) {
	SDL_IOStream *io = vfs_open(filename, VFS_MODE_READ);

	if(!io) {
		log_error("VFS error: %s", vfs_get_error());
	}

	return io;
}

bool parse_keyvalue_file_cb(const char *filename, KVCallback callback, void *data) {
	SDL_IOStream *io = kvparser_open(filename);

	if(!io) {
		return false;
	}

	bool ok = parse_keyvalue_stream_cb(io, callback, data);
	SDL_CloseIO(io);
	return ok;
}

bool parse_keyvalue_file(const char *filename, ht_str2ptr_t *ht) {
	SDL_IOStream *io = kvparser_open(filename);

	if(!io) {
		return false;
	}

	bool ok = parse_keyvalue_stream(io, ht);
	SDL_CloseIO(io);
	return ok;
}

bool parse_keyvalue_file_with_spec(const char *filename, KVSpec *spec) {
	SDL_IOStream *io = kvparser_open(filename);

	if(!io) {
		return false;
	}

	bool ok = parse_keyvalue_stream_with_spec(io, spec);
	SDL_CloseIO(io);
	return ok;
}
