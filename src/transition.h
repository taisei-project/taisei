/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef TRANSITION_H
#define TRANSITION_H

#include <stdbool.h>

typedef struct Transition Transition;
typedef void (*TransitionRule)(Transition *t);
typedef void (*TransitionCallback)(void *a);

struct Transition {
    double fade;
	int dur1; // first half
	int dur2; // second half
    TransitionCallback callback;
    void *arg;

    enum {
        TRANS_IDLE,
        TRANS_FADE_IN,
        TRANS_FADE_OUT,
    } state;

	TransitionRule rule;

    struct {
        int dur1;
        int dur2;
        TransitionRule rule;
        TransitionCallback callback;
        void *arg;
    } queued;
};

extern Transition transition;

void TransFadeBlack(Transition *t);
void TransFadeWhite(Transition *t);
void TransLoader(Transition *t);
void TransMenu(Transition *t);
void TransMenuDark(Transition *t);
void TransEmpty(Transition *t);

void set_transition(TransitionRule rule, int dur1, int dur2);
void set_transition_callback(TransitionRule rule, int dur1, int dur2, TransitionCallback cb, void *arg);
void draw_transition(void);

#endif
