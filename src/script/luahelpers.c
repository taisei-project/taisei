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

int luaTS_indexgetter(lua_State *L, const char *metatable) {
	// stack:
	// #1: receiver object
	// #2: key

	luaL_checkstring(L, 2);
	luaL_getmetatable(L, metatable);
	int s = lua_gettop(L);

	// temp = metatable[LKEY_PROPGETTERS]
	lua_pushstring(L, LKEY_PROPGETTERS);
	lua_rawget(L, -2);

	// temp = temp[key]
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);

	if(!lua_isnoneornil(L, -1)) {
		// return getter(receiver, key)
		lua_pushvalue(L, 1);
		lua_pushvalue(L, 2);
		lua_call(L, 2, 1);
		return 1;
	}

	// top = metatable
	lua_settop(L, s);

	// temp = metatable[LKEY_METHODS]
	lua_pushstring(L, LKEY_METHODS);
	lua_rawget(L, -2);

	// temp = temp[key]
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);

	if(lua_isnoneornil(L, -1)) {
		luaL_argerror(L, 2, lua_pushfstring(L, "can't retrieve property or method `%s` of %s", luaL_tolstring(L, 2, NULL), metatable));
	}

	return 1;
}

int luaTS_indexsetter(lua_State *L, const char *metatable) {
	// stack:
	// #1: receiver object
	// #2: key
	// #3: value

	luaL_checkstring(L, 2);
	luaL_getmetatable(L, metatable);

	// temp = metatable[LKEY_PROPSETTERS]
	lua_pushstring(L, LKEY_PROPSETTERS);
	lua_rawget(L, -2);

	// temp = temp[key]
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);

	if(lua_isnoneornil(L, -1)) {
		luaL_argerror(L, 2, lua_pushfstring(L, "can't set property `%s` of %s", luaL_tolstring(L, 2, NULL), metatable));
	}

	// setter(receiver, key, value)
	lua_pushvalue(L, 1);
	lua_pushvalue(L, 2);
	lua_pushvalue(L, 3);
	lua_call(L, 3, 0);

	return 0;
}
