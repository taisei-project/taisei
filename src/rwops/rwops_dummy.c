/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rwops_dummy.h"

// autoclose bit encoded as lowest bit of the context pointer, which is normally always 0
#define DUMMY_SOURCE(ctx)    ((SDL_IOStream*)((uintptr_t)(ctx) & ~1ull))
#define DUMMY_AUTOCLOSE(ctx) ((bool)((uintptr_t)(ctx) & 1ull))

static bool dummy_close(void *ctx) {
	bool result = true;

	if(DUMMY_AUTOCLOSE(ctx)) {
		result = SDL_CloseIO(DUMMY_SOURCE(ctx));
	}

	return result;
}

static int64_t dummy_seek(void *ctx, int64_t offset, SDL_IOWhence whence) {
	return SDL_SeekIO(DUMMY_SOURCE(ctx), offset, whence);
}

static int64_t dummy_size(void *ctx) {
	return SDL_GetIOSize(DUMMY_SOURCE(ctx));
}

static size_t dummy_read(void *ctx, void *ptr, size_t size, SDL_IOStatus *status) {
	size_t r = SDL_ReadIO(DUMMY_SOURCE(ctx), ptr, size);
	*status = SDL_GetIOStatus(DUMMY_SOURCE(ctx));
	return r;
}

static size_t dummy_write(void *ctx, const void *ptr, size_t size, SDL_IOStatus *status) {
	size_t r = SDL_WriteIO(DUMMY_SOURCE(ctx), ptr, size);
	*status = SDL_GetIOStatus(DUMMY_SOURCE(ctx));
	return r;
}

SDL_IOStream *SDL_RWWrapDummy(SDL_IOStream *src, bool autoclose, bool readonly) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	SDL_IOStreamInterface iface = {
		.version = sizeof(iface),
		.size = dummy_size,
		.seek = dummy_seek,
		.close = dummy_close,
		.read = dummy_read,
		.write = readonly ? NULL : dummy_write,
	};

	union {
		uintptr_t u;
		void *p;
	} ctx = { .p = src };
	ctx.u |= (autoclose & 1u);

	return SDL_OpenIO(&iface, ctx.p);
}
