/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "stages/stages.h"

void dynstage_init(void);
void dynstage_init_monitoring(void);
void dynstage_shutdown(void);
bool dynstage_reload_library(void);
StagesExports *dynstage_get_exports(void);

// NOTE: represents the amount of times the on-disk shared object was seen changing.
uint32_t dynstage_get_generation(void);
