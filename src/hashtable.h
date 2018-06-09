/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <stdlib.h>

#include "list.h"

#define HT_MIN_SIZE 16

typedef struct Hashtable Hashtable;
typedef struct HashtableIterator HashtableIterator;
typedef struct HashtableStats HashtableStats;
typedef union HashtableValue HashtableValue;
typedef uint32_t hash_t;

// This is what happens when your programming language doesn't have templates/generics.
// And when you're too stubborn to use the transparent unions extension.
// It's slightly more readable than whatever it generates.

#define _HT_CTYPE(typename) _ht_##typename##_t

typedef void*          _HT_CTYPE(pointer);
typedef const void*    _HT_CTYPE(cpointer);
typedef uint64_t       _HT_CTYPE(uint64);
typedef int64_t        _HT_CTYPE(int64);
typedef double         _HT_CTYPE(float64);
typedef HashtableValue _HT_CTYPE(vunion);

union HashtableValue {
	_HT_CTYPE(pointer) pointer;
	_HT_CTYPE(cpointer) cpointer;
	_HT_CTYPE(uint64) uint64;
	_HT_CTYPE(int64) int64;
	_HT_CTYPE(float64) float64;
};

#define HT_NULL_VAL ((HashtableValue) { .pointer = 0 })
#define HT_IS_NULL(val) ((val).pointer == NULL)

#define _HT_INLINE static inline attr_must_inline

#define _HT_VALUE_TOUNION(type, val) ((HashtableValue) { .type = (val) })
#define _HT_VALUE_pointer(val) _HT_VALUE_TOUNION(pointer, val)
#define _HT_VALUE_cpointer(val) _HT_VALUE_TOUNION(cpointer, val)
#define _HT_VALUE_uint64(val) _HT_VALUE_TOUNION(uint64, val)
#define _HT_VALUE_int64(val) _HT_VALUE_TOUNION(int64, val)
#define _HT_VALUE_float64(val) _HT_VALUE_TOUNION(float64, val)
#define _HT_VALUE_vunion(val) (val)
#define _HT_VALUE(type, val) _HT_VALUE_##type(val)

#define _HT_EXTRACT_VALUE_FIELD(type, val) (val).type
#define _HT_EXTRACT_VALUE_pointer(val) (void*)_HT_EXTRACT_VALUE_FIELD(pointer, val)
#define _HT_EXTRACT_VALUE_cpointer(val) (void*)_HT_EXTRACT_VALUE_FIELD(cpointer, val)
#define _HT_EXTRACT_VALUE_uint64(val) _HT_EXTRACT_VALUE_FIELD(uint64, val)
#define _HT_EXTRACT_VALUE_int64(val) _HT_EXTRACT_VALUE_FIELD(int64, val)
#define _HT_EXTRACT_VALUE_float64(val) _HT_EXTRACT_VALUE_FIELD(float64, val)
#define _HT_EXTRACT_VALUE_vunion(val) (val)
#define _HT_EXTRACT_VALUE(type, val) _HT_EXTRACT_VALUE_##type(val)

#define _HT_GENERIC_MAP(map_macro, map_macro_default, ...) \
	map_macro(HashtableValue, vunion,   __VA_ARGS__) \
	map_macro(int8_t,         int64,    __VA_ARGS__) \
	map_macro(uint8_t,        uint64,   __VA_ARGS__) \
	map_macro(int16_t,        int64,    __VA_ARGS__) \
	map_macro(uint16_t,       uint64,   __VA_ARGS__) \
	map_macro(int32_t,        int64,    __VA_ARGS__) \
	map_macro(uint32_t,       uint64,   __VA_ARGS__) \
	map_macro(int64_t,        int64,    __VA_ARGS__) \
	map_macro(uint64_t,       uint64,   __VA_ARGS__) \
	map_macro(float,          float64,  __VA_ARGS__) \
	map_macro(double,         float64,  __VA_ARGS__) \
	map_macro(const char*,    cpointer, __VA_ARGS__) \
	map_macro_default(pointer, __VA_ARGS__)

