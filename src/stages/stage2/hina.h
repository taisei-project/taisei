/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "boss.h"   // IWYU pragma: keep

Boss *stage2_spawn_hina(cmplx pos);
void stage2_draw_hina_spellbg(Boss *h, int time);
