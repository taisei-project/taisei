/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <stdbool.h>
#include <stdnoreturn.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h> // compiling under mingw may fail without this...
#include <png.h>
#include <SDL.h>
#include "util_sse42.h"
#include "hashtable.h"
#include "vfs/public.h"
#include "log.h"
#include "compat.h"
#include "hirestime.h"
#include "assert.h"

//
// string utils
//

#undef strlcat
#undef strlcpy
#define strlcat SDL_strlcat
#define strlcpy SDL_strlcpy

#ifdef strtok_r
#undef strtok_r
#endif

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
#undef strdup
#define strdup SDL_strdup

uint32_t* ucs4chr(const uint32_t *ucs4, uint32_t chr);
size_t ucs4len(const uint32_t *ucs4);
uint32_t* utf8_to_ucs4(const char *utf8);
char* ucs4_to_utf8(const uint32_t *ucs4);

//
// math utils
//

typedef struct FloatRect {
	float x;
	float y;
	float w;
	float h;
} FloatRect;

typedef struct IntRect {
	int x;
	int y;
	int w;
	int h;
} IntRect;

#include <complex.h>

// These definitions are common but non-standard, so we provide our own
#undef M_PI
#undef M_PI_2
#undef M_PI_4
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_PI_4 0.78539816339744830962

// This is a workaround to properly specify the type of our "complex" variables...
// Taisei code always uses just "complex" when it actually means "complex double", which is not really correct...
// gcc doesn't seem to care, other compilers do (e.g. clang)
#undef complex
#define complex _Complex double

// needed for mingw compatibility:
#undef min
#undef max

double min(double, double) attr_const;
double max(double, double) attr_const;
double clamp(double, double, double) attr_const;
double approach(double v, double t, double d) attr_const;
double psin(double) attr_const;
int sign(double) attr_const;
double swing(double x, double s) attr_const;
uint topow2(uint x) attr_const;
float ftopow2(float x) attr_const;
float smooth(float x) attr_const;
float smoothreclamp(float x, float old_min, float old_max, float new_min, float new_max) attr_const;
float sanitize_scale(float scale) attr_const;

#include <cglm/types.h>

//
// gl/video utils
//

void set_ortho(void);
void set_ortho_ex(float w, float h);
void colorfill(float r, float g, float b, float a);
void fade_out(float f);
void draw_stars(int x, int y, int numstars, int numfrags, int maxstars, int maxfrags, float alpha, float star_width);

//
// i/o utils
//

typedef void (*KVCallback)(const char *key, const char *value, void *data);

typedef struct KVSpec {
	const char *name;

	char **out_str;
	int *out_int;
	double *out_double;
	float *out_float;
} KVSpec;

char* read_all(const char *filename, int *size);
bool parse_keyvalue_stream_cb(SDL_RWops *strm, KVCallback callback, void *data);
bool parse_keyvalue_file_cb(const char *filename, KVCallback callback, void *data);
Hashtable* parse_keyvalue_stream(SDL_RWops *strm, size_t tablesize);
Hashtable* parse_keyvalue_file(const char *filename, size_t tablesize);
bool parse_keyvalue_stream_with_spec(SDL_RWops *strm, KVSpec *spec);
bool parse_keyvalue_file_with_spec(const char *filename, KVSpec *spec);
void png_init_rwops_read(png_structp png, SDL_RWops *rwops);
void png_init_rwops_write(png_structp png, SDL_RWops *rwops);

char* SDL_RWgets(SDL_RWops *rwops, char *buf, size_t bufsize);
size_t SDL_RWprintf(SDL_RWops *rwops, const char* fmt, ...) attr_printf(2, 3);

// This is for the very few legitimate uses for printf/fprintf that shouldn't be replaced with log_*
void tsfprintf(FILE *out, const char *restrict fmt, ...) attr_printf(2, 3);

char* try_path(const char *prefix, const char *name, const char *ext);

//
// misc utils
//

void* memdup(const void *src, size_t size);
int getenvint(const char *v, int defaultval) attr_pure;
void png_setup_error_handlers(png_structp png);
uint32_t crc32str(uint32_t crc, const char *str) attr_hot attr_pure;

#ifdef DEBUG
	typedef struct DebugInfo {
		const char *file;
		const char *func;
		uint line;
	} DebugInfo;

	#define _DEBUG_INFO_PTR_ (&(DebugInfo){ __FILE__, __func__, __LINE__ })
	#define set_debug_info(debug) _set_debug_info(debug, _DEBUG_INFO_PTR_)
	void _set_debug_info(DebugInfo *debug, DebugInfo *meta);
	DebugInfo* get_debug_info(void);
	DebugInfo* get_debug_meta(void);

	extern bool _in_draw_code;
	#define BEGIN_DRAW_CODE() do { if(_in_draw_code) { log_fatal("BEGIN_DRAW_CODE not followed by END_DRAW_CODE"); } _in_draw_code = true; } while(0)
	#define END_DRAW_CODE() do { if(!_in_draw_code) { log_fatal("END_DRAW_CODE not preceeded by BEGIN_DRAW_CODE"); } _in_draw_code = false; } while(0)
	#define IN_DRAW_CODE (_in_draw_code)
#else
	#define set_debug_info(debug)
	#define BEGIN_DRAW_CODE()
	#define END_DRAW_CODE()
	#define IN_DRAW_CODE (0)
#endif

//
// safeguards against some dangerous or otherwise undesirable practices
//

PRAGMA(GCC diagnostic push)
PRAGMA(GCC diagnostic ignored "-Wstrict-prototypes")

// clang generates lots of these warnings with _FORTIFY_SOURCE
PRAGMA(GCC diagnostic ignored "-Wignored-attributes")

#undef fopen
attr_deprecated("Use vfs_open or SDL_RWFromFile instead")
FILE* fopen();

#undef strncat
attr_deprecated("This function likely doesn't do what you expect, use strlcat")
char* strncat();

#undef strncpy
attr_deprecated("This function likely doesn't do what you expect, use strlcpy")
char* strncpy();

#undef errx
attr_deprecated("Use log_fatal instead")
noreturn void errx(int, const char*, ...);

#undef warnx
attr_deprecated("Use log_warn instead")
void warnx(const char*, ...);

#undef printf
attr_deprecated("Use log_info instead")
int printf(const char*, ...);

#undef fprintf
attr_deprecated("Use log_warn instead (or SDL_RWops if you want to write to a file)")
int fprintf(FILE*, const char*, ...);

#undef strtok
attr_deprecated("Use strtok_r instead")
char* strtok();

#undef sprintf
attr_deprecated("Use snprintf or strfmt instead")
int sprintf(char *, const char*, ...);

PRAGMA(GCC diagnostic pop)
