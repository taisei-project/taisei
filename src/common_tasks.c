/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "common_tasks.h"

DEFINE_EXTERN_TASK(common_drop_items) {
	complex p = *ARGS.pos;

	for(int i = 0; i < ITEM_LAST - ITEM_FIRST; ++i) {
		for(int j = ARGS.items.as_array[i]; j; --j) {
			spawn_item(p, i + ITEM_FIRST);
			WAIT(2);
		}
	}
}

void common_move_loop(complex *restrict pos, MoveParams *restrict mp) {
	for(;;) {
		move_update(pos, mp);
		YIELD;
	}
}

DEFINE_EXTERN_TASK(common_move) {
	if(ARGS.ent.ent) {
		TASK_BIND(ARGS.ent);
	}

	MoveParams p = ARGS.move_params;
	common_move_loop(ARGS.pos, &p);
}

DEFINE_EXTERN_TASK(common_move_ext) {
	if(ARGS.ent.ent) {
		TASK_BIND(ARGS.ent);
	}

	common_move_loop(ARGS.pos, ARGS.move_params);
}
