/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "entity.h"
#include "util.h"
#include "renderer/api.h"
#include "global.h"
#include "dynarray.h"

typedef struct EntityDrawHook EntityDrawHook;
typedef LIST_ANCHOR(EntityDrawHook) EntityDrawHookList;

struct EntityDrawHook {
	LIST_INTERFACE(EntityDrawHook);
	EntityDrawHookCallback callback;
	void *arg;
};

static struct {
	DYNAMIC_ARRAY(EntityInterface*) registered;
	uint32_t total_spawns;

	struct {
		EntityDrawHookList pre_draw;
		EntityDrawHookList post_draw;
	} hooks;
} entities;

static void add_hook(EntityDrawHookList *list, EntityDrawHookCallback cb, void *arg) {
	auto hook = ALLOC(EntityDrawHook);
	hook->callback = cb;
	hook->arg = arg;

	alist_append(list, hook);
}

static void remove_hook(EntityDrawHookList *list, EntityDrawHookCallback cb) {
	for(EntityDrawHook *hook = list->first; hook; hook = hook->next) {
		if(hook->callback == cb) {
			alist_unlink(list, hook);
			mem_free(hook);
			return;
		}
	}

	UNREACHABLE;
}

static void call_hooks(EntityDrawHookList *list, EntityInterface *ent) {
	for(EntityDrawHook *hook = list->first; hook; hook = hook->next) {
		hook->callback(ent, hook->arg);
	}
}

void ent_init(void) {
	memset(&entities, 0, sizeof(entities));
	dynarray_ensure_capacity(&entities.registered, 1024);
}

void ent_shutdown(void) {
	if(entities.registered.num_elements) {
		log_fatal_if_debug("%u entities were not properly unregistered, this is a bug!", entities.registered.num_elements);
	}

	dynarray_free_data(&entities.registered);

	assert(entities.hooks.post_draw.first == NULL);
	assert(entities.hooks.pre_draw.first == NULL);
}

void ent_register(EntityInterface *ent, EntityType type) {
	assert(type > _ENT_TYPE_ENUM_BEGIN && type < _ENT_TYPE_ENUM_END);
	ent->type = type;
	ent->spawn_id = ++entities.total_spawns;
	ent->index = entities.registered.num_elements;
	assume(ent->spawn_id > 0);
	dynarray_append(&entities.registered, ent);
}

void ent_unregister(EntityInterface *ent) {
	ent->spawn_id = 0;

	// Fast non-order-preserving removal by moving the last element into the removed element's position.

	assert(ent->index < entities.registered.num_elements);
	assert(dynarray_get(&entities.registered, ent->index) == ent);
	EntityInterface *sub = entities.registered.data[--entities.registered.num_elements];
	entities.registered.data[sub->index = ent->index] = sub;
}

static int ent_cmp(const void *ptr1, const void *ptr2) {
	const EntityInterface *ent1 = *(const EntityInterface**)ptr1;
	const EntityInterface *ent2 = *(const EntityInterface**)ptr2;

	int r = (ent1->draw_layer > ent2->draw_layer) - (ent1->draw_layer < ent2->draw_layer);

	if(r == 0) {
		// Same layer? Put whatever spawned later on top, then.
		r = (ent1->spawn_id > ent2->spawn_id) - (ent1->spawn_id < ent2->spawn_id);
	}

	return r;
}

static inline bool ent_is_drawable(EntityInterface *ent) {
	return (ent->draw_layer & ~LAYER_LOW_MASK) > LAYER_NODRAW && ent->draw_func;
}

