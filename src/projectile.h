/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_projectile_h
#define IGUARD_projectile_h

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
typedef LIST_INTERFACE(Projectile) ProjectileListInterface;

typedef int (*ProjRule)(Projectile *p, int t);
typedef void (*ProjDrawRule)(Projectile *p, int t);
typedef bool (*ProjPredicate)(Projectile *p);

typedef enum {
	PROJ_INVALID,

	PROJ_ENEMY,    // hazard, collides with player
	PROJ_DEAD,     // no collision, will be cleared shortly
	PROJ_PARTICLE, // no collision, not a hazard
	PROJ_PLAYER,   // collides with enemies and bosses
} ProjType;

typedef enum ProjFlags {
	// NOTE: Many of these flags are only meaningful for a specific type(s) of projectile.
	// The relevant ProjType enum(s) are specified in [square brackets], or [ALL] if all apply.

	PFLAG_NOSPAWNFLARE = (1 << 0),          // [PROJ_ENEMY] Don't spawn the standard particle effect when spawned.
	PFLAG_NOSPAWNFADE = (1 << 1),           // [PROJ_ENEMY] Don't fade in when spawned; assume 100% opacity immediately.
	PFLAG_NOGRAZE = (1 << 3),               // [PROJ_ENEMY] Can't be grazed.
	PFLAG_NOCLEAR = (1 << 4),               // [PROJ_ENEMY] Can't be cleared (unless forced).
	PFLAG_NOCLEAREFFECT = (1 << 5),         // [PROJ_ENEMY, PROJ_DEAD] Don't spawn the standard particle effect when cleared.
	PFLAG_NOCOLLISIONEFFECT = (1 << 6),     // [PROJ_ENEMY, PROJ_DEAD, PROJ_PLAYER] Don't spawn the standard particle effect on collision.
	PFLAG_NOCLEARBONUS = (1 << 7),          // [PROJ_ENEMY, PROJ_DEAD] Don't spawn any bonus items on clear.
	// PFLAG_RESERVED = (1 << 8),
	PFLAG_NOREFLECT = (1 << 9),             // [ALL] Don't render a "reflection" of this on the Stage 1 water surface.
	PFLAG_REQUIREDPARTICLE = (1 << 10),     // [PROJ_PARTICLE] Visible at "minimal" particles setting.
	PFLAG_PLRSPECIALPARTICLE = (1 << 11),   // [PROJ_PARTICLE] Apply Power Surge effect to this particle, as if it was a PROJ_PLAYER.
	PFLAG_NOCOLLISION = (1 << 12),          // [PROJ_ENEMY, PROJ_PLAYER] Disable collision detection.

	PFLAG_NOSPAWNEFFECTS = PFLAG_NOSPAWNFADE | PFLAG_NOSPAWNFLARE,
} ProjFlags;

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
	ShaderCustomParams shader_params;
	BlendMode blend;
	int birthtime;
	float damage;
	float angle;
	ProjType type;
	DamageType damage_type;
	int max_viewport_dist;
	ProjFlags flags;
	uint clear_flags;

	// XXX: this is in frames of course, but needs to be float
	// to avoid subtle truncation and integer division gotchas.
	float timeout;

	int graze_counter_reset_timer;
	int graze_cooldown;
	short graze_counter;

#ifdef PROJ_DEBUG
	DebugInfo debug;
#endif
};

typedef struct ProjArgs {
	ProjPrototype *proto;
	const Color *color;
	const char *sprite;
	Sprite *sprite_ptr;
	const char *shader;
	ShaderProgram *shader_ptr;
	const ShaderCustomParams *shader_params;
	ProjectileList *dest;
	ProjRule rule;
	complex args[RULE_ARGC];
	ProjDrawRule draw_rule;
	complex pos;
	complex size; // affects default draw order, out-of-viewport culling, and grazing
	complex collision_size; // affects collision with player (TODO: make this work for player projectiles too?)
	ProjType type;
	ProjFlags flags;
	BlendMode blend;
	float angle;
	float damage;
	DamageType damage_type;
	int max_viewport_dist;
	drawlayer_t layer;

	// XXX: this is in frames of course, but needs to be float
	// to avoid subtle truncation and integer division gotchas.
	float timeout;
} attr_designated_init ProjArgs;

struct ProjPrototype {
	void (*preload)(ProjPrototype *proto, ResourceRefGroup *rg);
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
	PCOL_ENTITY              = (1 << 0),
	PCOL_PLAYER_GRAZE        = (1 << 1),
	PCOL_VOID                = (1 << 2),
} ProjCollisionType;

typedef struct ProjCollisionResult {
	ProjCollisionType type;
	bool fatal; // for the projectile
	complex location;
	DamageInfo damage;
	EntityInterface *entity;
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
Projectile* spawn_projectile_highlight_effect(Projectile *proj);

void projectile_set_prototype(Projectile *p, ProjPrototype *proto);

bool clear_projectile(Projectile *proj, uint flags);

int linear(Projectile *p, int t);
int accelerated(Projectile *p, int t);
int asymptotic(Projectile *p, int t);

void ProjDrawCore(Projectile *proj, const Color *c);
void ProjDraw(Projectile *p, int t);
void ProjNoDraw(Projectile *proj, int t);

void Shrink(Projectile *p, int t);
void DeathShrink(Projectile *p, int t);
void Fade(Projectile *p, int t);
void GrowFade(Projectile *p, int t);
void ScaleFade(Projectile *p, int t);
void ScaleSquaredFade(Projectile *p, int t);

void Petal(Projectile *p, int t);
void petal_explosion(int n, complex pos);

void Blast(Projectile *p, int t);

void projectiles_preload(ResourceRefGroup *rg);
void projectiles_free(void);

complex projectile_graze_size(Projectile *p);

#endif // IGUARD_projectile_h
