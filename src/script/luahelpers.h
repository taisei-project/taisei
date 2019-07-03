/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_script_luahelpers_h
#define IGUARD_script_luahelpers_h

#include "taisei.h"

#include <lua.h>
#include <lauxlib.h>
#include "util.h"

// undef some compatibility macros to not accidentally use them
#undef lua_newuserdata
#undef lua_getuservalue
#undef lua_setuservalue

typedef struct luaTS_funcdefs {
	struct {
		luaL_Reg *array;
		size_t num;
	} methods, getters, setters;
} luaTS_funcdefs;

#define LUATS_FUNCARRAY(...) ((luaL_Reg[]) { __VA_ARGS__ })

#define LUATS_FUNCDEFS(field, ...) .field = { .array = LUATS_FUNCARRAY(__VA_ARGS__), .num = ARRAY_SIZE(LUATS_FUNCARRAY(__VA_ARGS__)) - 1 },

#define LUATS_METHODS(...) LUATS_FUNCDEFS(methods, __VA_ARGS__)
#define LUATS_GETTERS(...) LUATS_FUNCDEFS(getters, __VA_ARGS__)
#define LUATS_SETTERS(...) LUATS_FUNCDEFS(setters, __VA_ARGS__)

#define LKEY_PROPSETTERS "_p="
#define LKEY_PROPGETTERS "_p"
#define LKEY_METHODS "_m"

#define LUATS_INVALID_INSTANCE (~(lua_Integer)0)

const char *luaTS_typename(lua_State *L, int index);
int luaTS_defaultindexgetter(lua_State *L);  // for use as a function callback
int luaTS_defaultindexsetter(lua_State *L);  // for use as a function callback
int luaTS_indexgetter(lua_State *L);  // for calling manually (see definition for expected stack state)
int luaTS_indexsetter(lua_State *L);  // for calling manually (see definition for expected stack state)
int luaTS_setfunctables(lua_State *L, int idx, const luaTS_funcdefs *funcdefs, int upvalues);
int luaTS_rawgetfield(lua_State *L, int index, const char *k);
void luaTS_rawsetfield(lua_State *L, int index, const char *k);
int luaTS_pushproxy(lua_State *L, int rootidx, const char *proxyname, lua_CFunction mtinit);
int luaTS_pushinstancedproxy(lua_State *L, int rootidx, const char *proxyname, lua_CFunction mtinit);
void *luaTS_checkproxiedudata(lua_State *L, int idx, const char *mt, bool swap);
int luaTS_pushproxyinstance(lua_State *L, int idx);

INLINE void luaTS_pushconstlightuserdata(lua_State *L, const void *ud) {
	lua_pushlightuserdata(L, (void*)ud);
}

#define LPROPTABLES_PLACEHOLDERS \
	{ LKEY_PROPSETTERS, NULL }, \
	{ LKEY_PROPGETTERS, NULL }, \
	{ LKEY_METHODS, NULL }

#endif // IGUARD_script_luahelpers_h
