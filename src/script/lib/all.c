/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "all.h"
#include <lauxlib.h>

void lapi_open_all(lua_State *L) {
	const luaL_Reg libs[] = {
		{ LUA_GNAME, luaopen_base },
		// { LUA_LOADLIBNAME, luaopen_package },
		{ LUA_COLIBNAME, luaopen_coroutine },
		{ LUA_TABLIBNAME, luaopen_table },
		// { LUA_IOLIBNAME, luaopen_io },
		// { LUA_OSLIBNAME, luaopen_os },
		{ LUA_STRLIBNAME, luaopen_string },
		{ LUA_MATHLIBNAME, luaopen_math },
		{ LUA_UTF8LIBNAME, luaopen_utf8 },
#ifdef DEBUG
		{ LUA_DBLIBNAME, luaopen_debug },
#endif

		{ LAPI_COLOR_LIBNAME, luaopen_color, },
		{ LAPI_VFS_LIBNAME, luaopen_vfs, },
		{ LAPI_VIO_LIBNAME, luaopen_vio, },
		{ LAPI_RESOURCES_LIBNAME, luaopen_resources, },

		{ NULL },
	};

	lapi_baseext_register(L);

	for(const luaL_Reg *lib = libs; lib->func; ++lib) {
		luaL_requiref(L, lib->name, lib->func, 1);
		lua_pop(L, 1);
	}
}
