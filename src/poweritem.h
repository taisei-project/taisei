/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Code by: Juergen Kieslich
 */

#ifndef POWERITEM_H
#define POWERITEM_H

#include "texture.h"

struct Poweritem;
typedef void (*ItemRule)(int *y,int sy, float acc, long time, float *velo);

typedef struct Poweritem{
	struct Poweritem *next;
	struct Poweritem *prev;

	long birthtime;
	int x, y, sy;
	char angle;
	float acc;
	float velo;
	Texture *tex;

	ItemRule rule;
} Poweritem;

void create_poweritem(int x, int y, float acc, int angle, ItemRule rule);
void delete_poweritem(Poweritem *poweritem);
void draw_poweritems();
void free_poweritems();

int collision(Poweritem *p);
void process_poweritems();

void simpleItem(int *y, int sy, float acc, long time, float *velo);
#endif
