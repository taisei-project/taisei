/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#define ENTITIES_STAGEX(X, ...) \
	X(YumemiSlave, __VA_ARGS__) \
	X(BossShield, __VA_ARGS__) \
	END_OF_ENTITIES
