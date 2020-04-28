/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_stages_stagex_stagex_h
#define IGUARD_stages_stagex_stagex_h

#include "taisei.h"

#include "stageinfo.h"

extern StageProcs stagex_procs;

Boss *stagex_spawn_yumemi(cmplx pos);
void stagex_draw_yumemi_portrait_overlay(SpriteParams *sp);

DECLARE_EXTERN_TASK_WITH_INTERFACE(stagex_spell_infinity_network, BossAttack);

#endif // IGUARD_stages_stagex_stagex_h
