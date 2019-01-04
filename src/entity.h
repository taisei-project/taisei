/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "objectpool.h"

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

#define ENT_TYPES \
	ENT_TYPE(Projectile, ENT_PROJECTILE) \
	ENT_TYPE(Laser, ENT_LASER) \
	ENT_TYPE(Enemy, ENT_ENEMY) \
	ENT_TYPE(Boss, ENT_BOSS) \
	ENT_TYPE(Player, ENT_PLAYER) \
	ENT_TYPE(Item, ENT_ITEM) \

typedef enum EntityType {
	_ENT_TYPE_ENUM_BEGIN,
	#define ENT_TYPE(typename, id) id, _ENT_TYPEID_##typename = id,
	ENT_TYPES
	#undef ENT_TYPE
	_ENT_TYPE_ENUM_END,
} EntityType;

typedef struct EntityInterface EntityInterface;
typedef struct EntityListNode EntityListNode;

typedef enum DamageType {
	DMG_UNDEFINED,
	DMG_ENEMY_SHOT,
	DMG_ENEMY_COLLISION,
	DMG_PLAYER_SHOT,
	DMG_PLAYER_BOMB,
} DamageType;

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

#define ENTITY_INTERFACE_BASE(typename) struct { \
	OBJECT_INTERFACE(typename); \
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
	OBJECT_INTERFACE(typename); \
	EntityInterface entity_interface; \
	EntityInterface name; \
}

struct EntityInterface {
	ENTITY_INTERFACE_BASE(EntityInterface);
};

static inline attr_must_inline const char* ent_type_name(EntityType type) {
	switch(type) {
		#define ENT_TYPE(typename, id) case id: return #id;
		ENT_TYPES
		#undef ENT_TYPE
		default: return "ENT_INVALID";
	}

	UNREACHABLE;
}

#define ENT_TYPE_ID(typename) (_ENT_TYPEID_##typename)

#ifdef USE_GNU_EXTENSIONS
	#define ENT_CAST(ent, typename) (__extension__({ \
		static_assert(__builtin_types_compatible_p(EntityInterface, __typeof__(*(ent))), \
			"Expression is not an EntityInterface pointer"); \
		static_assert(__builtin_types_compatible_p(EntityInterface, __typeof__(((typename){}).entity_interface)), \
			#typename " doesn't implement EntityInterface"); \
		static_assert(__builtin_offsetof(typename, entity_interface) == 0, \
			"entity_interface has non-zero offset in " #typename); \
		if(ent->type != _ENT_TYPEID_##typename) { \
			log_fatal("Invalid entity cast from %s to " #typename, ent_type_name(ent->type)); \
		} \
		(typename *)(ent); \
	}))
#else
	#define ENT_CAST(ent, typename) ((typename *)(ent))
#endif

void ent_init(void);
void ent_shutdown(void);
void ent_register(EntityInterface *ent, EntityType type) attr_nonnull(1);
void ent_unregister(EntityInterface *ent) attr_nonnull(1);
void ent_draw(EntityPredicate predicate);
DamageResult ent_damage(EntityInterface *ent, const DamageInfo *damage) attr_nonnull(1, 2);
void ent_area_damage(complex origin, float radius, const DamageInfo *damage) attr_nonnull(3);

void ent_hook_pre_draw(EntityDrawHookCallback callback, void *arg);
void ent_unhook_pre_draw(EntityDrawHookCallback callback);
void ent_hook_post_draw(EntityDrawHookCallback callback, void *arg);
void ent_unhook_post_draw(EntityDrawHookCallback callback);
