/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "hirestime.h"

void time_init(void) {
}

void time_shutdown(void) {
}

hrtime_t time_get(void) {
	return SDL_GetTicksNS();
}
