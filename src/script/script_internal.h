/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_script_script_internal_h
#define IGUARD_script_script_internal_h

#include "taisei.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define SCRIPT_PATH_PREFIX "res/scripts"

extern struct script {
	lua_State *lstate;
} script;

int script_pcall_with_msghandler(lua_State *L, int narg, int nres);
void script_dumpstack(lua_State *L);

#endif // IGUARD_script_script_internal_h
