/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_plrmodes_reimu_b_entities_h
#define IGUARD_plrmodes_reimu_b_entities_h

#include "taisei.h"

#define ENTITIES_ReimuB(X, ...) \
	X(ReimuBController, __VA_ARGS__) \
	X(ReimuBSlave, __VA_ARGS__) \

#endif // IGUARD_plrmodes_reimu_b_entities_h
