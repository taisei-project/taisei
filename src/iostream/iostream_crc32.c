/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "iostream_crc32.h"
#include "iostream/iostream_util.h"

#include <zlib.h>

// #define IOSTREAM_CRC32_DEBUG

#ifdef IOSTREAM_CRC32_DEBUG
	#undef IOSTREAM_CRC32_DEBUG
	#define IOSTREAM_CRC32_DEBUG(...) log_debug(__VA_ARGS__)
#else
	#undef IOSTREAM_CRC32_DEBUG
	#define IOSTREAM_CRC32_DEBUG(...) ((void)0)
#endif

struct crc32_data {
	SDL_IOStream *src;
	uint32_t *crc32_ptr;
	bool autoclose;
};

static bool crc32_close(void *ctx) {
	struct crc32_data *cdata = ctx;

	bool result = true;

	if(cdata->autoclose) {
		result = SDL_CloseIO(cdata->src);
	}

	mem_free(cdata);
	return result;
}

static int64_t crc32_size(void *ctx) {
	struct crc32_data *cdata = ctx;
	return SDL_GetIOSize(cdata->src);
}

static void crc32_update_crc(struct crc32_data *cdata, const void *data, size_t size, char mode) {
	attr_unused uint32_t old = *cdata->crc32_ptr;
	*cdata->crc32_ptr = crc32(*cdata->crc32_ptr, data, size);
	IOSTREAM_CRC32_DEBUG("%c %zu bytes: 0x%08x --> 0x%08x", mode, size, old, *cdata->crc32_ptr);
}

static size_t crc32_read(void *ctx, void *ptr, size_t size, SDL_IOStatus *status) {
	struct crc32_data *cdata = ctx;
	size = SDL_ReadIO(cdata->src, ptr, size);
	*status = SDL_GetIOStatus(cdata->src);
	crc32_update_crc(cdata, ptr, size, 'R');
	return size;
}

static size_t crc32_write(void *ctx, const void *ptr, size_t size, SDL_IOStatus *status) {
	struct crc32_data *cdata = ctx;
	size = SDL_WriteIO(cdata->src, ptr, size);
	*status = SDL_GetIOStatus(cdata->src);
	crc32_update_crc(cdata, ptr, size, 'W');
	return size;
}

SDL_IOStream *iostream_wrap_crc32(SDL_IOStream *src, uint32_t *crc32_ptr, bool autoclose) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	SDL_IOStreamInterface iface = {
		.version = sizeof(iface),
		.size = crc32_size,
		.close = crc32_close,
		.read = crc32_read,
		.write = crc32_write,
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

	auto props = SDL_GetIOProperties(io);
	SDL_SetPointerProperty(props, PROP_IOSTREAM_WRAPPED_STREAM, src);

	return io;
}
