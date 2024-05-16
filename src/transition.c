/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "transition.h"
#include "menu/ingamemenu.h"
#include "global.h"
#include "util/graphics.h"

Transition transition;

static CallChain swap_cc(CallChain *pcc, CallChain newcc) {
	CallChain cc = *pcc;
	*pcc = newcc;
	return cc;
}

static CallChain pop_cc(CallChain *pcc) {
	return swap_cc(pcc, NO_CALLCHAIN);
}

static void run_cc(CallChain *pcc, bool canceled) {
	CallChain cc = pop_cc(pcc);
	run_call_chain(&cc, (void*)(uintptr_t)canceled);
}

static void swap_cc_withcancel(CallChain *pcc, CallChain newcc) {
	CallChain oldcc = swap_cc(pcc, newcc);
	run_cc(&oldcc, true);
}

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
		swap_cc_withcancel(&transition.cc, transition.queued.cc);
		transition.queued.cc = NO_CALLCHAIN;
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
	swap_cc_withcancel(&transition.queued.cc, cc);
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

	// FIXME can we just do this unconditionally?
	if(
		transition.state == TRANS_IDLE ||
		transition.state == TRANS_FADE_OUT ||
		rule == transition.rule
	) {
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
			popq();
			run_cc(&transition.cc, false);
		}
	} else if(transition.state == TRANS_FADE_OUT) {
		transition.fade = transition.dur2 ? approach(transition.fade, 0.0, 1.0/transition.dur2) : 0.0;
		if(transition.fade == 0.0) {
			transition.state = TRANS_IDLE;
			popq();
		}
	}
}
