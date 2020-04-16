/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_plrmodes_marisa_a_entities_h
#define IGUARD_plrmodes_marisa_a_entities_h

#include "taisei.h"

#define ENTITIES_MarisaA(X, ...) \
	X(MarisaAController, __VA_ARGS__) \
	X(MarisaAMasterSpark, __VA_ARGS__) \
	X(MarisaASlave, __VA_ARGS__) \

#endif // IGUARD_plrmodes_marisa_a_entities_h
