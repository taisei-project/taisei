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

void stage3_midboss_nonspell1(Boss *boss, int time);
void stage3_boss_nonspell1(Boss *boss, int time);
void stage3_boss_nonspell2(Boss *boss, int time);
void stage3_boss_nonspell3(Boss *boss, int time);

DECLARE_EXTERN_TASK_WITH_INTERFACE(stage3_midboss_nonspell_1, BossAttack);
