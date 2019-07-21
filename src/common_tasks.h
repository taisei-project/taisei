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

DECLARE_EXTERN_TASK(
	common_drop_items,
	{ complex *pos; ItemCounts items; }
);

#endif // IGUARD_common_tasks_h
