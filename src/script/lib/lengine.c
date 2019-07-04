/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "all.h"
#include "resource/resource.h"
#include "resource/sprite.h"
#include "resource/animation.h"
#include "resource/bgm_metadata.h"
#include "resource/model.h"
#include "resource/font.h"
#include "audio/audio.h"

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

static int lengine_res_ref_type(lua_State *L) {
	ResourceRef ref = getref(L, 1);
	lua_pushinteger(L, res_ref_type(ref));
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

static int lengine_res_ref_is_valid(lua_State *L) {
	ResourceRef ref = getref(L, 1);
	lua_pushboolean(L, res_ref_is_valid(ref));
	return 1;
}

static int lengine_res_unref_if_valid(lua_State *L) {
	ResourceRef ref = getref(L, 1);
	res_unref_if_valid(&ref, 1);
	return 0;
}

static int lengine_res_unref(lua_State *L) {
	ResourceRef ref = getref(L, 1);
	res_unref(&ref, 1);
	return 0;
}

static int lengine_res_ref_id(lua_State *L) {
	ResourceRef ref = getref(L, 1);
	lua_pushinteger(L, res_ref_id(ref));
	return 1;
}

#define GETTER_BASIC(type, prefix, suffix, field, pushfunc) \
	static int lengine_##prefix##_get_##suffix(lua_State *L) { \
		type *obj = lua_touserdata(L, 1); \
		pushfunc(L, obj->field); \
		return 1; \
	}

#define SETTER_BASIC(type, prefix, suffix, field, getfunc) \
	static int lengine_##prefix##_set_##suffix(lua_State *L) { \
		type *obj = lua_touserdata(L, 1); \
		obj->field = getfunc(L, 2); \
		return 0; \
	}

#define ACCESSORS_BASIC(type, prefix, suffix, field, pushfunc, getfunc) \
	GETTER_BASIC(type, prefix, suffix, field, pushfunc) \
	SETTER_BASIC(type, prefix, suffix, field, getfunc) \

#define GETTER_COMPLEX_SEPARATED(type, prefix, suffix, xfield, yfield) \
	static int lengine_##prefix##_get_##suffix(lua_State *L) { \
		type *obj = lua_touserdata(L, 1); \
		lua_pushnumber(L, CMPLX(obj->xfield, obj->yfield)); \
		return 1; \
	}

#define SETTER_COMPLEX_SEPARATED(type, prefix, suffix, xfield, yfield) \
	static int lengine_##prefix##_set_##suffix(lua_State *L) { \
		type *obj = lua_touserdata(L, 1); \
		complex val = luaL_checknumber(L, 2); \
		obj->xfield = l_real(val); \
		obj->yfield = l_imag(val); \
		return 0; \
	}

#define ACCESSORS_COMPLEX_SEPARATED(type, prefix, suffix, xfield, yfield) \
	GETTER_COMPLEX_SEPARATED(type, prefix, suffix, xfield, yfield) \
	SETTER_COMPLEX_SEPARATED(type, prefix, suffix, xfield, yfield) \

#define GETTER_FUNC(type, prefix, suffix, getterfunc, pushfunc) \
	static int lengine_##prefix##_get_##suffix(lua_State *L) { \
		pushfunc(L, getterfunc(lua_touserdata(L, 1))); \
		return 1; \
	}

#define CONSTRUCTOR(type, prefix, mtname, initfunc, deinitfunc) \
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

CONSTRUCTOR(Sprite, sprite, "Sprite", NULL, NULL)
ACCESSORS_COMPLEX_SEPARATED(Sprite, sprite, extent, extent.w, extent.h)
ACCESSORS_COMPLEX_SEPARATED(Sprite, sprite, offset, offset.x, offset.y)
ACCESSORS_COMPLEX_SEPARATED(Sprite, sprite, texarea_extent, tex_area.extent.w, tex_area.extent.h)
ACCESSORS_COMPLEX_SEPARATED(Sprite, sprite, texarea_offset, tex_area.offset.x, tex_area.offset.y)
ACCESSORS_BASIC(Sprite, sprite, texture_ref, tex, pushref, getref)

GETTER_BASIC(Animation, animation, sprite_count, sprite_count, lua_pushinteger)

static int lengine_animation_get_sequence_length(lua_State *L) {
	Animation *ani = lua_touserdata(L, 1);
	const char *seqname = luaL_checkstring(L, 2);

	AniSequence *seq = ht_str2ptr_get(&ani->sequences, seqname, NULL);

	if(seq == NULL) {
		luaL_argerror(L, 2, lua_pushfstring(L, "no sequence `%s` in animation", seqname));
	}

	lua_pushinteger(L, seq->length);
	return 1;
}

static int lengine_animation_get_sequence_frame(lua_State *L) {
	Animation *ani = lua_touserdata(L, 1);
	const char *seqname = luaL_checkstring(L, 2);
	lua_Integer frame = luaL_checkinteger(L, 3);

	AniSequence *seq = ht_str2ptr_get(&ani->sequences, seqname, NULL);

	if(seq == NULL) {
		luaL_argerror(L, 2, lua_pushfstring(L, "no sequence `%s` in animation", seqname));
	}

	if(frame < 1 || frame > seq->length) {
		luaL_argerror(L, 2, lua_pushfstring(L, "bad sequence frame index `%d`", frame));
	}

	AniSequenceFrame *f = seq->frames + frame - 1;
	lua_pushinteger(L, f->spriteidx + 1);
	lua_pushboolean(L, f->mirrored);
	pushref(L, ani->sprites[f->spriteidx]);
	return 3;
}

