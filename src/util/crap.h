/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_util_crap_h
#define IGUARD_util_crap_h

#include "taisei.h"

void* memdup(const void *src, size_t size);
void inherit_missing_pointers(uint num, void *dest[num], void *const base[num]);
bool is_main_thread(void);

#endif // IGUARD_util_crap_h
