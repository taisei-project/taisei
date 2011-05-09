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
	float a;
} Color;

struct Projectile;
typedef void (*ProjRule)(struct Projectile *p, int t);

typedef struct Projectile {
	struct Projectile *next;
	struct Projectile *prev;
	
	long birthtime;
	
	complex pos;
	complex pos0;
	
	float angle;
	void *parent;
	
	ProjRule rule;
	Texture *tex;
	
	enum { PlrProj, FairyProj } type;
	
	Color *clr;
	
	complex args[4];
} Projectile;

void load_projectiles();

Color *rgba(float r, float g, float b, float a);

inline Color *rgb(float r, float g, float b);

#define create_particle(name, pos, clr, rule, args) (create_projectile_d(&global.particles, name, pos, clr, rule, args))
#define create_projectile(name, pos, clr, rule, args) (create_projectile_d(&global.projs, name, pos, clr, rule, args))
Projectile *create_projectile_d(Projectile **dest, char *name, complex pos, Color *clr, ProjRule rule, complex args, ...);
void delete_projectile(Projectile **dest, Projectile *proj);
void delete_projectiles(Projectile **dest);
void draw_projectiles(Projectile *projs);
int collision_projectile(Projectile *p);
void process_projectiles(Projectile **projs, short collision);

void linear(Projectile *p, int t);
#endif