/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "projectile.h"

#include <stdlib.h>
#include "global.h"
#include "list.h"
#include "stageobjects.h"

static ProjArgs defaults_proj = {
	.sprite = "proj/",
	.draw_rule = ProjDraw,
	.dest = &global.projs,
	.type = EnemyProj,
	.color = RGB(1, 1, 1),
	.blend = BLEND_PREMUL_ALPHA,
	.shader = "sprite_bullet",
	.layer = LAYER_BULLET,
};

static ProjArgs defaults_part = {
	.sprite = "part/",
	.draw_rule = ProjDraw,
	.dest = &global.particles,
	.type = Particle,
	.color = RGB(1, 1, 1),
	.blend = BLEND_PREMUL_ALPHA,
	.shader = "sprite_default",
	.layer = LAYER_PARTICLE_HIGH,
};

static void process_projectile_args(ProjArgs *args, ProjArgs *defaults) {
	// Detect the deprecated way to spawn projectiles and remap it to prototypes,
	// if possible. This is so that I can conserve the remains of my sanity by
	// not having to convert every single PROJECTILE call in the game manually
	// and in one go. So, TODO: convert every single PROJECTILE call in the game
	// and remove this mess.

	if(!args->proto && args->sprite && args->size == 0) {
		static struct {
			const char *name;
			ProjPrototype *proto;
		} proto_map[] = {
			#define PP(name) { #name, &_pp_##name },
			#include "projectile_prototypes/all.inc.h"
		};

		for(int i = 0; i < sizeof(proto_map)/sizeof(*proto_map); ++i) {
			if(!strcmp(args->sprite, proto_map[i].name)) {
				args->proto = proto_map[i].proto;
				args->sprite = NULL;
				break;
			}
		}
	}

	if(args->proto && args->proto->process_args) {
		args->proto->process_args(args->proto, args);
		return;
	}

	// TODO: move this stuff into prototypes along with the defaults?

	if(args->sprite) {
		args->sprite_ptr = prefix_get_sprite(args->sprite, defaults->sprite);
	}

	if(!args->shader_ptr) {
		if(args->shader) {
			args->shader_ptr = r_shader_get(args->shader);
		} else {
			args->shader_ptr = defaults->shader_ptr;
		}
	}

	if(!args->draw_rule) {
		args->draw_rule = ProjDraw;
	}

	if(!args->blend) {
		args->blend = defaults->blend;
	}

	if(!args->dest) {
		args->dest = defaults->dest;
	}

	if(!args->type) {
		args->type = defaults->type;
	}

	if(!args->color) {
		args->color = defaults->color;
	}

	if(!args->max_viewport_dist && (args->type == Particle || args->type >= PlrProj)) {
		args->max_viewport_dist = 300;
	}

	if(!args->layer) {
		if(args->type >= PlrProj) {
			args->layer = LAYER_PLAYER_SHOT;
		} else {
			args->layer = defaults->layer;
		}
	}
}

static void projectile_size(Projectile *p, double *w, double *h) {
	if(p->type == Particle && p->sprite != NULL) {
		*w = p->sprite->w;
		*h = p->sprite->h;
	} else {
		*w = creal(p->size);
		*h = cimag(p->size);
	}

	assert(*w > 0);
	assert(*h > 0);
}

static double projectile_rect_area(Projectile *p) {
	double w, h;
	projectile_size(p, &w, &h);
	return w * h;
}

static void ent_draw_projectile(EntityInterface *ent);

static inline char* event_name(int ev) {
	switch(ev) {
		case EVENT_BIRTH: return "EVENT_BIRTH";
		case EVENT_DEATH: return "EVENT_DEATH";
	}

	log_fatal("Bad event %i", ev);
}

static inline int proj_call_rule(Projectile *p, int t) {
	int result = ACTION_NONE;

	if(p->timeout > 0 && t >= p->timeout) {
		result = ACTION_DESTROY;
	} else if(p->rule != NULL) {
		result = p->rule(p, t);

		if(t < 0 && result != ACTION_ACK) {
			set_debug_info(&p->debug);
			log_fatal(
				"Projectile rule didn't acknowledge %s (returned %i, expected %i)",
				event_name(t),
				result,
				ACTION_ACK
			);
		}
	}

	if(/*t == 0 ||*/ t == EVENT_BIRTH) {
		p->prevpos = p->pos;
	}

	return result;
}

