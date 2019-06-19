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

const char *luaTS_typename(lua_State *L, int index);
int luaTS_indexgetter(lua_State *L);
int luaTS_indexsetter(lua_State *L);

#define luaTS_setproptables(L, propgetters, propsetters, methods) \
	do { \
		lua_createtable(L, 0, ARRAY_SIZE(propgetters) - 1); \
		luaL_setfuncs(L, (propgetters), 0); \
		lua_setfield(L, -2, LKEY_PROPGETTERS); \
		lua_createtable(L, 0, ARRAY_SIZE(propsetters) - 1); \
		luaL_setfuncs(L, (propsetters), 0); \
		lua_setfield(L, -2, LKEY_PROPSETTERS); \
		lua_createtable(L, 0, ARRAY_SIZE(methods) - 1); \
		luaL_setfuncs(L, (methods), 0); \
		lua_setfield(L, -2, LKEY_METHODS); \
	} while(0)

#define LPROPTABLES_PLACEHOLDERS \
	{ LKEY_PROPSETTERS, NULL }, \
	{ LKEY_PROPGETTERS, NULL }, \
	{ LKEY_METHODS, NULL }

#endif // IGUARD_script_luahelpers_h
