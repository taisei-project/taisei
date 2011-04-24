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
#include <complex.h>

typedef struct {
	float r;
	float g;
	float b;
} Color;

typedef struct {
	float r;
	float g;
	float b;
	float a;
} ColorA;

struct Projectile;
typedef void (*ProjRule)(complex *pos, complex pos0, float *angle, int time, complex* args);

typedef struct Projectile {
	struct Projectile *next;
	struct Projectile *prev;
	
	long birthtime;
	
	complex pos;
	complex pos0;
	
	float angle;
	
	ProjRule rule;
	Texture *tex;
	
	enum { PlrProj, FairyProj } type;
	
	Color clr;
	
	complex args[4];
} Projectile;

void load_projectiles();

Projectile *create_projectile(char *name, complex pos, Color clr, ProjRule rule, complex args, ...);
void delete_projectile(Projectile *proj);
void draw_projectile(Projectile *proj);
void draw_projectiles();
void free_projectiles();

int test_collision(Projectile *p);
void process_projectiles();

void linear(complex *pos, complex pos0, float *angle, int time, complex* args);
#endif