/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Code by: Juergen Kieslich
 */

#ifndef ITEM_H
#define ITEM_H

#include <complex.h>

typedef struct Item Item;

typedef enum {
	Power,
	Point,
	BPoint,
	Life,
	Bomb
} Type;

struct Item{
	Item *next;
	Item *prev;

	int birthtime;
	complex pos;
	complex pos0;
	
	int auto_collect;
	Type type;
	
	complex v;
};

Item *create_item(complex pos, complex v, Type type);
void delete_item(Item *item);
void draw_items();
void delete_items();

int collision_item(Item *p);
void process_items();

void spawn_item(complex pos, Type type);
void spawn_items(complex pos, int point, int power, int bomb, int life);

#endif
