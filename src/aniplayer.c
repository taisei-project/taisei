/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "aniplayer.h"

#include "list.h"
#include "stageobjects.h"

void aniplayer_create(AniPlayer *plr, Animation *ani, const char *startsequence) {
	memset(plr,0,sizeof(AniPlayer));
	plr->ani = ani;

	aniplayer_queue(plr,startsequence,0);
}

Sprite *aniplayer_get_frame(AniPlayer *plr) {
	assert(plr->queue.first != NULL);
	return animation_get_frame(plr->ani, plr->queue.first->sequence, plr->queue.first->clock);
}

void aniplayer_free(AniPlayer *plr) {
	plr->queuesize = 0;
	alist_free_all(&plr->queue);
}

// Deletes the queue. If hard is set, even the last element is removed leaving the player in an invalid state.
static void aniplayer_reset(AniPlayer *plr, bool hard) {
	if(plr->queuesize == 0)
		return;
	if(hard) {
		alist_free_all(&plr->queue);
		plr->queuesize = 0;
		return;
	}

	list_free_all(&plr->queue.first->next);
	plr->queue.last = plr->queue.first;
	plr->queuesize = 1;
}

AniQueueEntry *aniplayer_queue(AniPlayer *plr, const char *seqname, int loops) {
	auto s = ALLOC(AniQueueEntry);
	alist_append(&plr->queue, s);
	plr->queuesize++;

	if(loops < 0)
		log_fatal("Negative number of loops passed: %d",loops);
	s->sequence = get_ani_sequence(plr->ani,seqname);

	s->duration = loops*s->sequence->length;

	return s;
}

AniQueueEntry *aniplayer_queue_frames(AniPlayer *plr, const char *seqname, int frames) {
	AniQueueEntry *s = aniplayer_queue(plr, seqname, 0);
	s->duration = frames;
	return s;
}

AniQueueEntry *aniplayer_soft_switch(AniPlayer *plr, const char *seqname, int loops) {
	aniplayer_reset(plr,false);
	return aniplayer_queue(plr,seqname,loops);
}

AniQueueEntry *aniplayer_hard_switch(AniPlayer *plr, const char *seqname, int loops) {
	aniplayer_reset(plr,true);
	return aniplayer_queue(plr,seqname,loops);
}

void aniplayer_update(AniPlayer *plr) {
	assert(plr->queue.first != NULL); // The queue should never be empty.
	AniQueueEntry *s = plr->queue.first;

	s->clock++;
	// The last condition assures that animations only switch at their end points
	if(s->clock >= s->duration && plr->queuesize > 1 && s->clock%s->sequence->length == 0) {
		mem_free(alist_pop(&plr->queue));
		plr->queuesize--;
	}
}

