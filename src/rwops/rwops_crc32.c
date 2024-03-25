/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rwops_crc32.h"

#include <zlib.h>

// #define RWCRC32_DEBUG

#ifdef RWCRC32_DEBUG
	#undef RWCRC32_DEBUG
	#define RWCRC32_DEBUG(...) log_debug(__VA_ARGS__)
#else
	#undef RWCRC32_DEBUG
	#define RWCRC32_DEBUG(...) ((void)0)
#endif

struct crc32_data {
	SDL_IOStream *src;
	uint32_t *crc32_ptr;
	bool autoclose;
};

static bool rwcrc32_close(void *ctx) {
	struct crc32_data *cdata = ctx;

	bool result = true;

	if(cdata->autoclose) {
		result = SDL_CloseIO(cdata->src);
	}

	mem_free(cdata);
	return result;
}

static int64_t rwcrc32_size(void *ctx) {
	struct crc32_data *cdata = ctx;
	return SDL_GetIOSize(cdata->src);
}

static void rwcrc32_update_crc(struct crc32_data *cdata, const void *data, size_t size, char mode) {
	attr_unused uint32_t old = *cdata->crc32_ptr;
	*cdata->crc32_ptr = crc32(*cdata->crc32_ptr, data, size);
	RWCRC32_DEBUG("%c %zu bytes: 0x%08x --> 0x%08x", mode, size, old, *cdata->crc32_ptr);
}

static size_t rwcrc32_read(void *ctx, void *ptr, size_t size, SDL_IOStatus *status) {
	struct crc32_data *cdata = ctx;
	size = SDL_ReadIO(cdata->src, ptr, size);
	*status = SDL_GetIOStatus(cdata->src);
	rwcrc32_update_crc(cdata, ptr, size, 'R');
	return size;
}

static size_t rwcrc32_write(void *ctx, const void *ptr, size_t size, SDL_IOStatus *status) {
	struct crc32_data *cdata = ctx;
	size = SDL_WriteIO(cdata->src, ptr, size);
	*status = SDL_GetIOStatus(cdata->src);
	rwcrc32_update_crc(cdata, ptr, size, 'W');
	return size;
}

SDL_IOStream *SDL_RWWrapCRC32(SDL_IOStream *src, uint32_t *crc32_ptr, bool autoclose) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	SDL_IOStreamInterface iface = {
		.version = sizeof(iface),
		.size = rwcrc32_size,
		.close = rwcrc32_close,
		.read = rwcrc32_read,
		.write = rwcrc32_write,
	};

	auto cdata = ALLOC(struct crc32_data, {
		.autoclose = autoclose,
		.crc32_ptr = crc32_ptr,
		.src = src,
	});

	SDL_IOStream *io = SDL_OpenIO(&iface, cdata);

	if(UNLIKELY(!io)) {
		mem_free(cdata);
		return NULL;
	}

	return io;
}
