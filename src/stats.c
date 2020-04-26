/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "config.h"

#include "stats.h"

#include "global.h"
#include "stage.h"

static bool stats_are_enabled(void) {
	log_debug("statistics: %d", config_get_int(CONFIG_STATS_ENABLED));
	return config_get_int(CONFIG_STATS_ENABLED);
}

void stats_init(Stats *stats) {
	memset(stats, 0, sizeof(*stats));
	if (stats_are_enabled()) {
		log_debug("statisics enabled for player");
		stats->settings.enabled = 1;
		// turn on stats in replays
		global.replay.flags |= REPLAY_GFLAG_STATS;
	}
}


void stats_append_life(Stats *stats) {
	stats->total.lives++;
	stats->stage.lives++;
	log_debug("lives used (total): %d", stats->total.lives);
	log_debug("lives used (stage): %d", stats->stage.lives);
}

void stats_append_bomb(Stats *stats) {
	stats->total.bombs++;
	stats->stage.bombs++;
	log_debug("bombs used (total): %d", stats->total.bombs);
	log_debug("bombs used (stage): %d", stats->stage.bombs);
}

void stats_append_continue(Stats *stats) {
	stats->total.continues++;
	stats->stage.continues++;
	log_debug("bombs used (total): %d", stats->total.continues);
	log_debug("bombs used (stage): %d", stats->stage.continues);
}

void stats_stage_reset(Stats *stats) {
	stats->stage.lives = 0;
	stats->stage.bombs = 0;
	stats->stage.continues = 0;
	log_debug("statistics reset");
}
