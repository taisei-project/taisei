/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "script.h"
#include "util.h"
#include "console.h"
#include "version.h"

#include "rwops_reader.h"
#include "script_internal.h"

struct script script;

static int script_msghandler(lua_State *L) {
	// NOTE: Copied almost verbatim from lua.c

	const char *msg = lua_tostring(L, 1);

	if(msg == NULL) {  /* is error object not a string? */
		if(
			luaL_callmeta(L, 1, "__tostring") &&  /* does it have a metamethod */
			lua_type(L, -1) == LUA_TSTRING
		) {  /* that produces a string? */
			return 1;  /* that is the message */
		} else {
			msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
		}
	}

	luaL_traceback(L, L, msg, 1);  /* append a standard traceback */
	return 1;  /* return the traceback */
}

static noreturn int script_panic(lua_State *L) {
	script_msghandler(L);
	const char *error = lua_tostring(L, 1);
	assume(error != NULL);
	log_fatal("Unexpected Lua error:\n\n%s", error);
}

int script_pcall_with_msghandler(lua_State *L, int narg, int nres) {
	// NOTE: Copied almost verbatim from lua.c

	int status;
	int base = lua_gettop(L) - narg;  /* function index */
	lua_pushcfunction(L, script_msghandler);  /* push message handler */
	lua_insert(L, base);  /* put it under function and args */
	status = lua_pcall(L, narg, nres, base);
	lua_remove(L, base);  /* remove message handler from the stack */
	return status;
}

static int script_print(lua_State *L) {
	int n = lua_gettop(L);

	for(int i = 1; i <= n; i++) {
		size_t l;
		const char *s = luaL_tolstring(L, i, &l);

		if(s == NULL) {
			s = "(null)";
		}

		if(i > 1) {
			con_print("\t");
		}

		con_print(s);
		lua_pop(L, 1);
	}

	con_print("\n");
	return 0;
}

static lua_State *script_state_create(void) {
	lua_State *lstate = luaL_newstate();

	if(!lstate) {
		log_fatal("Failed to create Lua state");
	}

	luaL_checkversion(lstate);
	lua_atpanic(lstate, script_panic);
	lua_gc(lstate, LUA_GCINC, 0, 0);
	luaL_openlibs(lstate);
	lua_pushglobaltable(lstate);
	luaL_setfuncs(lstate, (luaL_Reg[]) {
		{ "print", script_print },
		{ NULL }
	}, 0);
	lua_pop(lstate, 1);

	return lstate;
}

static void script_state_destroy(lua_State **lstate) {
	if(*lstate) {
		lua_close(*lstate);
		*lstate = NULL;
	}
}

static bool script_load_vfs_file(lua_State *L, const char *vfspath) {
	SDL_RWops *rw = vfs_open(vfspath, VFS_MODE_READ);

	if(!rw) {
		log_error("VFS error: %s", vfs_get_error());
		return false;
	}

	char *syspath = vfs_repr(vfspath, true);
	assume(syspath != NULL);
	char chunk_name[strlen(syspath) + 2];
	snprintf(chunk_name, sizeof(chunk_name), "@%s", syspath);
	free(syspath);

	lua_Reader reader;
	LuaRWopsReader *lrw = lrwreader_create(rw, true, &reader);

	bool ok = true;

	if(lua_load(L, reader, lrw, chunk_name, "t") != LUA_OK) {
		const char *msg = lua_tostring(L, -1);
		log_error("Lua error: %s", msg);
		lua_pop(L, 1);
		ok = false;
	}

	lrwreader_destroy(lrw);

	return ok;
}

void script_init(void) {
	script.lstate = script_state_create();

	con_printf("%s\n%s\n", TAISEI_VERSION_FULL, LUA_COPYRIGHT);

	if(!script_load_vfs_file(script.lstate, "res/scripts/init.lua")) {
		log_fatal("Failed to load the init script");
	}

	if(script_pcall_with_msghandler(script.lstate, 0, 0) != LUA_OK) {
		const char *msg = lua_tostring(script.lstate, -1);
		log_fatal("Lua error: %s", msg);
	}
}

void script_shutdown(void) {
	script_state_destroy(&script.lstate);
}
