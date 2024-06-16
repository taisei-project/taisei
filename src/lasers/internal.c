/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "internal.h"

LaserInternalData lintern;

void laserintern_init(void) {
	assert(lintern.segments.num_elements == 0);
	dynarray_ensure_capacity(&lintern.segments, 2048);
}

void laserintern_shutdown(void) {
	dynarray_free_data(&lintern.segments);
}
