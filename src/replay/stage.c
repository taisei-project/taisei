/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "stage.h"
#include "struct.h"

#include "plrmodes.h"

ReplayStage *replay_stage_new(Replay *rpy, StageInfo *stage, uint64_t start_time, uint64_t seed, Difficulty diff, Player *plr) {
	ReplayStage *s = dynarray_append(&rpy->stages, {});

	get_system_time(&s->init_time);
	dynarray_ensure_capacity(&s->events, REPLAY_ALLOC_INITIAL);

	s->stage = stage->id;
	s->start_time = start_time;
	s->rng_seed = seed;
	s->diff = diff;

	s->plr_pos_x = floor(re(plr->pos));
	s->plr_pos_y = floor(im(plr->pos));

	s->plr_points = plr->points;
	s->plr_total_lives_used = plr->stats.total.lives_used;
	s->plr_total_bombs_used = plr->stats.total.bombs_used;
	s->plr_total_continues_used = plr->stats.total.continues_used;
	s->plr_char = plr->mode->character->id;
	s->plr_shot = plr->mode->shot_mode;
	s->plr_lives = plr->lives;
	s->plr_life_fragments = plr->life_fragments;
	s->plr_bombs = plr->bombs;
	s->plr_bomb_fragments = plr->bomb_fragments;
	s->plr_power = plr->power_stored;
	s->plr_graze = plr->graze;
	s->plr_point_item_value = plr->point_item_value;
	s->plr_inputflags = plr->inputflags;

	log_debug("Created a new stage %p in replay %p", (void*)s, (void*)rpy);
	return s;
}

void replay_stage_sync_player_state(ReplayStage *stg, Player *plr) {
	plr->points = stg->plr_points;
	plr->stats.total.lives_used = stg->plr_total_lives_used;
	plr->stats.total.bombs_used = stg->plr_total_bombs_used;
	plr->stats.total.continues_used = stg->plr_total_continues_used;
	plr->mode = plrmode_find(stg->plr_char, stg->plr_shot);
	plr->pos = stg->plr_pos_x + I * stg->plr_pos_y;
	plr->lives = stg->plr_lives;
	plr->life_fragments = stg->plr_life_fragments;
	plr->bombs = stg->plr_bombs;
	plr->bomb_fragments = stg->plr_bomb_fragments;
	plr->power_stored = stg->plr_power;
	plr->graze = stg->plr_graze;
	plr->point_item_value = stg->plr_point_item_value;
	plr->inputflags = stg->plr_inputflags;
}

void replay_stage_update_final_stats(ReplayStage *stg, const Stats *stats) {
	stg->plr_stage_lives_used_final = stats->stage.lives_used;
	stg->plr_stage_bombs_used_final = stats->stage.bombs_used;
	stg->plr_stage_continues_used_final = stats->stage.continues_used;
}

void replay_stage_event(ReplayStage *stg, uint32_t frame, uint8_t type, uint16_t value) {
	dynarray_size_t old_capacity = stg->events.capacity;

	dynarray_append(&stg->events, {
		.frame = frame,
		.type = type,
		.value = value,
	});

	if(stg->events.capacity > old_capacity && stg->events.capacity > UINT16_MAX) {
		log_error("Too many events in replay; saving WILL FAIL!");
	}

	if(type == EV_OVER) {
		log_debug("The replay is OVER");
	}
}

void replay_stage_destroy_events(ReplayStage *stg) {
	dynarray_free_data(&stg->events);
}