void projectile_set_prototype(Projectile *p, ProjPrototype *proto) {
	if(p->proto && p->proto->deinit_projectile) {
		p->proto->deinit_projectile(p->proto, p);
	}

	if(proto && proto->init_projectile) {
		proto->init_projectile(proto, p);
	}

	p->proto = proto;
}

complex projectile_graze_size(Projectile *p) {
	if(p->type == EnemyProj && !(p->flags & PFLAG_NOGRAZE) && p->graze_counter < 5) {
		complex s = (p->size * 420 /* graze it */) / (2 * p->graze_counter + 1);
		return sqrt(creal(s)) + sqrt(cimag(s)) * I;
	}

	return 0;
}

static Projectile* _create_projectile(ProjArgs *args) {
	if(IN_DRAW_CODE) {
		log_fatal("Tried to spawn a projectile while in drawing code");
	}

	Projectile *p = (Projectile*)objpool_acquire(stage_object_pools.projectiles);

	p->birthtime = global.frames;
	p->pos = p->pos0 = p->prevpos = args->pos;
	p->angle = args->angle;
	p->rule = args->rule;
	p->draw_rule = args->draw_rule;
	p->shader = args->shader_ptr;
	p->shader_custom_param = args->shader_custom_param;
	p->blend = args->blend;
	p->sprite = args->sprite_ptr;
	p->type = args->type;
	p->color = *args->color;
	p->max_viewport_dist = args->max_viewport_dist;
	p->size = args->size;
	p->collision_size = args->collision_size;
	p->flags = args->flags;
	p->timeout = args->timeout;

	memcpy(p->args, args->args, sizeof(p->args));

	p->ent.draw_layer = args->layer;
	p->ent.draw_func = ent_draw_projectile;

	projectile_set_prototype(p, args->proto);
	// p->collision_size *= 10;
	// p->size *= 5;

	if((p->type == EnemyProj || p->type >= PlrProj) && (creal(p->size) <= 0 || cimag(p->size) <= 0)) {
		// FIXME: debug info is not actually attached at this point!
		set_debug_info(&p->debug);
		log_fatal("Tried to spawn a projectile with invalid size %f x %f", creal(p->size), cimag(p->size));
	}

	if(!(p->ent.draw_layer & LAYER_LOW_MASK)) {
		switch(p->type) {
			case EnemyProj:
			case FakeProj: {
				// 1. Large projectiles go below smaller ones.
				drawlayer_low_t sublayer = (LAYER_LOW_MASK - (drawlayer_low_t)projectile_rect_area(p));

				// 2. Additive projectiles go below others.
				sublayer <<= 1;
				sublayer |= 1 * (p->blend == BLEND_ADD || p->flags & PFLAG_DRAWADD);

				// If specific blending order is required, then you should set up the sublayer manually.
				p->ent.draw_layer |= sublayer;
				break;
			}

			case Particle: {
				// 1. Additive particles go above others.
				drawlayer_low_t sublayer = (p->blend == BLEND_ADD || p->flags & PFLAG_DRAWADD) * PARTICLE_ADDITIVE_SUBLAYER;

				// If specific blending order is required, then you should set up the sublayer manually.
				p->ent.draw_layer |= sublayer;
				break;
			}

			default:
				break;
		}
	}

	ent_register(&p->ent, ENT_PROJECTILE);

	// TODO: Maybe allow ACTION_DESTROY here?
	// But in that case, code that uses this function's return value must be careful to not dereference a NULL pointer.
	proj_call_rule(p, EVENT_BIRTH);

	return alist_append(args->dest, p);
}

Projectile* create_projectile(ProjArgs *args) {
	process_projectile_args(args, &defaults_proj);
	return _create_projectile(args);
}

Projectile* create_particle(ProjArgs *args) {
	process_projectile_args(args, &defaults_part);
	return _create_projectile(args);
}

#ifdef PROJ_DEBUG
Projectile* _proj_attach_dbginfo(Projectile *p, DebugInfo *dbg, const char *callsite_str) {
	// log_debug("Spawn: [%s]", callsite_str);
	memcpy(&p->debug, dbg, sizeof(DebugInfo));
	set_debug_info(dbg);
	return p;
}
#endif

