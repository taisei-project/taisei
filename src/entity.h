/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "known_entities.h"
#include "list.h"
#include "util/geometry.h"
#include "util/macrohax.h"

#define LAYER_LOW_BITS 16
#define LAYER_LOW_MASK ((1 << LAYER_LOW_BITS) - 1)
typedef uint32_t drawlayer_t;
typedef uint16_t drawlayer_low_t;

typedef enum DrawLayerID {
	LAYER_ID_NONE,
	#define LAYER(x) LAYER_ID_##x,
	#include "drawlayers.inc.h"
} DrawLayerID;

typedef enum DrawLayer {
	#define LAYER(x) LAYER_##x = (LAYER_ID_##x << LAYER_LOW_BITS),
	#include "drawlayers.inc.h"
} DrawLayer;

// NOTE: you can bit-or a drawlayer_low_t value with one of the LAYER_x constants
// for sub-layer ordering.

#define ENT_EMIT_TYPEDEFS(typename, ...) typedef struct typename typename;
ENTITIES(ENT_EMIT_TYPEDEFS,)
#undef ENT_EMIT_TYPEDEFS

#define ENT_TYPE_ID(typename) ENT_TYPEID_##typename

typedef enum EntityType {
	_ENT_TYPE_ENUM_BEGIN,
	#define ENT_EMIT_ENUMS(typename, ...) ENT_TYPE_ID(typename),
	ENTITIES(ENT_EMIT_ENUMS,)
	#undef ENT_EMIT_ENUMS
	_ENT_TYPE_ENUM_END,
} EntityType;

typedef struct EntityInterface EntityInterface;
typedef struct EntityListNode EntityListNode;
typedef struct BoxedEntity BoxedEntity;

typedef enum DamageType {
	DMG_UNDEFINED,
	DMG_ENEMY_SHOT,
	DMG_ENEMY_COLLISION,
	DMG_PLAYER_SHOT,
	DMG_PLAYER_BOMB,
	DMG_PLAYER_DISCHARGE,
} DamageType;

#define DAMAGETYPE_IS_PLAYER(dmg) (\
	(dmg) == DMG_PLAYER_SHOT || \
	(dmg) == DMG_PLAYER_BOMB || \
	(dmg) == DMG_PLAYER_DISCHARGE \
)

typedef enum DamageResult {
	DMG_RESULT_OK,
	DMG_RESULT_IMMUNE,
	DMG_RESULT_INAPPLICABLE,
} DamageResult;

typedef struct DamageInfo {
	float amount;
	DamageType type;
} DamageInfo;

typedef void (*EntityDrawFunc)(EntityInterface *ent);
typedef bool (*EntityPredicate)(EntityInterface *ent);
typedef DamageResult (*EntityDamageFunc)(EntityInterface *target, const DamageInfo *damage);
typedef void (*EntityDrawHookCallback)(EntityInterface *ent, void *arg);
typedef void (*EntityAreaDamageCallback)(EntityInterface *ent, cmplx ent_origin, void *arg);

#define ENTITY_INTERFACE_BASE(typename) struct { \
	LIST_INTERFACE(typename); \
	EntityDrawFunc draw_func; \
	EntityDamageFunc damage_func; \
	drawlayer_t draw_layer; \
	uint32_t spawn_id; \
	uint index; \
	EntityType type; \
}

#define ENTITY_INTERFACE(typename) union { \
	EntityInterface entity_interface; \
	ENTITY_INTERFACE_BASE(typename); \
}

#define ENTITY_INTERFACE_NAMED(typename, name) union { \
	LIST_INTERFACE(typename); \
	EntityInterface entity_interface; \
	EntityInterface name; \
}

struct EntityInterface {
	ENTITY_INTERFACE_BASE(EntityInterface);
};

#define DEFINE_ENTITY_TYPE(typename, ...) \
	struct typename { \
		ENTITY_INTERFACE_NAMED(typename, ent); \
		struct __VA_ARGS__; \
	}

INLINE const char *ent_type_name(EntityType type) {
	switch(type) {
		#define ENT_HANDLE_CASE(typename, ...) case ENT_TYPE_ID(typename): return #typename;
		ENTITIES(ENT_HANDLE_CASE,)
		#undef ENT_HANDLE_CASE
		default: return "<INVALID>";
	}
}

#define ENT_CAST(ent, typename) ({ \
	auto _ent = ent; \
	assert(_ent->type == ENT_TYPE_ID(typename)); \
	UNION_CAST(EntityInterface*, typename*, _ent); \
})

void ent_init(void);
void ent_shutdown(void);
void ent_register(EntityInterface *ent, EntityType type) attr_nonnull(1);
void ent_unregister(EntityInterface *ent) attr_nonnull(1);
void ent_draw(EntityPredicate predicate);
DamageResult ent_damage(EntityInterface *ent, const DamageInfo *damage) attr_nonnull(1, 2);
void ent_area_damage(cmplx origin, float radius, const DamageInfo *damage, EntityAreaDamageCallback callback, void *callback_arg) attr_nonnull(3);
void ent_area_damage_ellipse(Ellipse ellipse, const DamageInfo *damage, EntityAreaDamageCallback callback, void *callback_arg) attr_nonnull(2);

