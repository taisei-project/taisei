/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_renderer_common_sprite_batch_h
#define IGUARD_renderer_common_sprite_batch_h

#include "taisei.h"

#include "../api.h"

void _r_sprite_batch_init(void);
void _r_sprite_batch_shutdown(void);
void _r_sprite_batch_end_frame(void);
void _r_sprite_batch_texture_deleted(Texture *tex);

#endif // IGUARD_renderer_common_sprite_batch_h
