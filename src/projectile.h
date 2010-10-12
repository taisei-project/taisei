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

#ifndef PROJECTILE
#define PROJECTILE

#include "texture.h"

typedef struct {
	float r;
	float g;
	float b;
} Color;

// typedef float Color[3];

struct Projectile;
typedef void (*ProjRule)(struct Projectile*);
typedef void (*DrawRule)(int x, int y, float angle, Color *clr);

typedef struct Projectile {
	int birthtime;
	
	int x;
	int y;
	
	int sx;
	int sy;
	
	int v;
	float angle;
	
	ProjRule rule;
	DrawRule draw;	
	
	struct Projectile *next;
	struct Projectile *prev;
	
	Color clr;
} Projectile;

void create_projectile(int x, int y, int v, float angle, ProjRule rule, DrawRule draw, Color clr);
void delete_projectile(Projectile *proj);
void draw_projectile(Projectile *proj);
void free_projectiles();

void process_projectiles();

void simple(Projectile *p);
void ProjBall(int x, int y, float angle, Color *clr);
#endif