/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "transition.h"
#include "menu/ingamemenu.h"
#include "global.h"

Transition transition;

void TransFadeBlack(double fade) {
	fade_out(fade);
}

void TransFadeWhite(double fade) {
	colorfill(1, 1, 1, fade);
}

void TransLoader(double fade) {
	glColor4f(1, 1, 1, fade);
	draw_texture(SCREEN_W/2, SCREEN_H/2, "loading");
	glColor4f(1, 1, 1, 1);
}

void TransMenu(double fade) {
	glColor4f(1, 1, 1, fade);
	draw_texture(SCREEN_W/2, SCREEN_H/2, "mainmenu/mainmenubg");
	glColor4f(1, 1, 1, 1);
}

void TransMenuDark(double fade) {
	glColor4f(0.3, 0.3, 0.3, fade);
	draw_texture(SCREEN_W/2, SCREEN_H/2, "mainmenu/mainmenubg");
	glColor4f(1, 1, 1, 1);
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
		transition.callback = transition.queued.callback;
		transition.arg = transition.queued.arg;
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

static void setq(TransitionRule rule, int dur1, int dur2, TransitionCallback cb, void *arg) {
	transition.queued.rule = rule;
	transition.queued.dur1 = dur1;
	transition.queued.dur2 = dur2;
	transition.queued.callback = cb;
	transition.queued.arg = arg;
}

static void call_callback(void) {
	if(transition.callback) {
		transition.callback(transition.arg);
		transition.callback = NULL;
	}
}

void set_transition_callback(TransitionRule rule, int dur1, int dur2, TransitionCallback cb, void *arg) {
	static bool initialized = false;

	if(!rule) {
		return;
	}

	setq(rule, dur1, dur2, cb, arg);

	if(!initialized) {
		popq();
		initialized = true;
		transition.rule2 = NULL;
	}

	if(transition.state == TRANS_IDLE || rule == transition.rule) {
		popq();
	}
}

void set_transition(TransitionRule rule, int dur1, int dur2) {
	set_transition_callback(rule, dur1, dur2, NULL, NULL);
}

void draw_transition(void) {
	if(transition.state == TRANS_IDLE) {
		return;
	}

	if(!transition.rule) {
		transition.state = TRANS_IDLE;
		return;
	}

	transition.rule(transition.fade);

	if(transition.rule2 && transition.rule2 != transition.rule) {
		transition.rule2(transition.fade);
	}

	if(transition.state == TRANS_FADE_IN) {
		transition.fade = approach(transition.fade, 1.0, 1.0/transition.dur1);
		if(transition.fade == 1.0) {
			transition.state = TRANS_FADE_OUT;
			call_callback();
			if(popq()) {
				call_callback();
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

