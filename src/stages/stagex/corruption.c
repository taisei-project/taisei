/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "corruption.h"

#include "coroutine/taskdsl.h"
#include "global.h"
#include "projectile.h"
#include "stagedraw.h"
#include "util/fbutil.h"
#include "util/glm.h"
#include "util/graphics.h"

#define CORRUPTION_SDF_RANGE 64
#define MAX_ZONES 128

typedef union StageXCorruptionZoneRenderData {
	vec4_noalign vec;
	struct {
		cmplxf pos;
		float radius;
		float alpha;
	};
} StageXCorruptionZoneRenderData;

typedef struct StageXCorruption {
	BoxedProjectileArray zone_projectiles;
	StageXCorruptionZoneRenderData zone_renderdata[MAX_ZONES];
	Framebuffer *mask_fb;
	Projectile *sdf_generator;
	Projectile *sdf_applier;
	int num_active_zones;
	float smoothing_factor;
} StageXCorruption;

static float get_zone_visual_radius(Projectile *zone) {
	assert(re(zone->size) == im(zone->size));
	return (float)re(zone->size) * 0.5f;
}

TASK(corruption, { StageXCorruption **out_corruption; }) {
	BoxedProjectile zone_projectiles_data[MAX_ZONES];
	StageXCorruption corruption = {
		.zone_projectiles = {
			.array = zone_projectiles_data,
			.capacity = ARRAY_SIZE(zone_projectiles_data),
		},
	};
	*ARGS.out_corruption = &corruption;

	for(;;YIELD) {
		float k = 0, w = 0;

		corruption.num_active_zones = 0;
		ENT_ARRAY_FOREACH(&corruption.zone_projectiles, Projectile *zone, {
			float alpha = zone->color.a;

			if(alpha <= 0) {
				continue;
			}

			float r = get_zone_visual_radius(zone);

			k += r * zone->color.a;
			w += zone->color.a;

			corruption.zone_renderdata[corruption.num_active_zones++] = (StageXCorruptionZoneRenderData) {
				.pos = zone->pos,
				.radius = r,
				.alpha = zone->color.a,
			};
		});

		k /= w;
		corruption.smoothing_factor = k;
	}
}

static void corruption_render_mask(Projectile *p, int t, ProjDrawRuleArgs args) {
	StageXCorruption *corruption = args[0].as_ptr;

	if(corruption->num_active_zones < 1) {
		return;
	}

	r_state_push();
	r_framebuffer(corruption->mask_fb);
	r_clear(BUFFER_COLOR, RGBA(CORRUPTION_SDF_RANGE, 0, 0, 0), 1);
	r_blend(BLEND_NONE);
	r_uniform_float("sdf_range", CORRUPTION_SDF_RANGE);
	r_uniform_vec2("viewport", VIEWPORT_W, VIEWPORT_H);
	r_uniform_int("num_zones", corruption->num_active_zones);
	r_uniform_vec4_array("zones", 0, corruption->num_active_zones ?: 1, &corruption->zone_renderdata->vec);
	r_mat_proj_push_ortho(VIEWPORT_W, VIEWPORT_H);
	r_mat_mv_push();
	r_mat_mv_scale(VIEWPORT_W, VIEWPORT_H, 1);
	r_mat_mv_translate(0.5, 0.5, 0);
	r_draw_quad();
	r_mat_mv_pop();
	r_mat_proj_pop();
	r_state_pop();
}

static void corruption_draw(Projectile *p, int t, ProjDrawRuleArgs args) {
	StageXCorruption *corruption = args[0].as_ptr;

	if(corruption->num_active_zones < 1) {
		return;
	}

	r_state_push();
	r_blend(BLEND_PREMUL_ALPHA);
	r_uniform_sampler("noise", "cell_noise");
	r_uniform_float("time", 0.25f * global.frames / (float)FPS);
	r_uniform_float("sdf_range", CORRUPTION_SDF_RANGE);
	draw_framebuffer_tex(corruption->mask_fb, VIEWPORT_W, VIEWPORT_H);
	r_state_pop();
}

StageXCorruption *stagex_corruption_create(void) {
	StageXCorruption *corruption = NULL;
	INVOKE_TASK(corruption, &corruption);
	NOT_NULL(corruption);

	int q = 16;

	FBAttachmentConfig cfg = {
		.attachment = FRAMEBUFFER_ATTACH_COLOR0,
		.tex_params = {
			.class = TEXTURE_CLASS_2D,
			.filter.mag = TEX_FILTER_LINEAR,
			.filter.min = TEX_FILTER_LINEAR,
			.width = VIEWPORT_W / q,
			.height = VIEWPORT_H / q,
			.type = TEX_TYPE_RG_16_FLOAT,
			.wrap.s = TEX_WRAP_CLAMP,
			.wrap.t = TEX_WRAP_CLAMP,
		},
	};

	corruption->mask_fb = stage_add_static_framebuffer("Corruption mask FB", 1, &cfg);

	corruption->sdf_generator = PARTICLE(
		.pos = CMPLX(VIEWPORT_W, VIEWPORT_H) * 0.5,
		.size = 1+I,
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE | PFLAG_NOMOVE | PFLAG_MANUALANGLE,
		.layer = LAYER_BACKGROUND,
		.shader = "stagex_corruption_sdf_generate",
		.draw_rule = {
			.func = corruption_render_mask,
			.args[0].as_ptr = corruption,
		},
	);

	corruption->sdf_applier = PARTICLE(
		.pos = CMPLX(VIEWPORT_W, VIEWPORT_H) * 0.5,
		.size = 1+I,
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE | PFLAG_NOMOVE | PFLAG_MANUALANGLE,
		.layer = LAYER_BACKGROUND,
		.shader = "stagex_corruption_sdf_apply",
		.draw_rule = {
			.func = corruption_draw,
			.args[0].as_ptr = corruption,
		},
	);

	return corruption;
}

