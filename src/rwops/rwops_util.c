/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rwops_util.h"
#include "util/miscmath.h"

int64_t rwutil_compute_seek_pos(int64_t offset, int whence, int64_t pos, int64_t size) {
	int64_t new_pos;

	assert(pos >= 0);
	assert(size < 0 || pos <= size);

	switch(whence) {
		case SDL_IO_SEEK_SET : new_pos = offset;        break;
		case SDL_IO_SEEK_CUR : new_pos = pos + offset;  break;
		case SDL_IO_SEEK_END :
			if(size < 0) {
				return -1;
			}

			new_pos = size - offset;
			break;
		default: UNREACHABLE;
	}

	if(new_pos < 0) {
		new_pos = 0;
	}

	if(size > 0 && new_pos > size) {
		new_pos = size;
	}

	return new_pos;
}

int64_t rwutil_seek_emulated(
	SDL_IOStream *rw, int64_t offset, int whence,
	int64_t *pos, rwutil_reopen_func reopen, void *reopen_arg,
	size_t readbuf_size, void *readbuf
) {
	int64_t sz = SDL_GetIOSize(rw);
	int64_t new_pos = rwutil_compute_seek_pos(offset, whence, *pos, sz);

	if(new_pos < 0) {
		assert(sz < 0);
		return sz;  // assume SDL_GetIOSize set an error;
	}

	return rwutil_seek_emulated_abs(rw, new_pos, pos, reopen, reopen_arg, readbuf_size, readbuf);
}

int64_t rwutil_seek_emulated_abs(
	SDL_IOStream *rw,
	int64_t new_pos, int64_t *pos,
	rwutil_reopen_func reopen, void *reopen_arg,
	size_t readbuf_size, void *readbuf
) {
	assert(new_pos >= 0);

	if(new_pos < *pos) {
		int status;
		if((status = reopen(rw)) < 0) {
			return status;
		}
		assert(*pos == 0);
	}

	while(new_pos > *pos) {
		size_t want_read_size = min(new_pos - *pos, readbuf_size);
		size_t read_size = /* FIXME MIGRATION: double-check if you use the returned value of SDL_ReadIO() */
			SDL_ReadIO(rw, readbuf, want_read_size);
		assert(read_size <= want_read_size);

		if(read_size < readbuf_size) {
			break;
		}
	}

	assert(new_pos == *pos);
	return *pos;
}
