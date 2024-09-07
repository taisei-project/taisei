/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "io.h"

#include "assert.h"
#include "log.h"
#include "rwops/rwops_autobuf.h"
#include "stringops.h"
#include "vfs/public.h"

#ifdef TAISEI_BUILDCONF_HAVE_POSIX
#include <unistd.h>
#endif

char *SDL_RWgets(SDL_IOStream *rwops, char *buf, size_t bufsize) {
	char c, *ptr = buf, *end = buf + bufsize - 1;
	assert(end > ptr);

	while(ptr <= end) {
		if(!SDL_ReadU8(rwops, (uint8_t*)&c)) {
			if(SDL_GetIOStatus(rwops) != SDL_IO_STATUS_EOF) {
				log_sdl_error(LOG_ERROR, "SDL_ReadU8");
			}

			break;
		}

		if(!c) {
			break;
		}

		if((*ptr++ = c) == '\n') {
			break;
		}
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

char *SDL_RWgets_arena(SDL_IOStream *io, MemArena *arena, size_t *out_buf_size) {
	size_t chunk_size = 64;
	size_t buf_size = chunk_size;
	char *buf = marena_alloc(arena, buf_size);
	uint32_t pos = 0;

	while(true) {
		uint8_t c;
		bool ok = SDL_ReadU8(io, &c);

		if(!ok) {
			if(SDL_GetIOStatus(io) != SDL_IO_STATUS_EOF) {
				log_sdl_error(LOG_ERROR, "SDL_ReadU8");
			}

			break;
		}

		if(c == 0) {
			break;
		}

		buf[pos++] = (char)c;

		if(c == '\n') {
			break;
		}

		if(pos == buf_size) {
			buf_size += 1;
			buf = marena_realloc(arena, buf, buf_size - 1, buf_size);
		}
	}

	if(pos == buf_size) {
		buf_size += 1;
		buf = marena_realloc(arena, buf, buf_size - 1, buf_size);
	}

	buf[pos] = 0;

	if(out_buf_size) {
		*out_buf_size = buf_size;
	}

	return buf;
}

char *SDL_RWgets_realloc(SDL_IOStream *rwops, char **buf, size_t *bufsize) {
	char c, *ptr = *buf, *end = *buf + *bufsize - 1;
	assert(end >= ptr);

	while(true) {
		if(!SDL_ReadU8(rwops, (uint8_t*)&c)) {
			if(SDL_GetIOStatus(rwops) != SDL_IO_STATUS_EOF) {
				log_sdl_error(LOG_ERROR, "SDL_ReadU8");
			}

			break;
		}

		if(!c) {
			break;
		}

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

size_t SDL_RWprintf(SDL_IOStream *rwops, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	char *str = vstrfmt(fmt, args);
	va_end(args);

	size_t ret = /* FIXME MIGRATION: double-check if you use the returned value of SDL_WriteIO() */
		SDL_WriteIO(rwops, str, strlen(str));
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

static void *SDL_RWreadAll_known_size(SDL_IOStream *rwops, size_t file_size,
				      size_t *out_size) {
	char *start = mem_alloc(file_size);
	char *end = start + file_size;
	char *pbuf = start;

	assert(end >= start);

	for(;;) {
		size_t read = /* FIXME MIGRATION: double-check if you use the returned value of SDL_ReadIO() */
			SDL_ReadIO(rwops, pbuf, (end - pbuf));
		assert(read <= end - pbuf);

		if(read == 0) {
			*out_size = pbuf - start;
			return start;
		}

		pbuf += read;
	}
}

void *SDL_RWreadAll(SDL_IOStream *rwops, size_t *out_size, size_t max_size) {
	int64_t file_size = SDL_GetIOSize(rwops);

	if(file_size >= 0) {
		if(max_size && file_size > max_size) {
			SDL_SetError("File is too large (%zu bytes; max is %zu)", (size_t)file_size, max_size);
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
	SDL_IOStream *autobuf = NOT_NULL(SDL_RWAutoBuffer(&buf, chunk_size));
	void *chunk = mem_alloc(chunk_size);

	size_t total_size = 0;

	for(;;) {
		size_t read = /* FIXME MIGRATION: double-check if you use the returned value of SDL_ReadIO() */
			SDL_ReadIO(rwops, chunk, chunk_size);

		if(read == 0) {
			mem_free(chunk);
			SDL_CloseIO(rwops);
			buf = memdup(buf, total_size);
			SDL_CloseIO(autobuf);
			*out_size = total_size;
			return buf;
		}

		size_t write = /* FIXME MIGRATION: double-check if you use the returned value of SDL_WriteIO() */
			SDL_WriteIO(autobuf, chunk, chunk_size);

		if(UNLIKELY(write != read)) {
			mem_free(chunk);
			SDL_CloseIO(rwops);
			SDL_CloseIO(autobuf);
			return NULL;
		}

		total_size += write;

		if(max_size && UNLIKELY(total_size > max_size)) {
			SDL_SetError("File is too large (%zu bytes read so far; max is %zu)", total_size, max_size);
			mem_free(chunk);
			SDL_CloseIO(rwops);
			SDL_CloseIO(autobuf);
			return NULL;
		}
	}
}

void SDL_RWsync(SDL_IOStream *rwops) {
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
