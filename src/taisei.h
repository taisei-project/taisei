/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

// IWYU pragma: always_keep

#include "build_config.h"             // IWYU pragma: export
#include "util/compat.h"              // IWYU pragma: export
#include "util/assert.h"              // IWYU pragma: export
#include "util/consideredharmful.h"   // IWYU pragma: export
#include "memory/memory.h"            // IWYU pragma: export

#include <libintl.h>
#define _(str) gettext(str)

#ifdef TAISEI_BUILDCONF_DEVELOPER
	// TODO: maybe rename this
	#define DEBUG 1
	#define IF_DEBUG(statement) do { statement } while(0)
	#define IF_NOT_DEBUG(statement) ((void)0)
#else
	#define IF_DEBUG(statement) ((void)0)
	#define IF_NOT_DEBUG(statement) do { statement } while(0)
#endif

#ifdef TAISEI_BUILDCONF_LOG_FATAL_MSGBOX
	#define LOG_FATAL_MSGBOX
#endif
