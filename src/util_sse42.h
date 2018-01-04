/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <stdint.h>
#include "compat.h"

#ifdef HAVE_INTEL_INTRIN
    uint32_t crc32str_sse42(uint32_t crc, const char *str) __attribute__((hot, pure));
#else
    #define crc32str_sse42 crc32str
#endif
