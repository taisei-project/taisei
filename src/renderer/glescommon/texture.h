/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_renderer_glescommon_texture_h
#define IGUARD_renderer_glescommon_texture_h

#include "taisei.h"

#include "../glcommon/texture.h"

void gles_init_texformats_table(void);
GLTextureTypeInfo* gles_texture_type_info(TextureType type);

#endif // IGUARD_renderer_glescommon_texture_h