void ent_hook_pre_draw(EntityDrawHookCallback callback, void *arg);
void ent_unhook_pre_draw(EntityDrawHookCallback callback);
void ent_hook_post_draw(EntityDrawHookCallback callback, void *arg);
void ent_unhook_post_draw(EntityDrawHookCallback callback);

struct BoxedEntity {
	EntityInterface *ent;
	uint_fast32_t spawn_id;
};

attr_nonnull_all
INLINE BoxedEntity _ent_box_Entity(EntityInterface *ent) {
	return (BoxedEntity) { .ent = ent, .spawn_id = ent->spawn_id } ;
}

INLINE EntityInterface *_ent_unbox_Entity(BoxedEntity box) {
	EntityInterface *e = box.ent;

	if(e && e->spawn_id == box.spawn_id) {
		return e;
	}

	return NULL;
}

#define ENT_EMIT_BOX_DEFS(typename, ...) \
	typedef union Boxed##typename { \
		BoxedEntity as_generic; \
		struct { \
			typename *ent; \
			uint_fast32_t spawn_id; \
		}; \
	} Boxed##typename; \
	attr_nonnull_all INLINE Boxed##typename _ent_box_##typename(typename *ent) { \
		EntityInterface *generic = UNION_CAST(typename*, EntityInterface*, ent); \
		assert(generic->type == ENT_TYPE_ID(typename)); \
		return (Boxed##typename) { .as_generic = _ent_box_Entity(generic) }; \
	} \
	INLINE typename *_ent_unbox_##typename(Boxed##typename box) { \
		EntityInterface *generic = _ent_unbox_Entity(box.as_generic); \
		if(!generic) { return NULL; } \
		assert(generic->type == ENT_TYPE_ID(typename)); \
		return UNION_CAST(EntityInterface*, typename*, generic); \
	} \
	INLINE Boxed##typename _ent_boxed_passthrough_helper_##typename(Boxed##typename box) { return box; } \
	INLINE typename *_ent_unboxed_passthrough_helper_##typename(typename *ent) { return ent; }

ENTITIES(ENT_EMIT_BOX_DEFS,)
#undef ENT_EMIT_BOX_DEFS

INLINE BoxedEntity _ent_boxed_passthrough_helper_Entity(BoxedEntity box) { return box; }
INLINE EntityInterface *_ent_unboxed_passthrough_helper_Entity(EntityInterface *ent) { return ent; }

#define ENT_HANDLE_UNBOXED_DISPATCH(typename, func_prefix) \
	typename*: func_prefix##typename,

#define ENT_UNBOXED_DISPATCH_TABLE(func_prefix) \
	ENTITIES(ENT_HANDLE_UNBOXED_DISPATCH, func_prefix) \
	EntityInterface*: func_prefix##Entity

#define ENT_HANDLE_BOXED_DISPATCH(typename, func_prefix) \
	Boxed##typename: func_prefix##typename,

#define ENT_BOXED_DISPATCH_TABLE(func_prefix) \
	ENTITIES(ENT_HANDLE_BOXED_DISPATCH, func_prefix) \
	BoxedEntity: func_prefix##Entity

#define ENT_UNBOXED_DISPATCH_FUNCTION(func_prefix, ...) \
	_Generic((MACROHAX_FIRST(__VA_ARGS__)), \
		ENT_UNBOXED_DISPATCH_TABLE(func_prefix) \
	)(MACROHAX_EXPAND(__VA_ARGS__))

#define ENT_BOXED_DISPATCH_FUNCTION(func_prefix, ...) \
	_Generic((MACROHAX_FIRST(__VA_ARGS__)), \
		ENT_BOXED_DISPATCH_TABLE(func_prefix) \
	)(MACROHAX_EXPAND(__VA_ARGS__))

#define ENT_MIXED_DISPATCH_FUNCTION(func_prefix_unboxed, func_prefix_boxed, ...) \
	_Generic((MACROHAX_FIRST(__VA_ARGS__)), \
		ENT_UNBOXED_DISPATCH_TABLE(func_prefix_unboxed), \
		ENT_BOXED_DISPATCH_TABLE(func_prefix_boxed) \
	)(MACROHAX_EXPAND(__VA_ARGS__))

#define ENT_BOX(ent) ENT_UNBOXED_DISPATCH_FUNCTION(_ent_box_, ent)
#define ENT_BOX_OR_PASSTHROUGH(ent) ENT_MIXED_DISPATCH_FUNCTION(_ent_box_, _ent_boxed_passthrough_helper_, ent)

