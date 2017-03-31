/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef PROJECTILE
#define PROJECTILE

#include "util.h"
#include "resource/texture.h"
#include "color.h"

#include <stdbool.h>

enum {
	RULE_ARGC = 4
};

struct Projectile;
typedef int (*ProjRule)(struct Projectile *p, int t);
typedef void (*ProjDRule)(struct Projectile *p, int t);

typedef enum {
	FairyProj,
	DeadProj,
	Particle,
	PlrProj
} ProjType;

typedef struct Projectile {
	struct Projectile *next;
	struct Projectile *prev;

	complex pos;
	complex pos0;

	long birthtime;

	float angle;

	ProjRule rule;
	ProjDRule draw;
	Texture *tex;

	ProjType type;

	Color clr;

	complex args[RULE_ARGC];
	int grazed;
} Projectile;

#define create_particle3c(n,p,c,d,r,a1,a2,a3) create_particle4c(n,p,c,d,r,a1,a2,a3,0)
#define create_particle2c(n,p,c,d,r,a1,a2) create_particle4c(n,p,c,d,r,a1,a2,0,0)
#define create_particle1c(n,p,c,d,r,a1) create_particle4c(n,p,c,d,r,a1,0,0,0)

#define create_projectile3c(n,p,c,r,a1,a2,a3) create_projectile4c(n,p,c,r,a1,a2,a3,0)
#define create_projectile2c(n,p,c,r,a1,a2) create_projectile4c(n,p,c,r,a1,a2,0,0)
#define create_projectile1c(n,p,c,r,a1) create_projectile4c(n,p,c,r,a1,0,0,0)

Projectile *create_particle4c(char *name, complex pos, Color clr, ProjDRule draw, ProjRule rule, complex a1, complex a2, complex a3, complex a4);
Projectile *create_projectile4c(char *name, complex pos, Color clr, ProjRule rule, complex a1, complex a2, complex a3, complex a4);
Projectile *create_projectile_p(Projectile **dest, Texture *tex, complex pos, Color clr, ProjDRule draw, ProjRule rule, complex a1, complex a2, complex a3, complex a4);
void delete_projectile(Projectile **dest, Projectile *proj);
void delete_projectiles(Projectile **dest);
void draw_projectiles(Projectile *projs);
int collision_projectile(Projectile *p);
void process_projectiles(Projectile **projs, bool collision);

Projectile *get_proj(Projectile *hay, int birthtime);

int linear(Projectile *p, int t);
int accelerated(Projectile *p, int t);
int asymptotic(Projectile *p, int t);
void ProjDraw(Projectile *p, int t);
void _ProjDraw(Projectile *p, int t);
void PartDraw(Projectile *p, int t);

void ProjDrawAdd(Projectile *p, int t);
void ProjDrawSub(Projectile *p, int t);

void Blast(Projectile *p, int t);

void Shrink(Projectile *p, int t);
void ShrinkAdd(Projectile *p, int t);
void GrowFade(Projectile *p, int t);
void GrowFadeAdd(Projectile *p, int t);
int bullet_flare_move(Projectile *p, int t);

void Fade(Projectile *p, int t);
void FadeAdd(Projectile *p, int t);
int timeout(Projectile *p, int t);

void DeathShrink(Projectile *p, int t);
int timeout_linear(Projectile *p, int t);

void Petal(Projectile *p, int t);
void petal_explosion(int n, complex pos);

void projectiles_preload(void);

#endif
