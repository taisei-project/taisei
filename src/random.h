/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "util/crap.h"
#include "util/miscmath.h"

#define RNG_DEPRECATED attr_deprecated("Use the new rng_ API")

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
void rng_make_active(RandomState *rng) attr_nonnull(1);
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

// NOTE: if you rename this, also update scripts/upkeep/check-rng-usage.py!
rng_val_t rng_next(void);
void rng_nextn(size_t n, rng_val_t v[n]) attr_nonnull(2);

#define RNG_ARRAY(name, size) rng_val_t name[size]; RNG_NEXT(name)

#define RNG_NEXT(_val) \
	rng_nextn(sizeof(_val)/sizeof(rng_val_t), _Generic((_val), \
		rng_val_t: &(_val), \
		rng_val_t*: (_val) \
	))

uint64_t vrng_u64(rng_val_t v) attr_pure;
#define rng_u64() vrng_u64(rng_next())
int64_t vrng_i64(rng_val_t v) attr_pure;
#define rng_i64() vrng_i64(rng_next())

uint32_t vrng_u32(rng_val_t v) attr_pure;
#define rng_u32() vrng_u32(rng_next())
int32_t vrng_i32(rng_val_t v) attr_pure;
#define rng_i32() vrng_i32(rng_next())

double vrng_f64(rng_val_t v) attr_pure;
#define rng_f64() vrng_f64(rng_next())
double vrng_f64s(rng_val_t v) attr_pure;
#define rng_f64s() vrng_f64s(rng_next())

float vrng_f32(rng_val_t v) attr_pure;
#define rng_f32() vrng_f32(rng_next())
float vrng_f32s(rng_val_t v) attr_pure;
#define rng_f32s() vrng_f32s(rng_next())

#define vrng_real(v) vrng_f64(v)
#define rng_real() vrng_real(rng_next())
#define vrng_sreal(v) vrng_f64s(v)
#define rng_sreal() vrng_sreal(rng_next())

bool vrng_bool(rng_val_t v);
#define rng_bool() vrng_bool(rng_next())

double vrng_f64_sign(rng_val_t v) attr_pure;
#define rng_f64_sign() vrng_f64_sign(rng_next())
float vrng_f32_sign(rng_val_t v) attr_pure;
#define rng_f32_sign() vrng_f32_sign(rng_next())

#define vrng_sign(v) vrng_f64_sign(v)
#define rng_sign() vrng_sign(rng_next())

double vrng_f64_range(rng_val_t v, double rmin, double rmax) attr_pure;
#define rng_f64_range(rmin, rmax) vrng_f64_range(rng_next(), rmin, rmax)
float vrng_f32_range(rng_val_t v, float rmin, float rmax) attr_pure;
#define rng_f32_range(rmin, rmax) vrng_f32_range(rng_next(), rmin, rmax)

#define vrng_range(v, rmin, rmax) _Generic((rmin), \
	float: \
		_Generic((rmax), \
			float: vrng_f32_range, \
			default: vrng_f64_range \
		), \
	default: vrng_f64_range \
)(v, rmin, rmax)

#define rng_range(rmin, rmax) vrng_range(rng_next(), rmin, rmax)

int64_t vrng_i64_range(rng_val_t v, int64_t rmin, int64_t rmax) attr_pure;
#define rng_i64_range(rmin, rmax) vrng_i64_range(rng_next(), rmin, rmax)
int32_t vrng_i32_range(rng_val_t v, int32_t rmin, int32_t rmax) attr_pure;
#define rng_i32_range(rmin, rmax) vrng_i32_range(rng_next(), rmin, rmax)

#define vrng_irange(v, rmin, rmax) vrng_i32_range(v, rmin, rmax)
#define rng_irange(rmin, rmax) vrng_irange(rng_next(), rmin, rmax)

double vrng_f64_angle(rng_val_t v) attr_pure;
#define rng_f64_angle() vrng_f64_angle(rng_next())
float vrng_f32_angle(rng_val_t v) attr_pure;
#define rng_f32_angle() vrng_f32_angle(rng_next())

#define vrng_angle(v) vrng_f64_angle(v)
#define rng_angle() vrng_angle(rng_next())

cmplx vrng_dir(rng_val_t v) attr_pure;
#define rng_dir() vrng_dir(rng_next())

bool vrng_f64_chance(rng_val_t v, double chance) attr_pure;
#define rng_f64_chance(chance) vrng_f64_chance(rng_next(), chance)
bool vrng_f32_chance(rng_val_t v, float chance) attr_pure;
#define rng_f32_chance(chance) vrng_f32_chance(rng_next(), chance)

#define vrng_chance(v, chance) _Generic((chance), \
	float: vrng_f32_chance, \
	default: vrng_f64_chance \
)(v, chance)

#define rng_chance(chance) vrng_chance(rng_next(), chance)

/*
 * Deprecated APIs; to be removed
 */

uint32_t tsrand_p(RandomState *rng) RNG_DEPRECATED;
uint64_t tsrand64_p(RandomState *rng) RNG_DEPRECATED;

uint32_t tsrand(void) RNG_DEPRECATED;
uint64_t tsrand64(void) RNG_DEPRECATED;

double frand(void) RNG_DEPRECATED;  // Range: [0.0; 1.0)
double nfrand(void) RNG_DEPRECATED;  // Range: (-1.0; 1.0)
bool rand_bool(void) RNG_DEPRECATED;
double rand_sign(void) RNG_DEPRECATED;  // 1.0 or -1.0
float rand_signf(void) RNG_DEPRECATED;  // 1.0f or -1.0f

void _tsrand_fill_p(RandomState *rnd, int amount, const char *file, uint line) RNG_DEPRECATED;
void _tsrand_fill(int amount, const char *file, uint line) RNG_DEPRECATED;
uint32_t _tsrand_a(int idx, const char *file, uint line) RNG_DEPRECATED;
uint64_t _tsrand64_a(int idx, const char *file, uint line) RNG_DEPRECATED;
double _afrand(int idx, const char *file, uint line) RNG_DEPRECATED;
double _anfrand(int idx, const char *file, uint line) RNG_DEPRECATED;

#define tsrand_fill_p(rnd,amount) _tsrand_fill_p(rnd, amount, __FILE__, __LINE__)
#define tsrand_fill(amount) _tsrand_fill(amount, __FILE__, __LINE__)
#define tsrand_a(idx) _tsrand_a(idx, __FILE__, __LINE__)
#define afrand(idx) _afrand(idx, __FILE__, __LINE__)
#define anfrand(idx) _anfrand(idx, __FILE__, __LINE__)

#define TSRAND_ARRAY_LIMIT 16