static void* _delete_projectile(ListAnchor *projlist, List *proj, void *arg) {
	Projectile *p = (Projectile*)proj;
	proj_call_rule(p, EVENT_DEATH);

	del_ref(proj);
	ent_unregister(&p->ent);
	objpool_release(stage_object_pools.projectiles, (ObjectInterface*)alist_unlink(projlist, proj));

	return NULL;
}

void delete_projectile(ProjectileList *projlist, Projectile *proj) {
	_delete_projectile((ListAnchor*)projlist, (List*)proj, NULL);
}

void delete_projectiles(ProjectileList *projlist) {
	alist_foreach(projlist, _delete_projectile, NULL);
}

void calc_projectile_collision(Projectile *p, ProjCollisionResult *out_col) {
	assert(out_col != NULL);

	out_col->type = PCOL_NONE;
	out_col->entity = NULL;
	out_col->fatal = false;
	out_col->location = p->pos;
	out_col->damage = 0;

	if(p->type == EnemyProj) {
		Ellipse e_proj = {
			.axes = p->collision_size,
			.angle = p->angle + M_PI/2,
		};

		LineSegment seg = {
			.a = global.plr.pos - global.plr.velocity - p->prevpos,
			.b = global.plr.pos - p->pos
		};

		attr_unused double seglen = cabs(seg.a - seg.b);

		if(seglen > 30) {
			log_debug(
				seglen > VIEWPORT_W
					? "Lerp over HUGE distance %f; this is ABSOLUTELY a bug! Player speed was %f. Spawned at %s:%d (%s)"
					: "Lerp over large distance %f; this is either a bug or a very fast projectile, investigate. Player speed was %f. Spawned at %s:%d (%s)",
				seglen,
				cabs(global.plr.velocity),
				p->debug.file,
				p->debug.line,
				p->debug.func
			);
		}

		if(lineseg_ellipse_intersect(seg, e_proj)) {
			out_col->type = PCOL_PLAYER;
			out_col->entity = &global.plr;
			out_col->fatal = true;
		} else {
			e_proj.axes = projectile_graze_size(p);

			if(creal(e_proj.axes) > 1 && lineseg_ellipse_intersect(seg, e_proj)) {
				out_col->type = PCOL_PLAYER_GRAZE;
				out_col->entity = &global.plr;
				out_col->location = global.plr.pos;
			}
		}
	} else if(p->type >= PlrProj) {
		int damage = p->type - PlrProj;

		for(Enemy *e = global.enemies.first; e; e = e->next) {
			if(e->hp != ENEMY_IMMUNE && cabs(e->pos - p->pos) < 30) {
				out_col->type = PCOL_ENEMY;
				out_col->entity = e;
				out_col->fatal = true;
				out_col->damage = damage;

				return;
			}
		}

		if(global.boss && cabs(global.boss->pos - p->pos) < 42) {
			if(boss_is_vulnerable(global.boss)) {
				out_col->type = PCOL_BOSS;
				out_col->entity = global.boss;
				out_col->fatal = true;
				out_col->damage = damage;
			}
		}
	}

	if(out_col->type == PCOL_NONE && !projectile_in_viewport(p)) {
		out_col->type = PCOL_VOID;
		out_col->fatal = true;
	}
}

void apply_projectile_collision(ProjectileList *projlist, Projectile *p, ProjCollisionResult *col) {
	switch(col->type) {
		case PCOL_NONE: {
			break;
		}

		case PCOL_PLAYER: {
			if(global.frames - abs(((Player*)col->entity)->recovery) >= 0) {
				player_death(col->entity);
			}

			break;
		}

		case PCOL_PLAYER_GRAZE: {
			if(p->flags & PFLAG_GRAZESPAM) {
				player_graze(col->entity, col->location, 10, 2);
			} else {
				player_graze(col->entity, col->location, 10 + 10 * p->graze_counter, 3 + p->graze_counter);
			}

			p->graze_counter++;
			p->graze_counter_reset_timer = global.frames;

			break;
		}

		case PCOL_ENEMY: {
			Enemy *e = col->entity;
			player_add_points(&global.plr, col->damage * 0.5);
			player_register_damage(&global.plr, col->damage);
			enemy_damage(e, col->damage);
			break;
		}

		case PCOL_BOSS: {
			if(boss_damage(col->entity, col->damage)) {
				player_add_points(&global.plr, col->damage * 0.2);
				player_register_damage(&global.plr, col->damage);
			}
			break;
		}

		case PCOL_VOID: {
			break;
		}

		default: {
			log_fatal("Invalid collision type %x", col->type);
		}
	}

	if(col->fatal) {
		delete_projectile(projlist, p);
	}
}

