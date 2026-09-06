/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "stages/common_imports.h"   // IWYU pragma: export
#include "../yumemi.h"   // IWYU pragma: export

DECLARE_EXTERN_TASK_WITH_INTERFACE(stagex_spell_stack_smashing, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stagex_spell_fork_bomb, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stagex_spell_infinity_network, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stagex_spell_sierpinski, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stagex_spell_mem_copy, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stagex_spell_pipe_dream, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stagex_spell_alignment, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stagex_spell_rings, BossAttack);
