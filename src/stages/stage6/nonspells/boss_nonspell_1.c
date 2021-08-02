/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"
#include "../elly.h"

DEFINE_EXTERN_TASK(stage6_boss_nonspell_1) {
	Boss *boss = stage6_elly_init_scythe_attack(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS.base);

	INVOKE_SUBTASK(stage6_elly_scythe_nonspell, ARGS.scythe);

	STALL;
}
