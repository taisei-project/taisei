/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"
#include "stagetext.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

void elly_paradigm_shift(Boss *b, int t) {
	if(global.stage->type == STAGE_SPELL) {
		GO_TO(b, BOSS_DEFAULT_GO_POS, 0.1)
	} else if(t == 0) {
		play_sfx("bossdeath");
	}

	TIMER(&t);

	AT(0) {
		Enemy *scythe = find_scythe();
		scythe->birthtime = global.frames;
		scythe->logic_rule = scythe_explode;
		elly_clap(b,150);
	}

	if(global.stage->type != STAGE_SPELL) {
		AT(80) {
			audio_bgm_stop(0.5);
		}
	}

	AT(100) {
		if(global.stage->type != STAGE_SPELL) {
			stage_unlock_bgm("stage6boss_phase1");
			stage_start_bgm("stage6boss_phase2");
			stagetext_add("Paradigm Shift!", VIEWPORT_W/2+I*(VIEWPORT_H/2+64), ALIGN_CENTER, get_font("big"), RGB(1, 1, 1), 0, 120, 10, 30);
		}

		elly_spawn_baryons(b->pos);
	}

	AT(120) {
		stage_shake_view(200);
	}
}
