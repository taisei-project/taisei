/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "vfs/public.h"
#include "all.h"
#include <lauxlib.h>

static int lvfs_open(lua_State *L) {
	const char *path = lua_tolstring(L, 1, NULL);
	lua_Integer mode = luaL_optinteger(L, 2, VFS_MODE_READ);

	SDL_RWops *rw = vfs_open(path, mode);

	if(rw) {
		lvio_wrap_rw(L, rw);
		return 1;
	} else {
		lua_pushnil(L);
		lua_pushstring(L, vfs_get_error());
		return 2;
	}
}

LUAMOD_API int luaopen_vfs(lua_State *L) {
	luaL_newlib(L, ((luaL_Reg[]) {
		{ "open", lvfs_open },
		{ "MODE_READ", NULL },
		{ "MODE_WRITE", NULL },
		{ "MODE_SEEKABLE", NULL },
		{ "MODE_RWMASK", NULL },
		{ NULL },
	}));

	lua_pushinteger(L, VFS_MODE_READ);
	lua_setfield(L, -2, "MODE_READ");

	lua_pushinteger(L, VFS_MODE_WRITE);
	lua_setfield(L, -2, "MODE_WRITE");

	lua_pushinteger(L, VFS_MODE_SEEKABLE);
	lua_setfield(L, -2, "MODE_SEEKABLE");

	lua_pushinteger(L, VFS_MODE_RWMASK);
	lua_setfield(L, -2, "MODE_RWMASK");

	return 1;
}
