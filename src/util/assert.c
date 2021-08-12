/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "assert.h"
#include "util.h"
#include "log.h"

void _ts_assert_fail(const char *cond, const char *func, const char *file, int line, bool use_log) {
	use_log = use_log && log_initialized();

	if(use_log) {
		_taisei_log(LOG_FAKEFATAL, func, file, line, "%s:%i: assertion `%s` failed", file, line, cond);
		log_sync();
	} else {
		tsfprintf(stderr, "%s:%i: %s(): assertion `%s` failed\n", file, line, func, cond);
	}
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
void _emscripten_trap(void) {
	EM_ASM({
		throw new Error("You just activated my trap card!");
	});
}
#endif
