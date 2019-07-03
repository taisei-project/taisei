/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stages_stage6_events_h
#define IGUARD_stages_stage6_events_h

#include "taisei.h"

#include "boss.h"
#include "enemy.h"

void elly_spellbg_classic(Boss*, int);
void elly_spellbg_modern(Boss*, int);
void elly_spellbg_modern_dark(Boss*, int);
void elly_spellbg_toe(Boss*, int);

void elly_kepler(Boss*, int);
void elly_newton(Boss*, int);
void elly_maxwell(Boss*, int);
void elly_eigenstate(Boss*, int);
void elly_broglie(Boss*, int);
void elly_ricci(Boss*, int);
void elly_lhc(Boss*, int);
void elly_theory(Boss*, int);
void elly_curvature(Boss*, int);
void elly_intro(Boss*, int);

void elly_spawn_baryons(complex pos);

void stage6_events(void);
Boss* stage6_spawn_elly(complex pos);

void Scythe(Enemy *e, int t, bool render);
void scythe_common(Enemy *e, int t);
int scythe_reset(Enemy *e, int t);

// #define ELLY_TOE_TARGET_POS (VIEWPORT_W/2+298.0*I)
#define ELLY_TOE_TARGET_POS (VIEWPORT_W/2+VIEWPORT_H/2*I)

#endif // IGUARD_stages_stage6_events_h
