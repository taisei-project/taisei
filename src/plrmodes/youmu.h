/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once

#include "plrmodes.h"
#include "youmu_a.h"
#include "youmu_b.h"

extern PlayerCharacter character_youmu;

int youmu_common_particle_spin(Projectile *p, int t);
void youmu_common_particle_slice_draw(Projectile *p, int t);
int youmu_common_particle_slice_logic(Projectile *p, int t);
void youmu_common_shot(Player *plr);
void youmu_common_draw_proj(Projectile *p, Color c, float scale);
