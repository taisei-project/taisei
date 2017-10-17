/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once

#include "boss.h"

void elly_spellbg_classic(Boss*, int);
void elly_spellbg_modern(Boss*, int);
void elly_kepler(Boss*, int);
void elly_newton(Boss*, int);
void elly_maxwell(Boss*, int);
void elly_eigenstate(Boss*, int);
void elly_broglie(Boss*, int);
void elly_ricci(Boss*, int);
void elly_lhc(Boss*, int);
void elly_theory(Boss*, int);
void elly_curvature(Boss*, int);

void stage6_events(void);
Boss* stage6_spawn_elly(complex pos);