void ent_draw(EntityPredicate predicate) {
	call_hooks(&entities.hooks.pre_draw, NULL);
	dynarray_qsort(&entities.registered, ent_cmp);

	if(predicate) {
		dynarray_foreach(&entities.registered, int i, EntityInterface **pent, {
			EntityInterface *ent = *pent;
			ent->index = i;

			if(ent_is_drawable(ent) && predicate(ent)) {
				call_hooks(&entities.hooks.pre_draw, ent);
				r_state_push();
				ent->draw_func(ent);
				r_state_pop();
				call_hooks(&entities.hooks.post_draw, ent);
			}
		});
	} else {
		dynarray_foreach(&entities.registered, int i, EntityInterface **pent, {
			EntityInterface *ent = *pent;
			ent->index = i;

			if(ent_is_drawable(ent)) {
				call_hooks(&entities.hooks.pre_draw, ent);
				r_state_push();
				ent->draw_func(ent);
				r_state_pop();
				call_hooks(&entities.hooks.post_draw, ent);
			}
		});
	}

	call_hooks(&entities.hooks.post_draw, NULL);
}

DamageResult ent_damage(EntityInterface *ent, const DamageInfo *damage) {
	if(ent->damage_func == NULL) {
		return DMG_RESULT_INAPPLICABLE;
	}

	DamageInfo new_damage;

	if(DAMAGETYPE_IS_PLAYER(damage->type)) {
		new_damage = *damage;
		player_damage_hook(&global.plr, ent, &new_damage);
		damage = &new_damage;
	}

	DamageResult res = ent->damage_func(ent, damage);

	if(res == DMG_RESULT_OK) {
		player_register_damage(&global.plr, ent, damage);
	}

	return res;
}

void ent_area_damage(cmplx origin, float radius, const DamageInfo *damage, EntityAreaDamageCallback callback, void *callback_arg) {
	for(Enemy *e = global.enemies.first; e; e = e->next) {
		if(
			cabs(origin - e->pos) < radius &&
			ent_damage(&e->ent, damage) == DMG_RESULT_OK &&
			callback != NULL
		) {
			callback(&e->entity_interface, e->pos, callback_arg);
		}
	}

	if(
		global.boss != NULL &&
		cabs(origin - global.boss->pos) < radius &&
		ent_damage(&global.boss->ent, damage) == DMG_RESULT_OK &&
		callback != NULL
	) {
		callback(&global.boss->entity_interface, global.boss->pos, callback_arg);
	}
}

void ent_area_damage_ellipse(Ellipse ellipse, const DamageInfo *damage, EntityAreaDamageCallback callback, void *callback_arg) {
	for(Enemy *e = global.enemies.first; e; e = e->next) {
		if(
			point_in_ellipse(e->pos, ellipse) &&
			ent_damage(&e->ent, damage) == DMG_RESULT_OK &&
			callback != NULL
		) {
			callback(&e->entity_interface, e->pos, callback_arg);
		}
	}

	if(
		global.boss != NULL &&
		point_in_ellipse(global.boss->pos, ellipse) &&
		ent_damage(&global.boss->ent, damage) == DMG_RESULT_OK &&
		callback != NULL
	) {
		callback(&global.boss->entity_interface, global.boss->pos, callback_arg);
	}
}

void ent_hook_pre_draw(EntityDrawHookCallback callback, void *arg) {
	add_hook(&entities.hooks.pre_draw, callback, arg);
}

void ent_unhook_pre_draw(EntityDrawHookCallback callback) {
	remove_hook(&entities.hooks.pre_draw, callback);
}

void ent_hook_post_draw(EntityDrawHookCallback callback, void *arg) {
	add_hook(&entities.hooks.post_draw, callback, arg);
}

void ent_unhook_post_draw(EntityDrawHookCallback callback) {
	remove_hook(&entities.hooks.post_draw, callback);
}

void _ent_array_compact_Entity(BoxedEntityArray *a) {
	for(int i = 0; i < a->size; ++i) {
		while(ENT_UNBOX(a->array[i]) == NULL) {
			if(i >= --a->size) {
				return;
			}
			memmove(a->array + i, a->array + i + 1, (a->size - i) * sizeof(a->array[0]));
		}
	}
}
