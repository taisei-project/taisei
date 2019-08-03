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
#define TRACE_AUTOCLOSE(rw) ((bool)((rw)->hidden.unknown.data2))

static int trace_close(SDL_RWops *rw) {
	int ret = 0;

	if(TRACE_AUTOCLOSE(rw)) {
		ret = SDL_RWclose(TRACE_SOURCE(rw));
		log_debug("[%lx :: %p] close() = %i", SDL_ThreadID(), (void*)rw, ret);
	}

	SDL_FreeRW(rw);
	log_debug("[%lx :: %p] closed", SDL_ThreadID(), (void*)rw);
	return ret;
}

static int64_t trace_seek(SDL_RWops *rw, int64_t offset, int whence) {
	int64_t p = SDL_RWseek(TRACE_SOURCE(rw), offset, whence);
	log_debug("[%lx :: %p] seek(offset=%"PRIi64"; whence=%i) = %"PRIi64, SDL_ThreadID(), (void*)rw, offset, whence, p);
	return p;
}

static int64_t trace_size(SDL_RWops *rw) {
	int64_t s = SDL_RWsize(TRACE_SOURCE(rw));
	log_debug("[%lx :: %p] size() = %"PRIi64, SDL_ThreadID(), (void*)rw, s);
	return s;
}

static size_t trace_read(SDL_RWops *rw, void *ptr, size_t size, size_t maxnum) {
	size_t r = SDL_RWread(TRACE_SOURCE(rw), ptr, size, maxnum);
	log_debug("[%lx :: %p] read(dest=%p; size=%zu; num=%zu) = %zu", SDL_ThreadID(), (void*)rw, ptr, size, maxnum, r);
	log_debug("[%lx :: %p] `--> %"PRIi64, SDL_ThreadID(), (void*)rw, SDL_RWtell(TRACE_SOURCE(rw)));

	if(size > 0 && maxnum > 0 && r == 0) {
		// abort();
	}

	return r;
}

static size_t trace_write(SDL_RWops *rw, const void *ptr, size_t size, size_t maxnum) {
	size_t w = SDL_RWwrite(TRACE_SOURCE(rw), ptr, size, maxnum);
	log_debug("[%lx :: %p] write(dest=%p; size=%zu; num=%zu) = %zu", SDL_ThreadID(), (void*)rw, ptr, size, maxnum, w);
	return w;
}

SDL_RWops *SDL_RWWrapTrace(SDL_RWops *src, bool autoclose) {
	if(!src) {
		return NULL;
	}

	SDL_RWops *rw = SDL_AllocRW();
	memset(rw, 0, sizeof(SDL_RWops));

	rw->hidden.unknown.data1 = src;
	rw->hidden.unknown.data2 = (void*)(intptr_t)autoclose;
	rw->type = SDL_RWOPS_UNKNOWN;

	rw->size = trace_size;
	rw->seek = trace_seek;
	rw->close = trace_close;
	rw->read = trace_read;
	rw->write = trace_write;

	log_debug("[%lx :: %p] opened; src=%p", SDL_ThreadID(), (void*)rw, (void*)src);
	return rw;
}
