/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "transition.h"
#include "menu/ingamemenu.h"
#include "global.h"

Transition transition;

void TransFadeBlack(double fade) {
	fade_out(fade);
}

void TransFadeWhite(double fade) {
	colorfill(fade, fade, fade, fade);
}

void TransLoader(double fade) {
	r_color4(fade, fade, fade, fade);
	fill_screen("loading");
	r_color4(1, 1, 1, 1);
}

void TransEmpty(double fade) { }

static bool popq(void) {
	if(transition.queued.rule) {
		transition.rule2 = transition.rule;
		transition.rule = transition.queued.rule;

		if(transition.state == TRANS_IDLE || transition.rule2 == transition.rule) {
			transition.rule2 = NULL;
		}

		transition.dur1 = transition.queued.dur1;
		transition.dur2 = transition.queued.dur2;
		transition.cc = transition.queued.cc;
		transition.queued.rule = NULL;

		if(transition.dur1 <= 0) {
			transition.fade = 1.0;
			transition.state = TRANS_FADE_OUT;
		} else {
			transition.state = TRANS_FADE_IN;
		}

		return true;
	}

	return false;
}

static void setq(TransitionRule rule, int dur1, int dur2, CallChain cc) {
	transition.queued.rule = rule;
	transition.queued.dur1 = dur1;
	transition.queued.dur2 = dur2;
	transition.queued.cc = cc;
}

void set_transition(TransitionRule rule, int dur1, int dur2, CallChain cc) {
	static bool initialized = false;

	if(!rule) {
		return;
	}

	setq(rule, dur1, dur2, cc);

	if(!initialized) {
		popq();
		initialized = true;
		transition.rule2 = NULL;
	}

	if(transition.state == TRANS_IDLE || rule == transition.rule) {
		popq();
	}
}

static bool check_transition(void) {
	if(transition.state == TRANS_IDLE) {
		return false;
	}

	if(!transition.rule) {
		transition.state = TRANS_IDLE;
		return false;
	}

	return true;
}

void draw_transition(void) {
	if(!check_transition()) {
		return;
	}

	transition.rule(transition.fade);

	if(transition.rule2 && transition.rule2 != transition.rule) {
		transition.rule2(transition.fade);
	}
}

void update_transition(void) {
	if(!check_transition()) {
		return;
	}

	if(transition.state == TRANS_FADE_IN) {
		transition.fade = approach(transition.fade, 1.0, 1.0/transition.dur1);
		if(transition.fade == 1.0) {
			transition.state = TRANS_FADE_OUT;
			run_call_chain(&transition.cc, NULL);
			if(popq()) {
				run_call_chain(&transition.cc, NULL);
			}
		}
	} else if(transition.state == TRANS_FADE_OUT) {
		transition.fade = transition.dur2 ? approach(transition.fade, 0.0, 1.0/transition.dur2) : 0.0;
		if(transition.fade == 0.0) {
			transition.state = TRANS_IDLE;
			popq();
		}
	}
}
