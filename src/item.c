/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "item.h"
#include "global.h"
#include "list.h"
#include "stageobjects.h"

// Instant collection radius.
// This is not the same as the player's PLR_PROP_COLLECT_RADIUS property, which is the minimum
// distance to begin attracting the item towards the player.
#define ITEM_GRAB_RADIUS 10

static const char* item_sprite_name(ItemType type) {
	static const char *const map[] = {
		[ITEM_BOMB          - ITEM_FIRST] = "item/bomb",
		[ITEM_BOMB_FRAGMENT - ITEM_FIRST] = "item/bombfrag",
		[ITEM_LIFE          - ITEM_FIRST] = "item/life",
		[ITEM_LIFE_FRAGMENT - ITEM_FIRST] = "item/lifefrag",
		[ITEM_PIV           - ITEM_FIRST] = "item/bullet_point",
		[ITEM_POINTS        - ITEM_FIRST] = "item/point",
		[ITEM_POWER         - ITEM_FIRST] = "item/power",
		[ITEM_POWER_MINI    - ITEM_FIRST] = "item/minipower",
		[ITEM_SURGE         - ITEM_FIRST] = "item/surge",
		[ITEM_VOLTAGE       - ITEM_FIRST] = "item/voltage",
	};

	uint index = type - 1;

	assert(index < ARRAY_SIZE(map));
	return map[index];
}

static const char* item_indicator_sprite_name(ItemType type) {
	static const char *const map[] = {
		[ITEM_BOMB          - ITEM_FIRST] = "item/bomb_indicator",
		[ITEM_BOMB_FRAGMENT - ITEM_FIRST] = "item/bombfrag_indicator",
		[ITEM_LIFE          - ITEM_FIRST] = "item/life_indicator",
		[ITEM_LIFE_FRAGMENT - ITEM_FIRST] = "item/lifefrag_indicator",
		[ITEM_PIV           - ITEM_FIRST] = NULL,
		[ITEM_POINTS        - ITEM_FIRST] = "item/point_indicator",
		[ITEM_POWER         - ITEM_FIRST] = "item/power_indicator",
		[ITEM_POWER_MINI    - ITEM_FIRST] = NULL,
		[ITEM_SURGE         - ITEM_FIRST] = NULL,
		[ITEM_VOLTAGE       - ITEM_FIRST] = "item/voltage_indicator",
	};

	uint index = type - 1;

	assert(index < ARRAY_SIZE(map));
	return map[index];
}

static Sprite* item_sprite(ItemType type) {
	return res_sprite(item_sprite_name(type));
}

static Sprite* item_indicator_sprite(ItemType type) {
	const char *name = item_indicator_sprite_name(type);
	if(name == NULL) {
		return NULL;
	}
	return res_sprite(name);
}

static void ent_draw_item(EntityInterface *ent) {
	Item *i = ENT_CAST(ent, Item);

	const int indicator_display_y = 6;

	float y = cimag(i->pos);
	if(y < 0) {
		Sprite *s = item_indicator_sprite(i->type);

		float alpha = -tanh(y*0.1)/(1+0.1*fabs(y));

		if(s != NULL) {
			r_draw_sprite(&(SpriteParams) {
				.sprite_ptr = s,
				.pos = { creal(i->pos), indicator_display_y },
				.color = RGBA_MUL_ALPHA(1, 1, 1, alpha),
			});
		}
	}


	float alpha = 1;
	if(i->type == ITEM_PIV && !i->auto_collect) {
		alpha *=  clamp(2.0 - (global.frames - i->birthtime) / 60.0, 0.1, 1.0);
	}

	Color *c = RGBA_MUL_ALPHA(1, 1, 1, alpha);

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = item_sprite(i->type),
		.pos = { creal(i->pos), y },
		.color = c,
	});

}

Item* create_item(cmplx pos, cmplx v, ItemType type) {
	if((creal(pos) < 0 || creal(pos) > VIEWPORT_W)) {
		// we need this because we clamp the item position to the viewport boundary during motion
		// e.g. enemies that die offscreen shouldn't spawn any items inside the viewport
		return NULL;
	}

	if(type == ITEM_POWER_MINI && player_is_powersurge_active(&global.plr)) {
		type = ITEM_SURGE;
	}

	Item *i = objpool_acquire(&stage_object_pools.items);
	alist_append(&global.items, i);

	i->pos = pos;
	i->pos0 = pos;
	i->v = v;
	i->birthtime = global.frames;
	i->auto_collect = 0;
	i->collecttime = 0;
	i->type = type;

	i->ent.draw_layer = LAYER_ITEM | i->type;
	i->ent.draw_func = ent_draw_item;
	ent_register(&i->ent, ENT_TYPE_ID(Item));

	return i;
}

