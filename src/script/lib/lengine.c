
#include "all.h"
#include "resource/resource.h"
#include "resource/sprite.h"

static_assert(sizeof(RES_INVALID_REF.as_int) <= sizeof(lua_Integer), "");
static_assert(sizeof(res_id_t) <= sizeof(lua_Integer), "");

#define MAX_REF_SIZE 8
static_assert(sizeof(RES_INVALID_REF.as_int) <= MAX_REF_SIZE, "");

#if TAISEI_BUILDCONF_SIZEOF_PTR >= MAX_REF_SIZE
// ref fits in lightuserdata

INLINE void pushref(lua_State *L, ResourceRef ref) {
	lua_pushlightuserdata(L, (void*)ref.as_int);
}

INLINE ResourceRef getref(lua_State *L, int idx) {
	ResourceRef ref = { .as_int = (uintptr_t)lua_touserdata(L, idx) };
	return ref;
}
#else
// ref doesn't fit in lightuserdata, resort to integers

INLINE void pushref(lua_State *L, ResourceRef ref) {
	lua_pushinteger(L, ref.as_int);
}

INLINE ResourceRef getref(lua_State *L, int idx) {
	ResourceRef ref = { .as_int = luaL_checkinteger(L, idx) };
	return ref;
}
#endif

static int lengine_res_ref(lua_State *L) {
	lua_Integer res_type = luaL_checkinteger(L, 1);
	const char *name = luaL_checklstring(L, 2, NULL);
	lua_Integer res_flags = luaL_checkinteger(L, 3);
	pushref(L, res_ref(res_type, name, res_flags));
	return 1;
}

static int lengine_res_ref_wrap_external_data(lua_State *L) {
	lua_Integer res_type = luaL_checkinteger(L, 1);
	void *data = lua_touserdata(L, 1);
	pushref(L, res_ref_wrap_external_data(res_type, data));
	return 1;
}

static int lengine_res_ref_copy(lua_State *L) {
	ResourceRef ref = getref(L, 1);
	pushref(L, res_ref_copy(ref));
	return 1;
}

static int lengine_res_ref_data(lua_State *L) {
	ResourceRef ref = getref(L, 1);
	lua_pushlightuserdata(L, res_ref_data(ref));
	return 1;
}

static int lengine_res_ref_status(lua_State *L) {
	ResourceRef ref = getref(L, 1);
	lua_pushinteger(L, res_ref_status(ref));
	return 1;
}

static int lengine_res_ref_wait_ready(lua_State *L) {
	ResourceRef ref = getref(L, 1);
	lua_pushinteger(L, res_ref_wait_ready(ref));
	return 1;
}

static int lengine_res_ref_name(lua_State *L) {
	ResourceRef ref = getref(L, 1);
	lua_pushstring(L, res_ref_name(ref));
	return 1;
}

static int lengine_res_refs_are_equivalent(lua_State *L) {
	ResourceRef ref1 = getref(L, 1);
	ResourceRef ref2 = getref(L, 2);
	lua_pushboolean(L, res_refs_are_equivalent(ref1, ref2));
	return 1;
}

static int lengine_res_unref(lua_State *L) {
	ResourceRef ref = getref(L, 1);
	res_unref(&ref, 1);
	return 0;
}

#define GETTER_BASIC(prefix, suffix, type, field, pushfunc) \
	static int lengine_##prefix##_get_##suffix(lua_State *L) { \
		type *obj = lua_touserdata(L, 1); \
		pushfunc(L, obj->field); \
		return 1; \
	}

#define SETTER_BASIC(prefix, suffix, type, field, getfunc) \
	static int lengine_##prefix##_set_##suffix(lua_State *L) { \
		type *obj = lua_touserdata(L, 1); \
		obj->field = getfunc(L, 2); \
		return 0; \
	}

#define ACCESSORS_BASIC(prefix, suffix, type, field, pushfunc, getfunc) \
	GETTER_BASIC(prefix, suffix, type, field, pushfunc) \
	SETTER_BASIC(prefix, suffix, type, field, getfunc) \

#define GETTER_COMPLEX_SEPARATED(prefix, suffix, type, xfield, yfield) \
	static int lengine_##prefix##_get_##suffix(lua_State *L) { \
		type *obj = lua_touserdata(L, 1); \
		lua_pushnumber(L, CMPLX(obj->xfield, obj->yfield)); \
		return 1; \
	}

#define SETTER_COMPLEX_SEPARATED(prefix, suffix, type, xfield, yfield) \
	static int lengine_##prefix##_set_##suffix(lua_State *L) { \
		type *obj = lua_touserdata(L, 1); \
		complex val = luaL_checknumber(L, 2); \
		obj->xfield = l_real(val); \
		obj->yfield = l_imag(val); \
		return 0; \
	}

