/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "../api.h"

void _r_sprite_batch_init(void);
void _r_sprite_batch_shutdown(void);
void _r_sprite_batch_end_frame(void);
void _r_sprite_batch_texture_deleted(Texture *tex);
