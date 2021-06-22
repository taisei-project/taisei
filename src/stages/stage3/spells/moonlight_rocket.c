/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"
#include "../wriggle.h"

#include "common_tasks.h"
#include "global.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

void wriggle_moonlight_rocket(Boss *boss, int time) {
	int i, j, cnt = 1 + global.diff;
	TIMER(&time)

	AT(EVENT_DEATH) {
		enemy_kill_all(&global.enemies);
		return;
	}

	if(time < 0)
		GO_TO(boss, VIEWPORT_W/2 + VIEWPORT_H*I/2.5, 0.05)
	else if(time == 0) {
		for(j = -1; j < 2; j += 2) for(i = 0; i < cnt; ++i)
			create_enemy3c(boss->pos, ENEMY_IMMUNE, wriggle_slave_visual, wriggle_spell_slave, add_ref(boss), i*2*M_PI/cnt, j);
	}
}
