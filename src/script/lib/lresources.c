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
#include "resource/sprite.h"

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

static int lres_index_fallback(lua_State *L) {
	// stack:
	// #1: ref
	// #2: key
	// #3: metatable

	ResourceRef *ref = luaL_checkudata(L, 1, RESREF_NAME);
	ResourceType type = res_ref_type(*ref);

	lua_pushstring(L, "_tsprops");
	lua_rawget(L, -2);
	lua_rawgeti(L, -1, type + 1);
	lua_replace(L, 3);
	lua_settop(L, 3);
	lua_pushnil(L);

	return luaTS_indexgetter(L);
}

static int lres_index(lua_State *L) {
	luaL_getmetatable(L, RESREF_NAME);
	lua_pushcfunction(L, lres_index_fallback);
	return luaTS_indexgetter(L);
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

attr_unused
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

static void *lres_checktype(lua_State *L, int idx, ResourceType type, ResourceRef *out_ref) {
	ResourceRef *ref = luaL_checkudata(L, idx, RESREF_NAME);
	ResourceType rtype = res_ref_type(*ref);

	if(rtype != type) {
		luaL_argerror(L, idx, lua_pushfstring(L, "expected "RESREF_NAME"<%s>, got "RESREF_NAME"<%s>", res_type_idname(type), res_type_idname(rtype)));
	}

	if(out_ref) {
		*out_ref = *ref;
	}

	void *data = res_ref_data(*ref);

	if(data == NULL) {
		lua_pushfstring(L, "%s `%s` is not loaded", res_type_fullname(type), res_ref_name(*ref));
		lua_error(L);
	}

	return data;
}

static int lres_sprite_get_texture(lua_State *L) {
	Sprite *spr = lres_checktype(L, 1, RES_SPRITE, NULL);
	lres_wrapref(L, res_ref_copy(spr->tex));
	return 1;
}

static int lres_sprite_get_offset(lua_State *L) {
	Sprite *spr = lres_checktype(L, 1, RES_SPRITE, NULL);
	lua_pushnumber(L, CMPLX(spr->offset.x, spr->offset.y));
	return 1;
}

static int lres_sprite_get_extent(lua_State *L) {
	Sprite *spr = lres_checktype(L, 1, RES_SPRITE, NULL);
	lua_pushnumber(L, CMPLX(spr->extent.w, spr->extent.h));
	return 1;
}

static int lres_sprite_get_texturearea(lua_State *L) {
	Sprite *spr = lres_checktype(L, 1, RES_SPRITE, NULL);

	// TODO come up with something more efficient...
	lua_createtable(L, 2, 2);
	lua_pushstring(L, "offset");
	lua_pushnumber(L, CMPLX(spr->tex_area.x, spr->tex_area.y));
	lua_rawset(L, -3);
	lua_pushstring(L, "extent");
	lua_pushnumber(L, CMPLX(spr->tex_area.w, spr->tex_area.h));
	lua_rawset(L, -3);

	return 1;
}

static void setup_subtypes(lua_State *L) {
	lua_createtable(L, RES_NUMTYPES, 0);

	// BEGIN RES_TEXTURE
	lua_createtable(L, 0, 3);
	luaTS_setproptables(L, ((luaL_Reg[]) {
		// getters
		{ NULL },
	}), ((luaL_Reg[]) {
		// setters
		{ NULL },
	}), ((luaL_Reg[]) {
		// methods
		{ NULL },
	}));
	lua_seti(L, -2, RES_TEXTURE + 1);
	// END RES_TEXTURE

	// BEGIN RES_ANIMATION
	lua_createtable(L, 0, 3);
	luaTS_setproptables(L, ((luaL_Reg[]) {
		// getters
		{ NULL },
	}), ((luaL_Reg[]) {
		// setters
		{ NULL },
	}), ((luaL_Reg[]) {
		// methods
		{ NULL },
	}));
	lua_seti(L, -2, RES_ANIMATION + 1);
	// END RES_ANIMATION

	// BEGIN RES_SOUND
	lua_createtable(L, 0, 3);
	luaTS_setproptables(L, ((luaL_Reg[]) {
		// getters
		{ NULL },
	}), ((luaL_Reg[]) {
		// setters
		{ NULL },
	}), ((luaL_Reg[]) {
		// methods
		{ NULL },
	}));
	lua_seti(L, -2, RES_SOUND + 1);
	// END RES_SOUND

	// BEGIN RES_MUSIC
	lua_createtable(L, 0, 3);
	luaTS_setproptables(L, ((luaL_Reg[]) {
		// getters
		{ NULL },
	}), ((luaL_Reg[]) {
		// setters
		{ NULL },
	}), ((luaL_Reg[]) {
		// methods
		{ NULL },
	}));
	lua_seti(L, -2, RES_MUSIC + 1);
	// END RES_MUSIC

	// BEGIN RES_MUSICMETA
	lua_createtable(L, 0, 3);
	luaTS_setproptables(L, ((luaL_Reg[]) {
		// getters
		{ NULL },
	}), ((luaL_Reg[]) {
		// setters
		{ NULL },
	}), ((luaL_Reg[]) {
		// methods
		{ NULL },
	}));
	lua_seti(L, -2, RES_MUSICMETA + 1);
	// END RES_MUSICMETA

	// BEGIN RES_SHADEROBJ
	lua_createtable(L, 0, 3);
	luaTS_setproptables(L, ((luaL_Reg[]) {
		// getters
		{ NULL },
	}), ((luaL_Reg[]) {
		// setters
		{ NULL },
	}), ((luaL_Reg[]) {
		// methods
		{ NULL },
	}));
	lua_seti(L, -2, RES_SHADEROBJ + 1);
	// END RES_SHADEROBJ

	// BEGIN RES_SHADERPROG
	lua_createtable(L, 0, 3);
	luaTS_setproptables(L, ((luaL_Reg[]) {
		// getters
		{ NULL },
	}), ((luaL_Reg[]) {
		// setters
		{ NULL },
	}), ((luaL_Reg[]) {
		// methods
		{ NULL },
	}));
	lua_seti(L, -2, RES_SHADERPROG + 1);
	// END RES_SHADERPROG

	// BEGIN RES_MODEL
	lua_createtable(L, 0, 3);
	luaTS_setproptables(L, ((luaL_Reg[]) {
		// getters
		{ NULL },
	}), ((luaL_Reg[]) {
		// setters
		{ NULL },
	}), ((luaL_Reg[]) {
		// methods
		{ NULL },
	}));
	lua_seti(L, -2, RES_MODEL + 1);
	// END RES_MODEL

	// BEGIN RES_POSTPROCESS
	lua_createtable(L, 0, 3);
	luaTS_setproptables(L, ((luaL_Reg[]) {
		// getters
		{ NULL },
	}), ((luaL_Reg[]) {
		// setters
		{ NULL },
	}), ((luaL_Reg[]) {
		// methods
		{ NULL },
	}));
	lua_seti(L, -2, RES_POSTPROCESS + 1);
	// END RES_POSTPROCESS

	// BEGIN RES_SPRITE
	lua_createtable(L, 0, 3);
	luaTS_setproptables(L, ((luaL_Reg[]) {
		{ "texture", lres_sprite_get_texture },
		{ "offset", lres_sprite_get_offset },
		{ "extent", lres_sprite_get_extent },
		{ "size", lres_sprite_get_extent },
		{ "texturearea", lres_sprite_get_texturearea },
		{ NULL },
	}), ((luaL_Reg[]) {
		// setters
		{ NULL },
	}), ((luaL_Reg[]) {
		// methods
		{ NULL },
	}));
	lua_seti(L, -2, RES_SPRITE + 1);
	// END RES_SPRITE

	// BEGIN RES_FONT
	lua_createtable(L, 0, 3);
	luaTS_setproptables(L, ((luaL_Reg[]) {
		// getters
		{ NULL },
	}), ((luaL_Reg[]) {
		// setters
		{ NULL },
	}), ((luaL_Reg[]) {
		// methods
		{ NULL },
	}));
	lua_seti(L, -2, RES_FONT + 1);
	// END RES_FONT
}

LUAMOD_API int luaopen_resources(lua_State *L) {
	luaL_newmetatable(L, RESREF_NAME);
	luaL_setfuncs(L, ((luaL_Reg[]) {
		{ "__gc", lres_ref_gc },
		{ "__close", lres_ref_gc },
		{ "__index", lres_index },
		{ "__eq", lres_eq },
		{ "_tsprops", lres_eq },   // for type-specific properties and methods
		LPROPTABLES_PLACEHOLDERS,
		{ NULL },
	}), 0);

	luaTS_setproptables(L, ((luaL_Reg[]) {
		// getters
		{ "isvalid", lres_isvalid },
		{ "name", lres_name },
		{ "status", lres_status },
		{ "type", lres_type },
		{ "typename", lres_typename },
		{ NULL },
	}), ((luaL_Reg[]) {
		// setters
		{ NULL },
	}), ((luaL_Reg[]) {
		// methods
		// { "copy", lres_copy },
		{ NULL },
	}));

	setup_subtypes(L);
	lua_setfield(L, -2, "_tsprops");

	#define RESOURCE_TYPE(idname, capname, fullname, pathprefix) \
		{ #idname, lres_ref_##idname },
	luaL_newlib(L, ((luaL_Reg[]) {
		// { "ref", lres_ref },
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
