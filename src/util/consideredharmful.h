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
#include <SDL3/SDL_cpuinfo.h>

#define HARMFUL_FUNC(func, msg) \
    __typeof__(func) (func) __attribute__((deprecated(msg)))

//
// safeguards against some dangerous or otherwise undesirable practices
//

PRAGMA(GCC diagnostic push)

#undef fopen
HARMFUL_FUNC(fopen, "Use vfs_open or SDL_IOFromFile instead");

#undef strncat
HARMFUL_FUNC(strncat, "This function likely doesn't do what you expect, use strlcat");

#undef strncpy
HARMFUL_FUNC(strncpy, "This function likely doesn't do what you expect, use strlcpy");

#undef printf
HARMFUL_FUNC(printf, "Use log_info instead");

#undef fprintf
HARMFUL_FUNC(fprintf, "Use log_warn or log_error instead (or SDL_IOStream if you want to write to a file)");

#undef strtok
HARMFUL_FUNC(strtok, "Use strtok_r instead");

#undef sprintf
HARMFUL_FUNC(sprintf, "Use snprintf or strfmt instead");

#ifdef TAISEI_BUILDCONF_HAVE_POSIX
#undef getenv
HARMFUL_FUNC(getenv, "Use env_get instead");

#undef setenv
HARMFUL_FUNC(setenv, "Use env_set instead");
#endif

#undef rand
HARMFUL_FUNC(rand, "Use the random.h API instead");

#undef srand
HARMFUL_FUNC(srand, "Use the random.h API instead");

// These wrappers are to be used in memory/backend_libc.c, or if using the libc allocator is specifically required.
INLINE void *libc_malloc(size_t size) { return malloc(size); }
INLINE void libc_free(void *ptr) { free(ptr); }
INLINE void *libc_calloc(size_t nmemb, size_t size) { return calloc(nmemb, size); }
INLINE void *libc_realloc(void *ptr, size_t size) { return realloc(ptr, size); }

#undef malloc
HARMFUL_FUNC(malloc, "Use the memory.h API instead");

#undef free
HARMFUL_FUNC(free, "Use the memory.h API instead, or libc_free if this is a foreign allocation");

#undef calloc
HARMFUL_FUNC(calloc, "Use the memory.h API instead");

#undef realloc
HARMFUL_FUNC(realloc, "Use the memory.h API instead");

#undef strdup
HARMFUL_FUNC(strdup, "Use mem_strdup from memory.h instead");

#undef HARMFUL_FUNC

PRAGMA(GCC diagnostic pop)
