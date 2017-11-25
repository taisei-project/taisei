/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "util.h"
#include "resource/texture.h"

typedef struct Item Item;

typedef enum {
	// from least important to most important
	// this affects the draw order
	BPoint = 1,
	Point,
	Power,
	BombFrag,
	LifeFrag,
	Bomb,
	Life,
} ItemType;

struct Item {
	Item *next;
	Item *prev;

	int birthtime;
	complex pos;
	complex pos0;

	int auto_collect;
	ItemType type;

	complex v;
};

Item *create_item(complex pos, complex v, ItemType type);
void delete_item(Item *item);
void draw_items(void);
void delete_items(void);

Item* create_bpoint(complex pos);

int collision_item(Item *p);
void process_items(void);

void spawn_item(complex pos, ItemType type);

// The varargs are: ItemType type1, int num1, ItemType type2, int num2, ..., NULL
// e.g.: Point 10, Power 3, LifeFrag 2, Bomb 1, NULL
// WARNING: if you pass a float or double as the amount, it will not work! You must explicitly cast it to an int.
void spawn_items(complex pos, ...) __attribute__((sentinel));

void items_preload(void);

#define POWER_VALUE 3
