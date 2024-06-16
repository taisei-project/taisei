/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "boss.h"  // IWYU pragma: keep

void stage5_events(void);
Boss *stage5_spawn_iku(cmplx);

DECLARE_EXTERN_TASK(stage5_timeline);
