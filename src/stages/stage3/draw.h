/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "util/fbpair.h"
#include "stagedraw.h"

void stage3_drawsys_init(void);
void stage3_drawsys_shutdown(void);
void stage3_draw(void);

extern ShaderRule stage3_bg_effects[];
extern ShaderRule stage3_postprocess[];
