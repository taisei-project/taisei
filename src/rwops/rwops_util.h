/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <SDL3/SDL.h>

typedef int (*rwutil_reopen_func)(void *context);

/*
 * Compute an absolute seek position from `offset` and `whence`, give a stream open at `pos` of
 * total size `size`.
 *
 * If `size` is negative, it is assumed to be unknown.
 *
 * The returned value is clamped to be >=0, and <=size if size is known.
 *
 * If whence is SDL_IO_SEEK_END and the size is unknown, this function will return a negative value.
 */
int64_t rwutil_compute_seek_pos(int64_t offset, int whence, int64_t pos, int64_t size);

/*
 * Emulate a seeking operation in a non-seekable stream.
 * Forward seeks are implemented by reading and discarding data.
 * Backward seeks are implemented by re-opening the stream and 'seeking' forward.
 *
 * `pos` must point to the current position within the stream. Read operations must update this
 * position correctly.
 * `reopen` must reset the stream position to 0.
 *
 * Returns the new value of *pos.
 */
int64_t rwutil_seek_emulated(
	SDL_IOStream *rw, int64_t offset, int whence,
	int64_t *pos, rwutil_reopen_func reopen, void *reopen_arg,
	size_t readbuf_size, void *readbuf
) attr_nonnull_all attr_nodiscard;

/*
 * Like rwutil_seek_emulated, but takes an absolute position instead of offset and whence.
 */
int64_t rwutil_seek_emulated_abs(
	SDL_IOStream *rw,
	int64_t new_pos, int64_t *pos,
	rwutil_reopen_func reopen, void *reopen_arg,
	size_t readbuf_size, void *readbuf
) attr_nonnull_all attr_nodiscard;
