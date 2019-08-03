/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_random_h
#define IGUARD_random_h

#include "taisei.h"

typedef struct RandomState {
	uint64_t state[4];
	bool locked;
} RandomState;

uint64_t splitmix64(uint64_t *state);
uint64_t makeseed(void);

void tsrand_init(RandomState *rnd, uint64_t seed);
void tsrand_switch(RandomState *rnd);
void tsrand_seed_p(RandomState *rnd, uint64_t seed);
uint32_t tsrand_p(RandomState *rnd);
uint64_t tsrand64_p(RandomState *rnd);

void tsrand_seed(uint64_t seed);
uint32_t tsrand(void);
uint64_t tsrand64(void);

void tsrand_lock(RandomState *rnd);
void tsrand_unlock(RandomState *rnd);

double frand(void);
double nfrand(void);

void _tsrand_fill_p(RandomState *rnd, int amount, const char *file, uint line);
void _tsrand_fill(int amount, const char *file, uint line);
uint32_t _tsrand_a(int idx, const char *file, uint line);
uint64_t _tsrand64_a(int idx, const char *file, uint line);
double _afrand(int idx, const char *file, uint line);
double _anfrand(int idx, const char *file, uint line);

#define tsrand_fill_p(rnd,amount) _tsrand_fill_p(rnd, amount, __FILE__, __LINE__)
#define tsrand_fill(amount) _tsrand_fill(amount, __FILE__, __LINE__)
#define tsrand_a(idx) _tsrand_a(idx, __FILE__, __LINE__)
#define afrand(idx) _afrand(idx, __FILE__, __LINE__)
#define anfrand(idx) _anfrand(idx, __FILE__, __LINE__)

#define TSRAND_ARRAY_LIMIT 16

#endif // IGUARD_random_h
