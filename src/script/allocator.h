/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_script_allocator_h
#define IGUARD_script_allocator_h

#include "taisei.h"

void lalloc_init(void);
void lalloc_shutdown(void);
void *lalloc(void *restrict userdata, void *restrict ptr, size_t oldsize, size_t newsize);
void lmemstatstr(char *buffer, size_t bufsize);

#endif // IGUARD_script_allocator_h
