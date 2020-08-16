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

void iku_bolts(Boss*, int);
void iku_bolts2(Boss*, int);
void iku_bolts3(Boss*, int);
int induction_bullet(Projectile*, int);
int zigzag_bullet(Projectile*, int);
cmplx cathode_laser(Laser*, float);

int lightning_slave(Enemy*, int);
void iku_slave_visual(Enemy*, int, bool);

void lightning_particle(cmplx, int);
void cloud_common(void);

#endif // IGUARD_stages_stage5_iku_h