void delete_item(Item *item) {
	ent_unregister(&item->ent);
	objpool_release(&stage_object_pools.items, alist_unlink(&global.items, item));
}

Item *create_clear_item(cmplx pos, uint clear_flags) {
	ItemType type = ITEM_PIV;

	if(clear_flags & CLEAR_HAZARDS_SPAWN_VOLTAGE) {
		type = ITEM_VOLTAGE;
	}

	Item *i = create_item(pos, -10*I + 5*rng_sreal(), type);

	if(i) {
		PARTICLE(
			.sprite = "flare",
			.pos = pos,
			.timeout = 30,
			.draw_rule = pdraw_timeout_fade(1, 0),
			.layer = LAYER_BULLET+1
		);

		collect_item(i, 1);
	}

	return i;
}

void delete_items(void) {
	for(Item *i = global.items.first, *next; i; i = next) {
		next = i->next;
		delete_item(i);
	}
}

static cmplx move_item(Item *i) {
	int t = global.frames - i->birthtime;
	cmplx lim = 0 + 2.0*I;

	cmplx oldpos = i->pos;

	if(i->auto_collect && i->collecttime <= global.frames && global.frames - i->birthtime > 20) {
		i->pos -= (7 + i->auto_collect) * cnormalize(i->pos - global.plr.pos);
	} else {
		i->pos = i->pos0 + log(t/5.0 + 1)*5*(i->v + lim) + lim*t;

		cmplx v = i->pos - oldpos;
		double half = item_sprite(i->type)->w/2.0;  // TODO remove dependence on sprite size
		bool over = false;

		if((over = creal(i->pos) > VIEWPORT_W-half) || creal(i->pos) < half) {
			cmplx normal = over ? -1 : 1;
			v -= 2 * normal * (creal(normal)*creal(v));
			v = 1.5*creal(v) - I*fabs(cimag(v));

			i->pos = clamp(creal(i->pos), half, VIEWPORT_W-half) + I*cimag(i->pos);
			i->v = v;
			i->pos0 = i->pos;
			i->birthtime = global.frames;
		}
	}

	return i->pos - oldpos;
}

static bool item_out_of_bounds(Item *item) {
	double margin = fmax(item_sprite(item->type)->w, item_sprite(item->type)->h);

	return (
		creal(item->pos) < -margin ||
		creal(item->pos) > VIEWPORT_W + margin ||
		cimag(item->pos) > VIEWPORT_H + margin
	);
}

bool collect_item(Item *item, float value) {
	if(!player_is_alive(&global.plr)) {
		return false;
	}

	const float speed = 10;
	const int delay = 0;

	if(item->auto_collect) {
		item->auto_collect = imax(speed, item->auto_collect);
		item->pickup_value = fmax(clamp(value, ITEM_MIN_VALUE, ITEM_MAX_VALUE), item->pickup_value);
		item->collecttime = imin(global.frames + delay, item->collecttime);
	} else {
		item->auto_collect = speed;
		item->pickup_value = clamp(value, ITEM_MIN_VALUE, ITEM_MAX_VALUE);
		item->collecttime = global.frames + delay;
	}

	return true;
}

void collect_all_items(float value) {
	for(Item *i = global.items.first; i; i = i->next) {
		collect_item(i, value);
	}
}

