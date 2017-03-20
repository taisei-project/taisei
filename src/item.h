/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Code by: Juergen Kieslich
 */

#ifndef ITEM_H
#define ITEM_H

#include "util.h"

typedef struct Item Item;

typedef enum {
	// from least important to most important
	// this affects the draw order
	BPoint,
	Point,
	Power,
	Bomb,
	Life,
} ItemType;

struct Item{
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

int collision_item(Item *p);
void process_items(void);

void spawn_item(complex pos, ItemType type);
void spawn_items(complex pos, int point, int power, int bomb, int life);

void items_preload(void);

#define POWER_VALUE 3

#endif
