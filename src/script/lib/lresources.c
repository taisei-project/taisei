/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "all.h"

#include "resource/resource.h"

#define RESREF_NAME "ResourceRef"

static ResourceType lres_gettype(lua_State *L, int idx) {
	int isint;
	lua_Integer id;

	if(lua_isstring(L, idx)) {
		const char *str = luaL_tolstring(L, idx, NULL);

		for(ResourceType t = 0; t < RES_NUMTYPES; ++t) {
			if(!strcmp(str, res_type_idname(t))) {
				return t;
			}
		}

		luaL_argerror(L, idx, lua_pushfstring(L, "invalid resource type `%s`", str));
		UNREACHABLE;
	} else if((id = lua_tointegerx(L, idx, &isint)), isint) {
		if(id < 0 || id >= RES_NUMTYPES) {
			luaL_argerror(L, idx, lua_pushfstring(L, "invalid resource type `%d`", id));
			UNREACHABLE;
		}

		return id;
	} else {
		luaL_typeerror(L, idx, "integer or string");
		UNREACHABLE;
	}
}

static ResourceFlags lres_getflags(lua_State *L, int idx) {
	return luaL_optinteger(L, idx, RESF_DEFAULT) & (RESF_OPTIONAL | RESF_LAZY);
}

static int lres_ref_gc(lua_State *L) {
	ResourceRef *ref = luaL_checkudata(L, 1, RESREF_NAME);
	res_unref_if_valid(ref, 1);
	return 0;
}

static ResourceRef *lres_wrapref(lua_State *L, ResourceRef ref) {
	ResourceRef *pref = lua_newuserdata(L, sizeof(ref));
	*pref = ref;
	luaL_setmetatable(L, RESREF_NAME);
	return pref;
}

static int lres_ref(lua_State *L) {
	ResourceType type = lres_gettype(L, 1);
	const char *name = luaL_checkstring(L, 2);
	ResourceFlags flags = lres_getflags(L, 3);
	lres_wrapref(L, res_ref(type, name, flags));
	return 1;
}

static int lres_index(lua_State *L) {
	return luaTS_indexgetter(L, RESREF_NAME);
}

static int lres_name(lua_State *L) {
	ResourceRef *ref = luaL_checkudata(L, 1, RESREF_NAME);
	lua_pushstring(L, res_ref_name(*ref));
	return 1;
}

static int lres_type(lua_State *L) {
	ResourceRef *ref = luaL_checkudata(L, 1, RESREF_NAME);
	lua_pushinteger(L, res_ref_type(*ref));
	return 1;
}

static int lres_typename(lua_State *L) {
	ResourceRef *ref = luaL_checkudata(L, 1, RESREF_NAME);
	lua_pushstring(L, res_type_idname(res_ref_type(*ref)));
	return 1;
}

static int lres_isvalid(lua_State *L) {
	ResourceRef *ref = luaL_checkudata(L, 1, RESREF_NAME);
	lua_pushboolean(L, res_ref_is_valid(*ref));
	return 1;
}

static int lres_status(lua_State *L) {
	ResourceRef *ref = luaL_checkudata(L, 1, RESREF_NAME);
	lua_pushinteger(L, res_ref_status(*ref));
	return 1;
}

static int lres_copy(lua_State *L) {
	ResourceRef *ref = luaL_checkudata(L, 1, RESREF_NAME);
	lres_wrapref(L, res_ref_copy(*ref));
	return 1;
}

static int lres_eq(lua_State *L) {
	ResourceRef *ref1 = luaL_checkudata(L, 1, RESREF_NAME);
	ResourceRef *ref2 = luaL_checkudata(L, 2, RESREF_NAME);
	lua_pushboolean(L, res_refs_are_equivalent(*ref1, *ref2));
	return 1;
}

#define RESOURCE_TYPE(idname, capname, fullname, pathprefix) \
	static int lres_ref_##idname(lua_State *L) { \
		lua_pushinteger(L, RES_##capname); \
		lua_rotate(L, 1, 1); \
		return lres_ref(L); \
	}

RESOURCE_TYPES
#undef RESOURCE_TYPE

LUAMOD_API int luaopen_resources(lua_State *L) {
	luaL_newmetatable(L, RESREF_NAME);
	luaL_setfuncs(L, ((luaL_Reg[]) {
		{ "__gc", lres_ref_gc },
		{ "__close", lres_ref_gc },
		{ "__index", lres_index },
		{ "__eq", lres_eq },
		{ LKEY_PROPSETTERS, NULL },
		{ LKEY_PROPGETTERS, NULL },
		{ LKEY_METHODS, NULL },
		{ NULL },
	}), 0);

	luaL_newlib(L, ((luaL_Reg[]) {
		{ "copy", lres_copy },
		{ NULL },
	}));
	lua_setfield(L, -2, LKEY_METHODS);

	luaL_newlib(L, ((luaL_Reg[]) {
		{ "isvalid", lres_isvalid },
		{ "name", lres_name },
		{ "status", lres_status },
		{ "type", lres_type },
		{ "typename", lres_typename },
		{ NULL },
	}));
	lua_setfield(L, -2, LKEY_PROPGETTERS);

	luaL_newlib(L, ((luaL_Reg[]) {
		{ NULL },
	}));
	lua_setfield(L, -2, LKEY_PROPSETTERS);

	#define RESOURCE_TYPE(idname, capname, fullname, pathprefix) \
		{ #idname, lres_ref_##idname },
	luaL_newlib(L, ((luaL_Reg[]) {
		{ "ref", lres_ref },
		RESOURCE_TYPES
		{ NULL },
	}));
	#undef RESOURCE_TYPE

	struct lconst { const char *name; lua_Integer value; } consts[] = {
		#define RESOURCE_TYPE(idname, capname, fullname, pathprefix) \
			{ "TYPE_" #capname, RES_##capname },
		RESOURCE_TYPES
		#undef RESOURCE_TYPE

		{ "NUM_TYPES", RES_NUMTYPES },

		{ "STATUS_NOT_LOADED", RES_STATUS_LOADED },
		{ "STATUS_LOADING", RES_STATUS_LOADING },
		{ "STATUS_LOADED", RES_STATUS_LOADED },
		{ "STATUS_FAILED", RES_STATUS_FAILED },
		{ "STATUS_EXTERNAL", RES_STATUS_EXTERNAL },

		{ "FLAGS_DEFAULT", 0 },
		{ "FLAG_OPTIONAL", RESF_OPTIONAL },
		{ "FLAG_LAZY", RESF_LAZY },
		// { "FLAG_WEAK", RESF_WEAK },
		// { "FLAG_ALIEN", RESF_ALIEN },
	};

	for(int i = 0; i < ARRAY_SIZE(consts); ++i) {
		lua_pushinteger(L, consts[i].value);
		lua_setfield(L, -2, consts[i].name);
	}

	return 1;
}
