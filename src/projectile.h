/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
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
typedef void (*ProjRule)(float *x, float *y, int angle, int sx, int sy, int time, float* a);

typedef struct Projectile {
	struct Projectile *next;
	struct Projectile *prev;
	
	long birthtime;
	
	float x;
	float y;
	
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

void simple(float *x, float *y, int angle, int sx, int sy, int time, float* a);
#endif