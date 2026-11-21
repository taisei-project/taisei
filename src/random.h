/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "util/random.h"

// NOTE: if you rename this, also update scripts/upkeep/check-rng-usage.py!
rng_val_t rng_next(void);
void rng_nextn(size_t n, rng_val_t v[n]) attr_nonnull(2);

#define RNG_ARRAY(name, size) rng_val_t name[size]; RNG_NEXT(name)

#define RNG_NEXT(_val) \
	rng_nextn(sizeof(_val)/sizeof(rng_val_t), _Generic((_val), \
		rng_val_t: &(_val), \
		rng_val_t*: (_val) \
	))

#define rng_u64()                 vrng_u64(rng_next())
#define rng_i64()                 vrng_i64(rng_next())
#define rng_u32()                 vrng_u32(rng_next())
#define rng_i32()                 vrng_i32(rng_next())
#define rng_f64()                 vrng_f64(rng_next())
#define rng_f64s()                vrng_f64s(rng_next())
#define rng_f32()                 vrng_f32(rng_next())
#define rng_f32s()                vrng_f32s(rng_next())
#define rng_real()                vrng_real(rng_next())
#define rng_sreal()               vrng_sreal(rng_next())
#define rng_bool()                vrng_bool(rng_next())
#define rng_f64_sign()            vrng_f64_sign(rng_next())
#define rng_f32_sign()            vrng_f32_sign(rng_next())
#define rng_sign()                vrng_sign(rng_next())
#define rng_f64_range(rmin, rmax) vrng_f64_range(rng_next(), rmin, rmax)
#define rng_f32_range(rmin, rmax) vrng_f32_range(rng_next(), rmin, rmax)
#define rng_range(rmin, rmax)     vrng_range(rng_next(), rmin, rmax)
#define rng_i64_range(rmin, rmax) vrng_i64_range(rng_next(), rmin, rmax)
#define rng_i32_range(rmin, rmax) vrng_i32_range(rng_next(), rmin, rmax)
#define rng_irange(rmin, rmax)    vrng_irange(rng_next(), rmin, rmax)
#define rng_f64_angle()           vrng_f64_angle(rng_next())
#define rng_f32_angle()           vrng_f32_angle(rng_next())
#define rng_angle()               vrng_angle(rng_next())
#define rng_dir()                 vrng_dir(rng_next())
#define rng_f64_chance(chance)    vrng_f64_chance(rng_next(), chance)
#define rng_f32_chance(chance)    vrng_f32_chance(rng_next(), chance)
#define rng_chance(chance)        vrng_chance(rng_next(), chance)
