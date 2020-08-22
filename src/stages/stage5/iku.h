/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stages_stage5_iku_h
#define IGUARD_stages_stage5_iku_h

#include "taisei.h"

#include "boss.h"

void iku_slave_visual(Enemy*, int, bool);

void lightning_particle(cmplx, int);

int induction_bullet(Projectile*, int);

void cloud_common(void);

#endif // IGUARD_stages_stage5_iku_h
