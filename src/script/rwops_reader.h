/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_script_rwops_reader_h
#define IGUARD_script_rwops_reader_h

#include "taisei.h"

#include <SDL_rwops.h>
#include <lua.h>

typedef struct LuaRWopsReader LuaRWopsReader;

LuaRWopsReader *lrwreader_create(SDL_RWops *rwops, bool autoclose, lua_Reader *out_readfunc);
void lrwreader_destroy(LuaRWopsReader *lrw);

#endif // IGUARD_script_rwops_reader_h