TASK(corruption_zone, { StageXCorruption *corruption; BoxedProjectile zone; real radius; int duration; }) {
	auto zone = TASK_BIND(ARGS.zone);

	int expand_time = min(120, ARGS.duration / 2);
	int stable_time = (ARGS.duration - expand_time) / 5;
	int shrink_time = ARGS.duration - stable_time - expand_time;

	zone->size = 0;

	for(int t = 0; t < ARGS.duration; ++t, YIELD) {
		real size = re(zone->size);

		real expand_factor = min(1, t / (expand_time - 1.0));
		real shrink_factor = 1.0 - max(0, (t - expand_time - stable_time) / (shrink_time - 1.0));

		expand_factor = glm_ease_sine_out(expand_factor);
		shrink_factor = glm_ease_sine_out(shrink_factor);

		real size_factor = expand_factor * shrink_factor;
		real alpha_factor = expand_factor * shrink_factor;
		real collision_factor = size_factor;

		alpha_factor = glm_ease_exp_out(alpha_factor);
		// alpha_factor = glm_ease_circ_out(alpha_factor);

		zone->color.a = alpha_factor;

		size = lerp(0, ARGS.radius, size_factor);

		if(size > 0 && zone->color.a > 0.25) {
			zone->flags &= ~PFLAG_NOCOLLISION;
		} else {
			zone->flags |=  PFLAG_NOCOLLISION;
		}

		zone->size = CMPLX(size, size);
		zone->collision_size = collision_factor * CMPLX(size, size);
	}
}

Projectile *stagex_corruption_spawn_zone(StageXCorruption *corruption, cmplx origin, real radius, int duration) {
	auto zone = PROJECTILE(
		.pos = origin,
		.size = 1+I,
		.timeout = duration,
		.color = RGBA(3, 2, 0, 0),
		.layer = LAYER_NODRAW,
		.flags =
			PFLAG_NOCLEAR |
			PFLAG_MANUALANGLE |
			PFLAG_NOSPAWNEFFECTS |
			PFLAG_NOCLEAREFFECT |
			PFLAG_INDESTRUCTIBLE |
			PFLAG_NOAUTOREMOVE |
			PFLAG_NOCOLLISION |
			0,
	);

	auto zones = &corruption->zone_projectiles;

	if(zones->size == zones->capacity) {
		ENT_ARRAY_COMPACT(zones);
	}

	if(zones->size == zones->capacity) {
		int oldest_spawntime = global.frames + 1;
		int oldest_idx = 0;

		ENT_ARRAY_FOREACH_COUNTER(zones, int i, Projectile *p, {
			if(p->birthtime < oldest_spawntime) {
				oldest_idx = i;
				oldest_spawntime = p->birthtime;
			}
		});

		auto p = ENT_ARRAY_GET(zones, oldest_idx);
		kill_projectile(p);
		zones->array[oldest_idx] = (BoxedProjectile) {};

		log_warn("Too many corruption zones, killed #%i (%d frames old)", oldest_idx, global.frames - p->birthtime);
	}

	ENT_ARRAY_ADD_FIRSTFREE(zones, zone);

	INVOKE_TASK(corruption_zone, corruption, ENT_BOX(zone), radius, duration);
	return zone;
}

TASK(spawn_death_corruption, { BoxedEnemy e; StageXCorruption *corruption; }) {
	auto e = NOT_NULL(ENT_UNBOX(ARGS.e));
	auto pos = e->pos;

	real radius = 120;
	real radius_variance = radius * 0.1;
	real duration = 5 * 60;
	real duration_variance = duration * 0.3;

	real r = radius + rng_sreal() * radius_variance;
	real d = duration + rng_sreal() * duration_variance;
	stagex_corruption_spawn_zone(ARGS.corruption, pos, r, d);
}

TASK(enemy_corruption, { BoxedEnemy e; StageXCorruption *corruption; }) {
	int duration = 5 * 60;
	real duration_variance = duration * 0.3;
	int period = duration * 0.5;
	real radius = 64;
	real radius_variance = radius * 0.1;

	DECLARE_ENT_ARRAY(Projectile, zones, 8);

	Enemy *e;
	for(int t = 0; (e = ENT_UNBOX(ARGS.e)); ++t, YIELD) {
		if((t % period) == 0) {
			real r = radius + rng_sreal() * radius_variance;
			real d = duration + rng_sreal() * duration_variance;
			auto z = stagex_corruption_spawn_zone(ARGS.corruption, e->pos, r, d);

			if(z) {
				ENT_ARRAY_ADD_FIRSTFREE(&zones, z);
			}
		}

		ENT_ARRAY_FOREACH(&zones, Projectile *z, {
			z->move = move_towards(z->move.velocity, e->pos, 0.8);
		});
	}

	ENT_ARRAY_FOREACH(&zones, Projectile *z, {
		z->move = move_dampen(z->move.velocity, 0.9);
	});
}

void stagex_corrupt_enemy(StageXCorruption *corruption, Enemy *e) {
	INVOKE_TASK(enemy_corruption, ENT_BOX(e), NOT_NULL(corruption));
	INVOKE_TASK_AFTER(&e->events.killed, spawn_death_corruption, ENT_BOX(e), NOT_NULL(corruption));
}
