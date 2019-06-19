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
	lua_pushstring(L, LKEY_PROPGETTERS);
	lua_rawget(L, 3);

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
	lua_pushstring(L, LKEY_METHODS);
	lua_rawget(L, 3);

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
	lua_pushstring(L, LKEY_PROPSETTERS);
	lua_rawget(L, 4);

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
