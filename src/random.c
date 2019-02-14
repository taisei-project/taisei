/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "global.h"
#include "random.h"

static RandomState *tsrand_current;

/*
 *  Complementary-multiply-with-carry algorithm
 */

// CMWC engine
uint32_t tsrand_p(RandomState *rnd) {
	assert(!rnd->locked);

	uint64_t const a = 18782; // as Marsaglia recommends
	uint32_t const m = 0xfffffffe; // as Marsaglia recommends
	uint64_t t;
	uint32_t x;

	rnd->i = (rnd->i + 1) & (CMWC_CYCLE - 1);
	t = a * rnd->Q[rnd->i] + rnd->c;
	// Let c = t / 0xfffffff, x = t mod 0xffffffff
	rnd->c = t >> 32;
	x = t + rnd->c;

	if(x < rnd->c) {
		x++;
		rnd->c++;
	}

	return rnd->Q[rnd->i] = m - x;
}

void tsrand_seed_p(RandomState *rnd, uint32_t seed) {
	static const uint32_t phi = 0x9e3779b9;

	rnd->Q[0] = seed;
	rnd->Q[1] = seed + phi;
	rnd->Q[2] = seed + phi + phi;

	for(int i = 3; i < CMWC_CYCLE; ++i) {
		rnd->Q[i] = rnd->Q[i - 3] ^ rnd->Q[i - 2] ^ phi ^ i;
	}

	rnd->c = 0x8edc04;
	rnd->i = CMWC_CYCLE - 1;

	for(int i = 0; i < CMWC_CYCLE*16; ++i) {
		tsrand_p(rnd);
	}
}

void tsrand_switch(RandomState *rnd) {
	tsrand_current = rnd;
}

void tsrand_init(RandomState *rnd, uint32_t seed) {
	memset(rnd, 0, sizeof(RandomState));
	tsrand_seed_p(rnd, seed);
}

void tsrand_seed(uint32_t seed) {
	tsrand_seed_p(tsrand_current, seed);
}

uint32_t tsrand(void) {
	return tsrand_p(tsrand_current);
}

float frand(void) {
	return (float)((double)tsrand()/(double)TSRAND_MAX);
}

float nfrand(void) {
	return frand() * 2.0 - 1.0;
}

// we use this to support multiple rands in a single statement without breaking replays across different builds

static uint32_t tsrand_array[TSRAND_ARRAY_LIMIT];
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

void __tsrand_fill_p(RandomState *rnd, int amount, const char *file, uint line) {
	if(tsrand_fillflags) {
		TSRANDERR("Some indices left unused from the previous call");
		return;
	}

	tsrand_array_elems = amount;
	tsrand_fillflags = (1L << amount) - 1;

	for(int i = 0; i < amount; ++i) {
		tsrand_array[i] = tsrand_p(rnd);
	}
}

void __tsrand_fill(int amount, const char *file, uint line) {
	__tsrand_fill_p(tsrand_current, amount, file, line);
}

uint32_t __tsrand_a(int idx, const char *file, uint line) {
	if(idx >= tsrand_array_elems || idx < 0) {
		TSRANDERR("Index out of range (%i / %i)", idx, tsrand_array_elems);
		return 0;
	}

	if(tsrand_fillflags & (1L << idx)) {
		tsrand_fillflags &= ~(1L << idx);
		return tsrand_array[idx];
	}

	TSRANDERR("Index %i used multiple times", idx);
	return 0;
}

float __afrand(int idx, const char *file, uint line) {
	return (float)((double)__tsrand_a(idx, file, line) / (double)TSRAND_MAX);
}

float __anfrand(int idx, const char *file, uint line) {
	return __afrand(idx, file, line) * 2 - 1;
}

void tsrand_lock(RandomState *rnd) {
	rnd->locked = true;
}

void tsrand_unlock(RandomState *rnd) {
	rnd->locked = false;
}