#define _HT_GENERIC_MM(ctype, mapped, basename) \
	ctype : basename ## _ ## mapped,

#define _HT_GENERIC_MM_DEFAULT(mapped, basename) \
	default : basename ## _ ## mapped

#define _HT_GENERIC(basename, key) _Generic((key), \
	_HT_GENERIC_MAP(_HT_GENERIC_MM, _HT_GENERIC_MM_DEFAULT, basename) \
)

#define _HT_GENERIC_ID() _HT_GENERIC

#define EMPTY(...)
#define DEFER(...) __VA_ARGS__ EMPTY()
#define EXPAND(...) __VA_ARGS__

#define _HT_GENERIC_2_MM(ctype, mapped, basename, key2) \
	ctype : DEFER(_HT_GENERIC_ID)()(basename ## _ ## mapped, key2),

#define _HT_GENERIC_2_DEFAULT(mapped, basename, key2) \
	default : DEFER(_HT_GENERIC_ID)()(basename ## _ ## mapped, key2)

#define _HT_GENERIC_2_(basename, key1, key2) _Generic((key1), \
	_HT_GENERIC_MAP(_HT_GENERIC_2_MM, _HT_GENERIC_2_DEFAULT, basename, key2) \
)

#define _HT_GENERIC_2(...) EXPAND(_HT_GENERIC_2_(__VA_ARGS__))

#define _HT_INLINE_DEF(basename, typename) \
	_HT_INLINE _HT_INLINE_RETURNTYPE(typename) basename##_##typename _HT_INLINE_ARGS(typename) { _HT_INLINE_BODY(typename) }

#define _HT_INLINE_DEFS(basename) \
	_HT_INLINE_DEF(basename, pointer) \
	_HT_INLINE_DEF(basename, cpointer) \
	_HT_INLINE_DEF(basename, uint64) \
	_HT_INLINE_DEF(basename, int64) \
	_HT_INLINE_DEF(basename, float64) \

#define _HT_INLINE_DEF_2(basename, type1, type2) \
	_HT_INLINE _HT_INLINE_RETURNTYPE(type1, type2) basename##_##type1##_##type2 _HT_INLINE_ARGS(type1, type2) { _HT_INLINE_BODY(type1, type2) }

#define _HT_INLINE_DEFS_2_FORTYPE(basename, type) \
	_HT_INLINE_DEF_2(basename, vunion, type) \
	_HT_INLINE_DEF_2(basename, type, vunion) \
	_HT_INLINE_DEF_2(basename, type, pointer) \
	_HT_INLINE_DEF_2(basename, type, cpointer) \
	_HT_INLINE_DEF_2(basename, type, uint64) \
	_HT_INLINE_DEF_2(basename, type, int64) \
	_HT_INLINE_DEF_2(basename, type, float64) \

#define _HT_INLINE_DEFS_2(basename) \
	_HT_INLINE_DEFS_2_FORTYPE(basename, pointer) \
	_HT_INLINE_DEFS_2_FORTYPE(basename, cpointer) \
	_HT_INLINE_DEFS_2_FORTYPE(basename, uint64) \
	_HT_INLINE_DEFS_2_FORTYPE(basename, int64) \
	_HT_INLINE_DEFS_2_FORTYPE(basename, float64) \

struct HashtableStats {
	uint free_buckets;
	uint collisions;
	uint max_per_bucket;
	uint num_elements;
};

typedef bool (*HTCmpFunc)(HashtableValue key1, HashtableValue key2);
typedef hash_t (*HTHashFunc)(HashtableValue key);
typedef void (*HTCopyFunc)(HashtableValue *dstkey, HashtableValue srckey);
typedef void (*HTFreeFunc)(HashtableValue key);
typedef void* (*HTIterCallback)(HashtableValue key, HashtableValue data, void *arg);

Hashtable* hashtable_new(HTCmpFunc cmp_func, HTHashFunc hash_func, HTCopyFunc copy_func, HTFreeFunc free_func)
	attr_nonnull(2);

void hashtable_free(Hashtable *ht);

HashtableValue _hashtable_get_vunion(Hashtable *ht, HashtableValue key)
	attr_hot attr_nonnull(1);
#undef _HT_INLINE_RETURNTYPE
#undef _HT_INLINE_ARGS
#undef _HT_INLINE_BODY
#define _HT_INLINE_RETURNTYPE(T) HashtableValue
#define _HT_INLINE_ARGS(T) (Hashtable *ht, const _HT_CTYPE(T) key)
#define _HT_INLINE_BODY(T) return _hashtable_get_vunion(ht, _HT_VALUE(T, key));
_HT_INLINE_DEFS(_hashtable_get)
#define hashtable_get(ht, key) _HT_GENERIC(_hashtable_get, key)(ht, key)

HashtableValue _hashtable_get_unsafe_vunion(Hashtable *ht, HashtableValue key)
	attr_hot attr_nonnull(1);
#undef _HT_INLINE_RETURNTYPE
#undef _HT_INLINE_ARGS
#undef _HT_INLINE_BODY
#define _HT_INLINE_RETURNTYPE(T) HashtableValue
#define _HT_INLINE_ARGS(T) (Hashtable *ht, const _HT_CTYPE(T) key)
#define _HT_INLINE_BODY(T) return _hashtable_get_unsafe_vunion(ht, _HT_VALUE(T, key));
_HT_INLINE_DEFS(_hashtable_get_unsafe)
#define hashtable_get_unsafe(ht, key) _HT_GENERIC(_hashtable_get_unsafe, key)(ht, key)

void _hashtable_set_vunion_vunion(Hashtable *ht, HashtableValue key, HashtableValue data)
	attr_nonnull(1);
#undef _HT_INLINE_RETURNTYPE
#undef _HT_INLINE_ARGS
#undef _HT_INLINE_BODY
#define _HT_INLINE_RETURNTYPE(T,S) void
#define _HT_INLINE_ARGS(T,S) (Hashtable *ht, const _HT_CTYPE(T) key, const _HT_CTYPE(S) data)
#define _HT_INLINE_BODY(T,S) _hashtable_set_vunion_vunion(ht, _HT_VALUE(T, key), _HT_VALUE(S, data));
_HT_INLINE_DEFS_2(_hashtable_set)
#define hashtable_set(ht, key, data) _HT_GENERIC_2(_hashtable_set, key, data)(ht, key, data)

void _hashtable_unset_vunion(Hashtable *ht, HashtableValue key)
	attr_nonnull(1);
#undef _HT_INLINE_RETURNTYPE
#undef _HT_INLINE_ARGS
#undef _HT_INLINE_BODY
#define _HT_INLINE_RETURNTYPE(T) void
#define _HT_INLINE_ARGS(T) (Hashtable *ht, const _HT_CTYPE(T) key)
#define _HT_INLINE_BODY(T) _hashtable_unset_vunion(ht, _HT_VALUE(T, key));
_HT_INLINE_DEFS(_hashtable_unset)
#define hashtable_unset(ht, key) _HT_GENERIC(_hashtable_unset, key)(ht, key)

bool _hashtable_try_set_vunion_vunion(Hashtable *ht, HashtableValue key, HashtableValue data, HashtableValue (*datafunc)(HashtableValue), HashtableValue *val)
	attr_nonnull(1);
#undef _HT_INLINE_RETURNTYPE
#undef _HT_INLINE_ARGS
#undef _HT_INLINE_BODY
#define _HT_INLINE_RETURNTYPE(T,S) bool
#define _HT_INLINE_ARGS(T,S) (Hashtable *ht, const _HT_CTYPE(T) key, const _HT_CTYPE(S) data, HashtableValue (*datafunc)(HashtableValue), _HT_CTYPE(S) *val)
#define _HT_INLINE_BODY(T,S) \
	if(val == NULL) { \
		return _hashtable_try_set_vunion_vunion(ht, _HT_VALUE(T, key), _HT_VALUE(S, data), datafunc, NULL); \
	} \
	HashtableValue _val; \
	bool result = _hashtable_try_set_vunion_vunion(ht, _HT_VALUE(T, key), _HT_VALUE(S, data), datafunc, &_val); \
	*val = _HT_EXTRACT_VALUE(S, _val); \
	return result;
_HT_INLINE_DEFS_2(_hashtable_try_set)
#define hashtable_try_set(ht, key, data, datafunc, val) _HT_GENERIC_2(_hashtable_try_set, key, data)(ht, key, data, datafunc, val)

void hashtable_unset_all(Hashtable *ht);
void* hashtable_foreach(Hashtable *ht, HTIterCallback callback, void *arg);

// THIS IS NOT THREAD-SAFE. You have to use hashtable_lock/unlock to make it so.
HashtableIterator* hashtable_iter(Hashtable *ht);

bool _hashtable_iter_next_vunion_vunion(HashtableIterator *iter, HashtableValue *out_key, HashtableValue *out_data) attr_nonnull(1);
#undef _HT_INLINE_RETURNTYPE
#undef _HT_INLINE_ARGS
#undef _HT_INLINE_BODY
#define _HT_INLINE_RETURNTYPE(T,S) bool
#define _HT_INLINE_ARGS(T,S) (HashtableIterator *iter, _HT_CTYPE(T) *out_key, _HT_CTYPE(S) *out_data)
#define _HT_INLINE_BODY(T,S) \
	HashtableValue _key, _data; \
	bool result = _hashtable_iter_next_vunion_vunion(iter, &_key, &_data); \
	if(out_key) *out_key = _HT_EXTRACT_VALUE(T, _key); \
	if(out_data) *out_data = _HT_EXTRACT_VALUE(S, _data); \
	return result;
_HT_INLINE_DEFS_2(_hashtable_iter_next)
#define hashtable_iter_next(iter, out_key, out_data) _HT_GENERIC_2(_hashtable_iter_next, *(out_key), *(out_data))(iter, out_key, out_data)

bool hashtable_cmpfunc_string(HashtableValue str1, HashtableValue str2) attr_hot;
hash_t hashtable_hashfunc_string(HashtableValue vstr) attr_hot;
hash_t hashtable_hashfunc_string_sse42(HashtableValue vstr) attr_hot;
void hashtable_copyfunc_string(HashtableValue *dst, HashtableValue src);
void hashtable_freefunc_free(HashtableValue val);
#define hashtable_freefunc_string hashtable_freefunc_free
Hashtable* hashtable_new_stringkeys(void);

void* hashtable_iter_free_data(HashtableValue key, HashtableValue data, void *arg);
bool hashtable_cmpfunc_ptr(HashtableValue p1, HashtableValue p2);
void hashtable_copyfunc_ptr(HashtableValue *dst, HashtableValue src);
bool hashtable_cmpfunc_raw(HashtableValue p1, HashtableValue p2);
void hashtable_copyfunc_raw(HashtableValue *dst, HashtableValue src);
hash_t hashtable_hashfunc_int(HashtableValue val);

#define hashtable_hashfunc_ptr hashtable_hashfunc_int

void hashtable_print_stringkeys(Hashtable *ht);
size_t hashtable_get_approx_overhead(Hashtable *ht);
void hashtable_get_stats(Hashtable *ht, HashtableStats *stats);

void hashtable_lock(Hashtable *ht);
void hashtable_unlock(Hashtable *ht);
