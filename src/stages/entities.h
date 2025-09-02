/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "stage1/entities.h"
#include "stage2/entities.h"
#include "stage3/entities.h"
#include "stage4/entities.h"
#include "stage5/entities.h"
#include "stage6/entities.h"
#include "stagex/entities.h"

#define ENTITIES_STAGES(X, ...) \
	ENTITIES_STAGE1(X, __VA_ARGS__) \
	ENTITIES_STAGE2(X, __VA_ARGS__) \
	ENTITIES_STAGE3(X, __VA_ARGS__) \
	ENTITIES_STAGE4(X, __VA_ARGS__) \
	ENTITIES_STAGE5(X, __VA_ARGS__) \
	ENTITIES_STAGE6(X, __VA_ARGS__) \
	ENTITIES_STAGEX(X, __VA_ARGS__) \
	END_OF_ENTITIES
