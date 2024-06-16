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
#include "../scuttle.h"  // IWYU pragma: export
#include "../wriggle.h"  // IWYU pragma: export

DECLARE_EXTERN_TASK_WITH_INTERFACE(stage3_midboss_nonspell_1, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage3_boss_nonspell_1, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage3_boss_nonspell_2, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage3_boss_nonspell_3, BossAttack);
