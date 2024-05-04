/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "events.h"
#include "log.h"
#include "watchdog.h"

static struct {
	int countdown;
	int timeout;
} watchdog;

static inline bool watchdog_initialized(void) {
	return watchdog.timeout;
}

static void watchdog_tick(void) {
	assert(watchdog_initialized());

	if(watchdog.countdown > 0) {
		if(--watchdog.countdown == 0) {
			log_info("Watchdog signaled");
			events_emit(TE_WATCHDOG, 0, NULL, NULL);
		}
	}
}

static bool watchdog_event(SDL_Event *event, void *arg) {
	assert(watchdog_initialized());

	switch(event->type) {
		case SDL_KEYDOWN:
			watchdog_reset();
			return false;

		default: switch(TAISEI_EVENT(event->type)) {
			case TE_GAMEPAD_BUTTON_DOWN:
			case TE_GAMEPAD_AXIS_DIGITAL:
				watchdog_reset();
				return false;

			case TE_FRAME:
				watchdog_tick();
				return false;
		}
	}

	return false;
}

void watchdog_init(int timeout) {
	if(timeout <= 0) {
		return;
	}

	assert(!watchdog_initialized());
	events_register_handler(&(EventHandler) {
		.priority = EPRIO_SYSTEM,
		.proc = watchdog_event,
	});
	watchdog.timeout = timeout;
	watchdog_reset();
}

void watchdog_shutdown(void) {
	if(watchdog_initialized()) {
		events_unregister_handler(watchdog_event);
		watchdog.timeout = 0;
	}
}

bool watchdog_signaled(void) {
	return watchdog_initialized() && watchdog.countdown == 0;
}

void watchdog_reset(void) {
	if(watchdog_initialized()) {
		watchdog.countdown = watchdog.timeout;
	}
}

void watchdog_suspend(void) {
	if(watchdog_initialized()) {
		watchdog.countdown = -1;
	}
}
