/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "reimu_a_entities.h"
#include "reimu_b_entities.h"

#include "marisa_a_entities.h"
#include "marisa_b_entities.h"

#include "youmu_a_entities.h"
#include "youmu_b_entities.h"

#define ENTITIES_PLAYERMODES(X, ...) \
	ENTITIES_ReimuA(X, __VA_ARGS__) \
	ENTITIES_ReimuB(X, __VA_ARGS__) \
	ENTITIES_MarisaA(X, __VA_ARGS__) \
	ENTITIES_MarisaB(X, __VA_ARGS__) \
	ENTITIES_YoumuA(X, __VA_ARGS__) \
	ENTITIES_YoumuB(X, __VA_ARGS__) \
	END_OF_ENTITIES
