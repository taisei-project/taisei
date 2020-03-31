/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_entity_h
#define IGUARD_entity_h

#include "taisei.h"

#include "objectpool.h"
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

typedef struct CustomEntity CustomEntity;

#define ENT_TYPES \
	ENT_TYPE(Projectile, ENT_PROJECTILE) \
	ENT_TYPE(Laser, ENT_LASER) \
	ENT_TYPE(Enemy, ENT_ENEMY) \
	ENT_TYPE(Boss, ENT_BOSS) \
	ENT_TYPE(Player, ENT_PLAYER) \
	ENT_TYPE(Item, ENT_ITEM) \
	ENT_TYPE(CustomEntity, ENT_CUSTOM) \

typedef enum EntityType {
	_ENT_TYPE_ENUM_BEGIN,
	#define ENT_TYPE(typename, id) id, _ENT_TYPEID_##typename = id,
	ENT_TYPES
	#undef ENT_TYPE
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
	EntityType type; \
	EntityDrawFunc draw_func; \
	EntityDamageFunc damage_func; \
	drawlayer_t draw_layer; \
	uint32_t spawn_id; \
	uint index; \
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

struct CustomEntity {
	ENTITY_INTERFACE(CustomEntity);
};

INLINE const char *ent_type_name(EntityType type) {
	switch(type) {
		#define ENT_TYPE(typename, id) case id: return #id;
		ENT_TYPES
		#undef ENT_TYPE
		default: return "ENT_INVALID";
	}
}

#define ENT_TYPE_ID(typename) (_ENT_TYPEID_##typename)

#ifdef USE_GNU_EXTENSIONS
	#define _internal_ENT_CAST(ent, typename, ent_type_id) (__extension__ ({ \
		__auto_type _ent = ent; \
		static_assert(__builtin_types_compatible_p(EntityInterface, __typeof__(*(_ent))), \
			"Expression is not an EntityInterface pointer"); \
		static_assert(__builtin_types_compatible_p(EntityInterface, __typeof__(((typename){}).entity_interface)), \
			#typename " doesn't implement EntityInterface"); \
		static_assert(__builtin_offsetof(typename, entity_interface) == 0, \
			"entity_interface has non-zero offset in " #typename); \
		IF_DEBUG(if(_ent && _ent->type != ent_type_id) { \
			log_fatal("Invalid entity cast from %s to " #typename, ent_type_name(_ent->type)); \
		}); \
		CASTPTR_ASSUME_ALIGNED(_ent, typename); \
	}))
#else
	#define _internal_ENT_CAST(ent, typename, ent_type_id) CASTPTR_ASSUME_ALIGNED(ent, typename)
#endif

#define ENT_CAST(ent, typename) _internal_ENT_CAST(ent, typename, ENT_TYPE_ID(typename))
#define ENT_CAST_CUSTOM(ent, typename) _internal_ENT_CAST(ent, typename, _ENT_TYPEID_CustomEntity)

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
	alignas(alignof(void*)) uintptr_t ent;  // not an actual pointer to avert the temptation to use it directly.
	uint_fast32_t spawn_id;
};

BoxedEntity _ent_box_Entity(EntityInterface *ent);
EntityInterface *_ent_unbox_Entity(BoxedEntity box);

#define ENT_TYPE(typename, id) \
	typedef union Boxed##typename { \
		BoxedEntity as_generic; \
		struct { \
			uintptr_t ent; \
			uint_fast32_t spawn_id; \
		}; \
	} Boxed##typename; \
	struct typename; \
	Boxed##typename _ent_box_##typename(struct typename *ent); \
	struct typename *_ent_unbox_##typename(Boxed##typename box); \
	INLINE Boxed##typename _ent_boxed_passthrough_helper_##typename(Boxed##typename box) { return box; }

ENT_TYPES
#undef ENT_TYPE

INLINE BoxedEntity _ent_boxed_passthrough_helper_Entity(BoxedEntity box) { return box; }

#define ENT_UNBOXED_DISPATCH_TABLE(func_prefix) \
	struct Projectile*: func_prefix##Projectile, \
	struct Laser*: func_prefix##Laser, \
	struct Enemy*: func_prefix##Enemy, \
	struct Boss*: func_prefix##Boss, \
	struct Player*: func_prefix##Player, \
	struct Item*: func_prefix##Item, \
	EntityInterface*: func_prefix##Entity \

