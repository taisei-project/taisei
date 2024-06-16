/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <stdio.h>

// XXX: this header trips some of these deprecation warnings; include it early as a workaround
#include <SDL_cpuinfo.h>

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
attr_deprecated("Use log_warn or log_error instead (or SDL_RWops if you want to write to a file)")
int fprintf(FILE*, const char*, ...);

#undef strtok
attr_deprecated("Use strtok_r instead")
char* strtok();

#undef sprintf
attr_deprecated("Use snprintf or strfmt instead")
int sprintf(char *, const char*, ...);

#undef getenv
attr_deprecated("Use env_get instead")
char* getenv();

#undef setenv
attr_deprecated("Use env_set instead")
int setenv();

#undef rand
attr_deprecated("Use tsrand instead")
int rand(void);

#undef srand
attr_deprecated("Use tsrand_seed instead")
void srand(uint);

INLINE void *libc_malloc(size_t size) { return malloc(size); }
#undef malloc
attr_deprecated("Use the memory.h API instead")
void *malloc(size_t size);

INLINE void libc_free(void *ptr) { free(ptr); }
#undef free
attr_deprecated("Use the memory.h API instead, or libc_free if this is a foreign allocation")
void free(void *ptr);

INLINE void *libc_calloc(size_t nmemb, size_t size) { return calloc(nmemb, size); }
#undef calloc
attr_deprecated("Use the memory.h API instead")
void *calloc(size_t nmemb, size_t size);

INLINE void *libc_realloc(void *ptr, size_t size) { return realloc(ptr, size); }
#undef realloc
attr_deprecated("Use the memory.h API instead")
void *realloc(void *ptr, size_t size);

PRAGMA(GCC diagnostic pop)
