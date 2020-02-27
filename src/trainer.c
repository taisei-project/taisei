/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "trainer.h"

#include "global.h"
#include "stage.h"


void trainer_init(Trainer *tnr) {
	memset(tnr, 0, sizeof(Trainer));
	// store two sets of values
    // one is "per stage"
    // the other is for the entire game session
    tnr->trainer_lives_stage = 0;
    tnr->trainer_bombs_stage = 0;
    tnr->trainer_hits_stage = 0;

    tnr->trainer_lives_total = 0;
    tnr->trainer_bombs_total = 0;
    tnr->trainer_hits_total = 0;
}


bool trainer_enabled(void) {
    if (config_get_int(CONFIG_TRAINER_MODE) && config_get_int(CONFIG_TRAINER_STATS)) {
        if (config_get_int(CONFIG_TRAINER_LIVES) || config_get_int(CONFIG_TRAINER_BOMBS) || config_get_int(CONFIG_TRAINER_INVULN)) {
            return 1;
        }
    }
    return 0;
}

bool trainer_bombs_enabled(void) {
    if (config_get_int(CONFIG_TRAINER_MODE) && config_get_int(CONFIG_TRAINER_BOMBS)) {
        return 1;
    }
    return 0;
}

bool trainer_invulnerable_enabled(void) {
    if (config_get_int(CONFIG_TRAINER_MODE) && config_get_int(CONFIG_TRAINER_INVULN)) {
        return 1;
    }
    return 0;
}

bool trainer_lives_enabled(void) {
    if (config_get_int(CONFIG_TRAINER_MODE) && config_get_int(CONFIG_TRAINER_LIVES)) {
        return 1;
    }
    return 0;
}

bool trainer_no_powerdown_enabled(void) {
    if (config_get_int(CONFIG_TRAINER_MODE) && config_get_int(CONFIG_TRAINER_NO_PWRDN)) {
        return 1;
    }
    return 0;
}

void trainer_append_life_event(Trainer *tnr) {
	// count extra lives used
	tnr->trainer_lives_stage++;
	tnr->trainer_lives_total++;
	log_debug("Trainer Mode - Extra Lives Used: %d", tnr->trainer_lives_total);
	// TODO: capture more advanced stats here

}

void trainer_append_bomb_event(Trainer *tnr) {
	// count extra bombs used
	tnr->trainer_bombs_stage++;
	tnr->trainer_bombs_total++;
	log_debug("Trainer Mode - Extra Bombs Used: %d", tnr->trainer_bombs_total);
	// TODO: capture more advanced stats here
}

void trainer_append_hit_event(Trainer *tnr) {
	// count number of times hit with Invulnerability turned on
	tnr->trainer_hits_stage++;
	tnr->trainer_hits_total++;
	log_debug("Trainer Mode - Misses While Invulnerable: %d", tnr->trainer_hits_total);
	// TODO: capture more advanced stats here
}
