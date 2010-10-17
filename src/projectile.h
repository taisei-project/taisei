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

struct Projectile;
typedef void (*ProjRule)(struct Projectile*);

typedef struct Projectile {
	int birthtime;
	
	int x;
	int y;
	
	int sx;
	int sy;
	
	int v;
	float angle;
	
	ProjRule rule;
	Texture *tex;
	
	enum { PlrProj, FairyProj } type;
	
	struct Projectile *next;
	struct Projectile *prev;
	
	Color clr;
} Projectile;

typedef struct {
	Texture ball;
	Texture rice;
	Texture bigball;
	
	Texture youmu;
} ProjCache;

extern ProjCache _projs;

void load_projectiles();

Projectile *create_projectile(int x, int y, int v, float angle, ProjRule rule, Texture *tex, Color clr);
void delete_projectile(Projectile *proj);
void draw_projectile(Projectile *proj);
void free_projectiles();

int test_collision(Projectile *p);
void process_projectiles();

void simple(Projectile *p);
#endif