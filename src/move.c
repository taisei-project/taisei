/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "move.h"
#include "util/miscmath.h"

cmplx move_update(cmplx *restrict pos, MoveParams *restrict p) {
	cmplx v = p->velocity;

	*pos += v;
	p->velocity = p->acceleration + p->retention * v;

	if(p->attraction) {
		cmplx av = p->attraction_point - *pos;

		p->velocity += p->attraction * cnormalize(av) * pow(cabs(av), p->attraction_exponent);
	}

	return v;
}

cmplx move_update_multiple(uint times, cmplx *restrict pos, MoveParams *restrict p) {
	cmplx v = p->velocity;

	while(times--) {
		move_update(pos, p);
	}

	return v;
}
