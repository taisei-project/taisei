/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rwops_trace.h"
#include "log.h"

#define TRACE_SOURCE(rw) ((SDL_IOStream*)((rw)->hidden.unknown.data1))
#define TRACE_TDATA(rw) ((TData*)((rw)->hidden.unknown.data2))
#define TRACE_AUTOCLOSE(rw) (TRACE_TDATA(rw)->autoclose)
#define TRACE_TAG(rw) (TRACE_TDATA(rw)->tag)

#define TRACE(rw, fmt, ...) \
	log_debug("[%lx :: %p :: %s] " fmt, SDL_GetCurrentThreadID(), (void*)(rw), TRACE_TAG(rw), __VA_ARGS__)

#define TRACE_ERR(rw) \
	TRACE((rw), "Error: %s", SDL_GetError())

typedef struct TData {
	bool autoclose;
	char tag[];
} TData;

static int trace_close(SDL_IOStream *rw) {
	int ret = 0;

	if(TRACE_AUTOCLOSE(rw)) {
		ret = SDL_CloseIO(TRACE_SOURCE(rw));
		TRACE(rw, "close() = %i", ret);

		if(ret < 0) {
			TRACE_ERR(rw);
		}
	}

	TRACE(rw, "closed %i", ret);
	mem_free(rw->hidden.unknown.data2);
	SDL_FreeRW(rw);
	return ret;
}

static int64_t trace_seek(SDL_IOStream *rw, int64_t offset, int whence) {
	int64_t p = SDL_SeekIO(TRACE_SOURCE(rw), offset, whence);
	TRACE(rw, "seek(offset=%"PRIi64"; whence=%i) = %"PRIi64, offset, whence, p);
	if(p < 0) {
		TRACE_ERR(rw);
	}
	return p;
}

static int64_t trace_size(SDL_IOStream *rw) {
	int64_t s = SDL_SizeIO(TRACE_SOURCE(rw));
	TRACE(rw, "size() = %"PRIi64, s);
	if(s < 0) {
		TRACE_ERR(rw);
	}
	return s;
}

static size_t trace_read(SDL_IOStream *rw, void *ptr, size_t size,
			 size_t maxnum) {
	size_t r = /* FIXME MIGRATION: double-check if you use the returned value of SDL_ReadIO() */
		SDL_ReadIO(TRACE_SOURCE(rw), ptr, size * maxnum);
	TRACE(rw, "read(dest=%p; size=%zu; num=%zu) = %zu", ptr, size, maxnum, r);
	TRACE(rw, "`--> %"PRIi64, SDL_TellIO(TRACE_SOURCE(rw)));

	if(r < size * maxnum) {
		TRACE_ERR(rw);
	}

	return r;
}

static size_t trace_write(SDL_IOStream *rw, const void *ptr, size_t size,
			  size_t maxnum) {
	size_t w = /* FIXME MIGRATION: double-check if you use the returned value of SDL_WriteIO() */
		SDL_WriteIO(TRACE_SOURCE(rw), ptr, size * maxnum);
	TRACE(rw, "write(dest=%p; size=%zu; num=%zu) = %zu", ptr, size, maxnum, w);

	if(w < size * maxnum) {
		TRACE_ERR(rw);
	}

	return w;
}

SDL_IOStream *SDL_RWWrapTrace(SDL_IOStream *src, const char *tag,
			      bool autoclose) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	SDL_IOStream *rw = SDL_AllocRW();

	if(UNLIKELY(!rw)) {
		return NULL;
	}

	memset(rw, 0, sizeof(SDL_IOStream));

	auto tdata = ALLOC_FLEX(TData, strlen(tag) + 1);
	tdata->autoclose = autoclose;
	strcpy(tdata->tag, tag);

	rw->hidden.unknown.data1 = src;
	rw->hidden.unknown.data2 = tdata;
	rw->type = SDL_RWOPS_UNKNOWN;

	rw->size = trace_size;
	rw->seek = trace_seek;
	rw->close = trace_close;
	rw->read = trace_read;
	rw->write = trace_write;

	TRACE(rw, "opened; src=%p", (void*)src);
	return rw;
}