static int lengine_animation_get_sprite(lua_State *L) {
	Animation *ani = lua_touserdata(L, 1);
	lua_Integer num = luaL_checkinteger(L, 2);

	if(num < 1 || num > ani->sprite_count) {
		luaL_argerror(L, 2, lua_pushfstring(L, "bad sprite index `%d`", num));
	}

	pushref(L, ani->sprites[num - 1]);
	return 1;
}

GETTER_BASIC(Music, music, meta_ref, meta, pushref)

GETTER_BASIC(MusicMetadata, musicmeta, title, title, lua_pushstring)
GETTER_BASIC(MusicMetadata, musicmeta, artist, artist, lua_pushstring)
GETTER_BASIC(MusicMetadata, musicmeta, comment, comment, lua_pushstring)
GETTER_BASIC(MusicMetadata, musicmeta, loop_path, loop_path, lua_pushstring)
GETTER_BASIC(MusicMetadata, musicmeta, loop_point, loop_point, lua_pushnumber)

CONSTRUCTOR(Model, model, "Model", NULL, NULL)
ACCESSORS_BASIC(Model, model, vertex_array, vertex_array, lua_pushlightuserdata, lua_touserdata)
ACCESSORS_BASIC(Model, model, num_vertices, num_vertices, lua_pushinteger, luaL_checkinteger)
ACCESSORS_BASIC(Model, model, offset, offset, lua_pushinteger, luaL_checkinteger)
ACCESSORS_BASIC(Model, model, primitive, primitive, lua_pushinteger, luaL_checkinteger)
ACCESSORS_BASIC(Model, model, indexed, indexed, lua_pushboolean, lua_toboolean)

GETTER_FUNC(Font, font, ascent, font_get_ascent, lua_pushnumber)
GETTER_FUNC(Font, font, descent, font_get_descent, lua_pushnumber)
GETTER_FUNC(Font, font, lineskip, font_get_lineskip, lua_pushnumber)
GETTER_FUNC(Font, font, metrics, font_get_metrics, luaTS_pushconstlightuserdata)

#define EXPORT_FUNC(f) { #f, lengine_##f }
#define EXPORT_ACCESSORS(prefix, suffix) \
	EXPORT_FUNC(prefix##_get_##suffix), \
	EXPORT_FUNC(prefix##_set_##suffix)

#define EXPORT_CONST(c) { #c, c }

LUAMOD_API int luaopen_engine(lua_State *L) {
	luaL_newlib(L, ((luaL_Reg[]) {
		EXPORT_ACCESSORS(model, indexed),
		EXPORT_ACCESSORS(model, num_vertices),
		EXPORT_ACCESSORS(model, offset),
		EXPORT_ACCESSORS(model, primitive),
		EXPORT_ACCESSORS(model, vertex_array),
		EXPORT_ACCESSORS(sprite, extent),
		EXPORT_ACCESSORS(sprite, offset),
		EXPORT_ACCESSORS(sprite, texarea_extent),
		EXPORT_ACCESSORS(sprite, texarea_offset),
		EXPORT_ACCESSORS(sprite, texture_ref),
		EXPORT_FUNC(animation_get_sequence_frame),
		EXPORT_FUNC(animation_get_sequence_length),
		EXPORT_FUNC(animation_get_sprite),
		EXPORT_FUNC(animation_get_sprite_count),
		EXPORT_FUNC(font_get_ascent),
		EXPORT_FUNC(font_get_descent),
		EXPORT_FUNC(font_get_lineskip),
		EXPORT_FUNC(font_get_metrics),
		EXPORT_FUNC(model_new),
		EXPORT_FUNC(music_get_meta_ref),
		EXPORT_FUNC(musicmeta_get_artist),
		EXPORT_FUNC(musicmeta_get_comment),
		EXPORT_FUNC(musicmeta_get_loop_path),
		EXPORT_FUNC(musicmeta_get_loop_point),
		EXPORT_FUNC(musicmeta_get_title),
		EXPORT_FUNC(res_ref),
		EXPORT_FUNC(res_ref_copy),
		EXPORT_FUNC(res_ref_data),
		EXPORT_FUNC(res_ref_id),
		EXPORT_FUNC(res_ref_is_valid),
		EXPORT_FUNC(res_ref_name),
		EXPORT_FUNC(res_ref_status),
		EXPORT_FUNC(res_ref_type),
		EXPORT_FUNC(res_ref_wait_ready),
		EXPORT_FUNC(res_ref_wrap_external_data),
		EXPORT_FUNC(res_refs_are_equivalent),
		EXPORT_FUNC(res_unref),
		EXPORT_FUNC(res_unref_if_valid),
		EXPORT_FUNC(sprite_new),
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

		EXPORT_CONST(ALIGN_LEFT),
		EXPORT_CONST(ALIGN_CENTER),
		EXPORT_CONST(ALIGN_RIGHT),
	};

	for(int i = 0; i < ARRAY_SIZE(consts); ++i) {
		lua_pushinteger(L, consts[i].value);
		lua_setfield(L, -2, consts[i].name);
	}

	return 1;
}
