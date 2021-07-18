/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_known_entities_h
#define IGUARD_known_entities_h

#include "taisei.h"

#define ENTITIES_CORE(X, ...) \
	X(Boss, __VA_ARGS__) \
	X(Enemy, __VA_ARGS__) \
	X(Item, __VA_ARGS__) \
	X(Laser, __VA_ARGS__) \
	X(Player, __VA_ARGS__) \
	X(PlayerIndicators, __VA_ARGS__) \
	X(Projectile, __VA_ARGS__) \

#include "plrmodes/entities.h"
#include "stages/entities.h"

#define ENTITIES(X, ...) \
	ENTITIES_CORE(X, __VA_ARGS__) \
	ENTITIES_PLAYERMODES(X, __VA_ARGS__) \
	ENTITIES_STAGES(X, __VA_ARGS__) \

#endif // IGUARD_known_entities_h
