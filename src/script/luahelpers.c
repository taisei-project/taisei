/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "luahelpers.h"

#include <lauxlib.h>

const char *luaTS_typename(lua_State *L, int index) {
	const char *name = NULL;
	int top = lua_gettop(L);

	if(luaL_getmetafield(L, 1, "__name") == LUA_TSTRING) {
		name = lua_tostring(L, -1);
	} else if(lua_type(L, index) == LUA_TLIGHTUSERDATA) {
		name = "light userdata";
	} else {
		name = luaL_typename(L, index);
	}

	lua_settop(L, top);
	return name ? name : "<unknown>";
}

int luaTS_indexgetter(lua_State *L) {
	// stack:
	// #1: receiver object
	// #2: key
	// #3: metatable
	// #4: fallback function

	luaL_checkstring(L, 2);
	while(lua_gettop(L) < 4) lua_pushnil(L);

	// getters = metatable[LKEY_PROPGETTERS]
	luaTS_rawgetfield(L, 3, LKEY_PROPGETTERS);

	// getter = getters[key]
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);

	if(!lua_isnoneornil(L, -1)) {
		// return getter(receiver, key)
		lua_pushvalue(L, 1);
		lua_pushvalue(L, 2);
		lua_call(L, 2, 1);
		return 1;
	}

	// methods = metatable[LKEY_METHODS]
	luaTS_rawgetfield(L, 3, LKEY_METHODS);

	// method = methods[key]
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);

	if(lua_isnoneornil(L, -1)) {
		if(lua_isnoneornil(L, 4)) {
			luaL_argerror(L, 2, lua_pushfstring(L, "can't retrieve property or method `%s` of %s", luaL_tolstring(L, 2, NULL), luaTS_typename(L, 1)));
		}

		// return fallback(receiver, key, metatable)
		lua_settop(L, 4);
		lua_rotate(L, 1, 1);
		lua_call(L, 3, 1);
		return 1;
	}

	// return method
	return 1;
}

int luaTS_indexsetter(lua_State *L) {
	// stack:
	// #1: receiver object
	// #2: key
	// #3: value
	// #4: metatable
	// #5: fallback function

	luaL_checkstring(L, 2);
	while(lua_gettop(L) < 5) lua_pushnil(L);

	// setters = metatable[LKEY_PROPSETTERS]
	luaTS_rawgetfield(L, 4, LKEY_PROPSETTERS);

	// setter = setters[key]
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);

	if(lua_isnoneornil(L, -1)) {
		if(lua_isnoneornil(L, 5)) {
			luaL_argerror(L, 2, lua_pushfstring(L, "can't set property `%s` of %s", luaL_tolstring(L, 2, NULL), luaTS_typename(L, 1)));
		}

		// fallback(receiver, key, value, metatable)
		lua_settop(L, 5);
		lua_rotate(L, 1, 1);
		lua_call(L, 4, 0);
		return 0;
	}

	// setter(receiver, key, value)
	lua_pushvalue(L, 1);
	lua_pushvalue(L, 2);
	lua_pushvalue(L, 3);
	lua_call(L, 3, 0);

	return 0;
}

int luaTS_defaultindexgetter(lua_State *L) {
	// stack:
	// #1: receiver object
	// #2: key

	lua_getmetatable(L, 1);
	return luaTS_indexgetter(L);
}

int luaTS_defaultindexsetter(lua_State *L) {
	// stack:
	// #1: receiver object
	// #2: key
	// #3: value

	lua_getmetatable(L, 1);
	return luaTS_indexsetter(L);
}

int luaTS_rawgetfield(lua_State *L, int index, const char *k) {
	index = lua_absindex(L, index);
	lua_pushstring(L, k);
	return lua_rawget(L, index);
}

void luaTS_rawsetfield(lua_State *L, int index, const char *k) {
	index = lua_absindex(L, index);
	lua_pushstring(L, k);
	lua_rotate(L, -2, 1);
	lua_rawset(L, index);
}

static void proxy_getmastertable(lua_State *L, int rootidx) {
	if(lua_getiuservalue(L, rootidx, 1), lua_isnoneornil(L, -1)) {
		lua_pop(L, 1);
		lua_newtable(L);
		lua_pushvalue(L, -1);
		attr_unused int success = lua_setiuservalue(L, rootidx, 1);
		assert(success != 0);
	}
}

static void proxy_setmetatable(lua_State *L, int idx, const char *proxyname, lua_CFunction mtinit) {
	idx = lua_absindex(L, idx);

	if(luaL_newmetatable(L, proxyname) != 0) {
		attr_unused int top = lua_gettop(L);
		mtinit(L);
		assert(top == lua_gettop(L));
	}

	lua_setmetatable(L, idx);
}

