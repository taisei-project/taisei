/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "iostream_dummy.h"

INLINE SDL_IOStream *iostream_wrap_readonly(SDL_IOStream *src, bool autoclose) {
	return iostream_wrap_dummy(src, &(IOStreamWrapDummyOpts) {
		.autoclose = autoclose,
		.readonly = true,
	});
}
