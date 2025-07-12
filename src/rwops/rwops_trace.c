/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rwops_trace.h"
#include "log.h"

#define TRACE(tdata, fmt, ...) \
	log_debug("[%llx :: %p :: %s] " fmt, (unsigned long long)SDL_GetCurrentThreadID(), (tdata), (tdata)->tag, __VA_ARGS__)

#define TRACE_ERR(rw) \
	TRACE((rw), "Error: %s", SDL_GetError())

typedef struct TData {
	SDL_IOStream *wrapped;
	bool autoclose;
	char tag[];
} TData;

static bool trace_close(void *ctx) {
	TData *tdata = ctx;
	bool ret = true;

	if(tdata->autoclose) {
		ret = SDL_CloseIO(tdata->wrapped);
		TRACE(tdata, "close() = %i", ret);

		if(!ret) {
			TRACE_ERR(tdata);
		}
	}

	TRACE(tdata, "closed %i", ret);
	mem_free(tdata);
	return ret;
}

static int64_t trace_seek(void *ctx, int64_t offset, SDL_IOWhence whence) {
	TData *tdata = ctx;
	int64_t p = SDL_SeekIO(tdata->wrapped, offset, whence);
	TRACE(tdata, "seek(offset=%"PRIi64"; whence=%i) = %"PRIi64, offset, whence, p);
	if(p < 0) {
		TRACE_ERR(tdata);
	}
	return p;
}

static int64_t trace_size(void *ctx) {
	TData *tdata = ctx;
	int64_t s = SDL_GetIOSize(tdata->wrapped);
	TRACE(tdata, "size() = %"PRIi64, s);
	if(s < 0) {
		TRACE_ERR(tdata);
	}
	return s;
}

static size_t trace_read(void *ctx, void *ptr, size_t size, SDL_IOStatus *status) {
	TData *tdata = ctx;
	size_t r = SDL_ReadIO(tdata->wrapped, ptr, size);
	*status = SDL_GetIOStatus(tdata->wrapped);

	TRACE(tdata, "read(dest=%p; size=%zu) = %zu; status = %i", ptr, size, r, *status);
	TRACE(tdata, "`--> %"PRIi64, SDL_TellIO(tdata->wrapped));

	if(*status != SDL_IO_STATUS_READY && *status != SDL_IO_STATUS_EOF) {
		TRACE_ERR(tdata);
	}

	return r;
}

static size_t trace_write(void *ctx, const void *ptr, size_t size, SDL_IOStatus *status) {
	TData *tdata = ctx;
	size_t w = SDL_WriteIO(tdata->wrapped, ptr, size);
	*status = SDL_GetIOStatus(tdata->wrapped);

	TRACE(tdata, "write(dest=%p; size=%zu) = %zu; status = %i", ptr, size, w, *status);

	if(*status != SDL_IO_STATUS_READY) {
		TRACE_ERR(tdata);
	}

	return w;
}

SDL_IOStream *SDL_RWWrapTrace(SDL_IOStream *src, const char *tag, bool autoclose) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	SDL_IOStreamInterface iface = {
		.version = sizeof(iface),
		.size = trace_size,
		.seek = trace_seek,
		.close = trace_close,
		.read = trace_read,
		.write = trace_write,
	};

	size_t taglen = strlen(tag) + 1;
	auto tdata = ALLOC_FLEX(TData, taglen);
	tdata->wrapped = src;
	tdata->autoclose = autoclose;
	memcpy(tdata->tag, tag, taglen);

	SDL_IOStream *io = SDL_OpenIO(&iface, tdata);

	if(UNLIKELY(!io)) {
		mem_free(tdata);
		return NULL;
	}

	TRACE(tdata, "opened; src=%p", (void*)src);
	return io;
}
