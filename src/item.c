/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>, Juergen Kieslich
 */

#include "item.h"
#include "global.h"
#include "list.h"

Item *create_item(complex pos, complex v, ItemType type) {
	Item *e, **d, **dest = &global.items;

	for(e = *dest; e && e->next; e = e->next)
		if(e->prev && type < e->type)
			break;

	if(e == NULL)
		d = dest;
	else
		d = &e;

	Item *i = create_element((void **)d, sizeof(Item));
	i->pos = pos;
	i->pos0 = pos;
	i->v = v;
	i->birthtime = global.frames;
	i->auto_collect = 0;
	i->type = type;

	return i;
}

void delete_item(Item *item) {
	delete_element((void **)&global.items, item);
}

void draw_items(void) {
	Item *p;
	Texture *tex = NULL;
	for(p = global.items; p; p = p->next) {
		switch(p->type){
			case Power:
				tex = get_tex("items/power");
				break;
			case Point:
				tex = get_tex("items/point");
				break;
			case Life:
				tex = get_tex("items/life");
				break;
			case Bomb:
				tex = get_tex("items/bomb");
				break;
			case LifeFrag:
				tex = get_tex("items/lifefrag");
				break;
			case BombFrag:
				tex = get_tex("items/bombfrag");
				break;
			case BPoint:
				tex = get_tex("items/bullet_point");
				break;
			default:
				break;
		}
		draw_texture_p(creal(p->pos), cimag(p->pos), tex);
	}
}

void delete_items(void) {
	delete_all_elements((void **)&global.items, delete_element);
}

void move_item(Item *i) {
	int t = global.frames - i->birthtime;
	complex lim = 0 + 2.0*I;

	if(i->auto_collect)
		i->pos -= (7+i->auto_collect)*cexp(I*carg(i->pos - global.plr.pos));
	else
		i->pos = i->pos0 + log(t/5.0 + 1)*5*(i->v + lim) + lim*t;
}

void process_items(void) {
	Item *item = global.items, *del = NULL;
    int v;

	float r = 30;
	if(global.plr.inputflags & INFLAG_FOCUS)
		r *= 2;

	while(item != NULL) {
		if((item->type == Power && global.plr.power >= PLR_MAX_POWER) ||
			// just in case we ever have some weird spell that spawns those...
		   (global.stage->type == STAGE_SPELL && (item->type == Life || item->type == Bomb))
		) {
			item->type = Point;
		}

		if(cimag(global.plr.pos) < POINT_OF_COLLECT || cabs(global.plr.pos - item->pos) < r
		|| global.frames - global.plr.recovery < 0)
			item->auto_collect = 1;

		move_item(item);

		v = collision_item(item);
		if(v == 1) {
			switch(item->type) {
			case Power:
				player_set_power(&global.plr, global.plr.power + POWER_VALUE);
				play_sound("item_generic");
				break;
			case Point:
				global.plr.points += 100;
				play_sound("item_generic");
				break;
			case BPoint:
				global.plr.points += 1;
				play_sound("item_generic");
				break;
			case Life:
				player_add_lives(&global.plr, 1);
				break;
			case Bomb:
				player_add_bombs(&global.plr, 1);
				break;
			case LifeFrag:
				player_add_life_fragments(&global.plr, 1);
				break;
			case BombFrag:
				player_add_bomb_fragments(&global.plr, 1);
				break;
			}
		}

		if(v == 1 || creal(item->pos) < -9 || creal(item->pos) > VIEWPORT_W + 9
			|| cimag(item->pos) > VIEWPORT_H + 8 ) {
			del = item;
			item = item->next;
			delete_item(del);
		} else {
			item = item->next;
		}
	}
}

int collision_item(Item *i) {
	if(cabs(global.plr.pos - i->pos) < 10)
		return 1;

	return 0;
}

void spawn_item(complex pos, ItemType type) {
	tsrand_fill(2);
	create_item(pos, 5*cexp(I*tsrand_a(0)/afrand(1)*M_PI*2), type);
}

void spawn_items(complex pos, ItemType first_type, int first_num, ...) {
	for(int i = 0; i < first_num; ++i) {
		spawn_item(pos, first_type);
	}

	va_list args;
	va_start(args, first_num);

	ItemType type;
	while(type = va_arg(args, ItemType)) {
		int num = va_arg(args, int);
		for(int i = 0; i < num; ++i) {
			spawn_item(pos, type);
		}
	}

	va_end(args);
}

void items_preload(void) {
	preload_resources(RES_TEXTURE, RESF_PERMANENT,
		"items/power",
		"items/point",
		"items/life",
		"items/bomb",
		"items/lifefrag",
		"items/bombfrag",
		"items/bullet_point",
	NULL);

	preload_resources(RES_SFX, RESF_OPTIONAL,
		"item_generic",
	NULL);
}
