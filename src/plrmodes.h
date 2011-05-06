/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef PLRMODES_H
#define PLRMODES_H

#include "enemy.h"
#include "projectile.h"

void youmu_opposite_draw(Enemy *e, int t);
void youmu_opposite_logic(Enemy *e, int t);

void youmu_homing(Projectile *p, int t);

#endif