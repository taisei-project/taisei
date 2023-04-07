/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "util/callchain.h"

typedef struct Transition Transition;
typedef void (*TransitionRule)(double fade);

#define TRANSITION_RESULT_CANCELED(ccr) ((bool)(uintptr_t)ccr.result)

struct Transition {
	double fade;
	int dur1; // first half
	int dur2; // second half
	CallChain cc;

	enum {
		TRANS_IDLE,
		TRANS_FADE_IN,
		TRANS_FADE_OUT,
	} state;

	TransitionRule rule;
	TransitionRule rule2;

	struct {
		int dur1;
		int dur2;
		TransitionRule rule;
		CallChain cc;
	} queued;
};

extern Transition transition;

void TransFadeBlack(double fade);
void TransFadeWhite(double fade);
void TransLoader(double fade);
void TransEmpty(double fade);

void set_transition(TransitionRule rule, int dur1, int dur2, CallChain cc);
void draw_transition(void);
void update_transition(void);
