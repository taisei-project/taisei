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
