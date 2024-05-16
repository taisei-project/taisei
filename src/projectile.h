/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "util.h"
#include "resource/resource.h"
#include "resource/sprite.h"
#include "resource/shader_program.h"
#include "color.h"
#include "objectpool.h"
#include "renderer/api.h"
#include "entity.h"
#include "move.h"
#include "coroutine.h"

#if defined(DEBUG) && !defined(RNG_API_CHECK)
	#define PROJ_DEBUG
#endif

#ifdef PROJ_DEBUG
	#define IF_PROJ_DEBUG(...) __VA_ARGS__
#else
	#define IF_PROJ_DEBUG(...)
#endif

enum {
	PROJ_DRAWRULE_NUMARGS = 4
};

typedef LIST_ANCHOR(Projectile) ProjectileList;
typedef LIST_INTERFACE(Projectile) ProjectileListInterface;

typedef int (*ProjRule)(Projectile *p, int t);
typedef bool (*ProjPredicate)(Projectile *p);

typedef union {
	float as_float[2];
	cmplxf as_cmplx;
	void *as_ptr;
} ProjDrawRuleArgs[PROJ_DRAWRULE_NUMARGS];

typedef struct ProjDrawRule {
	void (*func)(Projectile *p, int t, ProjDrawRuleArgs args);
	ProjDrawRuleArgs args;
} ProjDrawRule;

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
	PFLAG_NOMOVE = (1 << 8),                // [ALL] Don't call move_update for this projectile.
	PFLAG_NOREFLECT = (1 << 9),             // [ALL] Don't render a "reflection" of this on the Stage 1 water surface.
	PFLAG_REQUIREDPARTICLE = (1 << 10),     // [PROJ_PARTICLE] Visible at "minimal" particles setting.
	PFLAG_PLRSPECIALPARTICLE = (1 << 11),   // [PROJ_PARTICLE] Apply Power Surge effect to this particle, as if it was a PROJ_PLAYER.
	PFLAG_NOCOLLISION = (1 << 12),          // [PROJ_ENEMY, PROJ_PLAYER] Disable collision detection.
	PFLAG_INTERNAL_DEAD = (1 << 13),        // [ALL] Delete as soon as processed. (internal flag, do not use)
	PFLAG_MANUALANGLE = (1 << 14),          // [ALL] Don't automatically update the angle.
	PFLAG_NOAUTOREMOVE = (1 << 15),         // [ALL] Don't automatically remove when outside viewport.
	PFLAG_INDESTRUCTIBLE = (1 << 16),       // [PROJ_ENEMY, PROJ_PLAYER] Projectile doesn't get destroyed on collision.

	PFLAG_NOSPAWNEFFECTS = PFLAG_NOSPAWNFADE | PFLAG_NOSPAWNFLARE,
} ProjFlags;

typedef enum ProjCollisionType {
	PCOL_NONE                = 0,
	PCOL_ENTITY              = (1 << 0),
	PCOL_PLAYER_GRAZE        = (1 << 1),
	PCOL_VOID                = (1 << 2),
} ProjCollisionType;

typedef struct ProjCollisionResult {
	ProjCollisionType type;
	bool fatal; // for the projectile
	cmplx location;
	DamageInfo damage;
	EntityInterface *entity;
} ProjCollisionResult;

// FIXME: prototype stuff awkwardly shoved in this header because of dependency cycles.
typedef struct ProjPrototype ProjPrototype;

DEFINE_ENTITY_TYPE(Projectile, {
	cmplx pos;
	cmplx pos0;
	cmplx prevpos; // used to lerp trajectory for collision detection; set this to pos if you intend to "teleport" the projectile in the rule!
	cmplx size; // affects out-of-viewport culling and grazing
	cmplx collision_size; // affects collision with player (TODO: make this work for player projectiles too?)
	ShaderProgram *shader;
	Sprite *sprite;
	ProjPrototype *proto;
	ProjDrawRule draw_rule;

	/*
	 * This field is usually NULL except during handling of "collision" and "killed" events.
	 *
	 * "Collision" events are run before the collision result is applied, so they may modify this
	 * struct to affect the outcome.
	 *
	 * "Killed" events are run after the collision has happened, and thus shouldn't write to this
	 * field.
	 *
	 * Note that this may be NULL during "killed" events or cancelled "collision" events.
	 * In the former case this usually means the projectile was killed manually.
	 *
	 * Out of bounds auto-removals are considered collisions with "void" (PCOL_VOID).
	 *
	 * The pointer becomes invalid as soon as the event handler yields.
	*/
	ProjCollisionResult *collision;

	MoveParams move;
	COEVENTS_ARRAY(
		collision,
		cleared,
		killed
	) events;
	Color color;
	BlendMode blend;
	int birthtime;
	float damage;
	float angle;
	float angle_delta;

	cmplx _cached_delta_pos;
	real _cached_angle;

	ProjType type;
	DamageType damage_type;
	int max_viewport_dist;
	ProjFlags flags;
	uint clear_flags;

	cmplxf scale;
	float opacity;

	// XXX: this is in frames of course, but needs to be float
	// to avoid subtle truncation and integer division gotchas.
	float timeout;

	int graze_counter_reset_timer;
	int graze_cooldown;
	short graze_counter;

	IF_PROJ_DEBUG(
		DebugInfo debug;
	)
});

