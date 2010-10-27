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

#include <stdarg.h>

typedef struct {
	float r;
	float g;
	float b;
} Color;

struct Projectile;
typedef void (*ProjRule)(int *x, int *y, int angle, int sx, int sy, int time, float* a);

typedef struct Projectile {
	struct Projectile *next;
	struct Projectile *prev;
	
	int birthtime;
	
	int x;
	int y;
	
	int sx;
	int sy;
	
	int angle;
	
	ProjRule rule;
	Texture *tex;
	
	enum { PlrProj, FairyProj } type;
	
	Color clr;
	
	float args[4];
} Projectile;

typedef struct {
	Texture ball;
	Texture rice;
	Texture bigball;
	
	Texture youmu;
} ProjCache;

extern ProjCache _projs;

void load_projectiles();

Projectile *create_projectile(Texture *tex, int x, int y, int angle, Color clr, ProjRule rule, float args, ...);
void delete_projectile(Projectile *proj);
void draw_projectile(Projectile *proj);
void draw_projectiles();
void free_projectiles();

int test_collision(Projectile *p);
void process_projectiles();

void simple(int *x, int *y, int angle, int sx, int sy, int time, float* a);
#endif