#define ENT_UNBOX(box) ENT_BOXED_DISPATCH_FUNCTION(_ent_unbox_, box)
#define ENT_UNBOX_OR_PASSTHROUGH(ent) ENT_MIXED_DISPATCH_FUNCTION(_ent_unboxed_passthrough_helper_, _ent_unbox_, ent)

typedef struct BoxedEntityArray {
	BoxedEntity *array;
	uint capacity;
	uint size;
} BoxedEntityArray;

#define ENT_EMIT_ARRAY_DEFS(typename, ...) \
	typedef union Boxed##typename##Array { \
		BoxedEntityArray as_generic_UNSAFE; \
		struct { \
			Boxed##typename *array; \
			uint capacity; \
			uint size; \
		}; \
	} Boxed##typename##Array; \
	INLINE int _ent_array_add_##typename(Boxed##typename box, Boxed##typename##Array *a) { \
		return _ent_array_add_BoxedEntity(box.as_generic, &a->as_generic_UNSAFE); \
	} \
	INLINE int _ent_array_add_firstfree_##typename(Boxed##typename box, Boxed##typename##Array *a) { \
		return _ent_array_add_firstfree_BoxedEntity(box.as_generic, &a->as_generic_UNSAFE); \
	} \
	INLINE void _ent_array_compact_##typename(Boxed##typename##Array *a) { \
		_ent_array_compact_Entity(&a->as_generic_UNSAFE); \
	}

void _ent_array_compact_Entity(BoxedEntityArray *a);

INLINE int _ent_array_add_BoxedEntity(BoxedEntity box, BoxedEntityArray *a) {
	assert(a->size < a->capacity);
	int i = a->size++;
	a->array[i] = box;
	return i;
}

INLINE int _ent_array_add_Entity(struct EntityInterface *ent, BoxedEntityArray *a) {
	return _ent_array_add_BoxedEntity(ENT_BOX(ent), a);
}

int _ent_array_add_firstfree_BoxedEntity(BoxedEntity box, BoxedEntityArray *a);

INLINE int _ent_array_add_firstfree_Entity(
	struct EntityInterface *ent, BoxedEntityArray *a
) {
	return _ent_array_add_firstfree_BoxedEntity(ENT_BOX(ent), a);
}

ENTITIES(ENT_EMIT_ARRAY_DEFS,)
#undef ENT_EMIT_ARRAY_DEFS

#define ENT_ARRAY_ADD(_array, _ent) \
	ENT_BOXED_DISPATCH_FUNCTION(_ent_array_add_, ENT_BOX_OR_PASSTHROUGH(_ent), _array)
#define ENT_ARRAY_ADD_FIRSTFREE(_array, _ent) \
	ENT_BOXED_DISPATCH_FUNCTION(_ent_array_add_firstfree_, ENT_BOX_OR_PASSTHROUGH(_ent), _array)
#define ENT_ARRAY_GET_BOXED(_array, _index) ((_array)->array[_index])
#define ENT_ARRAY_GET(_array, _index) ENT_UNBOX(ENT_ARRAY_GET_BOXED(_array, _index))
#define ENT_ARRAY_COMPACT(_array) \
	_Generic((_array)->array[0], \
		ENT_BOXED_DISPATCH_TABLE(_ent_array_compact_) \
	)(_array)

#define DECLARE_ENT_ARRAY(_ent_type, _name, _capacity) \
	Boxed##_ent_type##Array _name; \
	_name.size = 0; \
	_name.capacity = _capacity; \
	Boxed##_ent_type _ent_array_data##_name[_name.capacity]; \
	_name.array = _ent_array_data##_name

#define ENT_ARRAY(_typename, _capacity) \
	((Boxed##_typename##Array) { .array = (Boxed##_typename[_capacity]) {}, .capacity = (_capacity), .size = 0 })

#define _ent_array_iterator MACROHAX_ADDLINENUM(_ent_array_iterator)
#define _ent_array_temp MACROHAX_ADDLINENUM(_ent_array_temp)

#define ENT_ARRAY_FOREACH(_array, _var, ...) do { \
	for(uint _ent_array_iterator = 0; _ent_array_iterator < (_array)->size; ++_ent_array_iterator) { \
		void *_ent_array_temp = ENT_ARRAY_GET((_array), _ent_array_iterator); \
		if(_ent_array_temp != NULL) { \
			_var = _ent_array_temp; \
			__VA_ARGS__ \
		} \
	} \
} while(0)

#define ENT_ARRAY_FOREACH_COUNTER(_array, _cntr_var, _ent_var, ...) do { \
	for(uint _ent_array_iterator = 0; _ent_array_iterator < (_array)->size; ++_ent_array_iterator) { \
		void *_ent_array_temp = ENT_ARRAY_GET((_array), _ent_array_iterator); \
		if(_ent_array_temp != NULL) { \
			_cntr_var = _ent_array_iterator; \
			_ent_var = _ent_array_temp; \
			__VA_ARGS__ \
		} \
	} \
} while(0)

#define ENT_ARRAY_CLEAR(_array) ((_array)->size = 0)
