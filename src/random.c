/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "random.h"

#include "util.h"
#include "util/miscmath.h"

static RandomState *rng_active_state;

uint64_t splitmix64(uint64_t *state) {
	// from http://xoshiro.di.unimi.it/splitmix64.c

	uint64_t z = (*state += 0x9e3779b97f4a7c15);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}

uint32_t splitmix32(uint32_t *state) {
	uint32_t z = (*state += 0x9e3779b9);
	z = (z ^ (z >> 15)) * 0x85ebca6b;
	z = (z ^ (z >> 13)) * 0xc2b2ae3d;
	return z ^ (z >> 16);
}

uint64_t makeseed(void) {
	static uint64_t s = 69;
	return splitmix64(&s) ^ SDL_GetPerformanceCounter();
}

static inline uint64_t rotl(uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}

static uint64_t xoshiro256plus(uint64_t s[4]) {
	// from http://xoshiro.di.unimi.it/xoshiro256plus.c

	const uint64_t result_plus = s[0] + s[3];
	const uint64_t t = s[1] << 17;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];
	s[2] ^= t;
	s[3] = rotl(s[3], 45);

	return result_plus;
}

void rng_init(RandomState *rng, uint64_t seed) {
	*rng = (typeof(*rng)) {};
	rng_seed(rng, seed);
}

void rng_seed(RandomState *rng, uint64_t seed) {
	rng->state[0] = splitmix64(&seed);
	rng->state[1] = splitmix64(&seed);
	rng->state[2] = splitmix64(&seed);
	rng->state[3] = splitmix64(&seed);
}

void rng_make_active(RandomState *rng) {
	rng_active_state = rng;
}

rng_val_t rng_next_p(RandomState *rng) {
	assert(!rng_is_locked(rng));
	return (rng_val_t) { xoshiro256plus(rng->state) };
}

rng_val_t rng_next(void) {
	return rng_next_p(rng_active_state);
}

void rng_nextn(size_t n, rng_val_t v[n]) {
	for(size_t i = 0; i < n; ++i) {
		v[i] = rng_next();
	}
}

/*
 * Output conversion functions
 */

uint64_t vrng_u64(rng_val_t v) {
	return v._value;
}

int64_t vrng_i64(rng_val_t v) {
	return (int64_t)v._value;
}

uint32_t vrng_u32(rng_val_t v) {
	return v._value >> 32;
}

int32_t vrng_i32(rng_val_t v) {
	return (int32_t)(v._value >> 32);
}

double vrng_f64(rng_val_t v) {
	return (v._value >> 11) * 0x1.0p-53;
}

double vrng_f64s(rng_val_t v) {
	DoubleBits db;
	db.val = vrng_f64((rng_val_t) { v._value << 1 });
	db.bits |= v._value & (UINT64_C(1) << 63);
	return db.val;
}

float vrng_f32(rng_val_t v) {
	return (v._value >> 40) * 0x1.0p-24f;
}

float vrng_f32s(rng_val_t v) {
	FloatBits fb;
	fb.val = vrng_f32((rng_val_t) { v._value << 1 });
	fb.bits |= vrng_u32(v) & (1u << 31);
	return fb.val;
}

bool vrng_bool(rng_val_t v) {
	return v._value >> 63;
}

double vrng_f64_sign(rng_val_t v) {
	return bits_to_double((UINT64_C(0x3FF) << 52) | (v._value & (UINT64_C(1) << 63)));
}

float vrng_f32_sign(rng_val_t v) {
	return bits_to_float((0x7f << 23) | (vrng_u32(v) & (1 << 31)));
}

double vrng_f64_range(rng_val_t v, double rmin, double rmax) {
	return vrng_f64(v) * (rmax - rmin) + rmin;
}

float vrng_f32_range(rng_val_t v, float rmin, float rmax) {
	return vrng_f32(v) * (rmax - rmin) + rmin;
}

int64_t vrng_i64_range(rng_val_t v, int64_t rmin, int64_t rmax) {
	// NOTE: strictly speaking non-uniform distribution in the general case, but seems good enough for small numbers.
	int64_t period = rmax - rmin;
	assume(period > 0);
	return (int64_t)(v._value % (uint64_t)period) + rmin;
}

int32_t vrng_i32_range(rng_val_t v, int32_t rmin, int32_t rmax) {
	// NOTE: strictly speaking non-uniform distribution in the general case, but seems good enough for small numbers.
	int32_t period = rmax - rmin;
	assume(period > 0);
	return (int32_t)(vrng_u32(v) % (uint32_t)period) + rmin;
}

double vrng_f64_angle(rng_val_t v) {
	return vrng_f64(v) * (M_PI * 2.0);
}

float vrng_f32_angle(rng_val_t v) {
	return vrng_f32(v) * (float)(M_PI * 2.0f);
}

cmplx vrng_dir(rng_val_t v) {
	return cdir(vrng_f64_angle(v));
}

bool vrng_f64_chance(rng_val_t v, double chance) {
	return vrng_f64(v) < chance;
}

bool vrng_f32_chance(rng_val_t v, float chance) {
	return vrng_f32(v) < chance;
}
