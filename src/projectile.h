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
typedef int (*ProjRule)(struct Projectile *p, int t);
typedef void (*ProjDRule)(struct Projectile *p, int t);

#define Pr (&global.projs)
#define Pa (&global.particles)

enum {
	RULE_ARGC = 4
};

typedef struct Projectile {
	struct Projectile *next;
	struct Projectile *prev;
	
	complex pos;
	complex pos0;
	
	long birthtime;
	
	float angle;
	void *parent;
	
	ProjRule rule;
	ProjDRule draw;
	Texture *tex;
	
	enum { PlrProj, FairyProj, DeadProj, Particle } type;
	
	Color *clr;
	
	complex args[RULE_ARGC];
} Projectile;

void load_projectiles();

Color *rgba(float r, float g, float b, float a);

inline complex *rarg(complex arg0, ...);
inline Color *rgb(float r, float g, float b);

Projectile *create_particle(char *name, complex pos, Color *clr, ProjDRule draw, ProjRule rule, complex *a);
Projectile *create_projectile(char *name, complex pos, Color *clr, ProjRule rule, complex *a);
Projectile *create_projectile_p(Projectile **dest, Texture *tex, complex pos, Color *clr, ProjDRule draw, ProjRule rule, complex *args);
void delete_projectile(Projectile **dest, Projectile *proj);
void delete_projectiles(Projectile **dest);
void draw_projectiles(Projectile *projs);
int collision_projectile(Projectile *p);
void process_projectiles(Projectile **projs, char collision);

Projectile *get_proj(Projectile *hay, int birthtime);

int linear(Projectile *p, int t);
void ProjDraw(Projectile *p, int t);

void Shrink(Projectile *p, int t);
int bullet_flare_move(Projectile *p, int t);

void Fade(Projectile *p, int t);
int timeout(Projectile *p, int t);

void DeathShrink(Projectile *p, int t);
int timeout_linear(Projectile *p, int t);

#endif