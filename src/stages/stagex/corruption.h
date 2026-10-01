/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "entity.h"

typedef struct StageXCorruption StageXCorruption;

StageXCorruption *stagex_corruption_create(void);
void stagex_corruption_destroy(StageXCorruption *corruption);
void stagex_corruption_render_mask(StageXCorruption *corruption);
void stagex_corruption_draw(StageXCorruption *corruption);
Projectile *stagex_corruption_spawn_zone(StageXCorruption *corruption, cmplx origin, real radius, int duration);

void stagex_corrupt_enemy(StageXCorruption *corruption, Enemy *e);
