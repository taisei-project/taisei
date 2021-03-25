/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "rwops_trace.h"
#include "util.h"

#define TRACE_SOURCE(rw) ((SDL_RWops*)((rw)->hidden.unknown.data1))
#define TRACE_TDATA(rw) ((TData*)((rw)->hidden.unknown.data2))
#define TRACE_AUTOCLOSE(rw) (TRACE_TDATA(rw)->autoclose)
#define TRACE_TAG(rw) (TRACE_TDATA(rw)->tag)

#define TRACE(rw, fmt, ...) \
	log_debug("[%lx :: %p :: %s] " fmt, SDL_ThreadID(), (void*)(rw), TRACE_TAG(rw), __VA_ARGS__)

#define TRACE_ERR(rw) \
	TRACE((rw), "Error: %s", SDL_GetError())

typedef struct TData {
	bool autoclose;
	char tag[];
} TData;

static int trace_close(SDL_RWops *rw) {
	int ret = 0;

	if(TRACE_AUTOCLOSE(rw)) {
		ret = SDL_RWclose(TRACE_SOURCE(rw));
		TRACE(rw, "close() = %i", ret);

		if(ret < 0) {
			TRACE_ERR(rw);
		}
	}

	TRACE(rw, "closed %i", ret);
	free(rw->hidden.unknown.data2);
	SDL_FreeRW(rw);
	return ret;
}

static int64_t trace_seek(SDL_RWops *rw, int64_t offset, int whence) {
	int64_t p = SDL_RWseek(TRACE_SOURCE(rw), offset, whence);
	TRACE(rw, "seek(offset=%"PRIi64"; whence=%i) = %"PRIi64, offset, whence, p);
	if(p < 0) {
		TRACE_ERR(rw);
	}
	return p;
}

static int64_t trace_size(SDL_RWops *rw) {
	int64_t s = SDL_RWsize(TRACE_SOURCE(rw));
	TRACE(rw, "size() = %"PRIi64, s);
	if(s < 0) {
		TRACE_ERR(rw);
	}
	return s;
}

static size_t trace_read(SDL_RWops *rw, void *ptr, size_t size, size_t maxnum) {
	size_t r = SDL_RWread(TRACE_SOURCE(rw), ptr, size, maxnum);
	TRACE(rw, "read(dest=%p; size=%zu; num=%zu) = %zu", ptr, size, maxnum, r);
	TRACE(rw, "`--> %"PRIi64, SDL_RWtell(TRACE_SOURCE(rw)));

	if(r < size * maxnum) {
		TRACE_ERR(rw);
	}

	return r;
}

static size_t trace_write(SDL_RWops *rw, const void *ptr, size_t size, size_t maxnum) {
	size_t w = SDL_RWwrite(TRACE_SOURCE(rw), ptr, size, maxnum);
	TRACE(rw, "write(dest=%p; size=%zu; num=%zu) = %zu", ptr, size, maxnum, w);

	if(w < size * maxnum) {
		TRACE_ERR(rw);
	}

	return w;
}

SDL_RWops *SDL_RWWrapTrace(SDL_RWops *src, const char *tag, bool autoclose) {
	if(!src) {
		return NULL;
	}

	SDL_RWops *rw = SDL_AllocRW();
	memset(rw, 0, sizeof(SDL_RWops));

	TData *tdata = calloc(1, sizeof(*tdata) + strlen(tag) + 1);
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
