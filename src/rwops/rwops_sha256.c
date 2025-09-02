/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rwops_sha256.h"

struct sha256_ctx {
	SDL_IOStream *src;
	SHA256State *sha256;
	bool autoclose;
};

static bool rwsha256_close(void *ctx) {
	struct sha256_ctx *sctx = ctx;

	bool result = true;

	if(sctx->autoclose) {
		result = SDL_CloseIO(sctx->src);
	}

	mem_free(sctx);
	return result;
}

static int64_t rwsha256_seek(void *ctx, int64_t offset, SDL_IOWhence whence) {
	struct sha256_ctx *sctx = ctx;

	if(!offset && whence == SDL_IO_SEEK_CUR) {
		return SDL_SeekIO(sctx->src, offset, whence);
	}

	SDL_SetError("Stream is not seekable");
	return -1;
}

static int64_t rwsha256_size(void *ctx) {
	struct sha256_ctx *sctx = ctx;
	return SDL_GetIOSize(sctx->src);
}

static void rwsha256_update_sha256(struct sha256_ctx *sctx, const void *data, size_t size) {
	sha256_update(sctx->sha256, data, size);
}

static size_t rwsha256_read(void *ctx, void *ptr, size_t size, SDL_IOStatus *status) {
	struct sha256_ctx *sctx = ctx;
	size_t result = SDL_ReadIO(sctx->src, ptr, size);
	*status = SDL_GetIOStatus(sctx->src);
	rwsha256_update_sha256(sctx, ptr, result);
	return result;
}

static size_t rwsha256_write(void *ctx, const void *ptr, size_t size, SDL_IOStatus *status) {
	struct sha256_ctx *sctx = ctx;
	size_t result = SDL_WriteIO(sctx->src, ptr, size);
	*status = SDL_GetIOStatus(sctx->src);
	rwsha256_update_sha256(sctx, ptr, result);
	return result;
}

SDL_IOStream *SDL_RWWrapSHA256(SDL_IOStream *src, SHA256State *sha256, bool autoclose) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	SDL_IOStreamInterface iface = {
		.version = sizeof(iface),
		.size = rwsha256_size,
		.seek = rwsha256_seek,
		.close = rwsha256_close,
		.read = rwsha256_read,
		.write = rwsha256_write,
	};

	auto sctx = ALLOC(struct sha256_ctx, {
		.autoclose = autoclose,
		.sha256 = sha256,
		.src = src,
	});

	SDL_IOStream *io = SDL_OpenIO(&iface, sctx);

	if(UNLIKELY(!io)) {
		mem_free(sctx);
		return NULL;
	}

	return io;
}
