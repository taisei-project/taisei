/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "../common/shader.h"

extern ShaderLangInfo *glcommon_shader_lang_table;

void glcommon_build_shader_lang_table(void);
void glcommon_free_shader_lang_table(void);
