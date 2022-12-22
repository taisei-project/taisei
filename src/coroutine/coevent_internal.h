/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

// #define EVT_DEBUG

#ifdef EVT_DEBUG
	#undef EVT_DEBUG
	#define EVT_DEBUG(...) log_debug(__VA_ARGS__)
#else
	#define EVT_DEBUG(...) ((void)0)
#endif

void coevent_cleanup_subscribers(CoEvent *evt);
void coevent_add_subscriber(CoEvent *evt, CoTask *task);
