/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "random.h"
#include "taisei_err.h"

/*
 *	Multiply-with-carry algorithm
 */

static RandomState *tsrand_current;

void tsrand_switch(RandomState *rnd) {
	tsrand_current = rnd;
}

void tsrand_init(RandomState *rnd, uint32_t seed) {
	memset(rnd, 0, sizeof(RandomState));
	tsrand_seed_p(rnd, seed);
}

void tsrand_seed_p(RandomState *rnd, uint32_t seed) {
	// might be not the best way to seed this
	rnd->w = (seed >> 16u) + TSRAND_W_SEED_COEFF * (seed & 0xFFFFu);
	rnd->z = (seed >> 16u) + TSRAND_Z_SEED_COEFF * (seed & 0xFFFFu);
}

uint32_t tsrand_p(RandomState *rnd) {
	if(rnd->locked) {
		warnx("Attempted to use a locked RNG state!\n");
		return 0;
	}

	rnd->w = (rnd->w >> 16u) + TSRAND_W_COEFF * (rnd->w & 0xFFFFu);
	rnd->z = (rnd->z >> 16u) + TSRAND_Z_COEFF * (rnd->z & 0xFFFFu);

	return (uint32_t)((rnd->z << 16u) + rnd->w) % TSRAND_MAX;
}

void tsrand_seed(uint32_t seed) {
	tsrand_seed_p(tsrand_current, seed);
}

uint32_t tsrand(void) {
	return tsrand_p(tsrand_current);
}

double frand(void) {
	return tsrand()/(double)TSRAND_MAX;
}

double nfrand(void) {
	return frand() * 2 - 1;
}

int tsrand_test(void) {
#if defined(TSRAND_FLOATTEST)
	RandomState rnd;

	tsrand_init(&rnd, time(0));
	tsrand_switch(&rnd);
	
	FILE *fp;
	int i;
	
	fp = fopen("/tmp/rand_test", "w");
	for(i = 0; i < 10000; ++i)
		fprintf(fp, "%f\n", frand());
	
	return 1;
#elif defined(TSRAND_SEEDTEST)
	RandomState rnd;
	tsrand_switch(&rnd);
	
	int seed = 1337, i, j;
	printf("SEED: %d\n", seed);
	
	for(i = 0; i < 5; ++i) {
		printf("RUN #%i\n", i);
		tsrand_seed(seed);
		
		for(j = 0; j < 5; ++j) {
			printf("-> %i\n", tsrand());
		}
	}
	
	return 1;
#else
	return 0;
#endif
}

// we use this to support multiple rands in a single statement without breaking replays across different builds

static uint32_t tsrand_array[TSRAND_ARRAY_LIMIT];
static int tsrand_array_elems;
static uint64_t tsrand_fillflags = 0;

static void tsrand_error(const char *file, uint line, const char *fmt, ...) {
	char buf[2048] = { 0 };
	va_list args;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	errx(-1, "tsrand error: %s [%s:%u]", buf, file, line);
}

#define TSRANDERR(...) tsrand_error(file, line, __VA_ARGS__)

void __tsrand_fill_p(RandomState *rnd, int amount, const char *file, uint line) {
	if(tsrand_fillflags) {
		TSRANDERR("tsrand_fill_p: some indices left unused from the previous call");
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
		TSRANDERR("tsrand_a: index out of range (%i / %i)", idx, tsrand_array_elems);
		return 0;
	}

	if(tsrand_fillflags & (1L << idx)) {
		tsrand_fillflags &= ~(1L << idx);
		return tsrand_array[idx];
	}

	TSRANDERR("tsrand_a: index %i used multiple times", idx);
	return 0;
}

double __afrand(int idx, const char *file, uint line) {
	return __tsrand_a(idx, file, line) / (double)TSRAND_MAX;
}

double __anfrand(int idx, const char *file, uint line) {
	return __afrand(idx, file, line) * 2 - 1;
}

void tsrand_lock(RandomState *rnd) {
	rnd->locked = True;
}

void tsrand_unlock(RandomState *rnd) {
	rnd->locked = False;
}
