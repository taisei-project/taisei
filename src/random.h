/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#ifndef TSRAND_H
#define TSRAND_H

#include <stdint.h>

typedef struct RandomState {
	uint32_t w;
	uint32_t z;
} RandomState;

int tsrand_test(void);
void tsrand_switch(RandomState *rnd);
void tsrand_seed_p(RandomState *rnd, uint32_t seed);
int tsrand_p(RandomState *rnd);

inline void tsrand_seed(uint32_t seed);
inline int tsrand(void);

inline double frand();
inline double nfrand();

#define TSRAND_MAX INT32_MAX

#define srand USE_tsrand_seed_INSTEAD_OF_srand
#define rand USE_tsrand_INSTEAD_OF_rand

/*
 *	These have to be distinct 16-bit constants k, for which both k*2^16-1 and k*2^15-1 are prime numbers
 */

#define TSRAND_W_COEFF 30963
#define TSRAND_W_SEED_COEFF 19164
#define TSRAND_Z_COEFF 29379
#define TSRAND_Z_SEED_COEFF 31083

#endif
