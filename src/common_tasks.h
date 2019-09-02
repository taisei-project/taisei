/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_common_tasks_h
#define IGUARD_common_tasks_h

#include "taisei.h"

#include "coroutine.h"
#include "item.h"
#include "move.h"
#include "entity.h"

DECLARE_EXTERN_TASK(
	common_drop_items,
	{ complex *pos; ItemCounts items; }
);

DECLARE_EXTERN_TASK(
	common_move,
	{ complex *pos; MoveParams move_params; BoxedEntity ent; }
);

DECLARE_EXTERN_TASK(
	common_move_ext,
	{ complex *pos; MoveParams *move_params; BoxedEntity ent; }
);

void common_move_loop(complex *restrict pos, MoveParams *restrict mp);

INLINE Rect viewport_bounds(double margin) {
	return (Rect) {
		.top_left = CMPLX(margin, margin),
		.bottom_right = CMPLX(VIEWPORT_W - margin, VIEWPORT_H - margin),
	};
}

complex common_wander(complex origin, double dist, Rect bounds);

#endif // IGUARD_common_tasks_h
