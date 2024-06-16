/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "../common/shaderlib/shaderlib.h"
#include "dynarray.h"

typedef DYNAMIC_ARRAY(ShaderLangInfo) ShaderLangInfoArray;
extern ShaderLangInfoArray glcommon_shader_lang_table;

void glcommon_build_shader_lang_table(void);
void glcommon_free_shader_lang_table(void);
