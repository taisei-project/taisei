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
		{ LAPI_VIO_LIBNAME, luaopen_vio, },
		{ LAPI_VFS_LIBNAME, luaopen_vfs, },
		{ NULL },
	};

	for(const luaL_Reg *lib = libs; lib->func; ++lib) {
		luaL_requiref(L, lib->name, lib->func, 1);
		lua_pop(L, 1);
	}
}
