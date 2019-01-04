/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "entity.h"
#include "util.h"
#include "renderer/api.h"
#include "global.h"

typedef struct EntityDrawHook EntityDrawHook;
typedef LIST_ANCHOR(EntityDrawHook) EntityDrawHookList;

struct EntityDrawHook {
	LIST_INTERFACE(EntityDrawHook);
	EntityDrawHookCallback callback;
	void *arg;
};

static struct {
	EntityInterface **array;
	uint num;
	uint capacity;
	uint32_t total_spawns;

	struct {
		EntityDrawHookList pre_draw;
		EntityDrawHookList post_draw;
	} hooks;
} entities;

static void add_hook(EntityDrawHookList *list, EntityDrawHookCallback cb, void *arg) {
	EntityDrawHook *hook = calloc(1, sizeof(*hook));
	hook->callback = cb;
	hook->arg = arg;
	alist_append(list, hook);
}

static void remove_hook(EntityDrawHookList *list, EntityDrawHookCallback cb) {
	for(EntityDrawHook *hook = list->first; hook; hook = hook->next) {
		if(hook->callback == cb) {
			alist_unlink(list, hook);
			free(hook);
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

#define FOR_EACH_ENT(ent) for(EntityInterface **_ent = entities.array, *ent = *entities.array; _ent < entities.array + entities.num; ent = *(++_ent))

void ent_init(void) {
	memset(&entities, 0, sizeof(entities));
	entities.capacity = 4096;
	entities.array = calloc(entities.capacity, sizeof(EntityInterface*));
}

void ent_shutdown(void) {
	if(entities.num) {
		log_warn("%u entities were not properly unregistered", entities.num);
	}

	FOR_EACH_ENT(ent) {
		ent_unregister(ent);
	}

	free(entities.array);

	assert(entities.hooks.post_draw.first == NULL);
	assert(entities.hooks.pre_draw.first == NULL);
}

void ent_register(EntityInterface *ent, EntityType type) {
	assert(type > _ENT_TYPE_ENUM_BEGIN && type < _ENT_TYPE_ENUM_END);
	ent->type = type;
	ent->index = entities.num++;
	ent->spawn_id = ++entities.total_spawns;

	if(ent->spawn_id == 0) {
		// This is not really an error, but it may result in weird draw order
		// in some corner cases. If this overflow ever actually occurs, though,
		// then you've probably got much bigger problems.
		log_debug("spawn_id just overflowed. You might be spawning stuff waaaay too often");
	}

	if(entities.capacity < entities.num) {
		entities.capacity *= 2;
		entities.array = realloc(entities.array, entities.capacity * sizeof(EntityInterface*));
	}

	entities.array[ent->index] = ent;

	assert(ent->index < entities.num);
	assert(entities.array[ent->index] == ent);
}

void ent_unregister(EntityInterface *ent) {
	EntityInterface *sub = entities.array[--entities.num];
	assert(ent->index <= entities.num);
	assert(entities.array[ent->index] == ent);
	entities.array[sub->index = ent->index] = sub;
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

void ent_draw(EntityPredicate predicate) {
	call_hooks(&entities.hooks.pre_draw, NULL);
	qsort(entities.array, entities.num, sizeof(EntityInterface*), ent_cmp);

	if(predicate) {
		FOR_EACH_ENT(ent) {
			ent->index = _ent - entities.array;
			assert(entities.array[ent->index] == ent);

			if(ent->draw_func && predicate(ent)) {
				call_hooks(&entities.hooks.pre_draw, ent);
				r_state_push();
				ent->draw_func(ent);
				r_state_pop();
				call_hooks(&entities.hooks.post_draw, ent);
			}
		}
	} else {
		FOR_EACH_ENT(ent) {
			ent->index = _ent - entities.array;
			assert(entities.array[ent->index] == ent);

			if(ent->draw_func) {
				call_hooks(&entities.hooks.pre_draw, ent);
				r_state_push();
				ent->draw_func(ent);
				r_state_pop();
				call_hooks(&entities.hooks.post_draw, ent);
			}
		}
	}

	call_hooks(&entities.hooks.post_draw, NULL);
}

DamageResult ent_damage(EntityInterface *ent, const DamageInfo *damage) {
	if(ent->damage_func == NULL) {
		return DMG_RESULT_INAPPLICABLE;
	}

	DamageResult res = ent->damage_func(ent, damage);

	if(res == DMG_RESULT_OK) {
		player_register_damage(&global.plr, ent, damage);
	}

	return res;
}

void ent_area_damage(complex origin, float radius, const DamageInfo *damage) {
	for(Enemy *e = global.enemies.first; e; e = e->next) {
		if(cabs(origin - e->pos) < radius) {
			ent_damage(&e->ent, damage);
		}
	}

	if(global.boss && cabs(origin - global.boss->pos) < radius) {
		ent_damage(&global.boss->ent, damage);
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