#define ENT_BOXED_DISPATCH_TABLE(func_prefix) \
	BoxedProjectile: func_prefix##Projectile, \
	BoxedLaser: func_prefix##Laser, \
	BoxedEnemy: func_prefix##Enemy, \
	BoxedBoss: func_prefix##Boss, \
	BoxedPlayer: func_prefix##Player, \
	BoxedItem: func_prefix##Item, \
	BoxedEntity: func_prefix##Entity \

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
#define ENT_UNBOX(box) ENT_BOXED_DISPATCH_FUNCTION(_ent_unbox_, box)
#define ENT_BOX_OR_PASSTHROUGH(ent) ENT_MIXED_DISPATCH_FUNCTION(_ent_box_, _ent_boxed_passthrough_helper_, ent)

#define ENT_BOX_CUSTOM(ent) _ent_box_Entity(&(ent)->entity_interface)
#define ENT_UNBOX_CUSTOM(box, type) ENT_CAST_CUSTOM(_ent_unbox_Entity(box), type)

typedef struct BoxedEntityArray {
	BoxedEntity *array;
	uint capacity;
	uint size;
} BoxedEntityArray;

#define ENT_TYPE(typename, id) \
	typedef union Boxed##typename##Array { \
		BoxedEntityArray as_generic_UNSAFE; \
		struct { \
			Boxed##typename *array; \
			uint capacity; \
			uint size; \
		}; \
	} Boxed##typename##Array; \
	INLINE void _ent_array_add_##typename(Boxed##typename box, Boxed##typename##Array *a) { \
		assert(a->size < a->capacity); \
		a->array[a->size++] = box; \
	}

ENT_TYPES
#undef ENT_TYPE

INLINE void _ent_array_add_BoxedEntity(BoxedEntity box, BoxedEntityArray *a) {
	assert(a->size < a->capacity);
	a->array[a->size++] = box;
}

INLINE void _ent_array_add_Entity(struct EntityInterface *ent, BoxedEntityArray *a) {
	_ent_array_add_BoxedEntity(ENT_BOX(ent), a);
}

#define ENT_ARRAY_ADD(_array, _ent) ENT_BOXED_DISPATCH_FUNCTION(_ent_array_add_, ENT_BOX_OR_PASSTHROUGH(_ent), _array)
#define ENT_ARRAY_GET_BOXED(_array, _index) ((_array)->array[_index])
#define ENT_ARRAY_GET(_array, _index) ENT_UNBOX(ENT_ARRAY_GET_BOXED(_array, _index))

#define DECLARE_ENT_ARRAY(_ent_type, _name, _capacity) \
	Boxed##_ent_type##Array _name; \
	_name.size = 0; \
	_name.capacity = _capacity; \
	Boxed##_ent_type _ent_array_data##_name[_name.capacity]; \
	_name.array = _ent_array_data##_name

#define ENT_ARRAY(_typename, _capacity) \
	((Boxed##_typename##Array) { .array = (Boxed##_typename[_capacity]) { 0 }, .capacity = (_capacity), .size = 0 })

#define ENT_ARRAY_FOREACH(_array, _var, _block) do { \
	for(uint MACROHAX_ADDLINENUM(_ent_array_iterator) = 0; MACROHAX_ADDLINENUM(_ent_array_iterator) < (_array)->size; ++MACROHAX_ADDLINENUM(_ent_array_iterator)) { \
		void *MACROHAX_ADDLINENUM(_ent_array_temp) = ENT_ARRAY_GET((_array), MACROHAX_ADDLINENUM(_ent_array_iterator)); \
		if(MACROHAX_ADDLINENUM(_ent_array_temp) != NULL) { \
			_var = MACROHAX_ADDLINENUM(_ent_array_temp); \
			_block \
		} \
	} \
} while(0)

#define ENT_ARRAY_FOREACH_COUNTER(_array, _cntr_var, _ent_var, _block) do { \
	for(uint MACROHAX_ADDLINENUM(_ent_array_iterator) = 0; MACROHAX_ADDLINENUM(_ent_array_iterator) < (_array)->size; ++MACROHAX_ADDLINENUM(_ent_array_iterator)) { \
		void *MACROHAX_ADDLINENUM(_ent_array_temp) = ENT_ARRAY_GET((_array), MACROHAX_ADDLINENUM(_ent_array_iterator)); \
		if(MACROHAX_ADDLINENUM(_ent_array_temp) != NULL) { \
			_cntr_var = MACROHAX_ADDLINENUM(_ent_array_iterator); \
			_ent_var = MACROHAX_ADDLINENUM(_ent_array_temp); \
			_block \
		} \
	} \
} while(0)

#define ENT_ARRAY_CLEAR(_array) ((_array)->size = 0)

#endif // IGUARD_entity_h
