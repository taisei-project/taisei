/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stages_stage6_elly_h
#define IGUARD_stages_stage6_elly_h

#include "taisei.h"
#include "enemy.h"
#include "boss.h"

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
int baryon_nattack(Enemy*, int);
int baryon_explode(Enemy*, int);

int broglie_charge(Projectile*, int);

int wait_proj(Projectile*, int);

void elly_toe_laser_logic(Laser*, int);
int elly_toe_boson(Projectile*, int);

Enemy* find_scythe(void);

Color* boson_color(Color*, int, int);

#endif // IGUARD_stages_stage6_elly
