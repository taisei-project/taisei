/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "boss.h"

void iku_slave_visual(Enemy*, int, bool);
void iku_nonspell_spawn_cloud(void);
void iku_lightning_particle(cmplx, int);

int iku_induction_bullet(Projectile*, int);
