/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

// NOTE: these are tightly coupled with player and stage systems
typedef enum ReplayEventCode {
	EV_PRESS            = 0x00,
	EV_RELEASE          = 0x01,
	EV_OVER             = 0x02, // replay-only
	EV_AXIS_LR          = 0x03,
	EV_AXIS_UD          = 0x04,
	EV_CHECK_DESYNC     = 0x05, // replay-only
	EV_FPS              = 0x06, // replay-only
	EV_INFLAGS          = 0x07,
	EV_CONTINUE         = 0x08,
	EV_RESUME           = 0x09, // replay-only
} ReplayEventCode;
