/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "io.h"
#include "log.h"
#include "vfs/public.h"
#include "assert.h"
#include "stringops.h"
#include "rwops/rwops_autobuf.h"
#include "util/crap.h"

#ifdef TAISEI_BUILDCONF_HAVE_POSIX
#include <unistd.h>
#endif

char *SDL_RWgets(SDL_RWops *rwops, char *buf, size_t bufsize) {
	char c, *ptr = buf, *end = buf + bufsize - 1;
	assert(end > ptr);

	while((c = SDL_ReadU8(rwops)) && ptr <= end) {
		if((*ptr++ = c) == '\n')
			break;
	}

	if(ptr == buf)
		return NULL;

	if(ptr > end) {
		*end = 0;
		log_warn("Line too long (%zu bytes max): %s", bufsize, buf);
	} else {
		*ptr = 0;
	}

	return buf;
}

char *SDL_RWgets_realloc(SDL_RWops *rwops, char **buf, size_t *bufsize) {
	char c, *ptr = *buf, *end = *buf + *bufsize - 1;
	assert(end >= ptr);

	while((c = SDL_ReadU8(rwops))) {
		*ptr++ = c;

		if(ptr > end) {
			ptrdiff_t ofs = ptr - *buf;
			*bufsize *= 2;
			*buf = mem_realloc(*buf, *bufsize);
			end = *buf + *bufsize - 1;
			ptr = *buf + ofs;
			*end = 0;
		}

		if(c == '\n') {
			break;
		}
	}

	if(ptr == *buf)
		return NULL;

	assert(ptr <= end);
	*ptr = 0;

	return *buf;
}

size_t SDL_RWprintf(SDL_RWops *rwops, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	char *str = vstrfmt(fmt, args);
	va_end(args);

	size_t ret = SDL_RWwrite(rwops, str, 1, strlen(str));
	mem_free(str);

	return ret;
}

void tsfprintf(FILE *out, const char *restrict fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vfprintf(out, fmt, args);
	va_end(args);
}

char *try_path(const char *prefix, const char *name, const char *ext) {
	char *p = strjoin(prefix, name, ext, NULL);

	if(vfs_query(p).exists) {
		return p;
	}

	mem_free(p);
	return NULL;
}

static void *SDL_RWreadAll_known_size(SDL_RWops *rwops, size_t file_size, size_t *out_size) {
	char *start = mem_alloc(file_size);
	char *end = start + file_size;
	char *pbuf = start;

	assert(end >= start);

	for(;;) {
		size_t read = SDL_RWread(rwops, pbuf, 1, end - pbuf);
		assert(read <= end - pbuf);

		if(read == 0) {
			*out_size = pbuf - start;
			return start;
		}

		pbuf += read;
	}
}

void *SDL_RWreadAll(SDL_RWops *rwops, size_t *out_size, size_t max_size) {
	ssize_t file_size = SDL_RWsize(rwops);

	if(file_size >= 0) {
		if(max_size && file_size > max_size) {
			SDL_SetError("File is too large (%zu bytes; max is %zu)", file_size, max_size);
			return NULL;
		}

		size_t sz;
		void *result = SDL_RWreadAll_known_size(rwops, file_size, &sz);

		if(result) {
			*out_size = sz;
		}

		return result;
	}

	static const size_t chunk_size = BUFSIZ;

	void *buf;
	SDL_RWops *autobuf = NOT_NULL(SDL_RWAutoBuffer(&buf, chunk_size));
	void *chunk = mem_alloc(chunk_size);

	size_t total_size = 0;

	for(;;) {
		size_t read = SDL_RWread(rwops, chunk, 1, chunk_size);

		if(read == 0) {
			mem_free(chunk);
			SDL_RWclose(rwops);
			buf = memdup(buf, total_size);
			SDL_RWclose(autobuf);
			*out_size = total_size;
			return buf;
		}

		size_t write = SDL_RWwrite(autobuf, chunk, 1, chunk_size);

		if(UNLIKELY(write != read)) {
			mem_free(chunk);
			SDL_RWclose(rwops);
			SDL_RWclose(autobuf);
			return NULL;
		}

		total_size += write;

		if(max_size && UNLIKELY(total_size > max_size)) {
			SDL_SetError("File is too large (%zu bytes read so far; max is %zu)", total_size, max_size);
			mem_free(chunk);
			SDL_RWclose(rwops);
			SDL_RWclose(autobuf);
			return NULL;
		}
	}
}

void SDL_RWsync(SDL_RWops *rwops) {
	#if HAVE_STDIO_H
	if(rwops->type == SDL_RWOPS_STDFILE) {
		FILE *fp = rwops->hidden.stdio.fp;

		if(UNLIKELY(!fp)) {
			return;
		}

		fflush(fp);

		#ifdef TAISEI_BUILDCONF_HAVE_POSIX
		fsync(fileno(fp));
		#endif
	}
	#endif
}
