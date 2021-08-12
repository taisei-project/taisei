/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "enemy.h"
#include "boss.h"

#define FERMIONTIME 1000
#define HIGGSTIME 1700
#define YUKAWATIME 2200
#define SYMMETRYTIME (HIGGSTIME+200)
#define BREAKTIME (YUKAWATIME+400)

void scythe_common(Enemy*, int t);
void scythe_draw(Enemy*, int, bool);
int scythe_mid(Enemy*, int);
int scythe_reset(Enemy*, int);
int scythe_infinity(Enemy*, int);
int scythe_explode(Enemy*, int);
void elly_clap(Boss*, int);
void elly_spawn_baryons(cmplx);

void set_baryon_rule(EnemyLogicRule);
int baryon_reset(Enemy*, int);

int wait_proj(Projectile*, int);

Enemy* find_scythe(void);
