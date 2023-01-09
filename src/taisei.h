/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "build_config.h"
#include "util/compat.h"
#include "util/consideredharmful.h"
#include "memory.h"

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
