/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "iostream_arena.h"

#include "util/miscmath.h"

INLINE size_t align_size(size_t v) {
	const size_t align = alignof(max_align_t);
	return v + ((align - v) & (align - 1));
}

static void arena_realloc(IOStreamArenaState *st, size_t newsize) {
	newsize = align_size(newsize);

	if(newsize > st->buffer_size) {
		st->buffer = marena_realloc(st->arena, st->buffer, st->buffer_size, newsize);
		st->buffer_size = newsize;
	}
}

static bool arena_close(void *ctx) {
	return true;
}

static int64_t arena_seek(void *ctx, int64_t offset, SDL_IOWhence whence) {
	IOStreamArenaState *st = ctx;
	int64_t new_ofs;

	switch(whence) {
		case SDL_IO_SEEK_SET:
			new_ofs = offset;
			break;

		case SDL_IO_SEEK_CUR:
			new_ofs = (int64_t)st->io_offset + offset;
			break;

		case SDL_IO_SEEK_END:
			new_ofs = (int64_t)st->buffer_size + offset;
			break;

		default: UNREACHABLE;
	}

	new_ofs = clamp(new_ofs, 0, (int64_t)st->buffer_size);
	st->io_offset = new_ofs;
	return st->io_offset;
}

static int64_t arena_size(void *ctx) {
	IOStreamArenaState *st = ctx;
	return st->buffer_size;
}

static size_t arena_read(void *ctx, void *ptr, size_t size, SDL_IOStatus *status) {
	IOStreamArenaState *st = ctx;

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

static size_t arena_write(void *ctx, const void *ptr, size_t size, SDL_IOStatus *status) {
	IOStreamArenaState *st = ctx;

	size_t available = st->buffer_size;
	size_t required = st->io_offset + size;

	if(available < required) {
		arena_realloc(st, required);
		available = st->buffer_size;
		assert(available >= required);
	}

	memcpy(st->buffer + st->io_offset, ptr, size);
	st->io_offset += size;
	return size;
}

SDL_IOStream *iostream_arena(MemArena *arena, uint32_t init_buffer_size, IOStreamArenaState *state) {
	SDL_IOStreamInterface iface = {
		.version = sizeof(iface),
		.close = arena_close,
		.read = arena_read,
		.seek = arena_seek,
		.size = arena_size,
		.write = arena_write,
	};

	SDL_IOStream *io = SDL_OpenIO(&iface, state);

	if(UNLIKELY(!io)) {
		return NULL;
	}

	init_buffer_size = align_size(init_buffer_size);

	*state = (IOStreamArenaState) {
		.arena = arena,
		.buffer = marena_alloc(arena, init_buffer_size),
		.buffer_size = init_buffer_size,
	};

	return io;
}
