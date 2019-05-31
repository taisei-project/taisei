/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_script_luahelpers_h
#define IGUARD_script_luahelpers_h

#include "taisei.h"

#include <lua.h>

#define LKEY_PROPSETTERS "_p="
#define LKEY_PROPGETTERS "_p"
#define LKEY_METHODS "_m"

int luaTS_indexgetter(lua_State *L, const char *metatable);
int luaTS_indexsetter(lua_State *L, const char *metatable);

#endif // IGUARD_script_luahelpers_h
