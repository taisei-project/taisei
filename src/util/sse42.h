/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_util_sse42_h
#define IGUARD_util_sse42_h

#include "taisei.h"

#ifdef TAISEI_BUILDCONF_USE_SSE42
	uint32_t crc32str_sse42(uint32_t crc, const char *str) attr_hot attr_pure;
#else
	#define crc32str_sse42 crc32str
#endif

#endif // IGUARD_util_sse42_h
