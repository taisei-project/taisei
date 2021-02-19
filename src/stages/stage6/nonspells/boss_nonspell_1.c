/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

void elly_frequency(Boss *b, int t) {
	TIMER(&t);
	Enemy *scythe;

	AT(EVENT_BIRTH) {
		scythe = find_scythe();
		aniplayer_queue(&b->ani, "snipsnip", 0);
		scythe->birthtime = global.frames;
		scythe->logic_rule = scythe_infinity;
		scythe->args[0] = 2;
	}

	AT(EVENT_DEATH) {
		scythe = find_scythe();
		scythe->birthtime = global.frames;
		scythe->logic_rule = scythe_reset;
		scythe->args[0] = 0;
	}
}