#define ACCESSORS_COMPLEX_SEPARATED(prefix, suffix, type, xfield, yfield) \
	GETTER_COMPLEX_SEPARATED(prefix, suffix, type, xfield, yfield) \
	SETTER_COMPLEX_SEPARATED(prefix, suffix, type, xfield, yfield) \

#define CONSTRUCTOR(prefix, type, mtname, initfunc, deinitfunc) \
	attr_unused static int lengine_##prefix##_gc(lua_State *L) { \
		((void (*)(type*))deinitfunc)(luaL_checkudata(L, 1, mtname)); \
		return 0; \
	} \
	\
	static int lengine_##prefix##_new(lua_State *L) { \
		type *obj = lua_newuserdatauv(L, sizeof(*obj), 0); \
		if(initfunc == NULL) { \
			memset(obj, 0, sizeof(*obj)); \
		} else { \
			((void (*)(type*))initfunc)(obj); \
		} \
		if(luaL_newmetatable(L, mtname) != 0) { \
			if(deinitfunc == NULL) { \
				luaL_setfuncs(L, ((luaL_Reg[]) { \
					{ NULL }, \
				}), 0); \
			} else { \
				luaL_setfuncs(L, ((luaL_Reg[]) { \
					{ "__gc", lengine_##prefix##_gc }, \
					{ NULL }, \
				}), 0); \
			} \
		} \
		lua_setmetatable(L, -2); \
		return 1; \
	}

CONSTRUCTOR(sprite, Sprite, "Sprite", NULL, NULL)
ACCESSORS_COMPLEX_SEPARATED(sprite, extent, Sprite, extent.w, extent.h)
ACCESSORS_COMPLEX_SEPARATED(sprite, offset, Sprite, offset.x, offset.y)
ACCESSORS_COMPLEX_SEPARATED(sprite, texarea_extent, Sprite, tex_area.extent.w, tex_area.extent.h)
ACCESSORS_COMPLEX_SEPARATED(sprite, texarea_offset, Sprite, tex_area.offset.x, tex_area.offset.y)
ACCESSORS_BASIC(sprite, texture_ref, Sprite, tex, pushref, getref)

#define EXPORT_FUNC(f) { #f, lengine_##f }
#define EXPORT_ACCESSORS(prefix, suffix) \
	EXPORT_FUNC(prefix##_get_##suffix), \
	EXPORT_FUNC(prefix##_set_##suffix)

#define EXPORT_CONST(c) { #c, c }

LUAMOD_API int luaopen_engine(lua_State *L) {
	luaL_newlib(L, ((luaL_Reg[]) {
		EXPORT_FUNC(sprite_new),
		EXPORT_ACCESSORS(sprite, extent),
		EXPORT_ACCESSORS(sprite, offset),
		EXPORT_ACCESSORS(sprite, texarea_extent),
		EXPORT_ACCESSORS(sprite, texarea_offset),
		EXPORT_ACCESSORS(sprite, texture_ref),
		EXPORT_FUNC(res_ref),
		EXPORT_FUNC(res_ref_copy),
		EXPORT_FUNC(res_ref_data),
		EXPORT_FUNC(res_ref_name),
		EXPORT_FUNC(res_ref_status),
		EXPORT_FUNC(res_ref_wait_ready),
		EXPORT_FUNC(res_ref_wrap_external_data),
		EXPORT_FUNC(res_refs_are_equivalent),
		EXPORT_FUNC(res_unref),
		{ NULL },
	}));

	struct lconst { const char *name; lua_Integer value; } consts[] = {
		#define RESOURCE_TYPE(idname, capname, fullname, pathprefix) \
			{ "RES_" #capname, RES_##capname },
		RESOURCE_TYPES
		#undef RESOURCE_TYPE

		EXPORT_CONST(RES_NUMTYPES),

		EXPORT_CONST(RES_STATUS_NOT_LOADED),
		EXPORT_CONST(RES_STATUS_LOADING),
		EXPORT_CONST(RES_STATUS_LOADED),
		EXPORT_CONST(RES_STATUS_FAILED),
		EXPORT_CONST(RES_STATUS_EXTERNAL),

		EXPORT_CONST(RESF_DEFAULT),
		EXPORT_CONST(RESF_OPTIONAL),
		EXPORT_CONST(RESF_LAZY),
		EXPORT_CONST(RESF_WEAK),
		EXPORT_CONST(RESF_ALIEN),
	};

	for(int i = 0; i < ARRAY_SIZE(consts); ++i) {
		lua_pushinteger(L, consts[i].value);
		lua_setfield(L, -2, consts[i].name);
	}

	return 1;
}
