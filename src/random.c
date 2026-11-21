/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "random.h"

static RandomState *rng_active_state;

void rng_make_active(RandomState *rng) {
	rng_active_state = rng;
}

rng_val_t rng_next(void) {
	return rng_next_p(rng_active_state);
}

void rng_nextn(size_t n, rng_val_t v[n]) {
	for(size_t i = 0; i < n; ++i) {
		v[i] = rng_next();
	}
}
