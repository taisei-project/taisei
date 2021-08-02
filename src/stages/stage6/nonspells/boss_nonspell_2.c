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

void elly_frequency2(Boss *b, int t) {
	TIMER(&t);
/*
	AT(0) {
		Enemy *scythe = find_scythe();
		aniplayer_queue(&b->ani, "snipsnip", 0);
		scythe->birthtime = global.frames;
		scythe->logic_rule = scythe_infinity;
		scythe->args[0] = 4;
	}

	AT(EVENT_DEATH) {
		Enemy *scythe = find_scythe();
		scythe->birthtime = global.frames;
		scythe->logic_rule = scythe_reset;
		scythe->args[0] = 0;
	}

	FROM_TO_SND("shot1_loop",0, 2000, 3-global.diff/2) {
		cmplx n = sin(t*0.12*global.diff)*cexp(t*0.02*I*global.diff);
		PROJECTILE(
			.proto = pp_plainball,
			.pos = b->pos+80*n,
			.color = RGB(0,0,0.7),
			.rule = asymptotic,
			.args = { 2*n/cabs(n), 3 }
		);
	}*/
}
