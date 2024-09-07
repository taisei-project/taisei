/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rwops_arena.h"

#include "util/miscmath.h"

INLINE size_t align_size(size_t v) {
	const size_t align = alignof(max_align_t);
	return v + ((align - v) & (align - 1));
}

static void rwarena_realloc(RWArenaState *st, size_t newsize) {
	newsize = align_size(newsize);

	if(newsize > st->buffer_size) {
		st->buffer = marena_realloc(st->arena, st->buffer, st->buffer_size, newsize);
		st->buffer_size = newsize;
	}
}

static bool rwarena_close(void *ctx) {
	return true;
}

static int64_t rwarena_seek(void *ctx, int64_t offset, SDL_IOWhence whence) {
	RWArenaState *st = ctx;
	int64_t new_ofs;

	switch(whence) {
		case SDL_IO_SEEK_SET:
			new_ofs = offset;
			break;

		case SDL_IO_SEEK_CUR:
			new_ofs = (int64_t)st->io_offset + offset;

		case SDL_IO_SEEK_END:
			new_ofs = (int64_t)st->buffer_size + offset;
			break;
	}

	new_ofs = clamp(new_ofs, 0, (int64_t)st->buffer_size);
	st->io_offset = new_ofs;
	return st->io_offset;
}

static int64_t rwarena_size(void *ctx) {
	RWArenaState *st = ctx;
	return st->buffer_size;
}

static size_t rwarena_read(void *ctx, void *ptr, size_t size, SDL_IOStatus *status) {
	RWArenaState *st = ctx;

	size_t available = (st->buffer_size - st->io_offset);
	size = min(size, available);

	if(!available) {
		*status = SDL_IO_STATUS_EOF;
	} else {
		memcpy(ptr, st->buffer, size);
	}

	st->io_offset += size;
	return size;
}

static size_t rwarena_write(void *ctx, const void *ptr, size_t size, SDL_IOStatus *status) {
	RWArenaState *st = ctx;

	size_t available = st->buffer_size;
	size_t required = st->io_offset + size;

	if(available < required) {
		rwarena_realloc(st, required);
		available = st->buffer_size;
		assert(available >= required);
	}

	memcpy(st->buffer + st->io_offset, ptr, size);
	st->io_offset += size;
	return size;
}

SDL_IOStream *SDL_RWArena(MemArena *arena, uint32_t init_buffer_size, RWArenaState *state) {
	SDL_IOStreamInterface iface = {
		.version = sizeof(iface),
		.close = rwarena_close,
		.read = rwarena_read,
		.seek = rwarena_seek,
		.size = rwarena_size,
		.write = rwarena_write,
	};

	SDL_IOStream *io = SDL_OpenIO(&iface, state);

	if(UNLIKELY(!io)) {
		return NULL;
	}

	init_buffer_size = align_size(init_buffer_size);

	*state = (RWArenaState) {
		.arena = arena,
		.buffer = marena_alloc(arena, init_buffer_size),
		.buffer_size = init_buffer_size,
	};

	return io;
}
