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

void aniplayer_free(AniPlayer *plr) {
	delete_all_elements((void **)&plr->queue,delete_element);
	aniplayer_reset(plr);
}

void aniplayer_reset(AniPlayer *plr) { // resets to a neutral state with empty queue.
	plr->stdrow = 0;
	if(plr->queuesize > 0) { // abort the animation in the fastest fluent way.
		delete_all_elements((void **)&plr->queue->next,delete_element);
		plr->queuesize = 1;
		plr->queue->delay = 0;
	}
}

AniSequence *aniplayer_queue(AniPlayer *plr, int row, int loops, int delay) {
	AniSequence *s = create_element_at_end((void **)plr->queue,sizeof(AniSequence));
	plr->queuesize++;
	s->row = row;

	if(loops < 0)
		log_fatal("Negative number of loops passed: %d",loops);

	s->clocks = (loops+1)*plr->ani->cols*plr->ani->speed;
	s->delay = delay;

	return s;
}

void aniplayer_update(AniPlayer *plr) {
	plr->clock++;
	if(plr->queue) {
		AniSequence *s = plr->queue;
		if(s->clocks > 0) {
			s->clocks--;
		} else if(s->delay > 0) {
			s->delay--;
		} else {
			delete_element((void **)&plr->queue,plr->queue);
			plr->queuesize--;
			plr->clock = 0;
		}
	}
}

void aniplayer_play(AniPlayer *plr, float x, float y) {
	int col = (plr->clock/plr->ani->speed) % plr->ani->cols;
	int row = plr->stdrow;
	bool mirror = plr->mirrored;
	if(plr->queue) {
		AniSequence *s = plr->queue;
		col = ((2*s->backwards-1)*s->clocks/plr->ani->speed) % plr->ani->cols;
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
