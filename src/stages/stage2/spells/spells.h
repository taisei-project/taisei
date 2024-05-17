/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "stages/common_imports.h"  // IWYU pragma: export
#include "../hina.h"  // IWYU pragma: export

DECLARE_EXTERN_TASK_WITH_INTERFACE(stage2_spell_amulet_of_harm, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage2_spell_wheel_of_fortune, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage2_spell_bad_pick, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage2_spell_monty_hall_danmaku, BossAttack);
