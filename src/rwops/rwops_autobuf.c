/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rwops_autobuf.h"

typedef struct Buffer {
	SDL_IOStream *memio;
	void *data;
	void **ptr;
	size_t size;
} Buffer;

static void auto_realloc(Buffer *b, size_t newsize) {
	size_t pos = SDL_TellIO(b->memio);
	SDL_CloseIO(b->memio);

	b->size = newsize;
	b->data = mem_realloc(b->data, b->size);
	b->memio = SDL_IOFromMem(b->data, b->size);

	if(b->ptr) {
		*b->ptr = b->data;
	}

	if(pos > 0) {
		SDL_SeekIO(b->memio, pos, SDL_IO_SEEK_SET);
	}
}

static bool auto_close(void *ctx) {
	Buffer *b = ctx;
	bool result = SDL_CloseIO(b->memio);
	mem_free(b->data);
	mem_free(b);
	return result;
}

static int64_t auto_seek(void *ctx, int64_t offset, SDL_IOWhence whence) {
	Buffer *b = ctx;
	return SDL_SeekIO(b->memio, offset, whence);
}

static int64_t auto_size(void *ctx) {
	Buffer *b = ctx;
	// return SDL_GetIOSize(b->memrw);
	return b->size;
}

static size_t auto_read(void *ctx, void *ptr, size_t size, SDL_IOStatus *status) {
	Buffer *b = ctx;
	size = SDL_ReadIO(b->memio, ptr, size);
	*status = SDL_GetIOStatus(b->memio);
	return size;
}

static size_t auto_write(void *ctx, const void *ptr, size_t size, SDL_IOStatus *status) {
	Buffer *b = ctx;
	size_t newsize = b->size;

	while(size > newsize - SDL_TellIO(b->memio)) {
		newsize *= 2;
	}

	if(newsize > b->size) {
		auto_realloc(b, newsize);
	}

	size = SDL_WriteIO(b->memio, ptr, size);
	*status = SDL_GetIOStatus(b->memio);
	return size;
}

SDL_IOStream *SDL_RWAutoBuffer(void **ptr, size_t initsize) {
	SDL_IOStreamInterface iface = {
		.version = sizeof(iface),
		.close = auto_close,
		.read = auto_read,
		.seek = auto_seek,
		.size = auto_size,
		.write = auto_write,
	};

	auto b = ALLOC(Buffer, {
		.size = initsize,
		.ptr = ptr,
	});

	SDL_IOStream *io = SDL_OpenIO(&iface, b);

	if(UNLIKELY(!io)) {
		mem_free(b);
		return NULL;
	}

	b->data = mem_alloc(initsize);
	b->memio = NOT_NULL(SDL_IOFromMem(b->data, b->size));

	if(ptr) {
		*ptr = b->data;
	}

	return io;
}
