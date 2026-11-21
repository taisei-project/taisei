/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#ifdef _WIN32
	#define PLATFORM_PATH_SEPARATORS "\\/"
#else
	// hope nothing else is insane enough to use something different
	#define PLATFORM_PATH_SEPARATORS "/"
#endif

// The preferred separator
#define PLATFORM_PATH_SEPARATOR PLATFORM_PATH_SEPARATORS[0]
