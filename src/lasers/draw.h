/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "entity.h"
#include "resource/resource.h"

void laserdraw_preload(ResourceGroup *rg);
void laserdraw_init(void);
void laserdraw_shutdown(void);

void laserdraw_ent_drawfunc(EntityInterface *ent);