typedef struct ProjArgs {
	ProjPrototype *proto;
	const Color *color;
	const char *sprite;
	Sprite *sprite_ptr;
	const char *shader;
	ShaderProgram *shader_ptr;
	ProjectileList *dest;
	ProjDrawRule draw_rule;
	cmplx pos;
	cmplx size; // affects default draw order, out-of-viewport culling, and grazing
	cmplx collision_size; // affects collision with player (TODO: make this work for player projectiles too?)
	MoveParams move;
	ProjType type;
	ProjFlags flags;
	BlendMode blend;
	float angle;
	float angle_delta;
	float damage;
	DamageType damage_type;
	int max_viewport_dist;
	drawlayer_t layer;

	cmplxf scale;
	float opacity;

	// XXX: this is in frames of course, but needs to be float
	// to avoid subtle truncation and integer division gotchas.
	float timeout;
} attr_designated_init ProjArgs;

struct ProjPrototype {
	void (*preload)(ProjPrototype *proto, ResourceGroup *rg);
	void (*process_args)(ProjPrototype *proto, ProjArgs *args);
	void (*reset)(ProjPrototype *proto);
	void (*init_projectile)(ProjPrototype *proto, Projectile *p);
	void (*deinit_projectile)(ProjPrototype *proto, Projectile *p);
	void *private;
};

#define PP(name) \
	extern ProjPrototype _pp_##name; \
	extern ProjPrototype *pp_##name; \

#include "projectile_prototypes/all.inc.h"

Projectile *create_projectile(ProjArgs *args)
	attr_nonnull_all attr_returns_nonnull attr_returns_max_aligned;
Projectile *create_particle(ProjArgs *args)
	attr_nonnull_all attr_returns_nonnull attr_returns_max_aligned;

#ifdef PROJ_DEBUG
	Projectile *_proj_attach_dbginfo(Projectile *p, DebugInfo *dbg, const char *callsite_str)
		attr_nonnull_all;
	#define _PROJ_WRAP_SPAWN(p) _proj_attach_dbginfo((p), _DEBUG_INFO_PTR_, #p)
#else
	#define _PROJ_WRAP_SPAWN(p) (p)
#endif

#define _PROJ_GENERIC_SPAWN(constructor, ...) _PROJ_WRAP_SPAWN((constructor)((&(ProjArgs) { __VA_ARGS__ })))

#define PROJECTILE(...) _PROJ_GENERIC_SPAWN(create_projectile, __VA_ARGS__)
#define PARTICLE(...) _PROJ_GENERIC_SPAWN(create_particle, __VA_ARGS__)

void delete_projectiles(ProjectileList *projlist) attr_nonnull_all;

void calc_projectile_collision(Projectile *p, ProjCollisionResult *out_col) attr_nonnull_all;
void apply_projectile_collision(ProjectileList *projlist, Projectile *p, ProjCollisionResult *col) attr_nonnull_all;
int trace_projectile(Projectile *p, ProjCollisionResult *out_col, ProjCollisionType stopflags, int timeofs) attr_nonnull_all;
bool projectile_in_viewport(Projectile *proj) attr_nonnull_all;
void process_projectiles(ProjectileList *projlist, bool collision) attr_hot attr_nonnull_all;
bool projectile_is_clearable(Projectile *p) attr_nonnull_all;

Projectile *spawn_projectile_collision_effect(Projectile *proj) attr_nonnull_all;
Projectile *spawn_projectile_clear_effect(Projectile *proj) attr_nonnull_all;
Projectile *spawn_projectile_highlight_effect(Projectile *proj) attr_nonnull_all;

void projectile_set_prototype(Projectile *p, ProjPrototype *proto) attr_nonnull(1);
void projectile_set_layer(Projectile *p, drawlayer_t layer) attr_nonnull_all;

bool clear_projectile(Projectile *proj, uint flags) attr_nonnull_all;
void kill_projectile(Projectile *proj) attr_nonnull_all;

ProjDrawRule pdraw_basic(void);
ProjDrawRule pdraw_timeout_scalefade_exp(cmplxf scale0, cmplxf scale1, float opacity0, float opacity1, float opacity_exp);
ProjDrawRule pdraw_timeout_scalefade(cmplxf scale0, cmplxf scale1, float opacity0, float opacity1);
ProjDrawRule pdraw_timeout_scale(cmplxf scale0, cmplxf scale1);
ProjDrawRule pdraw_timeout_fade(float opacity0, float opacity1);
ProjDrawRule pdraw_petal(float rot_angle, vec3 rot_axis);
ProjDrawRule pdraw_petal_random(void);
ProjDrawRule pdraw_blast(void);

void petal_explosion(int n, cmplx pos);

void projectiles_preload(ResourceGroup *rg);
void projectiles_free(void);

cmplx projectile_graze_size(Projectile *p);
float projectile_timeout_factor(Projectile *p);
int projectile_time(Projectile *p);

SpriteParams projectile_sprite_params(Projectile *proj, SpriteParamsBuffer *spbuf);
