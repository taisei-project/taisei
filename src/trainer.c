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

static bool trainer_bombs_enabled(void) {
	return (config_get_int(CONFIG_TRAINER_MODE) && config_get_int(CONFIG_TRAINER_BOMBS));
}

static bool trainer_invulnerable_enabled(void) {
	return (config_get_int(CONFIG_TRAINER_MODE) && config_get_int(CONFIG_TRAINER_INVULN));
}

static bool trainer_lives_enabled(void) {
	return (config_get_int(CONFIG_TRAINER_MODE) && config_get_int(CONFIG_TRAINER_LIVES));
}

static bool trainer_no_powerdown_enabled(void) {
	return (config_get_int(CONFIG_TRAINER_MODE) && config_get_int(CONFIG_TRAINER_NO_PWRDN));
}

static bool trainer_focus_dot_enabled(void) {
	return (config_get_int(CONFIG_TRAINER_MODE) && config_get_int(CONFIG_TRAINER_FOCUS_DOT));
}

static bool trainer_enabled(void) {
	return config_get_int(CONFIG_TRAINER_MODE);
}

bool trainer_check_config(void) {
	return (
		trainer_enabled()
		&& (
			config_get_int(CONFIG_TRAINER_LIVES) ||
			config_get_int(CONFIG_TRAINER_BOMBS) ||
			config_get_int(CONFIG_TRAINER_INVULN) ||
			config_get_int(CONFIG_TRAINER_FOCUS_DOT)
		   )
	   );
}

static bool trainer_hud_stats_enabled(void) {
	// determine whether stats should be displayed on the HUD
	// used primarily by stagedraw.c
	return (trainer_check_config() && config_get_int(CONFIG_TRAINER_STATS));
}

bool trainer_anything_enabled(Trainer *trainer) {
	return (
		trainer->settings.enabled
		&& (
			trainer->settings.dot ||
			trainer->settings.invulnerable ||
			trainer->settings.extra_lives ||
			trainer->settings.extra_bombs ||
			trainer->settings.no_powerdown
		   )
	   );
}

void trainer_init(Trainer *trainer) {
	// sets the default values
	memset(trainer, 0, sizeof(*trainer));

	if (trainer_check_config()) {
		trainer->settings.enabled = 1;
		trainer->settings.stats = trainer_hud_stats_enabled();

		trainer->settings.invulnerable = trainer_invulnerable_enabled();
		trainer->settings.extra_lives = trainer_lives_enabled();
		trainer->settings.extra_bombs = trainer_bombs_enabled();
		trainer->settings.no_powerdown = trainer_no_powerdown_enabled();
		trainer->settings.dot = trainer_focus_dot_enabled();
	}
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
