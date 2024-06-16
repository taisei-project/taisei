/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "move.h"
#include "util/miscmath.h"

cmplx move_update(cmplx *restrict pos, MoveParams *restrict p) {
	MoveParams o = *p;
	cmplx orig_velocity = o.velocity;
	*pos += orig_velocity;
	o.velocity = o.acceleration + cmul_finite(o.retention, o.velocity);

	if(o.attraction) {
		cmplx av = o.attraction_point - *pos;

		if(LIKELY(o.attraction_exponent == 1)) {
			o.velocity += cmul_finite(o.attraction, av);
		} else {
			real m = cabs2(av);
			assume(m >= 0);
			m = pow(m, o.attraction_exponent - 0.5);
			assert(isfinite(m));
			o.velocity += cmul_finite(o.attraction, av * m);
		}
	}

	p->velocity = o.velocity;
	return orig_velocity;
}

cmplx move_update_multiple(uint times, cmplx *restrict pos, MoveParams *restrict p) {
	cmplx v = p->velocity;

	while(times--) {
		move_update(pos, p);
	}

	return v;
}
