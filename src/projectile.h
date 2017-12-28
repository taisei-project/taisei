/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "util.h"
#include "resource/texture.h"
#include "color.h"
#include "recolor.h"
#include "objectpool.h"

#ifdef DEBUG
	#define PROJ_DEBUG
#endif

enum {
	RULE_ARGC = 4
};

typedef struct Projectile Projectile;

typedef int (*ProjRule)(Projectile *p, int t);
typedef void (*ProjDrawRule)(Projectile *p, int t);
typedef void (*ProjColorTransformRule)(Projectile *p, int t, Color c, ColorTransform *out);
typedef bool (*ProjPredicate)(Projectile *p);

void static_clrtransform_bullet(Color c, ColorTransform *out);
void static_clrtransform_particle(Color c, ColorTransform *out);

void proj_clrtransform_bullet(Projectile *p, int t, Color c, ColorTransform *out);
void proj_clrtransform_particle(Projectile *p, int t, Color c, ColorTransform *out);

typedef enum {
	_InvalidProj,

	EnemyProj, // hazard, collides with player
	DeadProj, // no collision, will be converted to a BPoint item shortly
	Particle, // no collision, not a hazard
	FakeProj, // hazard, but no collision
	PlrProj, // collides with enemies and bosses
} ProjType;

typedef enum ProjFlags {
	PFLAG_DRAWADD = (1 << 0),
	PFLAG_DRAWSUB = (1 << 1),
	PFLAG_NOSPAWNZOOM = (1 << 2),
	PFLAG_NOGRAZE = (1 << 3),
} ProjFlags;

struct Projectile {
	OBJECT_INTERFACE(Projectile);

	complex pos;
	complex pos0;
	complex size; // this is currently ignored if tex is not NULL.
	complex args[RULE_ARGC];
	ProjRule rule;
	ProjDrawRule draw_rule;
	ProjColorTransformRule color_transform_rule;
	Texture *tex;
	Color color;
	int birthtime;
	float angle;
	ProjType type;
	int max_viewport_dist;
	ProjFlags flags;
	bool grazed;

#ifdef PROJ_DEBUG
	DebugInfo debug;
#endif
};

typedef struct ProjArgs {
	const char *texture;
	complex pos;
	Color color;
	ProjRule rule;
	complex args[RULE_ARGC];
	float angle;
	ProjFlags flags;
	ProjDrawRule draw_rule;
	ProjColorTransformRule color_transform_rule;
	Projectile **dest;
	ProjType type;
	Texture *texture_ptr;
	complex size;
	int max_viewport_dist;
	ListInsertionRule insertion_rule;
} ProjArgs;

typedef enum ProjCollisionType {
	PCOL_NONE                = 0,
	PCOL_ENEMY               = (1 << 0),
	PCOL_BOSS                = (1 << 1),
	PCOL_PLAYER              = (1 << 2),
	PCOL_PLAYER_GRAZE        = (1 << 3),
	PCOL_VOID                = (1 << 4),
} ProjCollisionType;

typedef struct ProjCollisionResult {
	ProjCollisionType type;
	bool fatal; // for the projectile
	complex location;
	int damage;
	void *entity;
} ProjCollisionResult;

Projectile* create_projectile(ProjArgs *args);
Projectile* create_particle(ProjArgs *args);

#ifdef PROJ_DEBUG
	Projectile* _proj_attach_dbginfo(Projectile *p, DebugInfo *dbg, const char *callsite_str);
	#define _PROJ_WRAP_SPAWN(p) _proj_attach_dbginfo((p), _DEBUG_INFO_PTR_, #p)
#else
	#define _PROJ_WRAP_SPAWN(p) (p)
#endif

#define _PROJ_GENERIC_SPAWN(constructor, ...) _PROJ_WRAP_SPAWN((constructor)((&(ProjArgs) { __VA_ARGS__ })))

#define PROJECTILE(...) _PROJ_GENERIC_SPAWN(create_projectile, __VA_ARGS__)
#define PARTICLE(...) _PROJ_GENERIC_SPAWN(create_particle, __VA_ARGS__)

void delete_projectile(Projectile **dest, Projectile *proj);
void delete_projectiles(Projectile **dest);
void draw_projectiles(Projectile *projs, ProjPredicate predicate);

void calc_projectile_collision(Projectile *p, ProjCollisionResult *out_col);
void apply_projectile_collision(Projectile **projlist, Projectile *p, ProjCollisionResult *col);
void trace_projectile(Projectile *p, ProjCollisionResult *out_col, ProjCollisionType stopflags);
bool projectile_in_viewport(Projectile *proj);
void process_projectiles(Projectile **projs, bool collision);

int linear(Projectile *p, int t);
int accelerated(Projectile *p, int t);
int asymptotic(Projectile *p, int t);

void ProjDrawCore(Projectile *proj, Color c);
void ProjDraw(Projectile *p, int t);
void ProjNoDraw(Projectile *proj, int t);

void Shrink(Projectile *p, int t);
void DeathShrink(Projectile *p, int t);
void Fade(Projectile *p, int t);
void GrowFade(Projectile *p, int t);
void ScaleFade(Projectile *p, int t);


void Petal(Projectile *p, int t);
void petal_explosion(int n, complex pos);

int timeout(Projectile *p, int t);
int timeout_linear(Projectile *p, int t);

int blast_timeout(Projectile *p, int t);
void Blast(Projectile *p, int t);

void projectiles_preload(void);

List* proj_insert_sizeprio(List **dest, List *elem) __attribute__((hot));
