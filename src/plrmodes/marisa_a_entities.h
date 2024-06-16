/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#define ENTITIES_MarisaA(X, ...) \
	X(MarisaAController, __VA_ARGS__) \
	X(MarisaAMasterSpark, __VA_ARGS__) \
	X(MarisaASlave, __VA_ARGS__) \
	END_OF_ENTITIES
