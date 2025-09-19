/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#define END_OF_ENTITIES

#define ENTITIES_CORE(X, ...) \
	X(Boss, __VA_ARGS__) \
	X(Enemy, __VA_ARGS__) \
	X(Item, __VA_ARGS__) \
	X(Laser, __VA_ARGS__) \
	X(Player, __VA_ARGS__) \
	X(PlayerIndicators, __VA_ARGS__) \
	X(Projectile, __VA_ARGS__) \
	END_OF_ENTITIES

#include "plrmodes/entities.h"
#include "stages/entities.h"

#define ENTITIES(X, ...) \
	ENTITIES_CORE(X, __VA_ARGS__) \
	ENTITIES_PLAYERMODES(X, __VA_ARGS__) \
	ENTITIES_STAGES(X, __VA_ARGS__) \
	END_OF_ENTITIES
