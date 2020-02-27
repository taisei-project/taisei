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


void trainer_init(Trainer *trainer) {
	// sets the default values
	memset(trainer, 0, sizeof(*trainer));
}

bool trainer_enabled(void) {
	if (config_get_int(CONFIG_TRAINER_MODE)) {
		return true;
	}
	return false;
}

bool trainer_anything_enabled(void) {
	if (trainer_enabled() && (config_get_int(CONFIG_TRAINER_LIVES) || config_get_int(CONFIG_TRAINER_BOMBS) || config_get_int(CONFIG_TRAINER_INVULN))) {
		return true;
	}
	return false;

}

bool trainer_hud_stats_enabled(void) {
	// determine whether stats should be displayed on the HUD
	// used primarily by stagedraw.c
	if (trainer_enabled() && trainer_anything_enabled() && config_get_int(CONFIG_TRAINER_STATS)) {
			return true;
		}
	return false;
}

bool trainer_bombs_enabled(void) {
	if (config_get_int(CONFIG_TRAINER_MODE) && config_get_int(CONFIG_TRAINER_BOMBS)) {
		return true;
	}
	return false;
}

bool trainer_invulnerable_enabled(void) {
	if (config_get_int(CONFIG_TRAINER_MODE) && config_get_int(CONFIG_TRAINER_INVULN)) {
		return true;
	}
	return false;
}

bool trainer_lives_enabled(void) {
	if (config_get_int(CONFIG_TRAINER_MODE) && config_get_int(CONFIG_TRAINER_LIVES)) {
		return true;
	}
	return false;
}

bool trainer_no_powerdown_enabled(void) {
	if (config_get_int(CONFIG_TRAINER_MODE) && config_get_int(CONFIG_TRAINER_NO_PWRDN)) {
		return true;
	}
	return false;
}

void trainer_append_life_event(Trainer *trainer) {
	// count extra lives used
	trainer->stage.lives++;
	trainer->total.lives++;
	log_debug("Trainer Mode - Extra Lives Used: %d", trainer->total.lives);
	// TODO: capture more advanced stats here
}

void trainer_append_bomb_event(Trainer *trainer) {
	// count extra bombs used
	trainer->stage.bombs++;
	trainer->total.bombs++;
	log_debug("Trainer Mode - Extra Bombs Used: %d", trainer->total.bombs);
	// TODO: capture more advanced stats here
}

void trainer_append_hit_event(Trainer *trainer) {
	// count number of times hit with Invulnerability turned on
	trainer->stage.hits++;
	trainer->total.hits++;
	log_debug("Trainer Mode - Misses While Invulnerable: %d", trainer->total.hits);
	// TODO: capture more advanced stats here
}

void trainer_reset_stage_counters(Trainer *trainer) {
	// reset counters for stages
	trainer->stage.lives = 0;
	trainer->stage.bombs = 0;
	trainer->stage.hits = 0;
	log_debug("Trainer Mode - Stage Counters Reset");
}

