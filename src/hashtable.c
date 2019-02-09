/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "hashtable.h"
#include "util.h"

#define HT_IMPL
#include "hashtable_predefs.inc.h"

uint32_t (*htutil_hashfunc_string)(uint32_t crc, const char *str);

void htutil_init(void) {
	if(SDL_HasSSE42()) {
		log_info("Using SSE4.2-accelerated CRC32 as the string hash function");
		htutil_hashfunc_string = crc32str_sse42;
	} else {
		log_info("Using software fallback CRC32 as the string hash function");
		htutil_hashfunc_string = crc32str;
	}
}
