/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Code by: Juergen Kieslich
 */

#ifndef ITEM_H
#define ITEM_H

#include "texture.h"
#include <complex.h>

struct Item;

typedef enum {
	Power,
	Point,
	Life,
	Bomb
} Type;

typedef struct Item{
	struct Item *next;
	struct Item *prev;

	int birthtime;
	complex pos;
	complex pos0;
	
	int auto_collect;
	Type type;
	
	complex v;
} Item;

void create_item(complex pos, complex v, Type type);
void delete_item(Item *item);
void draw_items();
void delete_items();

int collision_item(Item *p);
void process_items();

#endif
