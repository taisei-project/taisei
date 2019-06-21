/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "util.h"
#include "vfs/public.h"
#include "all.h"
#include <lauxlib.h>

#define VDIR_NAME "VirtualDirectory"

static int lvfs_open(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);
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

static int lvfs_iterdir_iterator(lua_State *L) {
	VFSDir **vdirp = luaL_checkudata(L, lua_upvalueindex(1), VDIR_NAME);

	if(*vdirp == NULL) {
		return 0;
	}

	const char *entry = vfs_dir_read(*vdirp);

	if(entry == NULL) {
		vfs_dir_close(*vdirp);
		*vdirp = NULL;
		return 0;
	}

	lua_pushstring(L, entry);
	return 1;
}

static int lvfs_iterdir(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);

	VFSDir *vdir = vfs_dir_open(path);

	if(vdir == NULL) {
		lua_pushnil(L);
		lua_pushstring(L, vfs_get_error());
		return 2;
	}

	VFSDir **vdirp = lua_newuserdatauv(L, sizeof(vdir), 0);
	assume(vdirp != NULL);
	*vdirp = vdir;

	luaL_getmetatable(L, VDIR_NAME);
	lua_setmetatable(L, -2);

	// gets the vdrip userdata as its first and only upvalue
	lua_pushcclosure(L, lvfs_iterdir_iterator, 1);

	return 1;
}

static int lvfs_vdir_gc(lua_State *L) {
	VFSDir **vdirp = luaL_checkudata(L, 1, VDIR_NAME);

	if(*vdirp) {
		vfs_dir_close(*vdirp);
		*vdirp = NULL;
	}

	return 0;
}

static int lvfs_query(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);
	VFSInfo i = vfs_query(path);

	if(i.error) {
		lua_createtable(L, 0, 1);
		lua_pushstring(L, vfs_get_error());
		lua_setfield(L, -2, "error");
	} else {
		lua_createtable(L, 0, 1);
		lua_pushboolean(L, i.exists);
		lua_setfield(L, -2, "exists");
		lua_pushboolean(L, i.is_dir);
		lua_setfield(L, -2, "isdir");
		lua_pushboolean(L, i.is_readonly);
		lua_setfield(L, -2, "isreadonly");
	}

	return 1;
}

struct lvfs_sync {
	lua_State *L;
	int ref;
};

static void lvfs_sync_callback(CallChainResult ccr) {
	struct lvfs_sync ctx = *(struct lvfs_sync*)ccr.ctx;
	free(ccr.ctx);

	lua_geti(ctx.L, LUA_REGISTRYINDEX, ctx.ref);
	luaL_unref(ctx.L, LUA_REGISTRYINDEX, ctx.ref);
	lua_call(ctx.L, 0, 0);
}

static int lvfs_sync(lua_State *L) {
	lua_Integer mode = luaL_checkinteger(L, 1);

	CallChain cc = NO_CALLCHAIN;

	if(!lua_isnoneornil(L, 2)) {
		lua_pushvalue(L, 2);
		struct lvfs_sync ctx = {
			.L = L,
			.ref = luaL_ref(L, LUA_REGISTRYINDEX),
		};
		cc.callback = lvfs_sync_callback;
		cc.ctx = memdup(&ctx, sizeof(ctx));
	}

	vfs_sync(mode, cc);
	return 0;
}

static int lvfs_syspath(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);
	char *syspath = vfs_syspath(path);

	if(syspath) {
		lua_pushstring(L, syspath);
		free(syspath);
		return 1;
	} else {
		lua_pushnil(L);
		lua_pushstring(L, vfs_get_error());
		return 2;
	}
}

static int lvfs_repr(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);
	bool try_syspath = lua_toboolean(L, 2);
	char *repr = vfs_repr(path, try_syspath);

	if(repr) {
		lua_pushstring(L, repr);
		free(repr);
		return 1;
	} else {
		lua_pushnil(L);
		lua_pushstring(L, vfs_get_error());
		return 2;
	}
}

LUAMOD_API int luaopen_vfs(lua_State *L) {
	luaL_newmetatable(L, VDIR_NAME);
	luaL_setfuncs(L, ((luaL_Reg[]) {
		{ "__close", lvfs_vdir_gc, },
		{ "__gc", lvfs_vdir_gc, },
		{ NULL },
	}), 0);

	luaL_newlib(L, ((luaL_Reg[]) {
		{ "MODE_READ", NULL },
		{ "MODE_RWMASK", NULL },
		{ "MODE_SEEKABLE", NULL },
		{ "MODE_WRITE", NULL },
		{ "SYNC_LOAD", NULL },
		{ "SYNC_STORE", NULL },
		{ "iterdir", lvfs_iterdir },
		{ "open", lvfs_open },
		{ "query", lvfs_query },
		{ "repr", lvfs_repr, },
		{ "sync", lvfs_sync },
		{ "syspath", lvfs_syspath, },
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

	lua_pushinteger(L, VFS_SYNC_LOAD);
	lua_setfield(L, -2, "SYNC_LOAD");

	lua_pushinteger(L, VFS_SYNC_STORE);
	lua_setfield(L, -2, "SYNC_STORE");

	return 1;
}
