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
#include "resource/animation.h"

#define RESREF_NAME "ResourceRef"

#define RESREF_MAPPING_TABLE_NAME RESREF_NAME "Mapping"

#define RESREF_ANIMATION_PREFIX RESREF_NAME "Animation"
#define RESREF_ANIMATION_FRAMES_PROXY_NAME RESREF_ANIMATION_PREFIX "FramesProxy"
#define RESREF_ANIMATION_SEQUENCES_PROXY_NAME RESREF_ANIMATION_PREFIX "SequencesProxy"

#define RESREF_SPRITE_PREFIX RESREF_NAME "Sprite"
#define RESREF_SPRITE_TEXTUREAREA_PROXY_NAME RESREF_SPRITE_PREFIX "TextureAreaProxy"

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

static ResourceRef *lres_wrapref(lua_State *L, ResourceRef ref, bool copy_needed) {
	static_assert(sizeof(void*) >= sizeof(res_id_t), "");

	ResourceRef *refp = NULL;

	int top = lua_gettop(L);

	if(!luaL_getsubtable(L, LUA_REGISTRYINDEX, RESREF_MAPPING_TABLE_NAME)) {
		lua_createtable(L, 0, 1);
		lua_pushstring(L, "v");
		luaTS_rawsetfield(L, -2, "__mode");
		lua_setmetatable(L, -2);
		log_debug("Created " RESREF_NAME " mapping table");
	}

	void *id = (void*)res_ref_id(ref);

	if(lua_rawgetp(L, -1, id) == LUA_TUSERDATA) {
		refp = lua_touserdata(L, -1);
		assert(luaL_testudata(L, -1, RESREF_NAME) == refp);
		assert(res_refs_are_equivalent(ref, *refp));

		if(!copy_needed) {
			res_unref(&ref, 1);
		}
	} else {
		refp = lua_newuserdatauv(L, sizeof(ref), 1);

		if(copy_needed) {
			*refp = res_ref_copy(ref);
		} else {
			*refp = ref;
		}

		lua_pushvalue(L, -1);
		luaL_setmetatable(L, RESREF_NAME);
		lua_rawsetp(L, -4, id);
		lua_copy(L, -1, top + 1);
		lua_settop(L, top + 1);

		log_debug("Cached " RESREF_NAME " at %p (id: %p)", (void*)refp, id);
	}

	return refp;
}

