/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

INLINE size_t mem_util_calc_array_size(size_t num_members, size_t size) {
	size_t array_size;

	if(__builtin_mul_overflow(num_members, size, &array_size)) {
		assert(0 && "size_t overflow");
		abort();
	}

	return array_size;
}
