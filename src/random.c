/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include <time.h>

#include "global.h"
#include "random.h"

static RandomState *tsrand_current;

uint64_t splitmix64(uint64_t *state) {
	// from http://xoshiro.di.unimi.it/splitmix64.c

	uint64_t z = (*state += 0x9e3779b97f4a7c15);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}

uint64_t makeseed(void) {
	static uint64_t s;
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

uint32_t tsrand_p(RandomState *rnd) {
	assert(!rnd->locked);
	return xoshiro256plus(rnd->state) >> 32;
}

uint64_t tsrand64_p(RandomState *rnd) {
	assert(!rnd->locked);
	return xoshiro256plus(rnd->state);
}

void tsrand_seed_p(RandomState *rnd, uint64_t seed) {
	rnd->state[0] = splitmix64(&seed);
	rnd->state[1] = splitmix64(&seed);
	rnd->state[2] = splitmix64(&seed);
	rnd->state[3] = splitmix64(&seed);
}

void tsrand_switch(RandomState *rnd) {
	tsrand_current = rnd;
}

void tsrand_init(RandomState *rnd, uint64_t seed) {
	memset(rnd, 0, sizeof(RandomState));
	tsrand_seed_p(rnd, seed);
}

void tsrand_seed(uint64_t seed) {
	tsrand_seed_p(tsrand_current, seed);
}

uint32_t tsrand(void) {
	return tsrand_p(tsrand_current);
}

uint64_t tsrand64(void) {
	return tsrand64_p(tsrand_current);
}

static inline double makedouble(uint64_t x) {
	// Range: [0..1)
	return (x >> 11) * (1.0 / (UINT64_C(1) << 53));
}

double frand(void) {
	return makedouble(tsrand64());
}

double nfrand(void) {
	return frand() * 2.0 - 1.0;
}

// we use this to support multiple rands in a single statement without breaking replays across different builds

static uint64_t tsrand_array[TSRAND_ARRAY_LIMIT];
static int tsrand_array_elems;
static uint64_t tsrand_fillflags = 0;

static void tsrand_error(const char *file, const char *func, uint line, const char *fmt, ...) {
	char buf[2048] = { 0 };
	va_list args;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	log_fatal("%s(): %s [%s:%u]", func, buf, file, line);
}

#define TSRANDERR(...) tsrand_error(file, __func__, line, __VA_ARGS__)

void _tsrand_fill_p(RandomState *rnd, int amount, const char *file, uint line) {
	if(tsrand_fillflags) {
		TSRANDERR("Some indices left unused from the previous call");
		return;
	}

	tsrand_array_elems = amount;
	tsrand_fillflags = (UINT64_C(1) << amount) - 1;

	for(int i = 0; i < amount; ++i) {
		tsrand_array[i] = tsrand64_p(rnd);
	}
}

void _tsrand_fill(int amount, const char *file, uint line) {
	_tsrand_fill_p(tsrand_current, amount, file, line);
}

uint64_t _tsrand64_a(int idx, const char *file, uint line) {
	if(idx >= tsrand_array_elems || idx < 0) {
		TSRANDERR("Index out of range (%i / %i)", idx, tsrand_array_elems);
		return 0;
	}

	if(tsrand_fillflags & (UINT64_C(1) << idx)) {
		tsrand_fillflags &= ~(UINT64_C(1) << idx);
		return tsrand_array[idx];
	}

	TSRANDERR("Index %i used multiple times", idx);
	return 0;
}

uint32_t _tsrand_a(int idx, const char *file, uint line) {
	return _tsrand64_a(idx, file, line) >> 32;
}

double _afrand(int idx, const char *file, uint line) {
	return makedouble(_tsrand64_a(idx, file, line));
}

double _anfrand(int idx, const char *file, uint line) {
	return _afrand(idx, file, line) * 2 - 1;
}

void tsrand_lock(RandomState *rnd) {
	rnd->locked = true;
}

void tsrand_unlock(RandomState *rnd) {
	rnd->locked = false;
}
