/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef FAIRY_H
#define FAIRY_H

#include "animation.h"

struct Fairy;

typedef void (*FairyRule)(struct Fairy*);

typedef struct Fairy {
	struct Fairy *next;
	struct Fairy *prev;
	long birthtime;
	char hp;
	
	int x, y;
	int sx, sy;
	char angle;
	char v;
	
	char moving;
	char dir;
	
	Animation *ani;
	
	FairyRule rule;
} Fairy;

void create_fairy(int x, int y, int speed, int angle, int hp, FairyRule rule);
void delete_fairy(Fairy *fairy);
void draw_fairies();
void free_fairies();

void process_fairies();

#endif