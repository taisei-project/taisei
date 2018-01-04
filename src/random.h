/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <stdint.h>
#include <stdbool.h>

#define CMWC_CYCLE 4096 // as Marsaglia recommends
#define CMWC_C_MAX 809430660 // as Marsaglia recommends

struct RandomState {
    uint32_t Q[CMWC_CYCLE];
    uint32_t c; // must be limited with CMWC_C_MAX
    uint32_t i;
    bool locked;
};

typedef struct RandomState RandomState;

int tsrand_test(void);

void tsrand_init(RandomState *rnd, uint32_t seed);
void tsrand_switch(RandomState *rnd);
void tsrand_seed_p(RandomState *rnd, uint32_t seed);
uint32_t tsrand_p(RandomState *rnd);

void tsrand_seed(uint32_t seed);
uint32_t tsrand(void);

void tsrand_lock(RandomState *rnd);
void tsrand_unlock(RandomState *rnd);

float frand(void);
float nfrand(void);

void __tsrand_fill_p(RandomState *rnd, int amount, const char *file, unsigned int line);
void __tsrand_fill(int amount, const char *file, unsigned int line);
uint32_t __tsrand_a(int idx, const char *file, unsigned int line);
float __afrand(int idx, const char *file, unsigned int line);
float __anfrand(int idx, const char *file, unsigned int line);

#define tsrand_fill_p(rnd,amount) __tsrand_fill_p(rnd, amount, __FILE__, __LINE__)
#define tsrand_fill(amount) __tsrand_fill(amount, __FILE__, __LINE__)
#define tsrand_a(idx) __tsrand_a(idx, __FILE__, __LINE__)
#define afrand(idx) __afrand(idx, __FILE__, __LINE__)
#define anfrand(idx) __anfrand(idx, __FILE__, __LINE__)

#define TSRAND_MAX UINT32_MAX

#define TSRAND_ARRAY_LIMIT 64
#define srand USE_tsrand_seed_INSTEAD_OF_srand
#define rand USE_tsrand_INSTEAD_OF_rand
