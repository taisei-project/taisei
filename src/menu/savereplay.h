/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "menu.h"
#include "eventloop/eventloop.h"
#include "replay/replay.h"

void ask_save_replay(Replay *rpy, CallChain next)
	attr_nonnull(1);
