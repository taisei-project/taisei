/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "util.h"
#include "resource/sprite.h"
#include "resource/shader_program.h"
#include "color.h"
#include "objectpool.h"
#include "renderer/api.h"
#include "entity.h"

#ifdef DEBUG
	#define PROJ_DEBUG
#endif

enum {
	RULE_ARGC = 4
};

typedef struct Projectile Projectile;
typedef LIST_ANCHOR(Projectile) ProjectileList;

typedef int (*ProjRule)(Projectile *p, int t);
typedef void (*ProjDrawRule)(Projectile *p, int t);
typedef bool (*ProjPredicate)(Projectile *p);

typedef enum {
	_InvalidProj,

	EnemyProj, // hazard, collides with player
	DeadProj,  // no collision, will be converted to a BPoint item shortly
	Particle,  // no collision, not a hazard
	FakeProj,  // hazard, but no collision
	PlrProj,   // collides with enemies and bosses
} ProjType;

typedef enum ProjFlags {
	// PFLAG_DRAWADD = (1 << 0),
	// PFLAG_DRAWSUB = (1 << 1),
	PFLAG_NOSPAWNZOOM = (1 << 2),
	PFLAG_NOGRAZE = (1 << 3),
	PFLAG_NOCLEAR = (1 << 4),
	PFLAG_NOCLEAREFFECT = (1 << 5),
	PFLAG_NOCOLLISIONEFFECT = (1 << 6),
	PFLAG_NOCLEARBONUS = (1 << 7),
	PFLAG_GRAZESPAM = (1 << 8),
	PFLAG_NOREFLECT = (1 << 9),
	PFLAG_REQUIREDPARTICLE = (1 << 10),
} ProjFlags;

// attr_deprecated("Use .blend = BLEND_ADD instead")
static const ProjFlags PFLAG_DRAWADD = (1 << 0);

// attr_deprecated("Use .blend = BLEND_SUB instead")
static const ProjFlags PFLAG_DRAWSUB = (1 << 1);

// FIXME: prototype stuff awkwardly shoved in this header because of dependency cycles.
typedef struct ProjPrototype ProjPrototype;

struct Projectile {
	ENTITY_INTERFACE_NAMED(Projectile, ent);

	complex pos;
	complex pos0;
	complex prevpos; // used to lerp trajectory for collision detection; set this to pos if you intend to "teleport" the projectile in the rule!
	complex size; // affects out-of-viewport culling and grazing
	complex collision_size; // affects collision with player (TODO: make this work for player projectiles too?)
	complex args[RULE_ARGC];
	ProjRule rule;
	ProjDrawRule draw_rule;
	ShaderProgram *shader;
	Sprite *sprite;
	ProjPrototype *proto;
	Color color;
	float shader_custom_param; // FIXME: see renderer/api.c: struct SpriteParams
	BlendMode blend;
	int birthtime;
	float angle;
	ProjType type;
	int max_viewport_dist;
	ProjFlags flags;

	short graze_counter;
	int graze_counter_reset_timer;

	// XXX: this is in frames of course, but needs to be float
	// to avoid subtle truncation and integer division gotchas.
	float timeout;

#ifdef PROJ_DEBUG
	DebugInfo debug;
#endif
};

typedef struct ProjArgs {
	const char *sprite;
	complex pos;
	Color color;
	ProjRule rule;
	complex args[RULE_ARGC];
	float angle;
	ProjFlags flags;
	BlendMode blend;
	ProjDrawRule draw_rule;
	const char *shader;
	ShaderProgram *shader_ptr;
	ProjPrototype *proto;
	float shader_custom_param; // FIXME: see renderer/api.c: struct SpriteParams
	ProjectileList *dest;
	ProjType type;
	Sprite *sprite_ptr;
	complex size; // affects default draw order, out-of-viewport culling, and grazing
	complex collision_size; // affects collision with player (TODO: make this work for player projectiles too?)
	int max_viewport_dist;
	drawlayer_t layer;

	// XXX: this is in frames of course, but needs to be float
	// to avoid subtle truncation and integer division gotchas.
	float timeout;
} /* attr_designated_init */ ProjArgs;

struct ProjPrototype {
	void (*preload)(ProjPrototype *proto);
	void (*process_args)(ProjPrototype *proto, ProjArgs *args);
	void (*init_projectile)(ProjPrototype *proto, Projectile *p);
	void (*deinit_projectile)(ProjPrototype *proto, Projectile *p);
	void *private;
};

#define PP(name) \
	extern ProjPrototype _pp_##name; \
	extern ProjPrototype *pp_##name; \

#include "projectile_prototypes/all.inc.h"

#define PARTICLE_ADDITIVE_SUBLAYER (1 << 3)

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
	void *entity; // TODO: make this use the actual entity API
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

void delete_projectile(ProjectileList *projlist, Projectile *proj);
void delete_projectiles(ProjectileList *projlist);

void calc_projectile_collision(Projectile *p, ProjCollisionResult *out_col);
void apply_projectile_collision(ProjectileList *projlist, Projectile *p, ProjCollisionResult *col);
int trace_projectile(Projectile *p, ProjCollisionResult *out_col, ProjCollisionType stopflags, int timeofs);
bool projectile_in_viewport(Projectile *proj);
void process_projectiles(ProjectileList *projlist, bool collision);
bool projectile_is_clearable(Projectile *p);

Projectile* spawn_projectile_collision_effect(Projectile *proj);
Projectile* spawn_projectile_clear_effect(Projectile *proj);

void projectile_set_prototype(Projectile *p, ProjPrototype *proto);

bool clear_projectile(ProjectileList *projlist, Projectile *proj, bool force, bool now);

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

void Blast(Projectile *p, int t);

void projectiles_preload(void);

complex projectile_graze_size(Projectile *p);
