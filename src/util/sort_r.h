/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

void sort_r_simple(
	void *base, size_t nel, size_t w,
	int (*compar)(const void *_a, const void *_b, void *_arg),
	void *arg
);

void sort_r(
	void *base, size_t nel, size_t width,
	int (*compar)(const void *_a, const void *_b, void *_arg),
	void *arg
);
