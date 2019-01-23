/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_taisei_h
#define IGUARD_taisei_h

#include "taisei.h"

#include "build_config.h"
#include "util/compat.h"

#ifdef TAISEI_BUILDCONF_DEBUG
	#define DEBUG 1
#endif

#ifdef TAISEI_BUILDCONF_LOG_ENABLE_BACKTRACE
	#define LOG_ENABLE_BACKTRACE
#endif

#ifdef TAISEI_BUILDCONF_LOG_FATAL_MSGBOX
	#define LOG_FATAL_MSGBOX
#endif

#endif // IGUARD_taisei_h
