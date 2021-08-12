/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <stdio.h>

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

PRAGMA(GCC diagnostic pop)
