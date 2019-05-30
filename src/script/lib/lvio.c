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
#include <SDL_rwops.h>

#define VIO_NAME "VirtualIO"

static inline SDL_RWops *getrw(lua_State *L, int stackidx, char *op) {
	SDL_RWops *rw = *(SDL_RWops**)luaL_checkudata(L, stackidx, VIO_NAME);

	if(rw == NULL) {
		lua_pushfstring(L, "attempt to %s a closed "VIO_NAME" object", op);
		lua_error(L);
	}

	return rw;
}

static int getwhence(lua_State *L, int stackidx) {
	luaL_checkany(L, stackidx);

	int isnum;
	lua_Integer intval = lua_tointegerx(L, stackidx, &isnum);

	if(isnum) {
		switch(intval) {
			case RW_SEEK_SET: case RW_SEEK_CUR: case RW_SEEK_END:
				return intval;
			default:
				lua_pushfstring(L, "invalid whence value: %s", luaL_tolstring(L, stackidx, NULL));
				lua_error(L);
		}
	}

	static_assert(RW_SEEK_SET == 0, "");
	static_assert(RW_SEEK_CUR == 1, "");
	static_assert(RW_SEEK_END == 2, "");

	return luaL_checkoption(L, stackidx, NULL, (const char *[]) { "set", "cur", "end", NULL });
}

static int lvio_rw_read(lua_State *L) {
	SDL_RWops *rw = getrw(L, 1, "read from");
	lua_Integer maxbytes = luaL_optinteger(L, 2, LUA_MAXINTEGER);
	luaL_Buffer buf;
	luaL_buffinit(L, &buf);

	for(;;) {
		lua_Integer read_size = maxbytes > LUAL_BUFFERSIZE ? LUAL_BUFFERSIZE : maxbytes;

		if(read_size < 1) {
			break;
		}

		void *buffp = luaL_prepbuffsize(&buf, read_size);
		lua_Integer numread = SDL_RWread(rw, buffp, 1, read_size);

		if(numread <= 0) {
			break;
		}

		luaL_addsize(&buf, numread);
		maxbytes -= numread;
	}

	luaL_pushresult(&buf);
	return 1;
}

static int lvio_rw_write(lua_State *L) {
	SDL_RWops *rw = getrw(L, 1, "write to");
	size_t data_size;
	const char *data = luaL_tolstring(L, 2, &data_size);
	size_t written = SDL_RWwrite(rw, data, 1, data_size);
	lua_pushinteger(L, (lua_Integer)written);

	if(written == 0) {
		lua_pushstring(L, SDL_GetError());
		return 2;
	}

	return 1;
}

static int lvio_rw_seek(lua_State *L) {
	SDL_RWops *rw = getrw(L, 1, "seek in");
	lua_Integer ofs = luaL_checkinteger(L, 2);
	int whence = getwhence(L, 3);

	ofs = SDL_RWseek(rw, ofs, whence);
	lua_pushinteger(L, ofs);

	if(ofs < 0) {
		lua_pushstring(L, SDL_GetError());
		return 2;
	}

	return 1;
}

static int lvio_rw_size(lua_State *L) {
	SDL_RWops *rw = getrw(L, 1, "get size of");
	lua_Integer size = SDL_RWsize(rw);
	lua_pushinteger(L, size);

	if(size < 0) {
		lua_pushstring(L, SDL_GetError());
		return 2;
	}

	return 1;
}

static int lvio_rw_tell(lua_State *L) {
	SDL_RWops *rw = getrw(L, 1, "tell position in");
	lua_Integer ofs = SDL_RWtell(rw);
	lua_pushinteger(L, ofs);

	if(ofs < 0) {
		lua_pushstring(L, SDL_GetError());
		return 2;
	}

	return 1;
}

static int lvio_rw_close(lua_State *L) {
	SDL_RWops **rwp = luaL_checkudata(L, 1, VIO_NAME);

	if(*rwp == NULL) {
		lua_pushstring(L, "attempt to close a closed "VIO_NAME" object");
		lua_error(L);
	}

	lua_Integer status = SDL_RWclose(*rwp);
	*rwp = NULL;

	lua_pushinteger(L, status);

	if(status < 0) {
		lua_pushstring(L, SDL_GetError());
		return 2;
	}

	return 1;
}

static int lvio_rw_gc(lua_State *L) {
	SDL_RWops **rwp = luaL_checkudata(L, 1, VIO_NAME);

	if(*rwp) {
		SDL_RWclose(*rwp);
		*rwp = NULL;
	}

	return 0;
}

void lvio_wrap_rw(lua_State *L, SDL_RWops *rw) {
	SDL_RWops **vio = lua_newuserdata(L, sizeof(rw));
	*vio = rw;
	luaL_getmetatable(L, VIO_NAME);
	lua_setmetatable(L, -2);
}

LUAMOD_API int luaopen_vio(lua_State *L) {
	luaL_newlib(L, ((luaL_Reg[]) {
		{ "SEEK_CUR", NULL },
		{ "SEEK_END", NULL },
		{ "SEEK_SET", NULL },
		{ NULL },
	}));

	lua_pushinteger(L, RW_SEEK_CUR);
	lua_setfield(L, -2, "SEEK_CUR");
	lua_pushinteger(L, RW_SEEK_END);
	lua_setfield(L, -2, "SEEK_END");
	lua_pushinteger(L, RW_SEEK_SET);
	lua_setfield(L, -2, "SEEK_SET");

	luaL_newmetatable(L, VIO_NAME);
	luaL_setfuncs(L, ((luaL_Reg[]) {
		{ "__close", lvio_rw_gc, },
		{ "__gc", lvio_rw_gc, },
		{ "close", lvio_rw_close, },
		{ "read", lvio_rw_read, },
		{ "seek", lvio_rw_seek, },
		{ "size", lvio_rw_size, },
		{ "tell", lvio_rw_tell, },
		{ "write", lvio_rw_write, },
		{ NULL },
	}), 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	return 1;
}
