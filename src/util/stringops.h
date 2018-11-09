/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <time.h>
#include <SDL.h>

#define UNICODE_UNKNOWN 0xFFFD
#define UNICODE_BOM_NATIVE  0xFEFF
#define UNICODE_BOM_SWAPPED 0xFFFE
#define UNICODE_ELLIPSIS 0x2026

#undef strlcat
#define strlcat SDL_strlcat

#undef strlcpy
#define strlcpy SDL_strlcpy

#undef strdup
#define strdup SDL_strdup

#undef strtok_r
#define strtok_r _ts_strtok_r

char* copy_segment(const char *text, const char *delim, int *size);
bool strendswith(const char *s, const char *e) attr_pure;
bool strstartswith(const char *s, const char *p) attr_pure;
bool strendswith_any(const char *s, const char **earray) attr_pure;
bool strstartswith_any(const char *s, const char **earray) attr_pure;
void stralloc(char **dest, const char *src);
char* strjoin(const char *first, ...) attr_sentinel;
char* vstrfmt(const char *fmt, va_list args);
char* strfmt(const char *fmt, ...) attr_printf(1,  2);
void strip_trailing_slashes(char *buf);
char* strtok_r(char *str, const char *delim, char **nextp);
char* strappend(char **dst, char *src);
char* strftimealloc(const char *fmt, const struct tm *timeinfo);

uint32_t* ucs4chr(const uint32_t *ucs4, uint32_t chr);
size_t ucs4len(const uint32_t *ucs4);

void utf8_to_ucs4(const char *utf8, size_t bufsize, uint32_t buf[bufsize]) attr_nonnull(1, 3);
uint32_t* utf8_to_ucs4_alloc(const char *utf8) attr_nonnull(1) attr_returns_nonnull attr_nodiscard;

void ucs4_to_utf8(const uint32_t *ucs4, size_t bufsize, char buf[bufsize]) attr_nonnull(1, 3);
char* ucs4_to_utf8_alloc(const uint32_t *ucs4) attr_nonnull(1) attr_returns_nonnull attr_nodiscard;

uint32_t utf8_getch(const char **src) attr_nonnull(1);

uint32_t crc32str(uint32_t crc, const char *str);

// XXX: Not sure if this is the appropriate header for this

#ifdef TAISEI_BUILDCONF_HAVE_TIMESPEC
typedef struct timespec SystemTime;
#else
typedef struct SystemTime {
	time_t tv_sec;
	long tv_nsec;
} SystemTime;
#endif

#define FILENAME_TIMESTAMP_MIN_BUF_SIZE 23
void get_system_time(SystemTime *time) attr_nonnull(1);
size_t filename_timestamp(char *buf, size_t buf_size, const SystemTime time) attr_nonnull(1);
