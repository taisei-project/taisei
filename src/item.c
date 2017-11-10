/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "item.h"
#include "global.h"
#include "list.h"

static Texture* item_tex(ItemType type) {
	static const char *const map[] = {
		[Power]		= "items/power",
		[Point]		= "items/point",
		[Life]		= "items/life",
		[Bomb]		= "items/bomb",
		[LifeFrag]	= "items/lifefrag",
		[BombFrag]	= "items/bombfrag",
		[BPoint]	= "items/bullet_point",
	};

	// int cast to silence a WTF warning
	assert((int)type < sizeof(map)/sizeof(char*));
	return get_tex(map[type]);
}

Item* create_item(complex pos, complex v, ItemType type) {
	if((creal(pos) < 0 || creal(pos) > VIEWPORT_W)) {
		// we need this because we clamp the item position to the viewport boundary during motion
		// e.g. enemies that die offscreen shouldn't spawn any items inside the viewport
		return NULL;
	}

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

Item* create_bpoint(complex pos) {
	Item *i = create_item(pos, 0, BPoint);

	if(i) {
		PARTICLE("flare", pos, 0, timeout, { 30 }, .draw_rule = Fade);
		i->auto_collect = 10;
	}

	return i;
}

void draw_items(void) {
	Color white = rgba(1, 1, 1, 1);
	Color prevc = white;

	for(Item *i = global.items; i; i = i->next) {
		Color c = rgba(1, 1, 1,
			i->type == BPoint && !i->auto_collect
				? clamp(2.0 - (global.frames - i->birthtime) / 60.0, 0.1, 1.0)
				: 1.0
		);

		if(prevc != c) {
			parse_color_call(c, glColor4f);
			prevc = c;
		}

		draw_texture_p(creal(i->pos), cimag(i->pos), item_tex(i->type));
	}

	if(prevc != white) {
		glColor4f(1, 1, 1, 1);
	}
}

void delete_items(void) {
	delete_all_elements((void **)&global.items, delete_element);
}

void move_item(Item *i) {
	int t = global.frames - i->birthtime;
	complex lim = 0 + 2.0*I;

	if(i->auto_collect) {
		i->pos -= (7+i->auto_collect)*cexp(I*carg(i->pos - global.plr.pos));
	} else {
		complex oldpos = i->pos;
		i->pos = i->pos0 + log(t/5.0 + 1)*5*(i->v + lim) + lim*t;

		complex v = i->pos - oldpos;
		double half = item_tex(i->type)->w/2.0;
		bool over = false;

		if((over = creal(i->pos) > VIEWPORT_W-half) || creal(i->pos) < half) {
			complex normal = over ? -1 : 1;
			v -= 2 * normal * (creal(normal)*creal(v));
			v = 1.5*creal(v) - I*fabs(cimag(v));

			i->pos = clamp(creal(i->pos), half, VIEWPORT_W-half) + I*cimag(i->pos);
			i->v = v;
			i->pos0 = i->pos;
			i->birthtime = global.frames;
		}
	}
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

		if(cabs(global.plr.pos - item->pos) < r) {
			item->auto_collect = 1;
		} else {
			bool plr_alive = global.plr.deathtime <= global.frames && global.plr.deathtime == -1;

			if((cimag(global.plr.pos) < POINT_OF_COLLECT && plr_alive)
			|| global.frames - global.plr.recovery < 0)
				item->auto_collect = 1;

			if(item->auto_collect && !plr_alive) {
				item->auto_collect = 0;
				item->pos0 = item->pos;
				item->birthtime = global.frames;
				item->v = -10*I + 5*nfrand();
			}
		}

		move_item(item);

		v = collision_item(item);
		if(v == 1) {
			switch(item->type) {
			case Power:
				player_set_power(&global.plr, global.plr.power + POWER_VALUE);
				play_sound("item_generic");
				break;
			case Point:
				player_add_points(&global.plr, 100);
				play_sound("item_generic");
				break;
			case BPoint:
				player_add_points(&global.plr, 1);
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
	create_item(pos, (12 + 6 * afrand(0)) * (cexp(I*(3*M_PI/2 + anfrand(1)*M_PI/11))), type);
}

void spawn_items(complex pos, ...) {
	va_list args;
	va_start(args, pos);

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
