/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stages_stage5_nonspells_nonspells_h
#define IGUARD_stages_stage5_nonspells_nonspells_h

#include "stages/stage5/iku.h"
#include "taisei.h"

#include "boss.h"

void iku_bolts(Boss*, int);
void iku_bolts2(Boss*, int);
void iku_bolts3(Boss*, int);
int iku_explosion(Enemy*, int);

#endif // IGUARD_stages_stage5_nonspells_nonspells_h
