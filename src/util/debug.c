/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "debug.h"

#ifndef DEBUG
#error Your build system is broken
#endif

bool _in_draw_code;

// TODO: make this thread-safe (use a TLS)
static DebugInfo debug_info;
static DebugInfo debug_meta;

void _set_debug_info(DebugInfo *info, DebugInfo *meta) {
	// assume the char*s point to literals
	memcpy(&debug_info, info, sizeof(DebugInfo));
	memcpy(&debug_meta, meta, sizeof(DebugInfo));
}

DebugInfo* get_debug_info(void) {
	return &debug_info;
}

DebugInfo* get_debug_meta(void) {
	return &debug_meta;
}
