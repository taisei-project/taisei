/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "aniplayer.h"
#include "list.h"
#include "global.h"

void aniplayer_create(AniPlayer *plr, Animation *ani) {
	memset(plr,0,sizeof(AniPlayer));
	plr->ani = ani;
}

AniPlayer* aniplayer_create_copy(AniPlayer *src) {
	// XXX: maybe it needs another name since it allocates memory?
	//		or maybe aniplayer_create needs another name since it doesn't?

	AniPlayer *plr = malloc(sizeof(AniPlayer));
	aniplayer_copy(plr, src);
	return plr;
}

void aniplayer_free(AniPlayer *plr) {
	plr->queuesize = 0; // prevent aniplayer_reset from messing with the queue, since we're going to wipe all of it anyway
	list_free_all((List**)&plr->queue);
	aniplayer_reset(plr);
}

void aniplayer_reset(AniPlayer *plr) { // resets to a neutral state with empty queue.
	plr->stdrow = 0;
	if(plr->queuesize > 0) { // abort the animation in the fastest fluent way.
		list_free_all((List**)&plr->queue->next);
		plr->queuesize = 1;
		plr->queue->delay = 0;
	}
}

void aniplayer_copy(AniPlayer *dst, AniPlayer *src) {
	memcpy(dst, src, sizeof(AniPlayer));
	dst->queue = NULL;
	dst->queuesize = 0;
}

AniSequence *aniplayer_queue(AniPlayer *plr, int row, int loops, int delay) {
	AniSequence *s = (AniSequence*)list_append((List**)&plr->queue, calloc(1, sizeof(AniSequence)));
	plr->queuesize++;
	s->row = row;

	if(loops < 0)
		log_fatal("Negative number of loops passed: %d",loops);

	s->duration = (loops+1)*plr->ani->cols*plr->ani->speed;
	s->delay = delay;
	s->mirrored = plr->mirrored;

	return s;
}

AniSequence *aniplayer_queue_pro(AniPlayer *plr, int row, int start, int end, int delay, int speed) {
	AniSequence *s = aniplayer_queue(plr,row,0,delay);
	// i bet you didn’t expect the _pro function calling the plain one
	s->speed = speed;

	if(speed <= 0)
		speed = plr->ani->speed;
	s->duration = end*speed;
	s->clock = start*speed;
	return s;
}

void aniplayer_update(AniPlayer *plr) {
	plr->clock++;
	if(plr->queue) {
		AniSequence *s = plr->queue;
		if(s->clock < s->duration) {
			s->clock++;
		} else if(s->delay > 0) {
			s->delay--;
		} else {
			free(list_pop((List**)&plr->queue));
			plr->queuesize--;
			plr->clock = 0;
		}
	}
}

void aniplayer_play(AniPlayer *plr, float x, float y) {
	int col = (plr->clock/plr->ani->speed) % plr->ani->cols;
	int row = plr->stdrow;
	int speed = plr->ani->speed;
	bool mirror = plr->mirrored;
	if(plr->queue) {
		AniSequence *s = plr->queue;
		if(s->speed > 0)
			speed = s->speed;
		col = (s->clock/speed) % plr->ani->cols;
		if(s->backwards)
			col = ((s->duration-s->clock)/speed) % plr->ani->cols;
		row = s->row;

		mirror = s->mirrored;
	}

	if(mirror) {
		glPushMatrix();
		glCullFace(GL_FRONT);
		glTranslatef(x,y,0);
		x = y = 0;
		glScalef(-1,1,1);
	}

	draw_animation_p(x,y,col,row,plr->ani);

	if(mirror) {
		glCullFace(GL_BACK);
		glPopMatrix();
	}
}

void play_animation(Animation *ani, float x, float y, int row) { // the old way to draw animations without AniPlayer
	draw_animation_p(x,y,(global.frames/ani->speed)%ani->cols,row,ani);
}
