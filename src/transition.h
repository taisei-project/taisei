/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef TRANSITION_H
#define TRANSITION_H

typedef struct Transition Transition;
typedef void (*TransitionRule)(Transition *t);

struct Transition {
	int frames;
	int dur1; // first half
	int dur2; // second half
	
	TransitionRule rule;
};

void TransFadeBlack(Transition *t);
void TransFadeWhite(Transition *t);

void set_transition(TransitionRule rule, int dur1, int dur2);
void draw_transition(void);

#endif