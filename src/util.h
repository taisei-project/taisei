/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once

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
bool strendswith(const char *s, const char *e) __attribute__((pure));
bool strstartswith(const char *s, const char *p) __attribute__((pure));
bool strendswith_any(const char *s, const char **earray) __attribute__((pure));
bool strstartswith_any(const char *s, const char **earray) __attribute__((pure));
void stralloc(char **dest, const char *src);
char* strjoin(const char *first, ...) __attribute__((sentinel));
char* vstrfmt(const char *fmt, va_list args);
char* strfmt(const char *fmt, ...) __attribute__((format(FORMAT_ATTR, 1, 2)));
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

#include <complex.h>

// These definitions are common but non-standard, so we provide our own
#undef M_PI
#undef M_PI_2
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923

// This is a workaround to properly specify the type of our "complex" variables...
// Taisei code always uses just "complex" when it actually means "complex double", which is not really correct...
// gcc doesn't seem to care, other compilers do (e.g. clang)
#ifdef complex
    #undef complex
    #define complex _Complex double
#endif

// needed for mingw compatibility:
#undef min
#undef max

double min(double, double) __attribute__((const));
double max(double, double) __attribute__((const));
double clamp(double, double, double) __attribute__((const));
double approach(double v, double t, double d) __attribute__((const));
double psin(double) __attribute__((const));
int sign(double) __attribute__((const));
double swing(double x, double s) __attribute__((const));
unsigned int topow2(unsigned int x) __attribute__((const));
float ftopow2(float x) __attribute__((const));
float smooth(float x) __attribute__((const));
float smoothreclamp(float x, float old_min, float old_max, float new_min, float new_max) __attribute__((const));
float sanitize_scale(float scale) __attribute__((const));

//
// gl/video utils
//

typedef struct {
    hrtime_t frametimes[120]; // size = number of frames to average
    double fps; // average fps over the last X frames
    hrtime_t last_update_time; // internal; last time the average was recalculated
} FPSCounter;

void loop_at_fps(bool (*frame_func)(void*), bool (*limiter_cond_func)(void*), void *arg, uint32_t fps);
void fpscounter_reset(FPSCounter *fps);
void fpscounter_update(FPSCounter *fps);
void set_ortho(void);
void set_ortho_ex(float w, float h);
void colorfill(float r, float g, float b, float a);
void fade_out(float f);

//
// i/o utils
//

typedef void (*KVCallback)(const char *key, const char *value, void *data);

char* read_all(const char *filename, int *size);
bool parse_keyvalue_stream_cb(SDL_RWops *strm, KVCallback callback, void *data);
bool parse_keyvalue_file_cb(const char *filename, KVCallback callback, void *data);
Hashtable* parse_keyvalue_stream(SDL_RWops *strm, size_t tablesize);
Hashtable* parse_keyvalue_file(const char *filename, size_t tablesize);
void png_init_rwops_read(png_structp png, SDL_RWops *rwops);
void png_init_rwops_write(png_structp png, SDL_RWops *rwops);

char* SDL_RWgets(SDL_RWops *rwops, char *buf, size_t bufsize);
size_t SDL_RWprintf(SDL_RWops *rwops, const char* fmt, ...) __attribute__((format(FORMAT_ATTR, 2, 3)));

// This is for the very few legitimate uses for printf/fprintf that shouldn't be replaced with log_*
void tsfprintf(FILE *out, const char *restrict fmt, ...) __attribute__((format(FORMAT_ATTR, 2, 3)));

//
// misc utils
//

int getenvint(const char *v, int defaultval) __attribute__((pure));
void png_setup_error_handlers(png_structp png);
uint32_t crc32str(uint32_t crc, const char *str) __attribute__((hot, pure));

#ifdef HAVE_INTEL_INTRIN
uint32_t crc32str_sse42(uint32_t crc, const char *str) __attribute__((hot, pure));
#else
#define crc32str_sse42 crc32str
#endif

noreturn void _ts_assert_fail(const char *cond, const char *func, const char *file, int line, bool use_log);

#undef assert
#undef static_assert

#define static_assert _Static_assert

#ifdef NDEBUG
    #define _assert(cond,uselog)
#else
    #define _assert(cond,uselog) ((cond) ? (void)0 : _ts_assert_fail(#cond, __func__, __FILE__, __LINE__, uselog))
#endif

#define assert(cond) _assert(cond, true)
#define assert_nolog(cond) _assert(cond, false)

#ifdef DEBUG
    typedef struct DebugInfo {
        const char *file;
        const char *func;
        unsigned int line;
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

#undef fopen
FILE* fopen() __attribute__((deprecated(
    "Use vfs_open or SDL_RWFromFile instead")));

#undef strncat
char* strncat() __attribute__((deprecated(
    "This function likely doesn't do what you expect, use strlcat")));

#undef strncpy
char* strncpy() __attribute__((deprecated(
    "This function likely doesn't do what you expect, use strlcpy")));

#undef errx
noreturn void errx(int, const char*, ...) __attribute__((deprecated(
    "Use log_fatal instead")));

#undef warnx
void warnx(const char*, ...) __attribute__((deprecated(
    "Use log_warn instead")));

#undef printf
int printf(const char*, ...) __attribute__((deprecated(
    "Use log_info instead")));

#undef fprintf
int fprintf(FILE*, const char*, ...) __attribute__((deprecated(
    "Use log_warn instead (or SDL_RWops if you want to write to a file)")));

#undef strtok
char* strtok() __attribute__((deprecated(
    "Use strtok_r instead")));

#undef sprintf
int sprintf(char *, const char*, ...) __attribute__((deprecated(
    "Use snprintf or strfmt instead")));

PRAGMA(GCC diagnostic pop)
