/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <time.h>
#include <SDL.h>

#include "systime.h"
#include "memory.h"

#define UNICODE_UNKNOWN 0xFFFD
#define UNICODE_BOM_NATIVE  0xFEFF
#define UNICODE_BOM_SWAPPED 0xFFFE
#define UNICODE_ELLIPSIS 0x2026

#undef strlcat
#define strlcat SDL_strlcat

#undef strlcpy
#define strlcpy SDL_strlcpy

#undef strdup
#define strdup _ts_strdup
INLINE attr_returns_allocated attr_nonnull(1) char *strdup(const char *str) {
	size_t sz = strlen(str) + 1;
	return memcpy(mem_alloc(sz), str, sz);
}

#ifndef TAISEI_BUILDCONF_HAVE_STRTOK_R
#undef strtok_r
#define strtok_r _ts_strtok_r
char *strtok_r(char *str, const char *delim, char **nextp);
#endif

#ifndef TAISEI_BUILDCONF_HAVE_MEMRCHR
#undef memrchr
#define memrchr _ts_memrchr
void *memrchr(const void *s, int c, size_t n);
#endif

#ifndef TAISEI_BUILDCONF_HAVE_MEMMEM
#undef memmem
#define memmem _ts_memmem
void *memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen)
	attr_nonnull_all;
#endif

#undef strcasecmp
#define strcasecmp SDL_strcasecmp

bool strendswith(const char *s, const char *e) attr_pure;
bool strstartswith(const char *s, const char *p) attr_pure;
bool strendswith_any(const char *s, const char **earray) attr_pure;
bool strstartswith_any(const char *s, const char **earray) attr_pure;
void stralloc(char **dest, const char *src);
char* strjoin(const char *first, ...) attr_sentinel attr_returns_allocated;
char* vstrfmt(const char *fmt, va_list args) attr_returns_allocated;
char* strfmt(const char *fmt, ...) attr_printf(1,  2) attr_returns_allocated;
char* strappend(char **dst, char *src);
char* strftimealloc(const char *fmt, const struct tm *timeinfo) attr_returns_allocated;
void expand_escape_sequences(char *str);

uint32_t* ucs4chr(const uint32_t *ucs4, uint32_t chr) attr_dealloc(SDL_free, 1);
size_t ucs4len(const uint32_t *ucs4);

void utf8_to_ucs4(const char *utf8, size_t bufsize, uint32_t buf[bufsize]) attr_nonnull(1, 3);
uint32_t* utf8_to_ucs4_alloc(const char *utf8) attr_nonnull(1) attr_returns_allocated attr_nonnull(1);

void ucs4_to_utf8(const uint32_t *ucs4, size_t bufsize, char buf[bufsize]) attr_nonnull(1, 3);
char* ucs4_to_utf8_alloc(const uint32_t *ucs4) attr_nonnull(1) attr_returns_allocated attr_nonnull(1);

uint32_t utf8_getch(const char **src) attr_nonnull(1);

void format_huge_num(uint digits, uint64_t num, size_t bufsize, char buf[bufsize]);
void hexdigest(uint8_t *input, size_t input_size, char *output, size_t output_size);

#define FILENAME_TIMESTAMP_MIN_BUF_SIZE 23
size_t filename_timestamp(char *buf, size_t buf_size, const SystemTime time) attr_nonnull(1);

/*
 * Test if string matches a simple 'glob'-like pattern.
 *
 * Only '*' is supported as a metacharacter in the glob.
 * '*' matches zero or more characters lazily.
 * If the glob is not empty and does not end with a *, the last segment is matched greedily.
 * An empty glob matches everything.
 */
bool strnmatch(size_t globsize, const char glob[globsize], size_t insize, const char input[insize]);
bool strmatch(const char *glob, const char *input);