static void ent_draw_projectile(EntityInterface *ent) {
	Projectile *proj = ENT_CAST(ent, Projectile);

	// TODO: get rid of these legacy flags

	if(proj->flags & PFLAG_DRAWADD) {
		r_blend(BLEND_ADD);
	} else if(proj->flags & PFLAG_DRAWSUB) {
		r_blend(BLEND_SUB);
	} else {
		r_blend(proj->blend);
	}

	r_shader_ptr(proj->shader);

#ifdef PROJ_DEBUG
	if(proj->type == PlrProj) {
		set_debug_info(&proj->debug);
		log_fatal("Projectile with type PlrProj");
	}

	static Projectile prev_state;
	memcpy(&prev_state, proj, sizeof(Projectile));

	proj->draw_rule(proj, global.frames - proj->birthtime);

	if(memcmp(&prev_state, proj, sizeof(Projectile))) {
		set_debug_info(&proj->debug);
		log_fatal("Projectile modified its state in draw rule");
	}
#else
	proj->draw_rule(proj, global.frames - proj->birthtime);
#endif
}

bool projectile_in_viewport(Projectile *proj) {
	double w, h;
	int e = proj->max_viewport_dist;
	projectile_size(proj, &w, &h);

	return !(creal(proj->pos) + w/2 + e < 0 || creal(proj->pos) - w/2 - e > VIEWPORT_W
		  || cimag(proj->pos) + h/2 + e < 0 || cimag(proj->pos) - h/2 - e > VIEWPORT_H);
}

Projectile* spawn_projectile_collision_effect(Projectile *proj) {
	if(proj->flags & PFLAG_NOCOLLISIONEFFECT) {
		return NULL;
	}
	if(proj->sprite == NULL) {
		return NULL;
	}

	return PARTICLE(
		.sprite_ptr = proj->sprite,
		.size = proj->size,
		.pos = proj->pos,
		.color = &proj->color,
		.flags = proj->flags | PFLAG_NOREFLECT,
		.shader_ptr = proj->shader,
		.rule = linear,
		.draw_rule = DeathShrink,
		.angle = proj->angle,
		.args = { 5*cexp(I*proj->angle) },
		.timeout = 10,
	);
}

Projectile* spawn_projectile_clear_effect(Projectile *proj) {
	if(proj->flags & PFLAG_NOCLEAREFFECT) {
		return NULL;
	}

	return PARTICLE(
		.sprite_ptr = proj->sprite,
		.size = proj->size,
		.pos = proj->pos,
		.color = &proj->color,
		.flags = proj->flags | PFLAG_NOREFLECT,
		.shader_ptr = proj->shader,
		.draw_rule = Shrink,
		.angle = proj->angle,
		.timeout = 10,
		.args = { 10 },
	);
}

bool clear_projectile(ProjectileList *projlist, Projectile *proj, bool force, bool now) {
	if(proj->type >= PlrProj || (!force && !projectile_is_clearable(proj))) {
		return false;
	}

	if(now) {
		if(!(proj->flags & PFLAG_NOCLEARBONUS)) {
			create_bpoint(proj->pos);
		}

		spawn_projectile_clear_effect(proj);
		delete_projectile(projlist, proj);
	} else {
		proj->type = DeadProj;
	}

	return true;
}

void process_projectiles(ProjectileList *projlist, bool collision) {
	ProjCollisionResult col = { 0 };

	char killed = 0;
	int action;

	for(Projectile *proj = projlist->first, *next; proj; proj = next) {
		next = proj->next;
		proj->prevpos = proj->pos;
		action = proj_call_rule(proj, global.frames - proj->birthtime);

		if(proj->graze_counter && proj->graze_counter_reset_timer - global.frames <= -90) {
			proj->graze_counter--;
			proj->graze_counter_reset_timer = global.frames;
		}

		if(proj->type == DeadProj && killed < 5) {
			killed++;
			action = ACTION_DESTROY;

			if(clear_projectile(projlist, proj, true, true)) {
				continue;
			}
		}

		if(collision) {
			calc_projectile_collision(proj, &col);

			if(col.fatal && col.type != PCOL_VOID) {
				spawn_projectile_collision_effect(proj);
			}
		} else {
			memset(&col, 0, sizeof(col));

			if(!projectile_in_viewport(proj)) {
				col.fatal = true;
			}
		}

		if(action == ACTION_DESTROY) {
			col.fatal = true;
		}

		apply_projectile_collision(projlist, proj, &col);
	}
}