static int lres_ref(lua_State *L) {
	ResourceType type = lres_gettype(L, 1);
	const char *name = luaL_checkstring(L, 2);
	ResourceFlags flags = lres_getflags(L, 3);
	lres_wrapref(L, res_ref(type, name, flags), false);
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

static int lres_eq(lua_State *L) {
	ResourceRef *ref1 = luaL_checkudata(L, 1, RESREF_NAME);
	ResourceRef *ref2 = luaL_checkudata(L, 2, RESREF_NAME);
	lua_pushboolean(L, res_refs_are_equivalent(*ref1, *ref2));
	return 1;
}

INLINE const char *status_str(ResourceStatus st) {
	switch(st) {
		case RES_STATUS_EXTERNAL:   return "external";
		case RES_STATUS_FAILED:     return "failed";
		case RES_STATUS_LOADED:     return "loaded";
		case RES_STATUS_LOADING:    return "loading";
		case RES_STATUS_NOT_LOADED: return "not loaded";
	}

	UNREACHABLE;
}

static int lres_tostring(lua_State *L) {
	ResourceRef *ref = luaL_checkudata(L, 1, RESREF_NAME);
	lua_pushfstring(L, RESREF_NAME "<%s> '%s' (%s): %p",
		res_type_idname(res_ref_type(*ref)),
		res_ref_name(*ref),
		status_str(res_ref_status(*ref)),
		ref
	);
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

static void *lres_checktype(lua_State *L, int idx, ResourceType type) {
	ResourceRef *ref = luaTS_checkproxiedudata(L, idx, RESREF_NAME, false);
	ResourceType rtype = res_ref_type(*ref);

	if(rtype != type) {
		luaL_argerror(L, idx, lua_pushfstring(L, "expected "RESREF_NAME"<%s>, got "RESREF_NAME"<%s>", res_type_idname(type), res_type_idname(rtype)));
	}

	void *data = res_ref_data(*ref);

	if(data == NULL) {
		lua_pushfstring(L, "%s `%s` is not loaded", res_type_fullname(type), res_ref_name(*ref));
		lua_error(L);
	}

	return data;
}

// BEGIN RES_ANIMATION

static int lres_animation_frames_get_count(lua_State *L) {
	Animation *ani = lres_checktype(L, 1, RES_ANIMATION);
	lua_pushinteger(L, ani->sprite_count);
	return 1;
}

static int lres_animation_frames_index(lua_State *L) {
	Animation *ani = lres_checktype(L, 1, RES_ANIMATION);
	int isint;
	lua_Integer idx = lua_tointegerx(L, 2, &isint);

	if(!isint) {
		lua_settop(L, 2);
		return luaTS_defaultindexgetter(L);
	}

	idx -= 1;

	if(idx < 0 || idx >= ani->sprite_count) {
		return 0;
	}

	lres_wrapref(L, ani->sprites[idx], true);
	return 1;
}

static int lres_animation_frames_init_mt(lua_State *L) {
	luaL_setfuncs(L, ((luaL_Reg[]) {
		{ "__index", lres_animation_frames_index },
		{ "__newindex", luaTS_defaultindexsetter },
		{ "__len", lres_animation_frames_get_count },
		{ NULL },
	}), 0);

	luaTS_setfunctables(L, -1, &(luaTS_funcdefs) {
		LUATS_GETTERS(
			{ "count", lres_animation_frames_get_count },
			{ NULL },
		)
	}, 0);

	return 0;
}

static int lres_animation_get_frames(lua_State *L) {
	lres_checktype(L, 1, RES_ANIMATION);
	luaTS_pushproxy(L, 1, RESREF_ANIMATION_FRAMES_PROXY_NAME, lres_animation_frames_init_mt);
	return 1;
}

#if 0
static int lres_animation_sequences_index(lua_State *L) {
	Animation *ani = lres_checktype(L, 1, RES_ANIMATION);
	const char *seqname = luaL_checkstring(L, 2);

	AniSequence *seq = ht_get(&ani->sequences, seqname, NULL);

	return 1;
}

static int lres_animation_sequences_init_mt(lua_State *L) {
	luaL_setfuncs(L, ((luaL_Reg[]) {
		{ "__index", lres_animation_sequences_index },
		{ "__newindex", luaTS_defaultindexsetter },
		{ NULL },
	}), 0);

	luaTS_setfunctables(L, -1, &(luaTS_funcdefs) { 0 }, 0);

	return 0;
}

static int lres_animation_get_sequences(lua_State *L) {
	lres_checktype(L, 1, RES_ANIMATION);
	luaTS_pushproxy(L, 1, RESREF_ANIMATION_SEQUENCES_PROXY_NAME, lres_animation_sequences_init_mt);
	return 1;
}
#endif

// END RES_ANIMATION

// BEGIN RES_SPRITE

static int lres_sprite_texturearea_get_offset(lua_State *L) {
	Sprite *spr = lres_checktype(L, 1, RES_SPRITE);
	lua_pushnumber(L, CMPLX(spr->tex_area.offset.x, spr->tex_area.offset.y));
	return 1;
}

static int lres_sprite_texturearea_get_extent(lua_State *L) {
	Sprite *spr = lres_checktype(L, 1, RES_SPRITE);
	lua_pushnumber(L, CMPLX(spr->tex_area.extent.w, spr->tex_area.extent.h));
	return 1;
}

static int lres_sprite_texturearea_init_mt(lua_State *L) {
	luaL_setfuncs(L, ((luaL_Reg[]) {
		{ "__index", luaTS_defaultindexgetter },
		{ "__newindex", luaTS_defaultindexsetter },
		{ NULL },
	}), 0);

	luaTS_setfunctables(L, -1, &(luaTS_funcdefs) {
		LUATS_GETTERS(
			{ "offset", lres_sprite_texturearea_get_offset },
			{ "extent", lres_sprite_texturearea_get_extent },
			{ NULL },
		)
	}, 0);

	return 0;
}

static int lres_sprite_get_texture(lua_State *L) {
	Sprite *spr = lres_checktype(L, 1, RES_SPRITE);
	lres_wrapref(L, spr->tex, true);
	return 1;
}

static int lres_sprite_get_offset(lua_State *L) {
	Sprite *spr = lres_checktype(L, 1, RES_SPRITE);
	lua_pushnumber(L, CMPLX(spr->offset.x, spr->offset.y));
	return 1;
}

static int lres_sprite_get_extent(lua_State *L) {
	Sprite *spr = lres_checktype(L, 1, RES_SPRITE);
	lua_pushnumber(L, CMPLX(spr->extent.w, spr->extent.h));
	return 1;
}

static int lres_sprite_get_texturearea(lua_State *L) {
	lres_checktype(L, 1, RES_SPRITE);
	luaTS_pushproxy(L, 1, RESREF_SPRITE_TEXTUREAREA_PROXY_NAME, lres_sprite_texturearea_init_mt);
	return 1;
}

// END RES_SPRITE

static void setup_subtypes(lua_State *L) {
	lua_createtable(L, RES_NUMTYPES, 0);

	// BEGIN RES_TEXTURE
	lua_createtable(L, 0, 3);
	luaTS_setfunctables(L, -1, &(luaTS_funcdefs) { 0 }, 0);
	lua_seti(L, -2, RES_TEXTURE + 1);
	// END RES_TEXTURE

	// BEGIN RES_ANIMATION
	lua_createtable(L, 0, 3);
	luaTS_setfunctables(L, -1, &(luaTS_funcdefs) {
		LUATS_GETTERS(
			{ "frames", lres_animation_get_frames },
			// { "sequences", lres_animation_get_sequences },
			{ NULL },
		)
	}, 0);
	lua_seti(L, -2, RES_ANIMATION + 1);
	// END RES_ANIMATION

	// BEGIN RES_SOUND
	lua_createtable(L, 0, 3);
	luaTS_setfunctables(L, -1, &(luaTS_funcdefs) { 0 }, 0);
	lua_seti(L, -2, RES_SOUND + 1);
	// END RES_SOUND

	// BEGIN RES_MUSIC
	lua_createtable(L, 0, 3);
	luaTS_setfunctables(L, -1, &(luaTS_funcdefs) { 0 }, 0);
	lua_seti(L, -2, RES_MUSIC + 1);
	// END RES_MUSIC

	// BEGIN RES_MUSICMETA
	lua_createtable(L, 0, 3);
	luaTS_setfunctables(L, -1, &(luaTS_funcdefs) { 0 }, 0);
	lua_seti(L, -2, RES_MUSICMETA + 1);
	// END RES_MUSICMETA

	// BEGIN RES_SHADEROBJ
	lua_createtable(L, 0, 3);
	luaTS_setfunctables(L, -1, &(luaTS_funcdefs) { 0 }, 0);
	lua_seti(L, -2, RES_SHADEROBJ + 1);
	// END RES_SHADEROBJ

	// BEGIN RES_SHADERPROG
	lua_createtable(L, 0, 3);
	luaTS_setfunctables(L, -1, &(luaTS_funcdefs) { 0 }, 0);
	lua_seti(L, -2, RES_SHADERPROG + 1);
	// END RES_SHADERPROG

	// BEGIN RES_MODEL
	lua_createtable(L, 0, 3);
	luaTS_setfunctables(L, -1, &(luaTS_funcdefs) { 0 }, 0);
	lua_seti(L, -2, RES_MODEL + 1);
	// END RES_MODEL

	// BEGIN RES_POSTPROCESS
	lua_createtable(L, 0, 3);
	luaTS_setfunctables(L, -1, &(luaTS_funcdefs) { 0 }, 0);
	lua_seti(L, -2, RES_POSTPROCESS + 1);
	// END RES_POSTPROCESS

	// BEGIN RES_SPRITE
	lua_createtable(L, 0, 3);
	luaTS_setfunctables(L, -1, &(luaTS_funcdefs) {
		LUATS_GETTERS(
			{ "texture", lres_sprite_get_texture },
			{ "offset", lres_sprite_get_offset },
			{ "extent", lres_sprite_get_extent },
			{ "size", lres_sprite_get_extent },
			{ "texturearea", lres_sprite_get_texturearea },
			{ NULL },
		)
	}, 0);
	lua_seti(L, -2, RES_SPRITE + 1);
	// END RES_SPRITE

	// BEGIN RES_FONT
	lua_createtable(L, 0, 3);
	luaTS_setfunctables(L, -1, &(luaTS_funcdefs) { 0 }, 0);
	lua_seti(L, -2, RES_FONT + 1);
	// END RES_FONT
}

LUAMOD_API int luaopen_resources(lua_State *L) {
	luaL_newmetatable(L, RESREF_NAME);
	luaL_setfuncs(L, ((luaL_Reg[]) {
		{ "__gc", lres_ref_gc },
		{ "__close", lres_ref_gc },
		{ "__index", lres_index },
		{ "__newindex", luaTS_defaultindexsetter },
		{ "__tostring", lres_tostring },
		{ "__eq", lres_eq },
		{ "_tsprops", lres_eq },   // for type-specific properties and methods
		LPROPTABLES_PLACEHOLDERS,
		{ NULL },
	}), 0);

	luaTS_setfunctables(L, -1, &(luaTS_funcdefs) {
		LUATS_GETTERS(
			{ "isvalid", lres_isvalid },
			{ "name", lres_name },
			{ "status", lres_status },
			{ "type", lres_type },
			{ "typename", lres_typename },
			{ NULL },
		)
	}, 0);

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
