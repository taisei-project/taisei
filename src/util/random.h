/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

typedef struct RandomState {
	uint64_t state[4];
#ifdef DEBUG
	bool locked;
#endif
} RandomState;

typedef struct rng_val {
	uint64_t _value;
} rng_val_t;

uint64_t splitmix64(uint64_t *state) attr_nonnull(1);
uint32_t splitmix32(uint32_t *state) attr_nonnull(1);
uint64_t makeseed(void);

void rng_init(RandomState *rng, uint64_t seed) attr_nonnull(1);
void rng_seed(RandomState *rng, uint64_t seed) attr_nonnull(1);
rng_val_t rng_next_p(RandomState *rng) attr_nonnull(1);

#ifdef DEBUG
INLINE void rng_lock(RandomState *rng) { rng->locked = true; }
INLINE void rng_unlock(RandomState *rng) { rng->locked = false; }
INLINE bool rng_is_locked(RandomState *rng) { return rng->locked; }
#else
#define rng_lock(rng) ((void)(rng), (void)0)
#define rng_unlock(rng) ((void)(rng), (void)0)
#define rng_is_locked(rng) (false)
#endif

uint64_t vrng_u64(rng_val_t v) attr_pure;
int64_t vrng_i64(rng_val_t v) attr_pure;
uint32_t vrng_u32(rng_val_t v) attr_pure;
int32_t vrng_i32(rng_val_t v) attr_pure;

double vrng_f64(rng_val_t v) attr_pure;
double vrng_f64s(rng_val_t v) attr_pure;

float vrng_f32(rng_val_t v) attr_pure;
float vrng_f32s(rng_val_t v) attr_pure;

#define vrng_real(v)  vrng_f64(v)
#define vrng_sreal(v) vrng_f64s(v)

double vrng_f64_sign(rng_val_t v) attr_pure;
float vrng_f32_sign(rng_val_t v) attr_pure;
#define vrng_sign(v)  vrng_f64_sign(v)

bool vrng_bool(rng_val_t v);

double vrng_f64_range(rng_val_t v, double rmin, double rmax) attr_pure;
float vrng_f32_range(rng_val_t v, float rmin, float rmax) attr_pure;

#define vrng_range(v, rmin, rmax) _Generic((rmin), \
	float: \
		_Generic((rmax), \
			float: vrng_f32_range, \
			default: vrng_f64_range \
		), \
	default: vrng_f64_range \
)(v, rmin, rmax)

int64_t vrng_i64_range(rng_val_t v, int64_t rmin, int64_t rmax) attr_pure;
int32_t vrng_i32_range(rng_val_t v, int32_t rmin, int32_t rmax) attr_pure;
#define vrng_irange(v, rmin, rmax) vrng_i32_range(v, rmin, rmax)

double vrng_f64_angle(rng_val_t v) attr_pure;
float vrng_f32_angle(rng_val_t v) attr_pure;
#define vrng_angle(v) vrng_f64_angle(v)

cmplx vrng_dir(rng_val_t v) attr_pure;

bool vrng_f64_chance(rng_val_t v, double chance) attr_pure;
bool vrng_f32_chance(rng_val_t v, float chance) attr_pure;

#define vrng_chance(v, chance) _Generic((chance), \
	float: vrng_f32_chance, \
	default: vrng_f64_chance \
)(v, chance)