int trace_projectile(Projectile *p, ProjCollisionResult *out_col, ProjCollisionType stopflags, int timeofs) {
	int t;

	for(t = timeofs; p; ++t) {
		int action = p->rule(p, t);
		calc_projectile_collision(p, out_col);

		if(out_col->type & stopflags || action == ACTION_DESTROY) {
			return t;
		}
	}

	return t;
}

bool projectile_is_clearable(Projectile *p) {
	if(p->type == DeadProj) {
		return true;
	}

	if(p->type == EnemyProj || p->type == FakeProj) {
		return (p->flags & PFLAG_NOCLEAR) != PFLAG_NOCLEAR;
	}

	return false;
}

int linear(Projectile *p, int t) { // sure is physics in here; a[0]: velocity
	if(t == EVENT_DEATH) {
		return ACTION_ACK;
	}

	p->angle = carg(p->args[0]);

	if(t == EVENT_BIRTH) {
		return ACTION_ACK;
	}

	p->pos = p->pos0 + p->args[0]*t;

	return ACTION_NONE;
}

int accelerated(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		return ACTION_ACK;
	}

	p->angle = carg(p->args[0]);

	if(t == EVENT_BIRTH) {
		return ACTION_ACK;
	}

	p->pos += p->args[0];
	p->args[0] += p->args[1];

	return 1;
}

int asymptotic(Projectile *p, int t) { // v = a[0]*(a[1] + 1); a[1] -> 0
	if(t == EVENT_DEATH) {
		return ACTION_ACK;
	}

	p->angle = carg(p->args[0]);

	if(t == EVENT_BIRTH) {
		return ACTION_ACK;
	}

	p->args[1] *= 0.8;
	p->pos += p->args[0]*(p->args[1] + 1);

	return 1;
}

static inline void apply_common_transforms(Projectile *proj, int t) {
	r_mat_translate(creal(proj->pos), cimag(proj->pos), 0);
	r_mat_rotate_deg(proj->angle*180/M_PI+90, 0, 0, 1);

	if(t >= 16) {
		return;
	}

	if(proj->flags & PFLAG_NOSPAWNZOOM) {
		return;
	}

	if(proj->type != EnemyProj && proj->type != FakeProj) {
		return;
	}

	float s = 2.0-t/16.0;
	if(s != 1) {
		r_mat_scale(s, s, 1);
	}
}

void ProjDrawCore(Projectile *proj, const Color *c) {
	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = proj->sprite,
		.color = c,
		.custom = proj->shader_custom_param,
	});
}

void ProjDraw(Projectile *proj, int t) {
	r_mat_push();
	apply_common_transforms(proj, t);
	ProjDrawCore(proj, &proj->color);
	r_mat_pop();
}

void ProjNoDraw(Projectile *proj, int t) {
}

void Blast(Projectile *p, int t) {
	r_mat_push();
	r_mat_translate(creal(p->pos), cimag(p->pos), 0);
	r_mat_rotate_deg(creal(p->args[1]), cimag(p->args[1]), creal(p->args[2]), cimag(p->args[2]));

	if(t != p->timeout && p->timeout != 0) {
		r_mat_scale(t/(double)p->timeout, t/(double)p->timeout, 1);
	}

	float fade = 1.0 - t / (double)p->timeout;
	r_color(RGBA_MUL_ALPHA(0.3, 0.6, 1.0, fade));

	draw_sprite_batched_p(0,0,p->sprite);
	r_mat_scale(0.5+creal(p->args[2]),0.5+creal(p->args[2]),1);
	r_color4(0.3 * fade, 0.6 * fade, 1.0 * fade, 0);
	draw_sprite_batched_p(0,0,p->sprite);
	r_mat_pop();
}

void Shrink(Projectile *p, int t) {
	r_mat_push();
	apply_common_transforms(p, t);

	float s = 2.0-t/(double)p->timeout*2;
	if(s != 1) {
		r_mat_scale(s, s, 1);
	}

	ProjDrawCore(p, &p->color);
	r_mat_pop();
}

