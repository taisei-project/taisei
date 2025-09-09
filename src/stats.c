/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "stats.h"

#include "log.h"

void stats_init(Stats *stats) {
	*stats = (typeof(*stats)) {};
}

void stats_track_life_used(Stats *stats) {
	stats->total.lives_used++;
	stats->stage.lives_used++;
	log_debug("lives used (total): %d", stats->total.lives_used);
	log_debug("lives used (stage): %d", stats->stage.lives_used);
}

void stats_track_bomb_used(Stats *stats) {
	stats->total.bombs_used++;
	stats->stage.bombs_used++;
	log_debug("bombs used (total): %d", stats->total.bombs_used);
	log_debug("bombs used (stage): %d", stats->stage.bombs_used);
}

void stats_track_continue_used(Stats *stats) {
	stats->total.continues_used++;
	stats->stage.continues_used++;
	log_debug("bombs used (total): %d", stats->total.continues_used);
	log_debug("bombs used (stage): %d", stats->stage.continues_used);
}

void stats_stage_reset(Stats *stats) {
	stats->stage = (typeof(stats->stage)) {};
	log_debug("statistics reset");
}
