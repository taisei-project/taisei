/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "io.h"

#include "log.h"
#include "stringops.h"
#include "vfs/public.h"

#ifdef TAISEI_BUILDCONF_HAVE_POSIX
#include <unistd.h>
#endif

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

			if(pos == 0) {
				return NULL;
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

size_t SDL_RWprintf_arena(SDL_IOStream *rwops, MemArena *scratch, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	size_t ret = SDL_RWvprintf_arena(rwops, scratch, fmt, args);
	va_end(args);
	return ret;
}

size_t SDL_RWvprintf_arena(SDL_IOStream *rwops, MemArena *scratch, const char *fmt, va_list args) {
	auto snapshot = marena_snapshot(scratch);
	char *str = vstrfmt_arena(scratch, fmt, args);
	size_t ret = SDL_WriteIO(rwops, str, strlen(str));
	marena_rollback(scratch, &snapshot);
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
