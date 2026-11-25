/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei_app.h"

#include "eventloop/eventloop.h"
#include "log.h"
#include "util/env.h"

#include <SDL3/SDL.h>

static struct {
	SDL_AtomicInt quitting;
	bool kiosk_mode;
	bool transient;
} app;

void taisei_init(CLIAction *cli) {
	app.kiosk_mode = env_get("TAISEI_KIOSK", false);

	if(app.kiosk_mode) {
		SDL_SetHintWithPriority(SDL_HINT_NO_SIGNAL_HANDLERS, 0, SDL_HINT_DEFAULT);
	}

	if(cli->type == CLI_VerifyReplay) {
		eventloop_set_headless(true);
		app.transient = true;
	} else if(cli->frameskip) {
		log_warn("FPS limiter disabled. Gotta go fast! (frameskip = %i)", cli->frameskip);
		eventloop_set_frameskip(cli->frameskip);
	}
}

bool taisei_is_transient(void) {
	return app.transient;
}

bool taisei_is_kiosk_mode(void) {
	return app.kiosk_mode;
}

static bool taisei_is_quit_forbidden(void) {
	return app.kiosk_mode && env_get("TAISEI_KIOSK_PREVENT_QUIT", true);
}

bool taisei_is_quit_hidden(void) {
#ifdef __EMSCRIPTEN__
	return true;
#else
	return taisei_is_quit_forbidden();
#endif
}

void taisei_quit(void) {
	if(taisei_is_quit_forbidden()) {
		log_info("Running in kiosk mode; exit request ignored");
		return;
	}

	if(SDL_CompareAndSwapAtomicInt(&app.quitting, 0, 1)) {
		log_info("Exit requested");
	}
}

bool taisei_quit_requested(void) {
	return SDL_GetAtomicInt(&app.quitting);
}
