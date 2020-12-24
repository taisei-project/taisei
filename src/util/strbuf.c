/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "strbuf.h"
#include "util/miscmath.h"

#include <stdio.h>

int strbuf_printf(StringBuffer *strbuf, const char *format, ...) {
	va_list va;
	va_start(va, format);
	int r = strbuf_vprintf(strbuf, format, va);
	va_end(va);
	return r;
}

int strbuf_vprintf(StringBuffer *strbuf, const char *format, va_list args) {
	ptrdiff_t offset = strbuf->pos - strbuf->start;
	assume_nolog(offset >= 0);

	ptrdiff_t size_available = strbuf->buf_size - offset;
	assume_nolog(size_available >= 0);

	va_list args_copy;
	va_copy(args_copy, args);
	int size_required = vsnprintf(strbuf->pos, size_available, format, args_copy);
	va_end(args_copy);

	if(size_required >= size_available) {
		size_t new_size = topow2_u64(strbuf->buf_size + (size_required - size_available + 1));
		strbuf->start = realloc(strbuf->start, new_size);
		strbuf->pos = strbuf->start + offset;
		strbuf->buf_size = new_size;
		size_available = new_size - offset;

		va_copy(args_copy, args);
		size_required = vsnprintf(strbuf->pos, size_available, format, args_copy);
		va_end(args_copy);

		assume_nolog(size_required < size_available);
	}

	strbuf->pos += size_required;
	assume_nolog(*strbuf->pos == 0);

	return size_required;
}

void strbuf_clear(StringBuffer *strbuf) {
	strbuf->pos = strbuf->start;

	if(strbuf->start) {
		*strbuf->pos = 0;
	}
}

void strbuf_free(StringBuffer *strbuf) {
	free(strbuf->start);
	memset(strbuf, 0, sizeof(*strbuf));
}
