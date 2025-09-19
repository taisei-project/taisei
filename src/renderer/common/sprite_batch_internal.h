/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "sprite_batch.h"   // IWYU pragma: export

#include "../api.h"

void _r_sprite_batch_end_frame(void);
void _r_sprite_batch_texture_deleted(Texture *tex);