int luaTS_pushproxy(lua_State *L, int rootidx, const char *proxyname, lua_CFunction mtinit) {
	rootidx = lua_absindex(L, rootidx);
	proxy_getmastertable(L, rootidx);

	if(luaTS_rawgetfield(L, -1, proxyname), lua_isnoneornil(L, -1)) {
		lua_pop(L, 1);
		lua_newuserdatauv(L, 0, 1);
		assert(lua_rawlen(L, -1) == 0);
		lua_pushvalue(L, rootidx);
		attr_unused int success = lua_setiuservalue(L, -2, 1);
		assert(success != 0);
		proxy_setmetatable(L, -1, proxyname, mtinit);
		lua_pushvalue(L, -1);
		luaTS_rawsetfield(L, -3, proxyname);
	}

	return 1;
}

#if 0
int luaTS_pushuncachedproxy(lua_State *L, int rootidx, const char *proxyname, lua_CFunction mtinit) {
	rootidx = lua_absindex(L, rootidx);

	lua_newuserdatauv(L, 0, 1);
	lua_pushvalue(L, rootidx);
	attr_unused int success = lua_setiuservalue(L, -2, 1);
	assert(success != 0);
	lua_newtable(L);
	lua_pushstring(L, proxyname);
	luaTS_rawsetfield(L, -2, "__name");
	attr_unused int top = lua_gettop(L);
	mtinit(L);
	assert(top == lua_gettop(L));
	lua_setmetatable(L, -2);

	return 1;
}
#endif

int luaTS_pushinstancedproxy(lua_State *L, int rootidx, const char *proxyname, lua_CFunction mtinit) {
	int instanceid_idx = lua_gettop(L);
	rootidx = lua_absindex(L, rootidx);

	proxy_getmastertable(L, rootidx);
	luaL_getsubtable(L, -1, proxyname);
	lua_pushvalue(L, instanceid_idx);

	if(lua_rawget(L, -2), lua_isnoneornil(L, -1)) {
		lua_pop(L, 1);
		lua_newuserdatauv(L, 0, 2);

		lua_pushvalue(L, rootidx);
		lua_setiuservalue(L, -2, 1);

		lua_pushvalue(L, instanceid_idx);
		lua_setiuservalue(L, -2, 2);

		proxy_setmetatable(L, -1, proxyname, mtinit);

		lua_pushvalue(L, instanceid_idx);
		lua_pushvalue(L, -2);
		lua_rawset(L, -3);
	}

	return 1;
}

void *luaTS_checkproxiedudata(lua_State *L, int idx, const char *mt, bool swap) {
	idx = lua_absindex(L, idx);
	void *ud = NULL;

	if(lua_type(L, idx) == LUA_TUSERDATA) {
		if(lua_getiuservalue(L, idx, 1) == LUA_TUSERDATA) {
			ud = luaL_testudata(L, -1, mt);

			if(ud != NULL && swap) {
				lua_copy(L, -1, idx);
			}

			lua_pop(L, 1);
		} else {
			lua_pop(L, 1);
			ud = luaL_testudata(L, idx, mt);
		}
	}

	luaL_argexpected(L, ud != NULL, idx, lua_pushfstring(L, "%s or proxy", mt));
	return ud;
}

int luaTS_pushproxyinstance(lua_State *L, int idx) {
	return lua_getiuservalue(L, idx, 2);
}

#include "script_internal.h"

static void luaTS_setfuncs(lua_State *L, int idx, const luaL_Reg *l, int nup, int upvalueoffset) {
	// like luaL_setfuncs, but takes the table index and the upvalues index offset, and does not pop the upvalues.

	luaL_checkstack(L, nup, "too many upvalues");
	idx = lua_absindex(L, idx);

	for(; l->name != NULL; l++) {  /* fill the table with given functions */
		if(l->func == NULL) {  /* place holder? */
			lua_pushboolean(L, 0);
		} else {
			for(int i = 0; i < nup; i++) {  /* copy upvalues to the top */
				lua_pushvalue(L, -nup + upvalueoffset);
			}

			lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
		}

		lua_setfield(L, idx, l->name);
	}
}

int luaTS_setfunctables(lua_State *L, int idx, const luaTS_funcdefs *funcdefs, int upvalues) {
	idx = lua_absindex(L, idx);

	attr_unused int top = lua_gettop(L);

	lua_createtable(L, 0, funcdefs->methods.num);
	if(funcdefs->methods.array) {
		luaTS_setfuncs(L, -1, funcdefs->getters.array, upvalues, -1);
	}
	luaTS_rawsetfield(L, idx, LKEY_METHODS);

	lua_createtable(L, 0, funcdefs->getters.num);
	if(funcdefs->getters.array) {
		luaTS_setfuncs(L, -1, funcdefs->getters.array, upvalues, -1);
	}
	luaTS_rawsetfield(L, idx, LKEY_PROPGETTERS);

	lua_createtable(L, 0, funcdefs->setters.num);
	if(funcdefs->setters.array) {
		luaTS_setfuncs(L, -1, funcdefs->getters.array, upvalues, -1);
	}
	luaTS_rawsetfield(L, idx, LKEY_PROPSETTERS);

	assert(lua_gettop(L) == top);
	lua_pop(L, upvalues);

	return 0;
}
