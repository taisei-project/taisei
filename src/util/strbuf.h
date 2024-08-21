/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <stdarg.h>

typedef struct StringBuffer {
	char *start;
	char *pos;
	size_t buf_size;
} StringBuffer;

int strbuf_printf(StringBuffer *strbuf, const char *format, ...)
	attr_printf(2, 3) attr_nonnull(1, 2);

int strbuf_vprintf(StringBuffer *strbuf, const char *format, va_list args)
	attr_nonnull(1, 2);

void strbuf_clear(StringBuffer *strbuf)
	attr_nonnull(1);

void strbuf_free(StringBuffer *strbuf)
	attr_nonnull(1);

int strbuf_ncat(StringBuffer *strbuf, size_t datasize, const char data[])
	attr_nonnull(1, 3);

INLINE int strbuf_cat(StringBuffer *strbuf, const char *str) {
	return strbuf_ncat(strbuf, strlen(str), str);
}
