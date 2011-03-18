/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef FAIRY_H
#define FAIRY_H

#include "animation.h"
#include <complex.h>

struct Fairy;

typedef void (*FairyRule)(struct Fairy*);

typedef struct Fairy {
	struct Fairy *next;
	struct Fairy *prev;
	long birthtime;
	int hp;
	
	complex pos;
	complex pos0;
	
	char moving;
	char dir;
	
	Animation *ani;
	
	FairyRule rule;
	complex args[4];
} Fairy;

void create_fairy(complex pos, int hp, FairyRule rule, complex args, ...);
void delete_fairy(Fairy *fairy);
void draw_fairies();
void free_fairies();

void process_fairies();

#endif