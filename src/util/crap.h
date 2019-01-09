/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

void* memdup(const void *src, size_t size);
void inherit_missing_pointers(uint num, void *dest[num], void *const base[num]);
bool is_main_thread(void);
