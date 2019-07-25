/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "move.h"
#include "util/miscmath.h"

complex move_update(complex *restrict pos, MoveParams *restrict p) {
	complex v = p->velocity;

	*pos += v;
	p->velocity = p->acceleration + p->retention * v;

	if(p->attraction_norm || p->attraction) {
		complex av = p->attraction_point - *pos;
		p->velocity += p->attraction * av;
		p->velocity += p->attraction_norm * cnormalize(av);
	}

	return v;
}
