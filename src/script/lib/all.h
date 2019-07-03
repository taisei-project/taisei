/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_script_lib_all_h
#define IGUARD_script_lib_all_h

#include "taisei.h"

#include "../script_internal.h"
#include "../luahelpers.h"
#include "../console.h"
#include "color.h"

#include <lua.h>
#include <SDL_rwops.h>

void lapi_baseext_register(lua_State *L);

#define LAPI_VIO_LIBNAME "vio"
LUAMOD_API int luaopen_vio(lua_State *L);
void lvio_wrap_rw(lua_State *L, SDL_RWops *rw);

#define LAPI_VFS_LIBNAME "vfs"
LUAMOD_API int luaopen_vfs(lua_State *L);

#define LAPI_COLOR_LIBNAME "color"
LUAMOD_API int luaopen_color(lua_State *L);
Color *lcolor_tocolor(lua_State *L, int idx, bool copy);

#define LAPI_RESOURCES_LIBNAME "resources"
LUAMOD_API int luaopen_resources(lua_State *L);

#define LAPI_ENGINE_LIBNAME "engine"
LUAMOD_API int luaopen_engine(lua_State *L);

void lapi_open_all(lua_State *L);

#endif // IGUARD_script_lib_all_h
