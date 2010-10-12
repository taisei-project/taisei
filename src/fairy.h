/*
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA  02110-1301, USA.
 
 ---
 Copyright (C) 2010, Lukas Weber <laochailan@web.de>
 */

#ifndef FAIRY_H
#define FAIRY_H

#include "animation.h"

struct Fairy;

typedef void (*FairyRule)(struct Fairy*);

typedef struct Fairy {
	int birthtime;
	int hp;
	
	int x, y;
	int sx, sy;
	int angle;
	int v;
	
	char moving;
	char dir;
	
	Animation ani;
	
	FairyRule rule;
	
	struct Fairy *next;
	struct Fairy *prev;
} Fairy;

void create_fairy(int x, int y, int speed, int angle, int hp, FairyRule rule);
void delete_fairy(Fairy *fairy);
void draw_fairy(Fairy *fairy);
void free_fairies();

void process_fairies();

#endif