void DeathShrink(Projectile *p, int t) {
	r_mat_push();
	apply_common_transforms(p, t);

	float s = 2.0-t/(double)p->timeout*2;
	if(s != 1) {
		r_mat_scale(s, 1, 1);
	}

	ProjDrawCore(p, &p->color);
	r_mat_pop();
}

void GrowFade(Projectile *p, int t) {
	r_mat_push();
	apply_common_transforms(p, t);

	set_debug_info(&p->debug);
	assert(p->timeout != 0);

	float s = t/(double)p->timeout*(1 + (creal(p->args[2])? p->args[2] : p->args[1]));
	if(s != 1) {
		r_mat_scale(s, s, 1);
	}

	ProjDrawCore(p, color_mul_scalar(COLOR_COPY(&p->color), 1 - t/(double)p->timeout));
	r_mat_pop();
}

void Fade(Projectile *p, int t) {
	r_mat_push();
	apply_common_transforms(p, t);
	ProjDrawCore(p, color_mul_scalar(COLOR_COPY(&p->color), 1 - t/(double)p->timeout));
	r_mat_pop();
}

void ScaleFade(Projectile *p, int t) {
	r_mat_push();
	apply_common_transforms(p, t);

	double scale_min = creal(p->args[2]);
	double scale_max = cimag(p->args[2]);
	double timefactor = t / (double)p->timeout;
	double scale = scale_min * (1 - timefactor) + scale_max * timefactor;
	double alpha = pow(1 - timefactor, 2);

	r_mat_scale(scale, scale, 1);
	ProjDrawCore(p, color_mul_scalar(COLOR_COPY(&p->color), alpha));
	r_mat_pop();
}

void Petal(Projectile *p, int t) {
	float x = creal(p->args[2]);
	float y = cimag(p->args[2]);
	float z = creal(p->args[3]);

	float r = sqrt(x*x+y*y+z*z);
	x /= r; y /= r; z /= r;

	r_disable(RCAP_CULL_FACE);
	r_mat_push();
	r_mat_translate(creal(p->pos), cimag(p->pos),0);
	r_mat_rotate_deg(t*4.0 + cimag(p->args[3]), x, y, z);
	ProjDrawCore(p, &p->color);
	r_mat_pop();
}

void petal_explosion(int n, complex pos) {
	for(int i = 0; i < n; i++) {
		tsrand_fill(6);
		float t = frand();

		PARTICLE(
			.sprite = "petal",
			.pos = pos,
			.color = RGBA_MUL_ALPHA(sin(5*t), cos(5*t), 0.5, t),
			.rule = asymptotic,
			.draw_rule = Petal,
			.args = {
				(3+5*afrand(2))*cexp(I*M_PI*2*afrand(3)),
				5,
				afrand(4) + afrand(5)*I,
				afrand(1) + 360.0*I*afrand(0),
			},
			// TODO: maybe remove this noreflect, there shouldn't be a cull mode mess anymore
			.flags = PFLAG_NOREFLECT,
			.blend = BLEND_ADD,
		);
	}
}

void projectiles_preload(void) {
	// XXX: maybe split this up into stage-specific preloads too?
	// some of these are ubiquitous, but some only appear in very specific parts.

	preload_resources(RES_SHADER_PROGRAM, RESF_PERMANENT,
		defaults_proj.shader,
		defaults_part.shader,
	NULL);

	preload_resources(RES_TEXTURE, RESF_PERMANENT,
		"part/lasercurve",
	NULL);

	preload_resources(RES_SPRITE, RESF_PERMANENT,
		"part/blast",
		"part/flare",
		"part/petal",
		"part/smoke",
		"part/stain",
		"part/lightning0",
		"part/lightning1",
		"part/lightningball",
		"part/smoothdot",
	NULL);

	preload_resources(RES_SFX, RESF_PERMANENT,
		"shot1",
		"shot2",
		"shot1_loop",
		"shot_special1",
		"redirect",
	NULL);

	#define PP(name) (_pp_##name).preload(&_pp_##name);
	#include "projectile_prototypes/all.inc.h"

	defaults_proj.shader_ptr = r_shader_get(defaults_proj.shader);
	defaults_part.shader_ptr = r_shader_get(defaults_part.shader);
}