void process_items(void) {
	Item *item = global.items.first, *del = NULL;
	float attract_dist = player_property(&global.plr, PLR_PROP_COLLECT_RADIUS);
	bool plr_alive = player_is_alive(&global.plr);
	bool stage_cleared = stage_is_cleared();

	while(item != NULL) {
		bool may_collect = true;

		if(
			(item->type == ITEM_POWER_MINI && global.plr.power_stored >= PLR_MAX_POWER_EFFECTIVE) ||
			(item->type == ITEM_SURGE && !player_is_powersurge_active(&global.plr))
		) {
			item->type = ITEM_PIV;

			if(collect_item(item, 1)) {
				item->pos0 = item->pos;
				item->birthtime = global.frames;
				item->v = -20*I + 10*rng_sreal();
			}
		}

		if(global.stage->type == STAGE_SPELL && (item->type == ITEM_LIFE || item->type == ITEM_BOMB || item->type == ITEM_LIFE_FRAGMENT || item->type == ITEM_BOMB_FRAGMENT)) {
			// just in case we ever have some weird spell that spawns those...
			item->type = ITEM_POINTS;
		}

		if(global.frames - item->birthtime < 20) {
			may_collect = false;
		}

		bool grabbed = false;

		if(may_collect) {
			real item_dist2 = cabs2(global.plr.pos - item->pos);

			if(plr_alive) {
				if(cimag(global.plr.pos) < player_property(&global.plr, PLR_PROP_POC) || stage_cleared) {
					collect_item(item, 1);
				} else if(item_dist2 < attract_dist * attract_dist) {
					collect_item(item, 1 - cimag(global.plr.pos) / VIEWPORT_H);
					item->auto_collect = 2;
				}
			} else if(item->auto_collect) {
				item->auto_collect = 0;
				item->pos0 = item->pos;
				item->birthtime = global.frames;
				item->v = -10*I + 5*rng_sreal();
			}

			grabbed = (item_dist2 < ITEM_GRAB_RADIUS * ITEM_GRAB_RADIUS);
		}

		cmplx deltapos = move_item(item);

		if(grabbed) {
			switch(item->type) {
			case ITEM_POWER:
				player_add_power(&global.plr, POWER_VALUE);
				player_add_points(&global.plr, 25, item->pos);
				player_extend_powersurge(&global.plr, PLR_POWERSURGE_POSITIVE_GAIN*3, PLR_POWERSURGE_NEGATIVE_GAIN*3);
				play_sfx("item_generic");
				break;
			case ITEM_POWER_MINI:
				player_add_power(&global.plr, POWER_VALUE_MINI);
				player_add_points(&global.plr, 5, item->pos);
				play_sfx("item_generic");
				break;
			case ITEM_SURGE:
				player_extend_powersurge(&global.plr, PLR_POWERSURGE_POSITIVE_GAIN, PLR_POWERSURGE_NEGATIVE_GAIN);
				player_add_points(&global.plr, 25, item->pos);
				play_sfx("item_generic");
				break;
			case ITEM_POINTS:
				player_add_points(&global.plr, round(global.plr.point_item_value * item->pickup_value), item->pos);
				play_sfx("item_generic");
				break;
			case ITEM_PIV:
				player_add_piv(&global.plr, 1, item->pos);
				play_sfx("item_generic");
				break;
			case ITEM_VOLTAGE:
				player_add_voltage(&global.plr, 1);
				player_add_piv(&global.plr, 10, item->pos);
				play_sfx("item_generic");
				break;
			case ITEM_LIFE:
				player_add_lives(&global.plr, 1);
				break;
			case ITEM_BOMB:
				player_add_bombs(&global.plr, 1);
				break;
			case ITEM_LIFE_FRAGMENT:
				player_add_life_fragments(&global.plr, 1);
				break;
			case ITEM_BOMB_FRAGMENT:
				player_add_bomb_fragments(&global.plr, PLR_MAX_BOMB_FRAGMENTS / 5);
				break;
			}
		}

		if(grabbed || (cimag(deltapos) > 0 && item_out_of_bounds(item))) {
			del = item;
			item = item->next;
			delete_item(del);
		} else {
			item = item->next;
		}
	}
}

static void spawn_item_internal(cmplx pos, ItemType type, float collect_value) {
	cmplx v = rng_range(12, 18);
	v *= cdir(3*M_PI/2 + rng_sreal() * M_PI/11);
	v -= 3*I;

	Item *i = create_item(pos, v, type);

	if(i != NULL && collect_value >= 0) {
		collect_item(i, collect_value);
	}
}

void spawn_item(cmplx pos, ItemType type) {
	spawn_item_internal(pos, type, -1);
}

void spawn_and_collect_item(cmplx pos, ItemType type, float collect_value) {
	spawn_item_internal(pos, type, collect_value);
}

static void spawn_items_internal(cmplx pos, float collect_value, SpawnItemsArgs groups[]) {
	for(SpawnItemsArgs *g = groups; g->type > 0; ++g) {
		for(uint i = 0; i < g->count; ++i) {
			spawn_item_internal(pos, g->type, collect_value);
		}
	}
}

#undef spawn_items
void spawn_items(cmplx pos, SpawnItemsArgs groups[]) {
	spawn_items_internal(pos, -1, groups);
}

#undef spawn_and_collect_items
void spawn_and_collect_items(cmplx pos, float collect_value, SpawnItemsArgs groups[]) {
	spawn_items_internal(pos, collect_value, groups);
}

void items_preload(ResourceGroup *rg) {
	for(ItemType i = ITEM_FIRST; i <= ITEM_LAST; ++i) {
		res_group_preload(rg, RES_SPRITE, 0, item_sprite_name(i), NULL);
		const char *indicator = item_indicator_sprite_name(i);
		if(indicator != NULL) {
			res_group_preload(rg, RES_SPRITE, 0, indicator, NULL);
		}
	}

	res_group_preload(rg, RES_SFX, RESF_OPTIONAL,
		"item_generic",
	NULL);
}
